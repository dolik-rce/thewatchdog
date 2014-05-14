#!/bin/bash

export REPO_ROOT="$(pwd)"
export TEST_ROOT="$REPO_ROOT/tests/integration"
export TEST_TMP="$TEST_ROOT/tmp"

while [ $# -gt 0 ]; do
    case "$1" in
        -k) KEEP="yes" ;;
        *)    . $1 ;;
    esac
    shift
done

sql() {
    [ "$1" = "-q" ] && OUT="/dev/null" && shift || OUT="/dev/stdout"
    echo "SQL: $1" >> "$TEST_TMP/sql.log"
    if [ "$SQL" = "sqlite" ]; then
        echo "$1" | sqlite3 "$TEST_TMP/watchdog.db" | tee -a "$TEST_TMP/sql.log" >> "$OUT"
    elif [ "$SQL" = "mysql" ]; then
        mysql --skip-column-names $SQL_OPTS $SQL_DB -e "$1" | tee -a "$TEST_TMP/sql.log" >> "$OUT"
    else
        echo "ERROR: unknown SQL backend"
        test_end 2
    fi
}

test_result() {
    executed=$(( $executed + 1 ))
    if [ $1 -ne 0 ]; then
        errors=$(( $errors + 1 ))
        echo "ERROR (exit code $1)"
        echo "OUTPUT:"
        echo "$2"
    else
        if [ "$2" != "$3" ]; then
            fails=$(( $fails + 1 ))
            echo "FAILED"
            echo "ACTUAL:   $2"
            echo "EXPECTED: $3"
        else
            echo "OK"
        fi
    fi
}

clean_mysql_db() {
    [ "$SQL" = "mysql" ] || return
    mysqladmin $SQL_OPTS -f drop "$SQL_DB"
    mysqladmin $SQL_OPTS create "$SQL_DB"
}

clean_up() {
    while kill -0 "$WDS_PID" &> /dev/null; do
        kill "$WDS_PID"
        wget -q -O/dev/null 'http://localhost:8042'
    done
    [ "$KEEP" != "yes" ] && rm -rf "$TEST_TMP"
    [ "$KEEP" != "yes" ] && clean_mysql_db
}

test_end() {
    echo @ok=$(( $executed - $errors - $fails ))
    echo @failures=$fails
    echo @errors=$errors
    echo @skipped=$(( $total - $executed ))
    clean_up
    if [ "$1" ]; then
        exit $1
    elif [ "$fails" != "0" -o "$errors" -gt "0" ]; then
        exit 1
    else
        exit 0
    fi
}

wda() {
    bin/wda -C "$TEST_ROOT/wda.cfg" $@ | sed '/Loading configuration from/d'
}

wdc() {
    bin/wdc -C "$TEST_ROOT/wdc.cfg" $@ | sed '/Loading configuration from/d'
}

rm -rf "$TEST_TMP"
mkdir -p "$TEST_TMP"
clean_mysql_db

make CXX="g++ -ggdb3 -O0" FLAGS="GCC DEBUG"

total=$(ls $TEST_ROOT/test_*.sh | wc -l)
fails=0
errors=0
executed=0

for f in $(ls $TEST_ROOT/test_*.sh); do
    . "$f"
done

test_end
