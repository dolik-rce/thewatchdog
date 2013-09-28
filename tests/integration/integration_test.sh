#!/bin/bash

export REPO_ROOT="$(pwd)"
export TEST_ROOT="$REPO_ROOT/tests/integration"
export TEST_TMP="$TEST_ROOT/tmp"
 
rm -rf "$TEST_TMP"
mkdir -p "$TEST_TMP"

make bin/wds bin/wdc bin/wda lib/sqlite.so CXX="g++ -ggdb3 -O0" FLAGS="GCC DEBUG"

total=$(grep -c "test_result[^([]" $0)
fails=0
errors=0

sql() {
    [ "$1" = "-q" ] && OUT="/dev/null" && shift || OUT="/dev/stdout"
    echo "SQL: $1" >> "$TEST_TMP/sql.log"
    echo "$1" | sqlite3 "$TEST_TMP/watchdog.db" | tee -a "$TEST_TMP/sql.log" >> "$OUT"
}

test_result() {
    if [ $1 -ne 0 ]; then
        errors=$(( $errors + 1 ))
        echo "ERROR (exit code $1)"
        echo "OUTPUT:"
        echo "$2"
    else
        if [ "$2" != "$3" ]; then
            fails=$(( $fails + 1 ))
            echo "FAILED"
            echo "ACTUAL: $2"
            echo "EXPECTED: $3"
        else
            echo "OK"
        fi
    fi
}

clean_up() {
    while kill -0 $WDS_PID &> /dev/null; do
        kill $WDS_PID
        wget -q -O/dev/null 'http://localhost:8042'
    done
    rm -rf "$TEST_TMP"
    true
}

test_end() {
    echo @ok=$(( $total - $errors - $fails ))
    echo @failures=$fails
    echo @errors=$errors
    clean_up
    exit $1
}

echo "Starting wds"
bin/wds "$TEST_ROOT/wds.cfg" &
WDS_PID=$!

lim=""
until netstat -ntl | grep -q ":8042"; do
    [ "$lim" = "xxxxx" ] && test_result 1 && test_end 1
    lim="x$lim"
    sleep 1
done
echo "OK"

sql -q "insert into CLIENT (ID, NAME, PASSWORD, DESCR, SRC, SALT, BRANCHES) VALUES
  (0, 'admin', 'a1e2d68814ed9b1df3d7fe7bc3a243a2', 'Administrator account', '', 'QWER', '.*'),
  (1, 'Test1', '516dc66aa87480f104c3bcfd1c0d6f05', 'Test client', '.*', '3glx', '.*');"

echo "Testing wdc --get"
RES="$(bin/wdc -C "$TEST_ROOT/wdc.cfg" --get)"
test_result $? "$RES" "Nothing to do."

echo "Testing wda --clean"
RES="$(bin/wda -C "$TEST_ROOT/wda.cfg" --clean)"
test_result $? "$RES" "Instructing server to clean up ..."

echo "Testing wda --update"
DATE="$(date '+%Y/%m/%d %H:%M:%S')"
RES="$(/bin/echo -e "TestCommit1\tTestBranch\tCommit 1\tTester\t$DATE\tTest commit 1\tCommit msg\t" | bin/wda -C "$TEST_ROOT/wda.cfg" --update)"
test_result $? "$RES" ""

echo "Testing wda --state"
RES="$(bin/wda -C "$TEST_ROOT/wda.cfg" --state)"
test_result $? "$RES" "TestBranch	$DATE"

echo "Testing wdc --get"
RES="$(bin/wdc -C "$TEST_ROOT/wdc.cfg" --get)"
test_result $? "$RES" "TestCommit1"

echo "Testing import script"
RES="$(scripts/git/import.sh | bin/wda -C "$TEST_ROOT/wda.cfg" --update)"
test_result $? "$RES" ""

echo "Checking results of import script"
RES="$(sql "select count(*) from COMMITS;")"
test_result $? "$([ $RES -gt 1 ]; echo $?)" "0"

test_end
