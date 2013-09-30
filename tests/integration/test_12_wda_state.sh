echo "Testing wda --state"
RES="$(bin/wda -C "$TEST_ROOT/wda.cfg" --state)"
test_result $? "$RES" "TestBranch	$DATE"
