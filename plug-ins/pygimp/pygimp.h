/* -*- Mode: C; c-basic-offset: 4 -*-
    Gimp-Python - allows the writing of Gimp plugins in Python.
    Copyright (C) 1997-2002  James Henstridge <james@daa.com.au>

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
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef _PYGIMP_H_
#define _PYGIMP_H_

#include <Python.h>

#if PY_VERSION_HEX < 0x020300F0
#define PyBool_FromLong(v) PyInt_FromLong((v) ? 1L : 0L);
#endif

#include <libgimp/gimp.h>

G_BEGIN_DECLS

extern PyObject *pygimp_error;

PyObject *pygimp_param_to_tuple(int nparams, const GimpParam *params);
GimpParam *pygimp_param_from_tuple(PyObject *args, const GimpParamDef *ptype,
				   int nparams);


extern PyTypeObject PyGimpPDB_Type;
#define pygimp_pdb_check(v) (PyObject_TypeCheck(v, &PyGimpPDB_Type))
PyObject *pygimp_pdb_new(void);

extern PyTypeObject PyGimpPDBFunction_Type;
#define pygimp_pdb_function_check(v) (PyObject_TypeCheck(v, &PyGimpPDBFunction_Type))
PyObject *pygimp_pdb_function_new(const char *name, const char *blurb,
				  const char *help, const char *author,
				  const char *copyright, const char *date,
				  GimpPDBProcType proc_type,
				  int n_params, int n_return_vals,
				  GimpParamDef *params,
				  GimpParamDef *return_vals);

typedef struct {
    PyObject_HEAD
    gint32 ID;
} PyGimpImage;

extern PyTypeObject PyGimpImage_Type;
#define pygimp_image_check(v) (PyObject_TypeCheck(v, &PyGimpImage_Type))
PyObject *pygimp_image_new(gint32 ID);

typedef struct {
    PyObject_HEAD
    gint32 ID;
} PyGimpDisplay;

extern PyTypeObject PyGimpDisplay_Type;
#define pygimp_display_check(v) (PyObject_TypeCheck(v, &PyGimpDisplay_Type))
PyObject *pygimp_display_new(gint32 ID);


typedef struct {
    PyObject_HEAD
    gint32 ID;
    GimpDrawable *drawable;
} PyGimpDrawable, PyGimpLayer, PyGimpChannel;

extern PyTypeObject PyGimpDrawable_Type;
#define pygimp_drawable_check(v) (PyObject_TypeCheck(v, &PyGimpDrawable_Type))
PyObject *pygimp_drawable_new(GimpDrawable *drawable, gint32 ID);

extern PyTypeObject PyGimpLayer_Type;
#define pygimp_layer_check(v) (PyObject_TypeCheck(v, &PyGimpLayer_Type))
PyObject *pygimp_layer_new(gint32 ID);

extern PyTypeObject PyGimpChannel_Type;
#define pygimp_channel_check(v) (PyObject_TypeCheck(v, &PyGimpChannel_Type))
PyObject *pygimp_channel_new(gint32 ID);

typedef struct {
    PyObject_HEAD
    GimpTile *tile;
    PyGimpDrawable *drawable; /* we keep a reference to the drawable */
} PyGimpTile;

extern PyTypeObject PyGimpTile_Type;
#define pygimp_tile_check(v) (PyObject_TypeCheck(v, &PyGimpTile_Type))
PyObject *pygimp_tile_new(GimpTile *tile, PyGimpDrawable *drw);

typedef struct {
    PyObject_HEAD
    GimpPixelRgn pr;
    PyGimpDrawable *drawable; /* keep the drawable around */
} PyGimpPixelRgn;

extern PyTypeObject PyGimpPixelRgn_Type;
#define pygimp_pixel_rgn_check(v) (PyObject_TypeCheck(v, &PyGimpPixelRgn_Type))
PyObject *pygimp_pixel_rgn_new(PyGimpDrawable *drw, int x, int y,
			       int w, int h, int dirty, int shadow);

typedef struct {
    PyObject_HEAD
    GimpParasite *para;
} PyGimpParasite;

extern PyTypeObject PyGimpParasite_Type;
#define pygimp_parasite_check(v) (PyObject_TypeCheck(v, &PyGimpParasite_Type))
PyObject *pygimp_parasite_new(GimpParasite *para);

typedef struct {
    PyObject_HEAD
    gint32 ID;
} PyGimpVectors;

extern PyTypeObject PyGimpVectors_Type;
#define pygimp_vectors_check(v) (PyObject_TypeCheck(v, &PyGimpVectors_Type))
PyObject *pygimp_vectors_new(gint32 vectors_ID);

extern PyTypeObject PyGimpVectorsStroke_Type;
extern PyTypeObject PyGimpVectorsBezierStroke_Type;

G_END_DECLS

#endif
