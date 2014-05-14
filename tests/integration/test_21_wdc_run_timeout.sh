echo "Testing wdc --run"

DATE="$(date '+%Y/%m/%d %H:%M:%S')"
sql -q "$SQL_MODE insert into COMMITS (UID, BRANCH, CMT, AUTHOR, DT, MSG, PATH) values ('TestCommit21', '', 'Commit21', 'Tester', '$DATE', '', '');"

cat >"$TEST_TMP/script21.sh" <<EOF
#!/bin/sh
echo "This is a test"
sleep 5
echo "This should not execute"
echo @ok=1
EOF
chmod +x "$TEST_TMP/script21.sh"

RES="$(wdc --run "$TEST_TMP/script21.sh")"
RCODE=$?

echo "$RES"

OUT=$(sql "$SQL_MODE select STATUS,OK,FAIL,ERR,SKIP,BUILDER from RESULT where CMT_UID = 'TestCommit21' and CLIENT_ID=1;")

test_result $RCODE "$OUT" "4|0|0|1|0|test_runner_1"
