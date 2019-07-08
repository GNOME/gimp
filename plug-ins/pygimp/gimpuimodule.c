/* -*- Mode: C; c-basic-offset: 4 -*-
 * Gimp-Python - allows the writing of Gimp plugins in Python.
 * Copyright (C) 2005  Manish Singh <yosh@gimp.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <Python.h>

#include <pygobject.h>
#include <pygtk/pygtk.h>

#include <pycairo.h>
Pycairo_CAPI_t *Pycairo_CAPI;

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "pygimpcolor-api.h"
#include "pygimp-api.h"
#include "pygimp-util.h"


void gimpui_register_classes(PyObject *d);
void gimpui_add_constants(PyObject *module, const gchar *strip_prefix);
extern PyMethodDef gimpui_functions[];


static char gimpui_doc[] =
"This module provides interfaces to allow you to write gimp plug-ins"
;

void init_gimpui(void);

static gboolean
init_pycairo(void)
{
  Pycairo_IMPORT;
  if (Pycairo_CAPI == NULL)
    return FALSE;

  return TRUE;
}

extern const char *prog_name;

const char *prog_name = "pygimp";

PyMODINIT_FUNC
init_gimpui(void)
{
    PyObject *m, *d;
    PyObject *av;

    av = PySys_GetObject("argv");
    if (av != NULL) {
	if (PyList_Check(av) && PyList_Size(av) > 0 &&
	    PyString_Check(PyList_GetItem(av, 0)))
	    prog_name = PyString_AsString(PyList_GetItem(av, 0));
	else
	    PyErr_Warn(PyExc_Warning,
		       "ignoring sys.argv: it must be a list of strings");
    }


    pygimp_init_pygobject();

    init_pygtk();
    if (!init_pycairo())
      return;
    init_pygimpcolor();
    init_pygimp();

    m = Py_InitModule3("_gimpui", gimpui_functions, gimpui_doc);
    d = PyModule_GetDict(m);

    gimpui_register_classes(d);
    gimpui_add_constants(m, "GIMP_");

    if (PyErr_Occurred())
	Py_FatalError("can't initialize module _gimpui");
}
