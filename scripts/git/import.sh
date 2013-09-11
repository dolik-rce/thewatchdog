#!/bin/sh

if [ $# -eq 0 ]; then
  branches="$(git branch -a | sed -n '/HEAD/d;s|remotes/origin/\(.*\)|\1|p;')"
else
  branches="$@"
fi

scriptdir="$(dirname $(cd "${0%/*}" 2>/dev/null; echo "$PWD"/"${0##*/}"))"

for b in $branches; do
  if [ $b = "master" ]; then
    from=""
  else
    from="$(git merge-base origin/$b origin/master)"
  fi
  $scriptdir/parse-commits "$from" "$b" "$b"
done
