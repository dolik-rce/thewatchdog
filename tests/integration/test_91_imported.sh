echo "Checking results of import script"
RES="$(sql "select count(*) from COMMITS;")"
test_result $? "$([ $RES -gt 1 ]; echo $?)" "0"
