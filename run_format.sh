#!/bin/bash
# https://stackoverflow.com/questions/51326844/exclude-directory-clang-format
# https://stackoverflow.com/questions/55965712/how-do-i-add-clang-formatting-to-pre-commit-hook

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
