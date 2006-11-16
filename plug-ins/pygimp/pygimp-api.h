/* -*- Mode: C; c-basic-offset: 4 -*-
    Gimp-Python - allows the writing of Gimp plugins in Python.
    Copyright (C) 2005  Manish Singh <yosh@gimp.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
    02111-1307, USA.
 */

#ifndef _PYGIMP_API_H_
#define _PYGIMP_API_H_

#include <Python.h>

#include <libgimp/gimp.h>

struct _PyGimp_Functions {
    PyObject *(* image_new)(gint32 ID);
    PyObject *(* display_new)(gint32 ID);
    PyObject *(* drawable_new)(GimpDrawable *drawable, gint32 ID);
    PyObject *(* layer_new)(gint32 ID);
    PyObject *(* channel_new)(gint32 ID);
    PyObject *(* vectors_new)(gint32 ID);

    PyTypeObject *PDBFunction_Type;
    PyObject *(* pdb_function_new)(const char *name, const char *blurb,
                                   const char *help, const char *author,
                                   const char *copyright, const char *date,
                                   GimpPDBProcType proc_type,
                                   int n_params, int n_return_vals,
                                   GimpParamDef *params,
                                   GimpParamDef *return_vals);
};

#ifndef _INSIDE_PYGIMP_

#if defined(NO_IMPORT) || defined(NO_IMPORT_PYGIMP)
extern struct _PyGimp_Functions *_PyGimp_API;
#else
struct _PyGimp_Functions *_PyGimp_API;
#endif

#define pygimp_image_new        (_PyGimp_API->image_new)
#define pygimp_display_new      (_PyGimp_API->display_new)
#define pygimp_drawable_new     (_PyGimp_API->drawable_new)
#define pygimp_layer_new        (_PyGimp_API->layer_new)
#define pygimp_channel_new      (_PyGimp_API->channel_new)
#define pygimp_vectors_new      (_PyGimp_API->vectors_new)
#define PyGimpPDBFunction_Type  (_PyGimp_API->PDBFunction_Type)
#define pygimp_pdb_function_new (_PyGimp_API->pdb_function_new)

#define init_pygimp() G_STMT_START { \
    PyObject *gimpmodule = PyImport_ImportModule("gimp"); \
    if (gimpmodule != NULL) { \
        PyObject *mdict = PyModule_GetDict(gimpmodule); \
        PyObject *cobject = PyDict_GetItemString(mdict, "_PyGimp_API"); \
        if (PyCObject_Check(cobject)) \
            _PyGimp_API = PyCObject_AsVoidPtr(cobject); \
        else { \
            PyErr_SetString(PyExc_RuntimeError, \
                            "could not find _PyGimp_API object"); \
            return; \
        } \
    } else { \
        PyErr_SetString(PyExc_ImportError, \
                        "could not import gimp"); \
        return; \
    } \
} G_STMT_END

#endif /* ! _INSIDE_PYGIMP_ */

#endif /* _PYGIMP_API_H_ */
