#!/bin/bash
folder=.
format_files=\
`find -E . \
 -not -path "./cpp/lib/*"\
 -not -path "./build/*"\
 -iregex ".*\.(cpp|c|h|cc)"`

echo "Formatting:"
echo "${format_files}"

for file in $format_files
do
  if [[ "$file" =~ \.(c|h|cpp|cc)$ ]]; then clang-format -i "$file"
  fi
done
