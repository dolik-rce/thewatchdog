LC_ALL=C wget --spider -o "$TEST_TMP/wget.log" -e robots=off -r -p "http://localhost:8042" "http://localhost:8042/settings" "http://localhost:8042/config" "http://localhost:8042/sitemap.xml"
RCODE=$?
sed -n '/Found .* broken links/,$p;' "$TEST_TMP/wget.log"
test_result $RCODE
