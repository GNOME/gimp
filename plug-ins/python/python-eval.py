#!/usr/bin/env python3

#   Gimp-Python - allows the writing of Gimp plugins in Python.
#   Copyright (C) 2006  Manish Singh <yosh@gimp.org>
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
gi.require_version('Babl', '0.1')
from gi.repository import Babl
gi.require_version('Gegl', '0.4')
from gi.repository import Gegl
gi.require_version('Gimp', '3.0')
from gi.repository import Gimp
from gi.repository import GObject
from gi.repository import GLib
from gi.repository import Gio

import sys


def code_eval(procedure, run_mode, code, config, data):
    if code == '-':
        code = sys.stdin.read()
    exec(code, globals())
    return procedure.new_return_values(Gimp.PDBStatusType.SUCCESS, GLib.Error())

class PythonEval (Gimp.PlugIn):
    ## GimpPlugIn virtual methods ##
    def do_set_i18n(self, procname):
        return True, 'gimp30-python', None

    def do_query_procedures(self):
        return ['python-fu-eval']

    def do_create_procedure(self, name):
        procedure = Gimp.BatchProcedure.new(self, name, "Python 3",
                                            Gimp.PDBProcType.PLUGIN,
                                            code_eval, None)
        procedure.set_documentation ("Evaluate Python code",
                                     "Evaluate python code under the python interpreter (primarily for batch mode)",
                                     name)

        procedure.set_attribution("Manish Singh",
                                  "Manish Singh",
                                  "2006")

        return procedure


Gimp.main(PythonEval.__gtype__, sys.argv)
