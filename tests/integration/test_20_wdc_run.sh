echo "Testing wdc --run"

DATE="$(date '+%Y/%m/%d %H:%M:%S')"
sql -q "$SQL_MODE insert into COMMITS (UID, BRANCH, CMT, AUTHOR, DT, MSG, PATH) values ('TestCommit20', '', 'Commit20', 'Tester', '$DATE', '', '');"

cat >"$TEST_TMP/script20.sh" <<EOF
#!/bin/sh
echo "This is a test"
echo @ok=1
EOF
chmod +x "$TEST_TMP/script20.sh"

RES="$(wdc --run "$TEST_TMP/script20.sh")"
RCODE=$?

echo "$RES"

OUT=$(sql "$SQL_MODE select STATUS,OK,FAIL,ERR,SKIP,BUILDER from RESULT where CMT_UID = 'TestCommit20' and CLIENT_ID=1;")

test_result $RCODE "$OUT" "4|1|0|0|0|test_runner_1"
