#! /bin/sh
find . -name "*.[ch]" -exec grep -nHi fixme {} \; >TOFIX
find . -name "*.[ch]" -exec egrep -nHi "code-?me" {} \; >>TOFIX
