action="$1"
file="$2"
lock="/tmp/git_version_$(echo "$file" | md5sum | head -c 32).lock"
versionfile="src/Watchdog/Version.h"

setver() {
    sed 's/version = "[^"]*"/version = "'$(git describe --tags --dirty)'"/' $@
}

unsetver() {
    sed 's/version = "[^"]*"/version = ""/'
}

if [ $# -eq 0 ]; then
    # manual execution, for convenience...
    setver -i "$versionfile"
    exit 0
fi

# git describe invokes this filter too, so we must make sure 
# that we don't run into troubles with recursion
if mkdir "$lock" &>/dev/null; then
    if [ "$file" == "$versionfile" ]; then
        # set/unset the version on commit/add of the versionfile
        if [ "$action" == "--clean" ]; then
            unsetver
        elif [ "$action" == "--smudge" ]; then
            setver
        fi
    else
        # for other files just print them out and update the versionfile
        cat
        setver -i "$versionfile"
    fi
    rmdir "$lock"
else
    cat
fi
