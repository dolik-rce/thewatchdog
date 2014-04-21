#!/bin/sh
#
# Usage:
#   import.sh [--since date] [path, ...]
#
# Exmaple:
#   import.sh --since "2 days ago" /home/user/uppsvn/trunk
#   import.sh --since "1 month ago" http://project.googlecode.com/svn

if [ "$1" = "--since" ]; then
  since="$2"
  shift 2
else
  since="1 month ago"
fi

since=$(date -d "$since" +"%Y-%m-%d")
path="$@"

ECHO="$(which echo)"

function common_path() {
  lhs=$1
  rhs=$2
  path=
  OLD_IFS=$IFS; IFS=/
  for w in $rhs; do
    test "$path" = "" && try="$w" || try="$path/$w"
    case $lhs in
      $try*) ;;
      *) break ;;
    esac
    path=$try
  done
  IFS=$OLD_IFS
  echo $path
}

function common_path_all() {
  local path=$1
  shift
  for arg
  do
    path=$(common_path "$path" "$arg")
  done
  echo $path
}

print_commit() {
  DATE="$(date -d"$DATE" +"%Y/%m/%d %H:%M:%S")"
  DIR="$(common_path_all $PATHS)"
  MSG="$(echo "$MSG" | sed '1!s/^/<br>/;s/\t/    /;' | tr -d '\n')"
  BRANCH="trunk"
  $ECHO -e "$COMMIT\t$BRANCH\t$COMMIT\t$AUTHOR\t$DATE\t$MSG\t$DIR"
}

LOG="$(svn log -r {$since}:HEAD --xml --verbose --incremental $path | tr -d "\n" | sed 's|<path [^>]*>/||g;s|</path>| |g;s|</logentry>|&\n|g;' | sed -n 's|.* revision="\([^"]*\)".*<author>\([^<]*\)</author><date>\([^<]*\)</date><paths>\([^<]*\)</paths><msg>\([^<]*\)</msg>.*|COMMIT="r\1"; AUTHOR="\2"; DATE="\3"; PATHS="\4"; MSG="\5"; print_commit; |p' )"
eval "$LOG"
