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

#ifndef _PYGIMPCOLOR_API_H_
#define _PYGIMPCOLOR_API_H_

#include <Python.h>

#include <libgimpcolor/gimpcolor.h>

struct _PyGimpColor_Functions {
    PyTypeObject *RGB_Type;
    PyObject *(* rgb_new)(const GimpRGB *rgb);
    PyTypeObject *HSV_Type;
    PyObject *(* hsv_new)(const GimpHSV *hsv);
    PyTypeObject *HSL_Type;
    PyObject *(* hsl_new)(const GimpHSL *hsl);
    PyTypeObject *CMYK_Type;
    PyObject *(* cmyk_new)(const GimpCMYK *cmyk);
    int (* rgb_from_pyobject)(PyObject *object, GimpRGB *color);
};

#ifndef _INSIDE_PYGIMPCOLOR_

#if defined(NO_IMPORT) || defined(NO_IMPORT_PYGIMPCOLOR)
extern struct _PyGimpColor_Functions *_PyGimpColor_API;
#else
struct _PyGimpColor_Functions *_PyGimpColor_API;
#endif

#define PyGimpRGB_Type (_PyGimpColor_API->RGB_Type)
#define PyGimpHSV_Type (_PyGimpColor_API->HSV_Type)
#define PyGimpHSL_Type (_PyGimpColor_API->HSL_Type)
#define PyGimpCMYK_Type (_PyGimpColor_API->CMYK_Type)

#define pygimp_rgb_check(v) (pyg_boxed_check((v), GIMP_TYPE_RGB))
#define pygimp_hsv_check(v) (pyg_boxed_check((v), GIMP_TYPE_HSV))
#define pygimp_hsl_check(v) (pyg_boxed_check((v), GIMP_TYPE_HSL))
#define pygimp_cmyk_check(v) (pyg_boxed_check((v), GIMP_TYPE_CMYK))

#define pygimp_rgb_new (_PyGimpColor_API->rgb_new)
#define pygimp_hsv_new (_PyGimpColor_API->hsv_new)
#define pygimp_hsl_new (_PyGimpColor_API->hsl_new)
#define pygimp_cmyk_new (_PyGimpColor_API->cmyk_new)

#define pygimp_rgb_from_pyobject (_PyGimpColor_API->rgb_from_pyobject)

#define init_pygimpcolor() G_STMT_START { \
    PyObject *gimpcolormodule = PyImport_ImportModule("gimpcolor"); \
    if (gimpcolormodule != NULL) { \
	PyObject *mdict = PyModule_GetDict(gimpcolormodule); \
	PyObject *cobject = PyDict_GetItemString(mdict, "_PyGimpColor_API"); \
	if (PyCObject_Check(cobject)) \
	    _PyGimpColor_API = PyCObject_AsVoidPtr(cobject); \
	else { \
	    PyErr_SetString(PyExc_RuntimeError, \
		            "could not find _PyGimpColor_API object"); \
	    return; \
	} \
    } else { \
	PyErr_SetString(PyExc_ImportError, \
	                "could not import gimpcolor"); \
	return; \
    } \
} G_STMT_END

#endif /* ! _INSIDE_PYGIMPCOLOR_ */

#endif /* _PYGIMPCOLOR_API_H_ */
