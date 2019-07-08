/* -*- Mode: C; c-basic-offset: 4 -*-
 * Gimp-Python - allows the writing of Gimp plugins in Python.
 * Copyright (C) 2005-2006  Manish Singh <yosh@gimp.org>
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

#include <gdk-pixbuf/gdk-pixbuf.h>

#include <libgimpthumb/gimpthumb.h>

#include "pygimp-util.h"


void gimpthumb_register_classes(PyObject *d);
void gimpthumb_add_constants(PyObject *module, const gchar *strip_prefix);
extern PyMethodDef gimpthumb_functions[];


static char gimpthumb_doc[] =
"This module provides interfaces to allow you to write gimp plug-ins"
;

void initgimpthumb(void);

PyMODINIT_FUNC
initgimpthumb(void)
{
    PyObject *m, *d;

    pygimp_init_pygobject();

    m = Py_InitModule3("gimpthumb", gimpthumb_functions, gimpthumb_doc);
    d = PyModule_GetDict(m);

    gimpthumb_register_classes(d);
    gimpthumb_add_constants(m, "GIMP_THUMB_");

    if (PyErr_Occurred())
	Py_FatalError("can't initialize module gimpthumb");
}
