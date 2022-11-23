#!/usr/bin/env python3

#   Ligma-Python - allows the writing of Ligma plugins in Python.
#   Copyright (C) 2006  Manish Singh <yosh@ligma.org>
#
#   This program is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 3 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <https://www.gnu.org/licenses/>.

import gi
gi.require_version('Ligma', '3.0')
from gi.repository import Ligma
from gi.repository import GObject
from gi.repository import GLib
from gi.repository import Gio

import sys


def code_eval(procedure, run_mode, code, args, data):
    if code == '-':
        code = sys.stdin.read()
    exec(code, globals())
    return procedure.new_return_values(Ligma.PDBStatusType.SUCCESS, GLib.Error())


class PythonEval (Ligma.PlugIn):
    ## LigmaPlugIn virtual methods ##
    def do_set_i18n(self, procname):
        return True, 'ligma30-python', None

    def do_query_procedures(self):
        return ['python-fu-eval']

    def do_create_procedure(self, name):
        procedure = Ligma.BatchProcedure.new(self, name, "Python 3",
                                            Ligma.PDBProcType.PLUGIN,
                                            code_eval, None)
        procedure.set_documentation ("Evaluate Python code",
                                     "Evaluate python code under the python interpreter (primarily for batch mode)",
                                     name)

        procedure.set_attribution("Manish Singh",
                                  "Manish Singh",
                                  "2006")

        return procedure


Ligma.main(PythonEval.__gtype__, sys.argv)
