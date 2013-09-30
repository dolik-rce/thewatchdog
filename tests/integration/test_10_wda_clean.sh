echo "Testing wda --clean"
RES="$(bin/wda -C "$TEST_ROOT/wda.cfg" --clean)"
test_result $? "$RES" "Instructing server to clean up ..."
