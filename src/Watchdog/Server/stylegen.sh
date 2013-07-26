#!/bin/sh

# this script can be used to generate the .sRGB classes in style.css

hex="0 1 2 3 4 5 6 7 8 9 A B C D E F"

for r in $hex; do
    for g in $hex; do
        echo ".s${r}${g}8 { background-color: #${r}${g}8; }"
    done
done
