#!/bin/sh

GIMP_EXE=$1
TEST_FILE=$2
SRC_DIR=`dirname $TEST_FILE`
SRC_DIR=`realpath $SRC_DIR`
TEST_NAME=$3

cmd="import os; import sys; sys.path.insert(0, '$SRC_DIR'); from pygimp.utils import gimp_c_assert;"
cmd="$cmd proc = Gimp.get_pdb().lookup_procedure('$TEST_NAME'); gimp_c_assert('$TEST_FILE', 'Test PDB procedure does not exist: {}'.format('$TEST_NAME'), proc is not None);"
cmd="$cmd result = proc.run(None);"
cmd="$cmd print('SUCCESS') if result.index(0) == Gimp.PDBStatusType.SUCCESS else gimp_c_assert('$TEST_FILE', result.index(1), False);"
echo "$cmd" | "$GIMP_EXE" -nis --batch-interpreter "python-fu-eval" -b - --quit
