echo "Testing wdc --get"
RES="$(wdc --get)"
test_result $? "$RES" "TestCommit1"
