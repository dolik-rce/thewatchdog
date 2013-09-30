echo "Starting wds"
bin/wds "$TEST_ROOT/wds.cfg" &
WDS_PID=$!

lim=""
until netstat -ntl | grep -q ":8042"; do
    [ "$lim" = "xxxxxxxxxxxxxxxxxxxx" ] && test_result 1 && test_end 1
    lim="x$lim"
    sleep 1
done
test_result 0
