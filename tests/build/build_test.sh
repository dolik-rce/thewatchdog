#!/bin/bash

ok=0
fails=0
errors=0

build() {
    if make $1; then
        [ -f $1 ] && ok=$(( $ok + 1 )) || errors=$(( $errors + 1 ))
    else
        fails=$(( $fails + 1 ))
    fi
}

make clean
make update-uppsrc

build lib/sqlite.so
build lib/mysql.so
build bin/wdc
build bin/wda
build bin/wds

echo @ok=$ok
echo @failures=$fails
echo @errors=$errors
