#!/usr/bin/bash

IFS=$'\n'
for file in *.cpp *.hpp *.c *.h *.in *.md *.m4 *.am *.ac ; do
   ed -s "$file" <<< $'H\n,g/\r*$/s///\nwq'
done

