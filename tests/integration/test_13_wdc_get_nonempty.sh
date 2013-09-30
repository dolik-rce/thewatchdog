echo "Testing wdc --get"
RES="$(bin/wdc -C "$TEST_ROOT/wdc.cfg" --get)"
test_result $? "$RES" "TestCommit1"
