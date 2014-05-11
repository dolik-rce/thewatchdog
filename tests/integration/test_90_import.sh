echo "Testing import script"
RES="$(scripts/git/import.sh | wda --update)"
test_result $? "$RES" ""
