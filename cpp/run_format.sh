#!/bin/bash
# https://stackoverflow.com/questions/51326844/exclude-directory-clang-format
# https://stackoverflow.com/questions/55965712/how-do-i-add-clang-formatting-to-pre-commit-hook

folder=.
exclude_folder=./cpp/lib
format_files=`find "${folder}" -type f -not -path "${exclude_folder}" -prune`

for file in $format_files
do
  if [[ "$file" =~ \.(c|h|cpp|cc)$ ]]; then clang-format -i "$file"
  fi
done
