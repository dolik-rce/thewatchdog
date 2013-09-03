#!/bin/sh

if [ $# -eq 0 ]; then
  branches="$(git branch -a | sed -n 's|remotes/origin/\(.*\)|\1|p')"
else
  branches="$@"
fi

scriptdir="$(dirname $(cd "${0%/*}" 2>/dev/null; echo "$PWD"/"${0##*/}"))"

for b in $branches; do
  if [ $b = master ]; then
    from="0000000000000000000000000000000000000000"
  else
    from="$(git merge-base $b master)"
  fi
  echo "$from origin/$b $b" | $scriptdir/parse-commits
done
