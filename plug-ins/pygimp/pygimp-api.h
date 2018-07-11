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

#ifndef _PYGIMP_API_H_
#define _PYGIMP_API_H_

#include <Python.h>

#include <libgimp/gimp.h>

typedef struct {
    PyObject_HEAD
    gint32 ID;
} PyGimpImage, PyGimpItem;

typedef struct {
    PyObject_HEAD
    gint32 ID;
} PyGimpDisplay;

typedef struct {
    PyObject_HEAD
    gint32 ID;
    GimpDrawable *drawable;
} PyGimpDrawable, PyGimpLayer, PyGimpGroupLayer, PyGimpChannel;

typedef struct {
    PyObject_HEAD
    gint32 ID;
} PyGimpVectors;

struct _PyGimp_Functions {
    PyTypeObject *Image_Type;
    PyObject *(* image_new)(gint32 ID);

    PyTypeObject *Display_Type;
    PyObject *(* display_new)(gint32 ID);

    PyTypeObject *Item_Type;
    PyObject *(* item_new)(gint32 ID);

    PyTypeObject *Drawable_Type;
    PyObject *(* drawable_new)(GimpDrawable *drawable, gint32 ID);

    PyTypeObject *Layer_Type;
    PyObject *(* layer_new)(gint32 ID);

    PyTypeObject *GroupLayer_Type;
    PyObject *(* group_layer_new)(gint32 ID);

    PyTypeObject *Channel_Type;
    PyObject *(* channel_new)(gint32 ID);

    PyTypeObject *Vectors_Type;
    PyObject *(* vectors_new)(gint32 ID);

    PyObject *pygimp_error;
};

#ifndef _INSIDE_PYGIMP_

#if defined(NO_IMPORT) || defined(NO_IMPORT_PYGIMP)
extern struct _PyGimp_Functions *_PyGimp_API;
#else
struct _PyGimp_Functions *_PyGimp_API;
#endif

#define PyGimpImage_Type        (_PyGimp_API->Image_Type)
#define pygimp_image_new        (_PyGimp_API->image_new)
#define PyGimpDisplay_Type      (_PyGimp_API->Display_Type)
#define pygimp_display_new      (_PyGimp_API->display_new)
#define PyGimpItem_Type         (_PyGimp_API->Item_Type)
#define pygimp_item_new         (_PyGimp_API->item_new)
#define PyGimpDrawable_Type     (_PyGimp_API->Drawable_Type)
#define pygimp_drawable_new     (_PyGimp_API->drawable_new)
#define PyGimpLayer_Type        (_PyGimp_API->Layer_Type)
#define pygimp_layer_new        (_PyGimp_API->layer_new)
#define PyGimpGroupLayer_Type   (_PyGimp_API->GroupLayer_Type)
#define pygimp_group_layer_new  (_PyGimp_API->group_layer_new)
#define PyGimpChannel_Type      (_PyGimp_API->Channel_Type)
#define pygimp_channel_new      (_PyGimp_API->channel_new)
#define PyGimpVectors_Type      (_PyGimp_API->Vectors_Type)
#define pygimp_vectors_new      (_PyGimp_API->vectors_new)
#define pygimp_error            (_PyGimp_API->pygimp_error)

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
