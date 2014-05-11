echo "Testing wda --state"
RES="$(wda --state)"
test_result $? "$RES" "TestBranch	$DATE"
