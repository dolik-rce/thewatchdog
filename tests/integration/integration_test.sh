#!/bin/bash

export REPO_ROOT="$(pwd)"
export TEST_ROOT="$REPO_ROOT/tests/integration"
export TEST_TMP="$TEST_ROOT/tmp"
 
rm -rf "$TEST_TMP"
mkdir -p "$TEST_TMP"

make bin/wds bin/wdc bin/wda lib/sqlite.so CXX="g++ -ggdb3 -O0" FLAGS="GCC DEBUG"

total=3
fails=0
errors=0

test_result() {
    if [ $1 -ne 0 ]; then
        errors=$(( $errors + 1 ))
    else
        [ "$2" = "$3" ] || fails=$(( $fails + 1 ))
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

bin/wds "$TEST_ROOT/wds.cfg" &
WDS_PID=$!

lim=""
until netstat -ntl | grep -q ":8042"; do
    [ "$lim" = "xxxxx" ] && errors=$(( errors + 1 )) && test_end 1
    lim="x$lim"
    sleep 1
done

echo "insert into CLIENT (ID, NAME, PASSWORD, DESCR, SRC, SALT) VALUES 
  (0, 'admin', 'a1e2d68814ed9b1df3d7fe7bc3a243a2', 'Administrator account', '', 'QWER'),
  (1, 'Test1', '516dc66aa87480f104c3bcfd1c0d6f05', 'Test client', '.*', '3glx');
" | sqlite3 "$TEST_TMP/watchdog.db"

RES="$(bin/wdc -C "$TEST_ROOT/wdc.cfg" --get)"
test_result $? "$RES" "Nothing to do."

RES="$(bin/wda -C "$TEST_ROOT/wda.cfg" --clean)"
test_result $? "$RES" "Instructing server to clean up ..."

test_end
