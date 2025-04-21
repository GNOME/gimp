#!/usr/bin/env python3
import os
import sys
import subprocess
from pathlib import Path

GIMP_EXE = sys.argv[1]
TEST_FILE = sys.argv[2]
SRC_DIR = os.path.dirname(TEST_FILE)
SRC_DIR = Path(os.path.realpath(SRC_DIR)).resolve().as_posix()
TEST_NAME = sys.argv[3]

cmd = (
  f"import os; import sys; sys.path.insert(0, '{SRC_DIR}'); from pygimp.utils import gimp_c_assert; "
  f"proc = Gimp.get_pdb().lookup_procedure('{TEST_NAME}'); gimp_c_assert('{TEST_FILE}', 'Test PDB procedure does not exist: {{}}'.format('{TEST_NAME}'), proc is not None); "
  f"result = proc.run(None); "
  f"print('SUCCESS') if result.index(0) == Gimp.PDBStatusType.SUCCESS else gimp_c_assert('{TEST_FILE}', result.index(1), False);"
)
subprocess.run([sys.executable, GIMP_EXE, "-nis", "--batch-interpreter", "python-fu-eval", "-b", cmd, "--quit"], check=True)
