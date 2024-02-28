#!/bin/sh

GIMP_EXE=$1
TEST_FILE=$2
SRC_DIR=`dirname $TEST_FILE`
SRC_DIR=`realpath $SRC_DIR`

if [ ! -f "$TEST_FILE" ]; then
  echo "ERROR: file '$TEST_FILE' does not exist!"
  return 1;
fi

first_char=`head -c1 "$TEST_FILE"`

if [ $first_char != '#' ]; then
  # Note: I don't actually care that it's a shebang, just that it's a comment,
  # hence a useless line, because I'm going to remove it and replace it with
  # gimp_assert() import line.
  # This will make much easier to debug tests with meaningful line numbers.
  echo "ERROR: file '$TEST_FILE' should start with a shebang: #!/usr/bin/env python3"
  return 1;
fi

header="import os; import sys; sys.path.insert(0, '$SRC_DIR'); from pygimp.utils import gimp_assert;"
header="$header import pygimp.utils; pygimp.utils.gimp_test_filename = '$TEST_FILE'"

(echo "$header" && tail -n +2 "$TEST_FILE") | "$GIMP_EXE" -nis --batch-interpreter "python-fu-eval" -b - --quit
