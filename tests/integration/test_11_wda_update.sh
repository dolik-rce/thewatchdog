echo "Testing wda --update"
DATE="$(date '+%Y/%m/%d %H:%M:%S')"
RES="$(/bin/echo -e "TestCommit1\tTestBranch\tCommit 1\tTester\t$DATE\tTest commit 1\tCommit msg\t" | wda --update)"
test_result $? "$RES" ""
