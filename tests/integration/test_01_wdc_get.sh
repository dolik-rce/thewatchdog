echo "Testing wdc --get"

sql -q "$SQL_MODE
  insert into CLIENT (ID, NAME, PASSWORD, DESCR, SRC, SALT, BRANCHES) VALUES
  (0, 'admin', 'a1e2d68814ed9b1df3d7fe7bc3a243a2', 'Administrator account', '', 'QWER', '.*'),
  (1, 'Test1', '516dc66aa87480f104c3bcfd1c0d6f05', 'Test client', '.*', '3glx', '.*');"

RES="$(bin/wdc -C "$TEST_ROOT/wdc.cfg" --get)"
test_result $? "$RES" "Nothing to do."
