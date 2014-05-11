echo "Testing wda --clean"
RES="$(wda --clean)"
test_result $? "$RES" "Instructing server to clean up ..."
