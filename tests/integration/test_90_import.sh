echo "Testing import script"
RES="$(scripts/git/import.sh | bin/wda -C "$TEST_ROOT/wda.cfg" --update)"
test_result $? "$RES" ""
