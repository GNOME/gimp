/* -*- Mode: C; c-basic-offset: 4 -*- */
/*
    Gimp-Python - allows the writing of Gimp plugins in Python.
    Copyright (C) 1997-1999  James Henstridge <james@daa.com.au>

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

#include "Python.h"
#include "sysmodule.h"
#include "structmember.h"
#include <libgimp/gimp.h>

/* maximum bits per pixel ... */
#define MAX_BPP 4

/* part of Hans Breuer's pygimp on win32 patch */
#if defined(_MSC_VER)
/* reduce strict checking in glibconfig.h */
# pragma warning(default:4047)
# undef PyObject_HEAD_INIT
# define PyObject_HEAD_INIT(a) \
  1, NULL, /* must be set in init function */
#endif

static PyObject *ErrorObject;

#ifndef GIMP_CHECK_VERSION
#  define GIMP_CHECK_VERSION(major, minor, micro) \
    (GIMP_MAJOR_VERSION > (major) || \
     (GIMP_MAJOR_VERSION == (major) && GIMP_MINOR_VERSION > (minor)) || \
     (GIMP_MAJOR_VERSION == (major) && GIMP_MINOR_VERSION == (minor) && \
      GIMP_MICRO_VERSION >= (micro)))
#endif

#ifndef PG_DEBUG
# define PG_DEBUG 0
#endif

/* ----------------------------------------------------- */

/* Declarations for objects of type pdb */

typedef struct {
    PyObject_HEAD
    /* XXXX Add your own stuff here */
} pdbobject;

staticforward PyTypeObject Pdbtype;
#define pdb_check(v) ((v)->ob_type == &Pdbtype)


/* ---------------------------------------------------------------- */

/* Declarations for objects of type pdbFunc */

typedef struct {
    PyObject_HEAD
    char *name;
    PyObject *proc_name, *proc_blurb, *proc_help, *proc_author,
	*proc_copyright, *proc_date, *proc_type, *py_params,
	*py_return_vals;
    int nparams, nreturn_vals;
    GParamDef *params, *return_vals;
} pfobject;

staticforward PyTypeObject Pftype;
#define pf_check(v) ((v)->ob_type == &Pftype)
static pfobject *newpfobject(char *);

/* ---------------------------------------------------------------- */

/* Declarations for objects of type Image */

typedef struct {
    PyObject_HEAD
    gint32 ID;
} imgobject;

staticforward PyTypeObject Imgtype;
#define img_check(v) ((v)->ob_type == &Imgtype)
static imgobject *newimgobject(gint32);

/* ---------------------------------------------------------------- */

/* Declarations for objects of type Display */

typedef struct {
    PyObject_HEAD
    gint32 ID;
} dispobject;

staticforward PyTypeObject Disptype;
#define disp_check(v) ((v)->ob_type == &Disptype)
static dispobject *newdispobject(gint32);

/* ---------------------------------------------------------------- */

/* Declarations for objects of type Layer and channel */

typedef struct {
    PyObject_HEAD
    gint32 ID;
    GDrawable *drawable;
} drwobject, layobject, chnobject;

staticforward PyTypeObject Laytype;
#define lay_check(v) ((v)->ob_type == &Laytype)
static layobject *newlayobject(gint32);

staticforward PyTypeObject Chntype;
#define chn_check(v) ((v)->ob_type == &Chntype)
static chnobject *newchnobject(gint32);

/* The drawable type isn't really a type -- it can be a channel or layer */
#define drw_check(v) ((v)->ob_type == &Laytype || (v)->ob_type == &Chntype)
static drwobject *newdrwobject(GDrawable *, gint32);

/* ---------------------------------------------------------------- */

/* Declarations for objects of type Tile */

typedef struct {
    PyObject_HEAD
    GTile *tile;
    drwobject *drawable; /* we keep a reference to the drawable */
} tileobject;

staticforward PyTypeObject Tiletype;
#define tile_check(v) ((v)->ob_type == &Tiletype)
static tileobject *newtileobject(GTile *, drwobject *drw);

/* ---------------------------------------------------------------- */

/* Declarations for objects of type PixelRegion */

typedef struct {
    PyObject_HEAD
    GPixelRgn pr;
    drwobject *drawable; /* keep the drawable around */
} probject;

staticforward PyTypeObject Prtype;
#define pr_check(v) ((v)->ob_type == &Prtype)
static probject *newprobject(drwobject *drw, int, int, int, int, int, int);

/* ---------------------------------------------------------------- */

#ifdef GIMP_HAVE_PARASITES
/* Declarations for objects of type Parasite */

typedef struct {
    PyObject_HEAD
    Parasite *para;
} paraobject;

staticforward PyTypeObject Paratype;
#define para_check(v) ((v)->ob_type == &Paratype)
static paraobject *newparaobject(Parasite *para);

#endif
/* ---------------------------------------------------------------- */

/* routines to convert between Python tuples and gimp GParam's */

#if PG_DEBUG > 0

static void print_GParam(int nparams, GParam *params) {
    int i;

    for (i = 0; i < nparams; i++) {
	switch(params[i].type) {
	case PARAM_INT32:
	    fprintf(stderr, "%i. int %i\n", i,
		    params[i].data.d_int32);
	    break;
	case PARAM_INT16:
	    fprintf(stderr, "%i. int %i\n", i,
		    params[i].data.d_int16);
	    break;
	case PARAM_INT8:
	    fprintf(stderr, "%i. int %i\n", i,
		    params[i].data.d_int8);
	    break;
	case PARAM_FLOAT:
	    fprintf(stderr, "%i. float %f\n", i,
		    params[i].data.d_float);
	    break;
	case PARAM_STRING:
	    fprintf(stderr, "%i. string %s\n", i,
		    params[i].data.d_string);
	    break;
	case PARAM_INT32ARRAY:
	case PARAM_INT16ARRAY:
	case PARAM_INT8ARRAY:
	case PARAM_FLOATARRAY:
	case PARAM_STRINGARRAY:
	    fprintf(stderr, "%i. array of type %i %s\n", i,
		    params[i].type, params[i].data.d_int32array
		    == NULL ? "(null)":"");
	    break;
	case PARAM_STATUS:
	    fprintf(stderr, "%i. status %i\n", i,
		    params[i].data.d_status);
	    break;
	default:
	    fprintf(stderr, "%i. other %i\n", i,
		    params[i].data.d_int32);
	    break;
	}
    }
}

#endif

static PyObject *
GParam_to_tuple(int nparams, GParam *params) {
    PyObject *args, *tmp;
    int i, j, n;

    args = PyTuple_New(nparams);
    for (i = 0; i < nparams && params[i].type != PARAM_END; i++) {
	switch(params[i].type) {
	case PARAM_INT32:
	    PyTuple_SetItem(args, i, PyInt_FromLong(
						    (long) params[i].data.d_int32));
	    break;
	case PARAM_INT16:
	    PyTuple_SetItem(args, i, PyInt_FromLong(
						    (long) params[i].data.d_int16));
	    break;
	case PARAM_INT8:
	    PyTuple_SetItem(args, i, PyInt_FromLong(
						    (long) params[i].data.d_int8));
	    break;
	case PARAM_FLOAT:
	    PyTuple_SetItem(args, i, PyFloat_FromDouble(
							(double) params[i].data.d_float));
	    break;
	case PARAM_STRING:
	    if (params[i].data.d_string == NULL) {
		Py_INCREF(Py_None);
		PyTuple_SetItem(args, i, Py_None);
	    } else
		PyTuple_SetItem(args, i,
				PyString_FromString(
						    params[i].data.d_string));
	    break;

	    /* For these to work, the previous argument must have
	     * been an integer
	     */
	case PARAM_INT32ARRAY:
	    if (params[i].data.d_int32array == NULL) {
		PyTuple_SetItem(args,i,PyTuple_New(0));
		break;
	    }
	    if ((tmp=PyTuple_GetItem(args, i-1)) == NULL) {
		Py_DECREF(args);
		return NULL;
	    }
	    if (!PyInt_Check(tmp)) {
		PyErr_SetString(PyExc_TypeError,
				"count type must be integer");
		Py_DECREF(args);
		return NULL;
	    }
	    n = PyInt_AsLong(tmp);
	    tmp = PyTuple_New(n);
	    for (j = 0; j < n; j++)
		PyTuple_SetItem(tmp, j, PyInt_FromLong(
						       (long)params[i].data.d_int32array[j]));
	    PyTuple_SetItem(args, i, tmp);
	    break;
	case PARAM_INT16ARRAY:
	    if (params[i].data.d_int16array == NULL) {
		PyTuple_SetItem(args,i,PyTuple_New(0));
		break;
	    }
	    if ((tmp=PyTuple_GetItem(args, i-1)) == NULL) {
		Py_DECREF(args);
		return NULL;
	    }
	    if (!PyInt_Check(tmp)) {
		PyErr_SetString(PyExc_TypeError,
				"count type must be integer");
		Py_DECREF(args);
		return NULL;
	    }
	    n = PyInt_AsLong(tmp);
	    tmp = PyTuple_New(n);
	    for (j = 0; j < n; j++)
		PyTuple_SetItem(tmp, j, PyInt_FromLong(
						       (long)params[i].data.d_int16array[j]));
	    PyTuple_SetItem(args, i, tmp);
	    break;
	case PARAM_INT8ARRAY:
	    if (params[i].data.d_int8array == NULL) {
		PyTuple_SetItem(args,i,PyTuple_New(0));
		break;
	    }
	    if ((tmp=PyTuple_GetItem(args, i-1)) == NULL) {
		Py_DECREF(args);
		return NULL;
	    }
	    if (!PyInt_Check(tmp)) {
		PyErr_SetString(PyExc_TypeError,
				"count type must be integer");
		Py_DECREF(args);
		return NULL;
	    }
	    n = PyInt_AsLong(tmp);
	    tmp = PyTuple_New(n);
	    for (j = 0; j < n; j++)
		PyTuple_SetItem(tmp, j, PyInt_FromLong(
						       (long)params[i].data.d_int8array[j]));
	    PyTuple_SetItem(args, i, tmp);
	    break;
	case PARAM_FLOATARRAY:
	    if (params[i].data.d_floatarray == NULL) {
		PyTuple_SetItem(args,i,PyTuple_New(0));
		break;
	    }
	    if ((tmp=PyTuple_GetItem(args, i-1)) == NULL) {
		Py_DECREF(args);
		return NULL;
	    }
	    if (!PyInt_Check(tmp)) {
		PyErr_SetString(PyExc_TypeError,
				"count type must be integer");
		Py_DECREF(args);
		return NULL;
	    }
	    n = PyInt_AsLong(tmp);
	    tmp = PyTuple_New(n);
	    for (j = 0; j < n; j++)
		PyTuple_SetItem(tmp,j,
				PyFloat_FromDouble((double)
						   params[i].data.d_floatarray[j]));
	    PyTuple_SetItem(args, i, tmp);
	    break;
	case PARAM_STRINGARRAY:
	    if (params[i].data.d_stringarray == NULL) {
		PyTuple_SetItem(args,i,PyTuple_New(0));
		break;
	    }
	    if ((tmp=PyTuple_GetItem(args, i-1)) == NULL) {
		Py_DECREF(args);
		return NULL;
	    }
	    if (!PyInt_Check(tmp)) {
		PyErr_SetString(PyExc_TypeError,
				"count type must be integer");
		Py_DECREF(args);
		return NULL;
	    }
	    n = PyInt_AsLong(tmp);
	    tmp = PyTuple_New(n);
	    for (j = 0; j < n; j++)
		PyTuple_SetItem(tmp,j, params[i].data.d_stringarray[j] ?
			PyString_FromString(params[i].data.d_stringarray[j]) :
			PyString_FromString(""));
	    PyTuple_SetItem(args, i, tmp);
	    break;
	case PARAM_COLOR:
	    PyTuple_SetItem(args, i, Py_BuildValue("(iii)",
						   (long) params[i].data.d_color.red,
						   (long) params[i].data.d_color.green,
						   (long) params[i].data.d_color.blue));
	    break;
	case PARAM_REGION:
	    PyTuple_SetItem(args, i, Py_BuildValue("(iiii)",
						   (long) params[i].data.d_region.x,
						   (long) params[i].data.d_region.y,
						   (long) params[i].data.d_region.width,
						   (long) params[i].data.d_region.height));
	    break;
	case PARAM_DISPLAY:
	    PyTuple_SetItem(args, i, (PyObject *)
			    newdispobject(
					  params[i].data.d_display));
	    break;
	case PARAM_IMAGE:
	    PyTuple_SetItem(args, i, (PyObject *)
			    newimgobject(params[i].data.d_image));
	    break;
	case PARAM_LAYER:
	    PyTuple_SetItem(args, i, (PyObject *)
			    newlayobject(params[i].data.d_layer));
	    break;
	case PARAM_CHANNEL:
	    PyTuple_SetItem(args, i, (PyObject *)
			    newchnobject(params[i].data.d_channel));
	    break;
	case PARAM_DRAWABLE:
	    PyTuple_SetItem(args, i, (PyObject *)
			    newdrwobject(NULL,
					 params[i].data.d_drawable));
	    break;
	case PARAM_SELECTION:
	    PyTuple_SetItem(args, i, (PyObject *)
			    newlayobject(
					 params[i].data.d_selection));
	    break;
	case PARAM_BOUNDARY:
	    PyTuple_SetItem(args, i, PyInt_FromLong(
						    params[i].data.d_boundary));
	    break;
	case PARAM_PATH:
	    PyTuple_SetItem(args, i, PyInt_FromLong(
						    params[i].data.d_path));
	    break;
#ifdef GIMP_HAVE_PARASITES
	case PARAM_PARASITE:
	    PyTuple_SetItem(args, i,
		(PyObject *)newparaobject(parasite_copy(
					&(params[i].data.d_parasite))));
	    break;
#endif
	case PARAM_STATUS:
	    PyTuple_SetItem(args, i, PyInt_FromLong(
						    params[i].data.d_status));
	    break;
	case PARAM_END:
	    break;
	}
    }
    return args;
}

static GParam *
tuple_to_GParam(PyObject *args, GParamDef *ptype, int nparams) {
    PyObject *tuple, *item, *r, *g, *b, *x, *y, *w, *h;
    GParam *ret;
    int i, j, len;
    gint32 *i32a; gint16 *i16a; gint8 *i8a; gdouble *fa; gchar **sa;

    if (nparams == 0)
	tuple = PyTuple_New(0);
    else if (!PyTuple_Check(args) && nparams == 1)
	tuple = Py_BuildValue("(O)", args);
    else {
	Py_INCREF(args);
	tuple = args;
    }
    if (!PyTuple_Check(tuple)) {
	PyErr_SetString(PyExc_TypeError, "wrong type of parameter");
	return NULL;
    }

    if (PyTuple_Size(tuple) != nparams) {
	PyErr_SetString(PyExc_TypeError, "wrong number of parameters");
	return NULL;
    }

    ret = g_new(GParam, nparams+1);
    for (i = 0; i <= nparams; i++)
	ret[i].type = PARAM_STATUS;
#define check(expr) if (expr) { \
	    PyErr_SetString(PyExc_TypeError, "wrong parameter type"); \
	    Py_DECREF(tuple); \
	    gimp_destroy_params(ret, nparams); \
	    return NULL; \
	}
#define arraycheck(expr, ar) if (expr) { \
	    PyErr_SetString(PyExc_TypeError, "subscript of wrong type"); \
	    Py_DECREF(tuple); \
	    gimp_destroy_params(ret, nparams); \
	    g_free(ar); \
	    return NULL; \
	}
    for (i = 1; i <= nparams; i++) {
	item = PyTuple_GetItem(tuple, i-1);
	switch (ptype[i-1].type) {
	case PARAM_INT32:
	    check((x = PyNumber_Int(item)) == NULL)
		ret[i].data.d_int32 = PyInt_AsLong(x);
	    Py_DECREF(x);
	    break;
	case PARAM_INT16:
	    check((x = PyNumber_Int(item)) == NULL)
		ret[i].data.d_int16 = PyInt_AsLong(x);
	    Py_DECREF(x);
	    break;
	case PARAM_INT8:
	    check((x = PyNumber_Int(item)) == NULL)
		ret[i].data.d_int8 = PyInt_AsLong(x);
	    Py_DECREF(x);
	    break;
	case PARAM_FLOAT:
	    check((x = PyNumber_Float(item)) == NULL)
		ret[i].data.d_float = PyFloat_AsDouble(x);
	    Py_DECREF(x);
	    break;
	case PARAM_STRING:
	    check((x = PyObject_Str(item)) == NULL)
		ret[i].data.d_string = g_strdup(
						PyString_AsString(x));
	    Py_DECREF(x);
	    break;
	case PARAM_INT32ARRAY:
	    check(!PySequence_Check(item))
		len = PySequence_Length(item);
	    i32a = g_new(gint32, len);
	    for (j = 0; j < len; j++) {
		x = PySequence_GetItem(item, j);
		arraycheck((y=PyNumber_Int(x))==NULL,
			   i32a);
		i32a[j] = PyInt_AsLong(y);
		Py_DECREF(y);
	    }
	    ret[i].data.d_int32array = i32a;
	    break;
	case PARAM_INT16ARRAY:
	    check(!PySequence_Check(item))
		len = PySequence_Length(item);
	    i16a = g_new(gint16, len);
	    for (j = 0; j < len; j++) {
		x = PySequence_GetItem(item, j);
		arraycheck((y=PyNumber_Int(x))==NULL,
			   i16a);
		i16a[j] = PyInt_AsLong(y);
		Py_DECREF(y);
	    }
	    ret[i].data.d_int16array = i16a;
	    break;
	case PARAM_INT8ARRAY:
	    check(!PySequence_Check(item))
		len = PySequence_Length(item);
	    i8a = g_new(gint8, len);
	    for (j = 0; j < len; j++) {
		x = PySequence_GetItem(item, j);
		arraycheck((y=PyNumber_Int(x))==NULL,
			   i8a);
		i8a[j] = PyInt_AsLong(y);
		Py_DECREF(y);
	    }
	    ret[i].data.d_int8array = i8a;
	    break;
	case PARAM_FLOATARRAY:
	    check(!PySequence_Check(item))
		len = PySequence_Length(item);
	    fa = g_new(gdouble, len);
	    for (j = 0; j < len; j++) {
		x = PySequence_GetItem(item, j);
		arraycheck((y=PyNumber_Float(x))==NULL,
			   fa);
		fa[j] = PyFloat_AsDouble(y);
		Py_DECREF(y);
	    }
	    ret[i].data.d_floatarray = fa;
	    break;
	case PARAM_STRINGARRAY:
	    check(!PySequence_Check(item))
		len = PySequence_Length(item);
	    sa = g_new(gchar *, len);
	    for (j = 0; j < len; j++) {
		x = PySequence_GetItem(item, j);
		arraycheck((y=PyObject_Str(x))==NULL,
			   sa);
		sa[j] = g_strdup(PyString_AsString(y));
		Py_DECREF(y);
	    }
	    ret[i].data.d_stringarray = sa;
	    break;
	case PARAM_COLOR:
	    check(!PySequence_Check(item) ||
		  PySequence_Length(item) < 3)
		r = PySequence_GetItem(item, 0);
	    g = PySequence_GetItem(item, 1);
	    b = PySequence_GetItem(item, 2);
	    check(!PyInt_Check(r) || !PyInt_Check(g) ||
		  !PyInt_Check(b))
		ret[i].data.d_color.red = PyInt_AsLong(r);
	    ret[i].data.d_color.green = PyInt_AsLong(g);
	    ret[i].data.d_color.blue = PyInt_AsLong(b);
	    break;
	case PARAM_REGION:
	    check(!PySequence_Check(item) ||
		  PySequence_Length(item) < 4)
		x = PySequence_GetItem(item, 0);
	    y = PySequence_GetItem(item, 1);
	    w = PySequence_GetItem(item, 2);
	    h = PySequence_GetItem(item, 3);
	    check(!PyInt_Check(x) || !PyInt_Check(y) ||
		  !PyInt_Check(w) || !PyInt_Check(h))
		ret[i].data.d_region.x = PyInt_AsLong(x);
	    ret[i].data.d_region.y = PyInt_AsLong(y);
	    ret[i].data.d_region.width = PyInt_AsLong(w);
	    ret[i].data.d_region.height = PyInt_AsLong(h);
	    break;
	case PARAM_DISPLAY:
	    check(!disp_check(item))
		ret[i].data.d_display=((dispobject*)item)->ID;
	    break;
	case PARAM_IMAGE:
	    if (item == Py_None) {
		ret[i].data.d_image = -1;
		break;
	    }
	    check(!img_check(item))
		ret[i].data.d_image=((imgobject*)item)->ID;
	    break;
	case PARAM_LAYER:
	    if (item == Py_None) {
		ret[i].data.d_layer = -1;
		break;
	    }
	    check(!lay_check(item))
		ret[i].data.d_layer=((layobject*)item)->ID;
	    break;
	case PARAM_CHANNEL:
	    if (item == Py_None) {
		ret[i].data.d_channel = -1;
		break;
	    }
	    check(!chn_check(item))
		ret[i].data.d_channel=((chnobject*)item)->ID;
	    break;
	case PARAM_DRAWABLE:
	    if (item == Py_None) {
		ret[i].data.d_channel = -1;
		break;
	    }
	    check(!drw_check(item) && !lay_check(item) && !chn_check(item))
		ret[i].data.d_channel=((drwobject*)item)->ID;
	    break;
	case PARAM_SELECTION:
	    check(!lay_check(item))
		ret[i].data.d_selection=((layobject*)item)->ID;
	    break;
	case PARAM_BOUNDARY:
	    check(!PyInt_Check(item))
		ret[i].data.d_boundary = PyInt_AsLong(item);
	    break;
	case PARAM_PATH:
	    check(!PyInt_Check(item))
		ret[i].data.d_path = PyInt_AsLong(item);
	    break;
#ifdef GIMP_HAVE_PARASITES
	case PARAM_PARASITE:
	    /* can't do anything, since size of Parasite is not known */
	    break;
#endif
	case PARAM_STATUS:
	    check(!PyInt_Check(item))
		ret[i].data.d_status = PyInt_AsLong(item);
	    break;
	case PARAM_END:
	    break;
	}
#undef check
#undef arraycheck
	ret[i].type = ptype[i-1].type;
    }

    Py_DECREF(tuple);
    return ret;
}

/* ---------------------------------------------------------------- */
/* generic number conversions.  Most gimp objects contain an ID field
 * at the start.  This code fragment adds conversion of that ID to
 * an integer, hex, oct or float
 */

static PyObject *
gobj_int(self)
     imgobject *self; /* or layobject, drwobject, chnobject ... */
{
    return PyInt_FromLong((long)self->ID);
}

static PyObject *
gobj_long(self)
     imgobject *self;
{
    return PyLong_FromLong((long)self->ID);
}

static PyObject *
gobj_float(self)
     imgobject *self;
{
    return PyFloat_FromDouble((double)self->ID);
}

static PyObject *
gobj_oct(self)
     imgobject *self;
{
    char buf[20];

    if (self->ID == 0)
	strcpy(buf, "0");
    else
	sprintf(buf, "0%lo", (unsigned long)self->ID);
    return PyString_FromString(buf);
}

static PyObject *
gobj_hex(self)
     imgobject *self;
{
    char buf[20];

    sprintf(buf, "0x%lx", (unsigned long)self->ID);
    return PyString_FromString(buf);
}

static PyNumberMethods gobj_as_number = {
    (binaryfunc)0, /*nb_add*/
    (binaryfunc)0, /*nb_subtract*/
    (binaryfunc)0, /*nb_multiply*/
    (binaryfunc)0, /*nb_divide*/
    (binaryfunc)0, /*nb_remainder*/
    (binaryfunc)0, /*nb_divmod*/
    (ternaryfunc)0, /*nb_power*/
    (unaryfunc)0, /*nb_negative*/
    (unaryfunc)0, /*nb_positive*/
    (unaryfunc)0, /*nb_absolute*/
    (inquiry)0, /*nb_nonzero*/
    (unaryfunc)0, /*nb_invert*/
    (binaryfunc)0, /*nb_lshift*/
    (binaryfunc)0, /*nb_rshift*/
    (binaryfunc)0, /*nb_and*/
    (binaryfunc)0, /*nb_xor*/
    (binaryfunc)0, /*nb_or*/
    0,              /*nb_coerce*/
    (unaryfunc)gobj_int, /*nb_int*/
    (unaryfunc)gobj_long, /*nb_long*/
    (unaryfunc)gobj_float, /*nb_float*/
    (unaryfunc)gobj_oct, /*nb_oct*/
    (unaryfunc)gobj_hex, /*nb_hex*/
};

/* ---------------------------------------------------------------- */

static PyObject *
pdb_query(self, args)
     pdbobject *self;
     PyObject *args;
{
    char *n=".*", *b=".*", *h=".*", *a=".*", *c=".*", *d=".*", *t=".*";
    int num, i;
    char **names;
    PyObject *ret;
    if (!PyArg_ParseTuple(args, "|zzzzzzz:pdb.query", &n, &b, &h, &a,
			  &c, &d, &t))
	return NULL;
    gimp_query_database(n, b, h, a, c, d, t, &num, &names);
    ret = PyList_New(num);
    for (i = 0; i < num; i++)
	PyList_SetItem(ret, i, PyString_FromString(names[i]));
    g_free(names);
    return ret;
}

static struct PyMethodDef pdb_methods[] = {
    {"query", (PyCFunction)pdb_query, METH_VARARGS},
    {NULL,		NULL}		/* sentinel */
};

/* ---------- */


static pdbobject *
newpdbobject()
{
    pdbobject *self;
	
    self = PyObject_NEW(pdbobject, &Pdbtype);
    if (self == NULL)
	return NULL;
    /* XXXX Add your own initializers here */
    return self;
}


static void
pdb_dealloc(self)
     pdbobject *self;
{
    /* XXXX Add your own cleanup code here */
    PyMem_DEL(self);
}

static PyObject *
pdb_repr(self)
     pdbobject *self;
{
    return PyString_FromString("<Procedural-Database>");
}

/* Code to access pdb objects as mappings */

static int
pdb_length(self)
     pdbobject *self;
{
    PyErr_SetString(ErrorObject, "Can not size the procedural database.");
    return -1;
}

static PyObject *
pdb_subscript(self, key)
     pdbobject *self;
     PyObject *key;
{
    PyObject *r;

    if (!PyString_Check(key)) {
	PyErr_SetString(PyExc_TypeError, "Subscript must be a string.");
	return NULL;
    }
    r = (PyObject *)newpfobject(PyString_AsString(key));
    if (r == NULL) {
	PyErr_Clear();
	PyErr_SetObject(PyExc_KeyError, key);
    }
    return r;
}

static int
pdb_ass_sub(self, v, w)
     pdbobject *self;
     PyObject *v, *w;
{
    PyErr_SetString(ErrorObject,
		    "Use install_procedure to add to the PDB.");
    return -1;
}

static PyMappingMethods pdb_as_mapping = {
    (inquiry)pdb_length,		/*mp_length*/
    (binaryfunc)pdb_subscript,		/*mp_subscript*/
    (objobjargproc)pdb_ass_sub,	/*mp_ass_subscript*/
};

/* -------------------------------------------------------- */

static PyObject *
pdb_getattr(self, name)
     pdbobject *self;
     char *name;
{
    PyObject *r;

    r = Py_FindMethod(pdb_methods, (PyObject *)self, name);
    if (r == NULL) {
	PyErr_Clear();
	return (PyObject *)newpfobject(name);
    }
    return r;
}

static PyTypeObject Pdbtype = {
    PyObject_HEAD_INIT(&PyType_Type)
    0,				/*ob_size*/
    "pdb",			/*tp_name*/
    sizeof(pdbobject),		/*tp_basicsize*/
    0,				/*tp_itemsize*/
    /* methods */
    (destructor)pdb_dealloc,	/*tp_dealloc*/
    (printfunc)0,		/*tp_print*/
    (getattrfunc)pdb_getattr,	/*tp_getattr*/
    (setattrfunc)0,	/*tp_setattr*/
    (cmpfunc)0,		/*tp_compare*/
    (reprfunc)pdb_repr,		/*tp_repr*/
    0,			/*tp_as_number*/
    0,		/*tp_as_sequence*/
    &pdb_as_mapping,		/*tp_as_mapping*/
    (hashfunc)0,		/*tp_hash*/
    (ternaryfunc)0,		/*tp_call*/
    (reprfunc)0,		/*tp_str*/

    /* Space for future expansion */
    0L,0L,0L,0L,
    NULL /* Documentation string */
};

/* End of code for pdb objects */
/* -------------------------------------------------------- */


static pfobject *
newpfobject(name)
     char *name;
{
    pfobject *self;
    char *b,*h,*a,*c,*d;
    int pt, np, nr, i;
    GParamDef *p, *r;

    if (!gimp_query_procedure(name, &b, &h, &a, &c, &d, &pt,
			      &np, &nr, &p, &r)) {
	PyErr_SetString(ErrorObject, "procedure not found.");
	return NULL;
    }

    self = PyObject_NEW(pfobject, &Pftype);
    if (self == NULL)
	return NULL;

    self->name = g_strdup(name);
    self->proc_name = PyString_FromString(name);
    self->proc_blurb = PyString_FromString(b);
    self->proc_help = PyString_FromString(h);
    self->proc_author = PyString_FromString(a);
    self->proc_copyright = PyString_FromString(c);
    self->proc_date = PyString_FromString(d);
    self->proc_type = PyInt_FromLong(pt);
    self->nparams = np;
    self->nreturn_vals = nr;
    self->params = p;
    self->return_vals = r;

    self->py_params = PyTuple_New(np);
    for (i = 0; i < np; i++)
	PyTuple_SetItem(self->py_params, i,
			Py_BuildValue("(iss)", p[i].type, p[i].name,
				      p[i].description));
    self->py_return_vals = PyTuple_New(nr);
    for (i = 0; i < nr; i++)
	PyTuple_SetItem(self->py_return_vals, i,
			Py_BuildValue("(iss)", r[i].type, r[i].name,
				      r[i].description));

    g_free(b); g_free(h); g_free(a); g_free(c); g_free(d);
	
    return self;
}

#ifndef GIMP_HAVE_DESTROY_PARAMDEFS
static void
gimp_destroy_paramdefs (GParamDef *paramdefs,
                        int        nparams)
{
    while (nparams--) {
      g_free (paramdefs[nparams].name);
      g_free (paramdefs[nparams].description);
    }
  
    g_free (paramdefs);
}
#endif
static void
pf_dealloc(self)
     pfobject *self;
{
    g_free(self->name);
    Py_DECREF(self->proc_name);
    Py_DECREF(self->proc_blurb);
    Py_DECREF(self->proc_help);
    Py_DECREF(self->proc_author);
    Py_DECREF(self->proc_copyright);
    Py_DECREF(self->proc_date);
    Py_DECREF(self->proc_type);
    Py_DECREF(self->py_params);
    Py_DECREF(self->py_return_vals);
    gimp_destroy_paramdefs(self->params, self->nparams);
    gimp_destroy_paramdefs(self->return_vals, self->nreturn_vals);
    PyMem_DEL(self);
}
#define OFF(x) offsetof(pfobject, x)

static struct memberlist pf_memberlist[] = {
    {"proc_name",      T_OBJECT, OFF(proc_name),      RO},
    {"proc_blurb",     T_OBJECT, OFF(proc_blurb),     RO},
    {"proc_help",      T_OBJECT, OFF(proc_help),      RO},
    {"proc_author",    T_OBJECT, OFF(proc_author),    RO},
    {"proc_copyright", T_OBJECT, OFF(proc_copyright), RO},
    {"proc_date",      T_OBJECT, OFF(proc_date),      RO},
    {"proc_type",      T_OBJECT, OFF(proc_type),      RO},
    {"nparams",        T_INT,    OFF(nparams),        RO},
    {"nreturn_vals",   T_INT,    OFF(nreturn_vals),   RO},
    {"params",         T_OBJECT, OFF(py_params),      RO},
    {"return_vals",    T_OBJECT, OFF(py_return_vals), RO},
    {NULL}  /* Sentinel */
};

#undef OFF

static PyObject *
pf_getattr(self, name)
     pfobject *self;
     char *name;
{
    return PyMember_Get((char *)self, pf_memberlist, name);
}

static PyObject *
pf_repr(self)
     pfobject *self;
{
    PyObject *s;

    s = PyString_FromString("<pdb function ");
    PyString_Concat(&s, self->proc_name);
    PyString_ConcatAndDel(&s, PyString_FromString(">"));
    return s;
}

static PyObject *
pf_call(self, args, kwargs)
     pfobject *self;
     PyObject *args;
     PyObject *kwargs;
{
    GParam *params, *ret;
    int nret;
    PyObject *t = NULL, *r;

#if PG_DEBUG > 0
    fprintf(stderr, "--- %s --- ", PyString_AsString(self->proc_name));
#endif
    if (self->nparams > 0 && !strcmp(self->params[0].name, "run_mode")) {
	params = tuple_to_GParam(args, self->params+1, self->nparams-1);
	if (params == NULL)
	    return NULL;
	params[0].type = self->params[0].type;
	params[0].data.d_int32 = RUN_NONINTERACTIVE;
#if PG_DEBUG > 1
	print_GParam(self->nparams, params);
#endif
	ret = gimp_run_procedure2(self->name, &nret, self->nparams,
				  params);
    } else {
	params = tuple_to_GParam(args, self->params, self->nparams);
	if (params == NULL)
	    return NULL;
#if PG_DEBUG > 1
	print_GParam(self->nparams, params+1);
#endif
	ret = gimp_run_procedure2(self->name, &nret, self->nparams,
				  params+1);
    }
    gimp_destroy_params(params, self->nparams);
    if (!ret) {
	PyErr_SetString(ErrorObject, "no status returned");
#if PG_DEBUG >= 1
	fprintf(stderr, "ret == NULL\n");
#endif
	return NULL;
    }
    switch(ret[0].data.d_status) {
    case STATUS_EXECUTION_ERROR:
#if PG_DEBUG > 0
	fprintf(stderr, "execution error\n");
#endif
	gimp_destroy_params(ret, nret);
	PyErr_SetString(PyExc_RuntimeError, "execution error");
	return NULL;
	break;
    case STATUS_CALLING_ERROR:
#if PG_DEBUG > 0
	fprintf(stderr, "calling error\n");
#endif
	gimp_destroy_params(ret, nret);
	PyErr_SetString(PyExc_TypeError, "invalid arguments");
	return NULL;
	break;
    case STATUS_SUCCESS:
#if PG_DEBUG > 0
	fprintf(stderr, "success\n");
#endif
	t = GParam_to_tuple(nret-1, ret+1);
	gimp_destroy_params(ret, nret);
	if (t == NULL) {
	    PyErr_SetString(ErrorObject,
			    "couldn't make return value");
	    return NULL;
	}
	break;
    default:
#if PG_DEBUG > 0
	fprintf(stderr, "unknown - %i (type %i)\n",
		ret[0].data.d_status, ret[0].type);
#endif
	PyErr_SetString(ErrorObject, "unknown return code.");
	return NULL;
	break;
    }
    if (PyTuple_Size(t) == 1) {
	r = PyTuple_GetItem(t, 0);
	Py_INCREF(r);
	Py_DECREF(t);
	return r;
    }
    if (PyTuple_Size(t) == 0) {
	r = Py_None;
	Py_INCREF(r);
	Py_DECREF(t);
	return r;
    }
    return t;
}


static PyTypeObject Pftype = {
    PyObject_HEAD_INIT(&PyType_Type)
    0,				/*ob_size*/
    "pdbFunc",			/*tp_name*/
    sizeof(pfobject),		/*tp_basicsize*/
    0,				/*tp_itemsize*/
    /* methods */
    (destructor)pf_dealloc,	/*tp_dealloc*/
    (printfunc)0,		/*tp_print*/
    (getattrfunc)pf_getattr,	/*tp_getattr*/
    (setattrfunc)0,	/*tp_setattr*/
    (cmpfunc)0,		/*tp_compare*/
    (reprfunc)pf_repr,		/*tp_repr*/
    0,			/*tp_as_number*/
    0,		/*tp_as_sequence*/
    0,		/*tp_as_mapping*/
    (hashfunc)0,		/*tp_hash*/
    (ternaryfunc)pf_call,		/*tp_call*/
    (reprfunc)0,		/*tp_str*/

    /* Space for future expansion */
    0L,0L,0L,0L,
    NULL /* Documentation string */
};

/* End of code for pdbFunc objects */
/* -------------------------------------------------------- */


static PyObject *
img_add_channel(self, args)
     imgobject *self;
     PyObject *args;
{
    chnobject *chn;
    int pos;
	
    if (!PyArg_ParseTuple(args, "O!i:add_channel", &Chntype, &chn, &pos))
	return NULL;
    gimp_image_add_channel(self->ID, chn->ID, pos);
    Py_INCREF(Py_None);
    return Py_None;
}


static PyObject *
img_add_layer(self, args)
     imgobject *self;
     PyObject *args;
{
    layobject *lay;
    int pos;
	
    if (!PyArg_ParseTuple(args, "O!i:add_layer", &Laytype, &lay, &pos))
	return NULL;
    gimp_image_add_layer(self->ID, lay->ID, pos);
    Py_INCREF(Py_None);
    return Py_None;
}


static PyObject *
img_add_layer_mask(self, args)
     imgobject *self;
     PyObject *args;
{
    layobject *lay;
    chnobject *mask;
    if (!PyArg_ParseTuple(args, "O!O!:add_layer_mask", &Laytype, &lay,
			  &Chntype, &mask))
	return NULL;
    gimp_image_add_layer_mask(self->ID, lay->ID, mask->ID);
    Py_INCREF(Py_None);
    return Py_None;
}


static PyObject *
img_clean_all(self, args)
     imgobject *self;
     PyObject *args;
{
    if (!PyArg_ParseTuple(args, ":clean_all"))
	return NULL;
    gimp_image_clean_all(self->ID);
    Py_INCREF(Py_None);
    return Py_None;
}


static PyObject *
img_disable_undo(self, args)
     imgobject *self;
     PyObject *args;
{
    /*GParam *return_vals;
    int nreturn_vals;*/

    if (!PyArg_ParseTuple(args, ":disable_undo"))
	return NULL;
    /*return_vals = gimp_run_procedure("gimp_undo_push_group_start",
				     &nreturn_vals, PARAM_IMAGE, self->ID,
				     PARAM_END);
    gimp_destroy_params(return_vals, nreturn_vals);*/
    gimp_image_undo_disable(self->ID);
    Py_INCREF(Py_None);
    return Py_None;
}


static PyObject *
img_enable_undo(self, args)
     imgobject *self;
     PyObject *args;
{
    /*GParam *return_vals;
    int nreturn_vals;*/

    if (!PyArg_ParseTuple(args, ":enable_undo"))
	return NULL;
    /*return_vals = gimp_run_procedure("gimp_undo_push_group_start",
				     &nreturn_vals, PARAM_IMAGE, self->ID,
				     PARAM_END);
    gimp_destroy_params(return_vals, nreturn_vals);*/
    gimp_image_undo_enable(self->ID);
    Py_INCREF(Py_None);
    return Py_None;
}


static PyObject *
img_flatten(self, args)
     imgobject *self;
     PyObject *args;
{
    if (!PyArg_ParseTuple(args, ":flatten"))
	return NULL;
    gimp_image_flatten(self->ID);
    Py_INCREF(Py_None);
    return Py_None;
}


static PyObject *
img_lower_channel(self, args)
     imgobject *self;
     PyObject *args;
{
    chnobject *chn;
    if (!PyArg_ParseTuple(args, "O!:lower_channel", &Chntype, &chn))
	return NULL;
    gimp_image_lower_channel(self->ID, chn->ID);
    Py_INCREF(Py_None);
    return Py_None;
}


static PyObject *
img_lower_layer(self, args)
     imgobject *self;
     PyObject *args;
{
    layobject *lay;
    if (!PyArg_ParseTuple(args, "O!:lower_layer", &Laytype, &lay))
	return NULL;
    gimp_image_lower_layer(self->ID, lay->ID);
    Py_INCREF(Py_None);
    return Py_None;
}


static PyObject *
img_merge_visible_layers(self, args)
     imgobject *self;
     PyObject *args;
{
    gint32 id;
    int merge;
    if (!PyArg_ParseTuple(args, "i:merge_visible_layers", &merge))
	return NULL;
    id = gimp_image_merge_visible_layers(self->ID, merge);
    if (id == -1) {
	PyErr_SetString(ErrorObject, "Can't merge layers.");
	return NULL;
    }
    return (PyObject *)newlayobject(id);
}


static PyObject *
img_pick_correlate_layer(self, args)
     imgobject *self;
     PyObject *args;
{
    int x,y;
    gint32 id;
	
    if (!PyArg_ParseTuple(args, "ii:pick_correlate_layer", &x, &y))
	return NULL;
    id = gimp_image_pick_correlate_layer(self->ID, x, y);
    if (id == -1) {
	Py_INCREF(Py_None);
	return Py_None;
    }
    return (PyObject *)newlayobject(id);
}


static PyObject *
img_raise_channel(self, args)
     imgobject *self;
     PyObject *args;
{
    chnobject *chn;
	
    if (!PyArg_ParseTuple(args, "O!:raise_channel", &Chntype, &chn))
	return NULL;
    gimp_image_raise_channel(self->ID, chn->ID);
    Py_INCREF(Py_None);
    return Py_None;
}


static PyObject *
img_raise_layer(self, args)
     imgobject *self;
     PyObject *args;
{
    layobject *lay;
    if (!PyArg_ParseTuple(args, "O!:raise_layer", &Laytype, &lay))
	return NULL;
    gimp_image_raise_layer(self->ID, lay->ID);
    Py_INCREF(Py_None);
    return Py_None;
}


static PyObject *
img_remove_channel(self, args)
     imgobject *self;
     PyObject *args;
{
    chnobject *chn;
    if (!PyArg_ParseTuple(args, "O!:remove_channel", &Chntype, &chn))
	return NULL;
    gimp_image_remove_channel(self->ID, chn->ID);
    Py_INCREF(Py_None);
    return Py_None;
}


static PyObject *
img_remove_layer(self, args)
     imgobject *self;
     PyObject *args;
{
    layobject *lay;
    if (!PyArg_ParseTuple(args, "O!:remove_layer", &Laytype, &lay))
	return NULL;
    gimp_image_remove_layer(self->ID, lay->ID);
    Py_INCREF(Py_None);
    return Py_None;
}


static PyObject *
img_remove_layer_mask(self, args)
     imgobject *self;
     PyObject *args;
{
    layobject *lay;
    int mode;
    if (!PyArg_ParseTuple(args, "O!i:remove_layer_mask", &Laytype, &lay,
			  &mode))
	return NULL;
    gimp_image_remove_layer_mask(self->ID, lay->ID, mode);
    Py_INCREF(Py_None);
    return Py_None;
}


static PyObject *
img_resize(self, args)
     imgobject *self;
     PyObject *args;
{
    unsigned int new_w, new_h;
    int offs_x, offs_y;
    if (!PyArg_ParseTuple(args, "iiii:resize", &new_w, &new_h,
			  &offs_x, &offs_y))
	return NULL;
    gimp_image_resize(self->ID, new_w, new_h, offs_x, offs_y);
    Py_INCREF(Py_None);
    return Py_None;
}


static PyObject *
img_get_component_active(self, args)
     imgobject *self;
     PyObject *args;
{
    int comp;
    if (!PyArg_ParseTuple(args, "i:get_component_active", &comp))
	return NULL;
    return PyInt_FromLong((long)
			  gimp_image_get_component_active(self->ID, comp));
}


static PyObject *
img_get_component_visible(self, args)
     imgobject *self;
     PyObject *args;
{
    int comp;
	
    if (!PyArg_ParseTuple(args, "i:get_component_visible", &comp))
	return NULL;
	
    return PyInt_FromLong((long)
			  gimp_image_get_component_visible(self->ID, comp));
}


static PyObject *
img_set_component_active(self, args)
     imgobject *self;
     PyObject *args;
{
    int comp, a;
    if (!PyArg_ParseTuple(args, "ii:set_component_active", &comp, &a))
	return NULL;
    gimp_image_set_component_active(self->ID, comp, a);
    Py_INCREF(Py_None);
    return Py_None;
}


static PyObject *
img_set_component_visible(self, args)
     imgobject *self;
     PyObject *args;
{
    int comp, v;
    if (!PyArg_ParseTuple(args, "ii:set_component_visible", &comp, &v))
	return NULL;
    gimp_image_set_component_visible(self->ID, comp, v);
    Py_INCREF(Py_None);
    return Py_None;
}

#ifdef GIMP_HAVE_PARASITES
static PyObject *
img_parasite_find(self, args)
     imgobject *self;
     PyObject *args;
{
    char *name;
    if (!PyArg_ParseTuple(args, "s:parasite_find", &name))
	return NULL;
    return (PyObject *)newparaobject(gimp_image_parasite_find(self->ID, name));
}

static PyObject *
img_parasite_attach(self, args)
     imgobject *self;
     PyObject *args;
{
    paraobject *parasite;
    if (!PyArg_ParseTuple(args, "O!:parasite_attach", &Paratype, &parasite))
	return NULL;
    gimp_image_parasite_attach(self->ID, parasite->para);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
img_attach_new_parasite(self, args)
     imgobject *self;
     PyObject *args;
{
    char *name, *data;
    int flags, size;
    if (!PyArg_ParseTuple(args, "sis#:attach_new_parasite", &name, &flags,
			  &data, &size))
	return NULL;
    gimp_image_attach_new_parasite(self->ID, name, flags, size, data);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
img_parasite_detach(self, args)
     imgobject *self;
     PyObject *args;
{
    char *name;
    if (!PyArg_ParseTuple(args, "s:parasite_detach", &name))
	return NULL;
    gimp_image_parasite_detach(self->ID, name);
    Py_INCREF(Py_None);
    return Py_None;
}

#endif
#ifdef GIMP_HAVE_RESOLUTION_INFO
/* gimp_image_set_resolution
 * gimp_image_get_resolution
 * gimp_set_unit
 * gimp_get_unit
 */
#endif
#if GIMP_CHECK_VERSION(1,1,0)
static PyObject *
img_get_layer_by_tattoo(self, args)
     imgobject *self;
     PyObject *args;
{
    int tattoo;
    if (!PyArg_ParseTuple(args, "i:get_layer_by_tattoo", &tattoo))
	return NULL;
    return (PyObject *)newlayobject(gimp_image_get_layer_by_tattoo(self->ID,
								   tattoo));
}

static PyObject *
img_get_channel_by_tattoo(self, args)
     imgobject *self;
     PyObject *args;
{
    int tattoo;
    if (!PyArg_ParseTuple(args, "i:get_channel_by_tattoo", &tattoo))
	return NULL;
    return (PyObject *)newchnobject(gimp_image_get_channel_by_tattoo(self->ID,
								     tattoo));
}

static PyObject *
img_add_hguide(self, args)
     imgobject *self;
     PyObject *args;
{
    int ypos;
    if (!PyArg_ParseTuple(args, "i:add_hguide", &ypos))
	return NULL;
    gimp_image_add_hguide(self->ID, ypos);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
img_add_vguide(self, args)
     imgobject *self;
     PyObject *args;
{
    int xpos;
    if (!PyArg_ParseTuple(args, "i:add_vguide", &xpos))
	return NULL;
    gimp_image_add_vguide(self->ID, xpos);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
img_delete_guide(self, args)
     imgobject *self;
     PyObject *args;
{
    int guide;
    if (!PyArg_ParseTuple(args, "i:delete_guide", &guide))
	return NULL;
    gimp_image_delete_guide(self->ID, guide);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
img_find_next_guide(self, args)
     imgobject *self;
     PyObject *args;
{
    int guide;
    if (!PyArg_ParseTuple(args, "i:find_next_guide", &guide))
	return NULL;
    return PyInt_FromLong(gimp_image_find_next_guide(self->ID, guide));
}

static PyObject *
img_get_guide_orientation(self, args)
     imgobject *self;
     PyObject *args;
{
    int guide;
    if (!PyArg_ParseTuple(args, "i:get_guide_orientation", &guide))
	return NULL;
    return PyInt_FromLong(gimp_image_get_guide_orientation(self->ID, guide));
}

static PyObject *
img_get_guide_position(self, args)
     imgobject *self;
     PyObject *args;
{
    int guide;
    if (!PyArg_ParseTuple(args, "i:get_guide_position", &guide))
	return NULL;
    return PyInt_FromLong(gimp_image_get_guide_position(self->ID, guide));
}
#endif

static struct PyMethodDef img_methods[] = {
    {"add_channel",	(PyCFunction)img_add_channel,	METH_VARARGS},
    {"add_layer",	(PyCFunction)img_add_layer,	METH_VARARGS},
    {"add_layer_mask",	(PyCFunction)img_add_layer_mask,	METH_VARARGS},
    {"clean_all",	(PyCFunction)img_clean_all,	METH_VARARGS},
    {"disable_undo",	(PyCFunction)img_disable_undo,	METH_VARARGS},
    {"enable_undo",	(PyCFunction)img_enable_undo,	METH_VARARGS},
    {"flatten",	(PyCFunction)img_flatten,	METH_VARARGS},
    {"lower_channel",	(PyCFunction)img_lower_channel,	METH_VARARGS},
    {"lower_layer",	(PyCFunction)img_lower_layer,	METH_VARARGS},
    {"merge_visible_layers",	(PyCFunction)img_merge_visible_layers,	METH_VARARGS},
    {"pick_correlate_layer",	(PyCFunction)img_pick_correlate_layer,	METH_VARARGS},
    {"raise_channel",	(PyCFunction)img_raise_channel,	METH_VARARGS},
    {"raise_layer",	(PyCFunction)img_raise_layer,	METH_VARARGS},
    {"remove_channel",	(PyCFunction)img_remove_channel,	METH_VARARGS},
    {"remove_layer",	(PyCFunction)img_remove_layer,	METH_VARARGS},
    {"remove_layer_mask",	(PyCFunction)img_remove_layer_mask,	METH_VARARGS},
    {"resize",	(PyCFunction)img_resize,	METH_VARARGS},
    {"get_component_active",	(PyCFunction)img_get_component_active,	METH_VARARGS},
    {"get_component_visible",	(PyCFunction)img_get_component_visible,	METH_VARARGS},
    {"set_component_active",	(PyCFunction)img_set_component_active,	METH_VARARGS},
    {"set_component_visible",	(PyCFunction)img_set_component_visible,	METH_VARARGS},
#ifdef GIMP_HAVE_PARASITES
    {"parasite_find",       (PyCFunction)img_parasite_find,      METH_VARARGS},
    {"parasite_attach",     (PyCFunction)img_parasite_attach,    METH_VARARGS},
    {"attach_new_parasite", (PyCFunction)img_attach_new_parasite,METH_VARARGS},
    {"parasite_detach",     (PyCFunction)img_parasite_detach,    METH_VARARGS},
#endif
#if GIMP_CHECK_VERSION(1,1,0)
    {"get_layer_by_tattoo",(PyCFunction)img_get_layer_by_tattoo,METH_VARARGS},
    {"get_channel_by_tattoo",(PyCFunction)img_get_channel_by_tattoo,METH_VARARGS},
    {"add_hguide", (PyCFunction)img_add_hguide, METH_VARARGS},
    {"add_vguide", (PyCFunction)img_add_vguide, METH_VARARGS},
    {"delete_guide", (PyCFunction)img_delete_guide, METH_VARARGS},
    {"find_next_guide", (PyCFunction)img_find_next_guide, METH_VARARGS},
    {"get_guide_orientation",(PyCFunction)img_get_guide_orientation,METH_VARARGS},
    {"get_guide_position", (PyCFunction)img_get_guide_position, METH_VARARGS},
#endif
    {NULL,		NULL}		/* sentinel */
};

/* ---------- */


static imgobject *
newimgobject(gint32 ID)
{
    imgobject *self;

    if (ID == -1) {
	Py_INCREF(Py_None);
	return (imgobject *)Py_None;
    }
    self = PyObject_NEW(imgobject, &Imgtype);
    if (self == NULL)
	return NULL;
    self->ID = ID;
    return self;
}


static void
img_dealloc(self)
     imgobject *self;
{
    /* XXXX Add your own cleanup code here */
    PyMem_DEL(self);
}

static PyObject *
img_getattr(self, name)
     imgobject *self;
     char *name;
{
    gint32 *a, id;
    guchar *c;
    int n, i;
    PyObject *ret;
    if (!strcmp(name, "__members__"))
	return Py_BuildValue(
#ifdef GIMP_HAVE_RESOLUTION_INFO
			     "[ssssssssssss]",
#else
			     "[ssssssssss]",
#endif
			     "ID", "active_channel",
			     "active_layer", "base_type", "channels", "cmap",
			     "filename", "floating_selection",
			     "height", "layers",
#ifdef GIMP_HAVE_RESOLUTION_INFO
			     "resolution",
#endif
			     "selection",
#ifdef GIMP_HAVE_RESOLUTION_INFO
			     "unit",
#endif
			     "width");
    if (!strcmp(name, "ID"))
	return PyInt_FromLong(self->ID);
    if (!strcmp(name, "active_channel")) {
	id = gimp_image_get_active_channel(self->ID);
	if (id == -1) {
	    Py_INCREF(Py_None);
	    return Py_None;
	}
	return (PyObject *)newchnobject(id);
    }
    if (!strcmp(name, "active_layer")) {
	id = gimp_image_get_active_layer(self->ID);
	if (id == -1) {
	    Py_INCREF(Py_None);
	    return Py_None;
	}
	return (PyObject *)newlayobject(id);
    }
    if (!strcmp(name, "base_type"))
	return PyInt_FromLong(gimp_image_base_type(self->ID));
    if (!strcmp(name, "channels")) {
	a = gimp_image_get_channels(self->ID, &n);
	ret = PyList_New(n);
	for (i = 0; i < n; i++)
	    PyList_SetItem(ret, i, (PyObject *)newchnobject(a[i]));
	return ret;
    }
    if (!strcmp(name, "cmap")) {
	c = gimp_image_get_cmap(self->ID, &n);
	return PyString_FromStringAndSize(c, n * 3);
    }
    if (!strcmp(name, "filename"))
	return PyString_FromString(gimp_image_get_filename(self->ID));
    if (!strcmp(name, "floating_selection")) {
	id = gimp_image_floating_selection(self->ID);
	if (id == -1) {
	    Py_INCREF(Py_None);
	    return Py_None;
	}
	return (PyObject *)newlayobject(id);
    }
    if (!strcmp(name, "layers")) {
	a = gimp_image_get_layers(self->ID, &n);
	ret = PyList_New(n);
	for (i = 0; i < n; i++)
	    PyList_SetItem(ret, i, (PyObject *)newlayobject(a[i]));
	return ret;
    }
    if (!strcmp(name, "selection"))
	return (PyObject *)newchnobject(
					gimp_image_get_selection(self->ID));
    if (!strcmp(name, "height"))
	return PyInt_FromLong(gimp_image_height(self->ID));
    if (!strcmp(name, "width"))
	return PyInt_FromLong(gimp_image_width(self->ID));

#ifdef GIMP_HAVE_RESOLUTION_INFO
    if (!strcmp(name, "resolution")) {
	double xres, yres;
	gimp_image_get_resolution(self->ID, &xres, &yres);
	return Py_BuildValue("(dd)", xres, yres);
    }
    if (!strcmp(name, "unit"))
	return PyInt_FromLong(gimp_image_get_unit(self->ID));
#endif
		
    return Py_FindMethod(img_methods, (PyObject *)self, name);
}

static int
img_setattr(self, name, v)
     imgobject *self;
     char *name;
     PyObject *v;
{
    if (v == NULL) {
	PyErr_SetString(PyExc_TypeError, "can not delete attributes.");
	return -1;
    }
    if (!strcmp(name, "active_channel")) {
	if (!chn_check(v)) {
	    PyErr_SetString(PyExc_TypeError, "type mis-match.");
	    return -1;
	}
	gimp_image_set_active_channel(self->ID, ((chnobject *)v)->ID);
	return 0;
    }
    if (!strcmp(name, "active_layer")) {
	if (!lay_check(v)) {
	    PyErr_SetString(PyExc_TypeError, "type mis-match.");
	    return -1;
	}
	gimp_image_set_active_layer(self->ID, ((layobject *)v)->ID);
	return 0;
    }
    if (!strcmp(name, "cmap")) {
	if (!PyString_Check(v)) {
	    PyErr_SetString(PyExc_TypeError, "type mis-match.");
	    return -1;
	}
	gimp_image_set_cmap(self->ID, PyString_AsString(v),
			    PyString_Size(v) / 3);
	return 0;
    }
    if (!strcmp(name, "filename")) {
	if (!PyString_Check(v)) {
	    PyErr_SetString(PyExc_TypeError, "type mis-match.");
	    return -1;
	}
	gimp_image_set_filename(self->ID, PyString_AsString(v));
	return 0;
    }
#ifdef GIMP_HAVE_RESOLUTION_INFO
    if (!strcmp(name, "resolution")) {
	PyObject *xo, *yo;
	if (!PySequence_Check(v) ||
	    !PyFloat_Check(xo = PySequence_GetItem(v, 0)) ||
	    !PyFloat_Check(yo = PySequence_GetItem(v, 1))) {
	    PyErr_SetString(PyExc_TypeError, "type mis-match.");
	    return -1;
	}
	gimp_image_set_resolution(self->ID, PyFloat_AsDouble(xo),
				  PyFloat_AsDouble(yo));
    }
    if (!strcmp(name, "unit")) {
	if (!PyInt_Check(v)) {
	    PyErr_SetString(PyExc_TypeError, "type mis-match.");
	    return -1;
	}
	gimp_image_set_unit(self->ID, PyInt_AsLong(v));
    }
#endif
    if (!strcmp(name, "channels") || !strcmp(name, "layers") ||
	!strcmp(name, "selection") || !strcmp(name, "height") ||
	!strcmp(name, "base_type") || !strcmp(name, "width") ||
	!strcmp(name, "floating_selection") || !strcmp(name, "ID")) {
	PyErr_SetString(PyExc_TypeError, "read-only attribute.");
	return -1;
    }
		
    return -1;
}

static PyObject *
img_repr(self)
     imgobject *self;
{
    PyObject *s;

    s = PyString_FromString("<image ");
    PyString_ConcatAndDel(&s, PyString_FromString(
						  gimp_image_get_filename(self->ID)));
    PyString_ConcatAndDel(&s, PyString_FromString(">"));
    return s;
}

static int
img_cmp(self, other)
     imgobject *self, *other;
{
    return self->ID - other->ID;
}

static PyTypeObject Imgtype = {
    PyObject_HEAD_INIT(&PyType_Type)
    0,				/*ob_size*/
    "Image",			/*tp_name*/
    sizeof(imgobject),		/*tp_basicsize*/
    0,				/*tp_itemsize*/
    /* methods */
    (destructor)img_dealloc,	/*tp_dealloc*/
    (printfunc)0,		/*tp_print*/
    (getattrfunc)img_getattr,	/*tp_getattr*/
    (setattrfunc)img_setattr,	/*tp_setattr*/
    (cmpfunc)img_cmp,		/*tp_compare*/
    (reprfunc)img_repr,		/*tp_repr*/
    &gobj_as_number,			/*tp_as_number*/
    0,		/*tp_as_sequence*/
    0,		/*tp_as_mapping*/
    (hashfunc)0,		/*tp_hash*/
    (ternaryfunc)0,		/*tp_call*/
    (reprfunc)0,		/*tp_str*/

    /* Space for future expansion */
    0L,0L,0L,0L,
    NULL /* Documentation string */
};

/* End of code for Image objects */
/* -------------------------------------------------------- */


static struct PyMethodDef disp_methods[] = {
 
    {NULL,		NULL}		/* sentinel */
};

/* ---------- */


static dispobject *
newdispobject(gint32 ID)
{
    dispobject *self;
	
    self = PyObject_NEW(dispobject, &Disptype);
    if (self == NULL)
	return NULL;
    self->ID = ID;
    return self;
}


static void
disp_dealloc(self)
     dispobject *self;
{
    PyMem_DEL(self);
}

static PyObject *
disp_getattr(self, name)
     dispobject *self;
     char *name;
{
    return Py_FindMethod(disp_methods, (PyObject *)self, name);
}

static PyObject *
disp_repr(self)
     dispobject *self;
{
    PyObject *s;

    s = PyString_FromString("<display>");
    return s;
}

static PyTypeObject Disptype = {
    PyObject_HEAD_INIT(&PyType_Type)
    0,				/*ob_size*/
    "Display",			/*tp_name*/
    sizeof(dispobject),		/*tp_basicsize*/
    0,				/*tp_itemsize*/
    /* methods */
    (destructor)disp_dealloc,	/*tp_dealloc*/
    (printfunc)0,		/*tp_print*/
    (getattrfunc)disp_getattr,	/*tp_getattr*/
    (setattrfunc)0,	/*tp_setattr*/
    (cmpfunc)0,		/*tp_compare*/
    (reprfunc)disp_repr,		/*tp_repr*/
    0,			/*tp_as_number*/
    0,		/*tp_as_sequence*/
    0,		/*tp_as_mapping*/
    (hashfunc)0,		/*tp_hash*/
    (ternaryfunc)0,		/*tp_call*/
    (reprfunc)0,		/*tp_str*/

    /* Space for future expansion */
    0L,0L,0L,0L,
    NULL /* Documentation string */
};

/* End of code for Display objects */
/* -------------------------------------------------------- */


static void
ensure_drawable(self)
     drwobject *self;
{
    if (!self->drawable)
	self->drawable = gimp_drawable_get(self->ID);
}

static PyObject *
drw_flush(self, args)
     drwobject *self;
     PyObject *args;
{
    if (!PyArg_ParseTuple(args, ":flush"))
	return NULL;
    ensure_drawable(self);
    gimp_drawable_flush(self->drawable);
    Py_INCREF(Py_None);
    return Py_None;
}


static PyObject *
drw_update(self, args)
     drwobject *self;
     PyObject *args;
{
    int x, y;
    unsigned int w, h;
    if (!PyArg_ParseTuple(args, "iiii:update", &x, &y, &w, &h))
	return NULL;
    gimp_drawable_update(self->ID, x, y, w, h);
    Py_INCREF(Py_None);
    return Py_None;
}


static PyObject *
drw_merge_shadow(self, args)
     drwobject *self;
     PyObject *args;
{
    int u;
    if (!PyArg_ParseTuple(args, "i:merge_shadow", &u))
	return NULL;
    gimp_drawable_merge_shadow(self->ID, u);
    Py_INCREF(Py_None);
    return Py_None;
}


static PyObject *
drw_fill(self, args)
     drwobject *self;
     PyObject *args;
{
    int f;
    if (!PyArg_ParseTuple(args, "i:fill", &f))
	return NULL;
    gimp_drawable_fill(self->ID, f);
    Py_INCREF(Py_None);
    return Py_None;
}


static PyObject *
drw_get_tile(self, args)
     drwobject *self;
     PyObject *args;
{
    GTile *t;
    int shadow, r, c;
    if (!PyArg_ParseTuple(args, "iii:get_tile", &shadow, &r, &c))
	return NULL;
    ensure_drawable(self);
    t = gimp_drawable_get_tile(self->drawable, shadow, r, c);
    return (PyObject *)newtileobject(t, self);
}

static PyObject *
drw_get_tile2(self, args)
     drwobject *self;
     PyObject *args;
{
    GTile *t;
    int shadow, x, y;
    if (!PyArg_ParseTuple(args, "iii:get_tile2", &shadow, &x ,&y))
	return NULL;
    ensure_drawable(self);
    t = gimp_drawable_get_tile2(self->drawable, shadow, x, y);
    return (PyObject *)newtileobject(t, self);
}

static PyObject *
drw_get_pixel_rgn(self, args)
     drwobject *self;
     PyObject *args;
{
    int x, y, w, h, dirty = 1, shadow = 0;
    if (!PyArg_ParseTuple(args, "iiii|ii:get_pixel_rgn", &x,&y,
			  &w,&h, &dirty,&shadow))
	return NULL;
    ensure_drawable(self);
    return (PyObject *)newprobject(self, x, y, w, h,
				   dirty, shadow);
}

#ifdef GIMP_HAVE_PARASITES
static PyObject *
drw_parasite_find(self, args)
     drwobject *self;
     PyObject *args;
{
    char *name;
    if (!PyArg_ParseTuple(args, "s:parasite_find", &name))
	return NULL;
    return (PyObject *)newparaobject(gimp_drawable_parasite_find(self->ID,
								 name));
}

static PyObject *
drw_parasite_attach(self, args)
     drwobject *self;
     PyObject *args;
{
    paraobject *parasite;
    if (!PyArg_ParseTuple(args, "O!:parasite_attach", &Paratype, &parasite))
	return NULL;
    gimp_drawable_parasite_attach(self->ID, parasite->para);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
drw_attach_new_parasite(self, args)
     drwobject *self;
     PyObject *args;
{
    char *name, *data;
    int flags, size;
    if (!PyArg_ParseTuple(args, "sis#:attach_new_parasite", &name, &flags,
			  &data, &size))
	return NULL;
    gimp_drawable_attach_new_parasite(self->ID, name, flags, size, data);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
drw_parasite_detach(self, args)
     drwobject *self;
     PyObject *args;
{
    char *name;
    if (!PyArg_ParseTuple(args, "s:detach_parasite", &name))
	return NULL;
    gimp_drawable_parasite_detach(self->ID, name);
    Py_INCREF(Py_None);
    return Py_None;
}
#endif

/* for inclusion with the methods of layer and channel objects */
#ifdef GIMP_HAVE_PARASITES
#define drw_methods() \
    {"flush",	(PyCFunction)drw_flush,	METH_VARARGS}, \
    {"update",	(PyCFunction)drw_update,	METH_VARARGS}, \
    {"merge_shadow",	(PyCFunction)drw_merge_shadow,	METH_VARARGS}, \
    {"fill",	(PyCFunction)drw_fill,	METH_VARARGS}, \
    {"get_tile",	(PyCFunction)drw_get_tile,	METH_VARARGS}, \
    {"get_tile2",	(PyCFunction)drw_get_tile2,	METH_VARARGS}, \
    {"get_pixel_rgn", (PyCFunction)drw_get_pixel_rgn, METH_VARARGS}, \
    {"parasite_find",       (PyCFunction)drw_parasite_find, METH_VARARGS}, \
    {"parasite_attach",     (PyCFunction)drw_parasite_attach, METH_VARARGS},\
    {"attach_new_parasite",(PyCFunction)drw_attach_new_parasite,METH_VARARGS},\
    {"parasite_detach",     (PyCFunction)drw_parasite_detach, METH_VARARGS}
#else
#define drw_methods() \
    {"flush",	(PyCFunction)drw_flush,	METH_VARARGS}, \
    {"update",	(PyCFunction)drw_update,	METH_VARARGS}, \
    {"merge_shadow",	(PyCFunction)drw_merge_shadow,	METH_VARARGS}, \
    {"fill",	(PyCFunction)drw_fill,	METH_VARARGS}, \
    {"get_tile",	(PyCFunction)drw_get_tile,	METH_VARARGS}, \
    {"get_tile2",	(PyCFunction)drw_get_tile2,	METH_VARARGS}, \
    {"get_pixel_rgn", (PyCFunction)drw_get_pixel_rgn, METH_VARARGS}
#endif
/* ---------- */


static drwobject *
newdrwobject(GDrawable *d, gint32 ID)
{
    drwobject *self;

    if (d == NULL && ID == -1) {
	Py_INCREF(Py_None);
	return (drwobject *)Py_None;
    }
    if (d != NULL)
	ID = d->id;
    /* create the appropriate object type */
    if (gimp_drawable_is_layer(ID))
	self = newlayobject(ID);
    else
	self = newchnobject(ID);

    if (self == NULL)
	return NULL;

    self->drawable = d;

    return self;
}

/* End of code for Drawable objects */
/* -------------------------------------------------------- */


static PyObject *
lay_copy(self, args)
     layobject *self;
     PyObject *args;
{
    int add_alpha = 0, nreturn_vals;
    GParam *return_vals;
    gint32 id;

    /* start of long convoluted (working) layer_copy */
    if (!PyArg_ParseTuple(args, "|i:copy", &add_alpha))
	return NULL;

    return_vals = gimp_run_procedure("gimp_layer_copy",
				     &nreturn_vals, 
				     PARAM_LAYER, self->ID,
				     PARAM_INT32, add_alpha,
				     PARAM_END);
    if (return_vals[0].data.d_status != STATUS_SUCCESS) {
	PyErr_SetString(ErrorObject, "can't create new layer");
	return NULL;
    }	
    id = return_vals[1].data.d_layer;
    gimp_destroy_params(return_vals, nreturn_vals);
    return (PyObject *)newlayobject(id);

    /* This simple version of the code doesn't seem to work */
    /* return (PyObject *)newlayobject(gimp_layer_copy(self->ID));*/
}


static PyObject *
lay_add_alpha(self, args)
     layobject *self;
     PyObject *args;
{
    if (!PyArg_ParseTuple(args, ":add_alpha"))
	return NULL;
    gimp_layer_add_alpha(self->ID);
    Py_INCREF(Py_None);
    return Py_None;
}


static PyObject *
lay_create_mask(self, args)
     layobject *self;
     PyObject *args;
{
    int type;

    if (!PyArg_ParseTuple(args, "i:create_mask", &type))
	return NULL;
    return (PyObject *)newchnobject(gimp_layer_create_mask(self->ID,type));
}


static PyObject *
lay_resize(self, args)
     layobject *self;
     PyObject *args;
{
    unsigned int new_h, new_w;
    int offs_x, offs_y;
    if (!PyArg_ParseTuple(args, "iiii:resize", &new_w, &new_h,
			  &offs_x, &offs_y))
	return NULL;
    gimp_layer_resize(self->ID, new_w, new_h, offs_x, offs_y);
    Py_INCREF(Py_None);
    return Py_None;
}


static PyObject *
lay_scale(self, args)
     layobject *self;
     PyObject *args;
{
    unsigned int new_w, new_h;
    int local_origin;
    if (!PyArg_ParseTuple(args, "iii:scale", &new_w, &new_h,
			  &local_origin))
	return NULL;
    gimp_layer_scale(self->ID, new_w, new_h, local_origin);
    Py_INCREF(Py_None);
    return Py_None;
}


static PyObject *
lay_translate(self, args)
     layobject *self;
     PyObject *args;
{
    int offs_x, offs_y;
    if (!PyArg_ParseTuple(args, "ii:translate", &offs_x, &offs_y))
	return NULL;
    gimp_layer_translate(self->ID, offs_x, offs_y);
    Py_INCREF(Py_None);
    return Py_None;
}


static PyObject *
lay_set_offsets(self, args)
     layobject *self;
     PyObject *args;
{
    int offs_x, offs_y;
    if (!PyArg_ParseTuple(args, "ii:set_offsets", &offs_x, &offs_y))
	return NULL;
    gimp_layer_set_offsets(self->ID, offs_x, offs_y);
    Py_INCREF(Py_None);
    return Py_None;
}

#if GIMP_CHECK_VERSION(1,1,0)
static PyObject *
lay_get_tattoo(self, args)
     layobject *self;
     PyObject *args;
{
    if (!PyArg_ParseTuple(args, ":get_tattoo"))
	return NULL;
    return PyInt_FromLong(gimp_layer_get_tattoo(self->ID));
}
#endif

static struct PyMethodDef lay_methods[] = {
    {"copy",	(PyCFunction)lay_copy,	METH_VARARGS},
    {"add_alpha",	(PyCFunction)lay_add_alpha,	METH_VARARGS},
    {"create_mask",	(PyCFunction)lay_create_mask,	METH_VARARGS},
    {"resize",	(PyCFunction)lay_resize,	METH_VARARGS},
    {"scale",	(PyCFunction)lay_scale,	METH_VARARGS},
    {"translate",	(PyCFunction)lay_translate,	METH_VARARGS},
    {"set_offsets",	(PyCFunction)lay_set_offsets,	METH_VARARGS},
#if GIMP_CHECK_VERSION(1,1,0)
    {"get_tattoo",      (PyCFunction)lay_get_tattoo,    METH_VARARGS},
#endif

    drw_methods(), 
    {NULL,		NULL}		/* sentinel */
};

/* ---------- */


static layobject *
newlayobject(gint32 ID)
{
    layobject *self;

    if (ID == -1) {
	Py_INCREF(Py_None);
	return (layobject *)Py_None;
    }
    self = PyObject_NEW(layobject, &Laytype);
    if (self == NULL)
	return NULL;
    self->ID = ID;
    self->drawable = NULL;
    return self;
}


static void
lay_dealloc(self)
     layobject *self;
{
    if (self->drawable)
	gimp_drawable_detach(self->drawable);
    PyMem_DEL(self);
}

static PyObject *
lay_getattr(self, name)
     layobject *self;
     char *name;
{
    gint32 id;
	
    if (!strcmp(name, "__members__"))
	return Py_BuildValue("[ssssssssssssssssssss]", "ID", "apply_mask",
			     "bpp", "edit_mask", "has_alpha", "height",
			     "image", "is_color", "is_colour",
			     "is_floating_selection", "is_gray", "is_grey",
			     "is_indexed", "is_rgb", "mask", "mask_bounds",
			     "mode", "name", "offsets", "opacity",
			     "preserve_transparency", "show_mask", "type",
			     "visible", "width");
    if (!strcmp(name, "ID"))
	return PyInt_FromLong(self->ID);
    if (!strcmp(name, "bpp"))
	return PyInt_FromLong((long) gimp_layer_bpp(self->ID));
    if (!strcmp(name, "has_alpha"))
	return PyInt_FromLong(gimp_drawable_has_alpha(self->ID));
    if (!strcmp(name, "height"))
	return PyInt_FromLong((long) gimp_layer_height(self->ID));
    if (!strcmp(name, "image")) {
	id = gimp_layer_get_image_id(self->ID);
	if (id == -1) {
	    Py_INCREF(Py_None);
	    return Py_None;
	}
	return (PyObject *)newimgobject(id);
    }
    if (!strcmp(name, "is_color") || !strcmp(name, "is_colour") ||
	!strcmp(name, "is_rgb"))
	return PyInt_FromLong(gimp_drawable_is_rgb(self->ID));
    if (!strcmp(name, "is_floating_selection"))
	return PyInt_FromLong(
			      gimp_layer_is_floating_selection(self->ID));
    if (!strcmp(name, "is_gray") || !strcmp(name, "is_grey"))
	return PyInt_FromLong(gimp_drawable_is_gray(self->ID));
    if (!strcmp(name, "is_indexed"))
	return PyInt_FromLong(gimp_drawable_is_indexed(self->ID));
    if (!strcmp(name, "mask")) {
	id = gimp_layer_get_mask_id(self->ID);
	if (id == -1) {
	    Py_INCREF(Py_None);
	    return Py_None;
	}
	return (PyObject *)newchnobject(id);
    }
    if (!strcmp(name, "mask_bounds")) {
	gint x1, y1, x2, y2;
	gimp_drawable_mask_bounds(self->ID, &x1, &y1, &x2, &y2);
	return Py_BuildValue("(iiii)", x1, y1, x2, y2);
    }
    if (!strcmp(name, "apply_mask"))
	return PyInt_FromLong((long) gimp_layer_get_apply_mask(
							       self->ID));
    if (!strcmp(name, "edit_mask"))
	return PyInt_FromLong((long) gimp_layer_get_edit_mask(
							      self->ID));
    if (!strcmp(name, "mode"))
	return PyInt_FromLong((long) gimp_layer_get_mode(self->ID));
    if (!strcmp(name, "name"))
	return PyString_FromString(gimp_layer_get_name(self->ID));
    if (!strcmp(name, "offsets")) {
	gint x, y;
	gimp_drawable_offsets(self->ID, &x, &y);
	return Py_BuildValue("(ii)", x, y);
    }
    if (!strcmp(name, "opacity"))
	return PyFloat_FromDouble((double) gimp_layer_get_opacity(
								  self->ID));
    if (!strcmp(name, "preserve_transparency"))
	return PyInt_FromLong((long)
			      gimp_layer_get_preserve_transparency(self->ID));
    if (!strcmp(name, "show_mask"))
	return PyInt_FromLong((long) gimp_layer_get_show_mask(
							      self->ID));
    if (!strcmp(name, "type"))
	return PyInt_FromLong((long) gimp_layer_type(self->ID));
    if (!strcmp(name, "visible"))
	return PyInt_FromLong((long) gimp_layer_get_visible(self->ID));
    if (!strcmp(name, "width"))
	return PyInt_FromLong((long) gimp_layer_width(self->ID));
    return Py_FindMethod(lay_methods, (PyObject *)self, name);
}

static int
lay_setattr(self, name, v)
     layobject *self;
     char *name;
     PyObject *v;
{
    if (v == NULL) {
	PyErr_SetString(PyExc_TypeError, "can not delete attributes.");
	return -1;
    }
    /* Set attribute 'name' to value 'v'. v==NULL means delete */
    if (!strcmp(name, "apply_mask")) {
	if (!PyInt_Check(v)) {
	    PyErr_SetString(PyExc_TypeError, "type mis-match.");
	    return -1;
	}
	gimp_layer_set_apply_mask(self->ID, PyInt_AsLong(v));
	return 0;
    }
    if (!strcmp(name, "edit_mask")) {
	if (!PyInt_Check(v)) {
	    PyErr_SetString(PyExc_TypeError, "type mis-match.");
	    return -1;
	}
	gimp_layer_set_edit_mask(self->ID, PyInt_AsLong(v));
	return 0;
    }
    if (!strcmp(name, "mode")) {
	if (!PyInt_Check(v)) {
	    PyErr_SetString(PyExc_TypeError, "type mis-match.");
	    return -1;
	}
	gimp_layer_set_mode(self->ID, (GLayerMode)PyInt_AsLong(v));
	return 0;
    }
    if (!strcmp(name, "name")) {
	if (!PyString_Check(v)) {
	    PyErr_SetString(PyExc_TypeError, "type mis-match.");
	    return -1;
	}
	gimp_layer_set_name(self->ID, PyString_AsString(v));
	return 0;
    }
    if (!strcmp(name, "opacity")) {
	if (!PyFloat_Check(v)) {
	    PyErr_SetString(PyExc_TypeError, "type mis-match.");
	    return -1;
	}
	gimp_layer_set_opacity(self->ID, PyFloat_AsDouble(v));
	return 0;
    }
    if (!strcmp(name, "preserve_transparency")) {
	if (!PyInt_Check(v)) {
	    PyErr_SetString(PyExc_TypeError, "type mis-match.");
	    return -1;
	}
	gimp_layer_set_preserve_transparency(self->ID,
					     PyInt_AsLong(v));
	return 0;
    }
    if (!strcmp(name, "show_mask")) {
	if (!PyInt_Check(v)) {
	    PyErr_SetString(PyExc_TypeError, "type mis-match.");
	    return -1;
	}
	gimp_layer_set_show_mask(self->ID, PyInt_AsLong(v));
	return 0;
    }
    if (!strcmp(name, "visible")) {
	if (!PyInt_Check(v)) {
	    PyErr_SetString(PyExc_TypeError, "type mis-match.");
	    return -1;
	}
	gimp_layer_set_visible(self->ID, PyInt_AsLong(v));
	return 0;
    }
    if (!strcmp(name, "ID") ||
	!strcmp(name, "bpp") || !strcmp(name, "height") ||
	!strcmp(name, "image") || !strcmp(name, "mask") ||
	!strcmp(name, "type") || !strcmp(name, "width") ||
	!strcmp(name, "is_floating_selection") ||
	!strcmp(name, "offsets") || !strcmp(name, "mask_bounds") ||
	!strcmp(name, "has_alpha") || !strcmp(name, "is_color") ||
	!strcmp(name, "is_colour") || !strcmp(name, "is_rgb") ||
	!strcmp(name, "is_gray") || !strcmp(name, "is_grey") ||
	!strcmp(name, "is_indexed") || !strcmp(name, "__members__")) {
	PyErr_SetString(PyExc_TypeError, "read-only attribute.");
	return -1;
    }
    return -1;
}

static PyObject *
lay_repr(self)
     layobject *self;
{
    PyObject *s;

    s = PyString_FromString("<layer ");
    PyString_ConcatAndDel(&s, PyString_FromString(
						  gimp_layer_get_name(self->ID)));
    PyString_ConcatAndDel(&s, PyString_FromString(">"));
    return s;
}

static int
lay_cmp(self, other)
     layobject *self, *other;
{
    return self->ID - other->ID;
}

static PyTypeObject Laytype = {
    PyObject_HEAD_INIT(&PyType_Type)
    0,				/*ob_size*/
    "Layer",			/*tp_name*/
    sizeof(layobject),		/*tp_basicsize*/
    0,				/*tp_itemsize*/
    /* methods */
    (destructor)lay_dealloc,	/*tp_dealloc*/
    (printfunc)0,		/*tp_print*/
    (getattrfunc)lay_getattr,	/*tp_getattr*/
    (setattrfunc)lay_setattr,	/*tp_setattr*/
    (cmpfunc)lay_cmp,		/*tp_compare*/
    (reprfunc)lay_repr,		/*tp_repr*/
    &gobj_as_number,			/*tp_as_number*/
    0,		/*tp_as_sequence*/
    0,		/*tp_as_mapping*/
    (hashfunc)0,		/*tp_hash*/
    (ternaryfunc)0,		/*tp_call*/
    (reprfunc)0,		/*tp_str*/

    /* Space for future expansion */
    0L,0L,0L,0L,
    NULL /* Documentation string */
};

/* End of code for Layer objects */
/* -------------------------------------------------------- */


static PyObject *
chn_copy(self, args)
     chnobject *self;
     PyObject *args;
{
    gint32 id;
	
    if (!PyArg_ParseTuple(args, ":copy"))
	return NULL;
    id = gimp_channel_copy(self->ID);
    if (id == -1) {
	PyErr_SetString(ErrorObject, "can't copy channel");
	return NULL;
    }
    return (PyObject *)newchnobject(id);
}

#if GIMP_CHECK_VERSION(1,1,0)
static PyObject *
chn_get_tattoo(self, args)
     chnobject *self;
     PyObject *args;
{
    if (!PyArg_ParseTuple(args, ":get_tattoo"))
	return NULL;
    return PyInt_FromLong(gimp_channel_get_tattoo(self->ID));
}
#endif

static struct PyMethodDef chn_methods[] = {
    {"copy",	(PyCFunction)chn_copy,	METH_VARARGS},
#if GIMP_CHECK_VERSION(1,1,0)
    {"get_tattoo", (PyCFunction)chn_get_tattoo, METH_VARARGS},
#endif
    drw_methods(),
    {NULL,		NULL}		/* sentinel */
};

/* ---------- */


static chnobject *
newchnobject(gint32 ID)
{
    chnobject *self;

    if (ID == -1) {
	Py_INCREF(Py_None);
	return (chnobject *)Py_None;
    }
    self = PyObject_NEW(chnobject, &Chntype);
    if (self == NULL)
	return NULL;
    self->ID = ID;
    self->drawable = NULL;
    return self;
}

static void
chn_dealloc(self)
     chnobject *self;
{
    if (self->drawable)
	gimp_drawable_detach(self->drawable);
    PyMem_DEL(self);
}

static PyObject *
chn_getattr(self, name)
     chnobject *self;
     char *name;
{
    unsigned char r, g, b;
    gint32 id;
	
    if (!strcmp(name, "__members__"))
	return Py_BuildValue("[ssssssssssssssssssssssss]", "ID",
			     "bpp", "color", "colour", "has_alpha", "height",
			     "image", "is_color", "is_colour", "is_gray",
			     "is_grey", "is_indexed", "is_layer_mask",
			     "is_rgb", "layer", "layer_mask", "mask_bounds",
			     "name", "offsets", "opacity", "show_masked",
			     "type", "visible", "width");
    if (!strcmp(name, "ID"))
	return PyInt_FromLong(self->ID);
    if (!strcmp(name, "bpp"))
	return PyInt_FromLong(gimp_drawable_bpp(self->ID));
    if (!strcmp(name, "color") || !strcmp(name, "colour")) {
	gimp_channel_get_color(self->ID, &r, &g, &b);
	return Py_BuildValue("(iii)", (long)r, (long)g, (long)b);
    }
    if (!strcmp(name, "has_alpha"))
	return PyInt_FromLong(gimp_drawable_has_alpha(self->ID));
    if (!strcmp(name, "height"))
	return PyInt_FromLong((long)gimp_channel_height(self->ID));
    if (!strcmp(name, "image")) {
	id = gimp_channel_get_image_id(self->ID);
	if (id == -1) {
	    Py_INCREF(Py_None);
	    return Py_None;
	}
	return (PyObject *)newimgobject(id);
    }
    if (!strcmp(name, "is_color") || !strcmp(name, "is_colour") ||
	!strcmp(name, "is_rgb"))
	return PyInt_FromLong(gimp_drawable_is_rgb(self->ID));
    if (!strcmp(name, "is_gray") || !strcmp(name, "is_grey"))
	return PyInt_FromLong(gimp_drawable_is_gray(self->ID));
    if (!strcmp(name, "is_indexed"))
	return PyInt_FromLong(gimp_drawable_is_indexed(self->ID));
    if (!strcmp(name, "layer")) {
	id = gimp_channel_get_layer_id(self->ID);
	if (id == -1) {
	    Py_INCREF(Py_None);
	    return Py_None;
	}
	return (PyObject *)newlayobject(id);
    }
    if (!strcmp(name, "layer_mask") || !strcmp(name, "is_layer_mask"))
	return PyInt_FromLong(gimp_drawable_is_layer_mask(self->ID));
    if (!strcmp(name, "mask_bounds")) {
	gint x1, y1, x2, y2;
	gimp_drawable_mask_bounds(self->ID, &x1, &y1, &x2, &y2);
	return Py_BuildValue("(iiii)", x1, y1, x2, y2);
    }
    if (!strcmp(name, "name"))
	return PyString_FromString(gimp_channel_get_name(self->ID));
    if (!strcmp(name, "offsets")) {
	gint x, y;
	gimp_drawable_offsets(self->ID, &x, &y);
	return Py_BuildValue("(ii)", x, y);
    }
    if (!strcmp(name, "opacity"))
	return PyFloat_FromDouble(gimp_channel_get_opacity(self->ID));
    if (!strcmp(name, "show_masked"))
	return PyInt_FromLong(gimp_channel_get_show_masked(self->ID));
    if (!strcmp(name, "type"))
	return PyInt_FromLong(gimp_drawable_type(self->ID));
    if (!strcmp(name, "visible"))
	return PyInt_FromLong(gimp_channel_get_visible(self->ID));
    if (!strcmp(name, "width"))
	return PyInt_FromLong(gimp_channel_width(self->ID));
    return Py_FindMethod(chn_methods, (PyObject *)self, name);
}

static int
chn_setattr(self, name, v)
     chnobject *self;
     char *name;
     PyObject *v;
{
    PyObject *r, *g, *b;
    if (v == NULL) {
	PyErr_SetString(PyExc_TypeError, "can not delete attributes.");
	return -1;
    }
    if (!strcmp(name, "color") || !strcmp(name, "colour")) {
	if (!PySequence_Check(v) || PySequence_Length(v) < 3) {
	    PyErr_SetString(PyExc_TypeError, "type mis-match.");
	    return -1;
	}
	r = PySequence_GetItem(v, 0);
	g = PySequence_GetItem(v, 1);
	b = PySequence_GetItem(v, 2);
	if (!PyInt_Check(r) || !PyInt_Check(g) || !PyInt_Check(b)) {
	    PyErr_SetString(PyExc_TypeError, "type mis-match.");
	    Py_DECREF(r); Py_DECREF(g); Py_DECREF(b);
	    return -1;
	}
	gimp_channel_set_color(self->ID, PyInt_AsLong(r),
			       PyInt_AsLong(g), PyInt_AsLong(b));
	Py_DECREF(r); Py_DECREF(g); Py_DECREF(b);
	return 0;
    }
    if (!strcmp(name, "name")) {
	if (!PyString_Check(v)) {
	    PyErr_SetString(PyExc_TypeError, "type mis-match.");
	    return -1;
	}
	gimp_channel_set_name(self->ID, PyString_AsString(v));
	return 0;
    }
    if (!strcmp(name, "opacity")) {
	if (!PyFloat_Check(v)) {
	    PyErr_SetString(PyExc_TypeError, "type mis-match.");
	    return -1;
	}
	gimp_channel_set_opacity(self->ID, PyFloat_AsDouble(v));
	return 0;
    }
    /*	if (!strcmp(name, "show_masked")) {
	if (!PyInt_Check(v)) {
	PyErr_SetString(PyExc_TypeError, "type mis-match.");
	return -1;
	}
	gimp_channel_set_show_masked(self->ID, PyInt_AsLong(v));
	return 0;
	}
    */	if (!strcmp(name, "visible")) {
	if (!PyInt_Check(v)) {
	    PyErr_SetString(PyExc_TypeError, "type mis-match.");
	    return -1;
	}
	gimp_channel_set_visible(self->ID, PyInt_AsLong(v));
	return 0;
    }
    if (!strcmp(name, "height") || !strcmp(name, "image") ||
	!strcmp(name, "layer") || !strcmp(name, "width") ||
	!strcmp(name, "ID") || !strcmp(name, "bpp") ||
	!strcmp(name, "layer_mask") || !strcmp(name, "mask_bounds") ||
	!strcmp(name, "offsets") || !strcmp(name, "type") ||
	!strcmp(name, "has_alpha") || !strcmp(name, "is_color") ||
	!strcmp(name, "is_colour") || !strcmp(name, "is_rgb") ||
	!strcmp(name, "is_gray") || !strcmp(name, "is_grey") ||
	!strcmp(name, "is_indexed") || !strcmp(name, "is_layer_mask") ||
	!strcmp(name, "__members__")) {
	PyErr_SetString(PyExc_TypeError, "read-only attribute.");
	return -1;
    }
    return -1;
}

static PyObject *
chn_repr(self)
     chnobject *self;
{
    PyObject *s;

    s = PyString_FromString("<channel ");
    PyString_ConcatAndDel(&s, PyString_FromString(gimp_channel_get_name(
									self->ID)));
    PyString_ConcatAndDel(&s, PyString_FromString(">"));
    return s;
}

static int
chn_cmp(self, other)
     chnobject *self, *other;
{
    return self->ID - other->ID;
}

static PyTypeObject Chntype = {
    PyObject_HEAD_INIT(&PyType_Type)
    0,				/*ob_size*/
    "Channel",			/*tp_name*/
    sizeof(chnobject),		/*tp_basicsize*/
    0,				/*tp_itemsize*/
    /* methods */
    (destructor)chn_dealloc,	/*tp_dealloc*/
    (printfunc)0,		/*tp_print*/
    (getattrfunc)chn_getattr,	/*tp_getattr*/
    (setattrfunc)chn_setattr,	/*tp_setattr*/
    (cmpfunc)chn_cmp,		/*tp_compare*/
    (reprfunc)chn_repr,		/*tp_repr*/
    &gobj_as_number,			/*tp_as_number*/
    0,		/*tp_as_sequence*/
    0,		/*tp_as_mapping*/
    (hashfunc)0,		/*tp_hash*/
    (ternaryfunc)0,		/*tp_call*/
    (reprfunc)0,		/*tp_str*/

    /* Space for future expansion */
    0L,0L,0L,0L,
    NULL /* Documentation string */
};

/* End of code for Channel objects */
/* -------------------------------------------------------- */


static PyObject *
tile_flush(self, args)
     tileobject *self;
     PyObject *args;
{
    if (!PyArg_ParseTuple(args, ":flush"))
	return NULL;
    gimp_tile_flush(self->tile);
    Py_INCREF(Py_None);
    return Py_None;
}


static struct PyMethodDef tile_methods[] = {
    {"flush",	(PyCFunction)tile_flush,	METH_VARARGS},
 
    {NULL,		NULL}		/* sentinel */
};

/* ---------- */


static tileobject *
newtileobject(t, drw)
     GTile *t;
     drwobject *drw;
{
    tileobject *self;
	
    self = PyObject_NEW(tileobject, &Tiletype);
    if (self == NULL)
	return NULL;
    gimp_tile_ref(t);
    self->tile = t;

    Py_INCREF(drw);
    self->drawable = drw;

    return self;
}


static void
tile_dealloc(self)
     tileobject *self;
{
    gimp_tile_unref(self->tile, FALSE);
    Py_DECREF(self->drawable);
    PyMem_DEL(self);
}

#define OFF(x) offsetof(GTile, x)

static struct memberlist tile_memberlist[] = {
    {"ewidth",	T_UINT,		OFF(ewidth),	RO},
    {"eheight",	T_UINT,		OFF(eheight),	RO},
    {"bpp",		T_UINT,		OFF(bpp),	RO},
    {"ref_count",	T_USHORT,	OFF(ref_count),	RO},
    {"dirty",	T_INT,		0,	RO},
    {"shadow",	T_INT,		0,	RO},
    {"drawable",	T_INT,		OFF(drawable),	RO},
    {NULL} /* Sentinel */
};

#undef OFF

static PyObject *
tile_getattr(self, name)
     tileobject *self;
     char *name;
{
    PyObject *rv;

    if (!strcmp(name, "dirty"))
	return PyInt_FromLong(self->tile->dirty);
    if (!strcmp(name, "shadow"))
	return PyInt_FromLong(self->tile->shadow);
    if (!strcmp(name, "drawable"))
	return (PyObject *)newdrwobject(self->tile->drawable, 0);
    rv = PyMember_Get((char *)self->tile, tile_memberlist, name);
    if (rv)
	return rv;
    PyErr_Clear();
    return Py_FindMethod(tile_methods, (PyObject *)self, name);
}

static PyObject *
tile_repr(self)
     tileobject *self;
{
    PyObject *s;
    if (self->tile->shadow)
	s = PyString_FromString("<shadow tile for drawable ");
    else
	s = PyString_FromString("<tile for drawable ");
    PyString_ConcatAndDel(&s, PyString_FromString(gimp_drawable_name(
								     self->tile->drawable->id)));
    PyString_ConcatAndDel(&s, PyString_FromString(">"));
    return s;
}

static int
tile_length(self)
     tileobject *self;
{
    return self->tile->ewidth * self->tile->eheight;
}

static PyObject *
tile_subscript(self, sub)
     tileobject *self;
     PyObject *sub;
{
    GTile *tile = self->tile;
    int bpp = tile->bpp;
    long x, y;

    if (PyInt_Check(sub)) {
	x = PyInt_AsLong(sub);
	if (x < 0 || x >= tile->ewidth * tile->eheight) {
	    PyErr_SetString(PyExc_IndexError,"index out of range");
	    return NULL;
	}
	return PyString_FromStringAndSize(tile->data + bpp * x, bpp);
    }
    if (PyTuple_Check(sub)) {
	if (!PyArg_ParseTuple(sub, "ll", &x, &y))
	    return NULL;
	if (x < 0 || y < 0 || x >= tile->ewidth || y>=tile->eheight) {
	    PyErr_SetString(PyExc_IndexError,"index out of range");
	    return NULL;
	}
	return PyString_FromStringAndSize(tile->data + bpp * (x +
							      y * tile->ewidth), bpp);
    }
    PyErr_SetString(PyExc_TypeError, "tile subscript not int or 2-tuple");
    return NULL;
}

static int
tile_ass_sub(self, v, w)
     tileobject *self;
     PyObject *v, *w;
{
    GTile *tile = self->tile;
    int bpp = tile->bpp, i;
    long x, y;
    char *pix, *data;
	
    if (w == NULL) {
	PyErr_SetString(PyExc_TypeError,
			"can not delete pixels in tile");
	return -1;
    }
    if (!PyString_Check(w) && PyString_Size(w) == bpp) {
	PyErr_SetString(PyExc_TypeError, "invalid subscript");
	return -1;
    }
    pix = PyString_AsString(w);
    if (PyInt_Check(v)) {
	x = PyInt_AsLong(v);
	if (x < 0 || x >= tile->ewidth * tile->eheight) {
	    PyErr_SetString(PyExc_IndexError,"index out of range");
	    return -1;
	}
	data = tile->data + x * bpp;
	for (i = 0; i < bpp; i++)
	    data[i] = pix[i];
	tile->dirty = TRUE;
	return 0;
    }
    if (PyTuple_Check(v)) {
	if (!PyArg_ParseTuple(v, "ll", &x, &y))
	    return -1;
	if (x < 0 || y < 0 || x >= tile->ewidth || y>=tile->eheight) {
	    PyErr_SetString(PyExc_IndexError,"index out of range");
	    return -1;
	}
	data = tile->data + bpp * (x + y * tile->ewidth);
	for (i = 0; i < bpp; i++)
	    data[i] = pix[i];
	tile->dirty = TRUE;
	return 0;
    }
    PyErr_SetString(PyExc_TypeError, "tile subscript not int or 2-tuple");
    return -1;
}

static PyMappingMethods tile_as_mapping = {
    (inquiry)tile_length, /*length*/
    (binaryfunc)tile_subscript, /*subscript*/
    (objobjargproc)tile_ass_sub, /*ass_sub*/
};

static PyTypeObject Tiletype = {
    PyObject_HEAD_INIT(&PyType_Type)
    0,				/*ob_size*/
    "Tile",			/*tp_name*/
    sizeof(tileobject),		/*tp_basicsize*/
    0,				/*tp_itemsize*/
    /* methods */
    (destructor)tile_dealloc,	/*tp_dealloc*/
    (printfunc)0,		/*tp_print*/
    (getattrfunc)tile_getattr,	/*tp_getattr*/
    (setattrfunc)0,	/*tp_setattr*/
    (cmpfunc)0,		/*tp_compare*/
    (reprfunc)tile_repr,		/*tp_repr*/
    0,			/*tp_as_number*/
    0,		/*tp_as_sequence*/
    &tile_as_mapping,		/*tp_as_mapping*/
    (hashfunc)0,		/*tp_hash*/
    (ternaryfunc)0,		/*tp_call*/
    (reprfunc)0,		/*tp_str*/

    /* Space for future expansion */
    0L,0L,0L,0L,
    NULL /* Documentation string */
};

/* End of code for Tile objects */
/* -------------------------------------------------------- */


static PyObject *
pr_resize(self, args)
     probject *self;
     PyObject *args;
{
    int x, y, w, h;
    if (!PyArg_ParseTuple(args, "iiii:resize", &x, &y, &w, &h))
	return NULL;
    gimp_pixel_rgn_resize(&(self->pr), x, y, w, h);
    Py_INCREF(Py_None);
    return Py_None;
}



static struct PyMethodDef pr_methods[] = {
    {"resize",	(PyCFunction)pr_resize,	METH_VARARGS},
 
    {NULL,		NULL}		/* sentinel */
};

/* ---------- */


static probject *
newprobject(drawable, x, y, width, height, dirty, shadow)
     drwobject *drawable;
     int x, y, width, height, dirty, shadow;
{
    probject *self;
	
    self = PyObject_NEW(probject, &Prtype);
    if (self == NULL)
	return NULL;
    gimp_pixel_rgn_init(&(self->pr), drawable->drawable, x, y, width, height,
			dirty, shadow);
    self->drawable = drawable;
    Py_INCREF(drawable);
    return self;
}


static void
pr_dealloc(self)
     probject *self;
{
    Py_DECREF(self->drawable);
    PyMem_DEL(self);
}

/* Code to access pr objects as mappings */

static int
pr_length(self)
     probject *self;
{
    PyErr_SetString(ErrorObject, "Can't get size of pixel region.");
    return -1;
}

static PyObject *
pr_subscript(self, key)
     probject *self;
     PyObject *key;
{
    GPixelRgn *pr = &(self->pr);
    int bpp = pr->bpp;
    PyObject *x, *y;
    int x1, y1, x2, y2, xs, ys;

    if (!PyTuple_Check(key) || PyTuple_Size(key) != 2) {
	PyErr_SetString(PyExc_TypeError,"subscript must be a 2-tuple.");
	return NULL;
    }
    if (!PyArg_ParseTuple(key, "OO", &x, &y))
	return NULL;
    if (PyInt_Check(x)) {
	x1 = PyInt_AsLong(x);
	if (pr->x > x1 || x1 >= pr->x + pr->w) {
	    PyErr_SetString(PyExc_IndexError,
			    "x subscript out of range");
	    return NULL;
	}
	if (PyInt_Check(y)) {
	    char buf[MAX_BPP];
			
	    y1 = PyInt_AsLong(y);
	    if (pr->y > y1 || y1 >= pr->y + pr->h) {
		PyErr_SetString(PyExc_IndexError,
				"y subscript out of range");
		return NULL;
	    }
	    gimp_pixel_rgn_get_pixel(pr, buf, x1, y1);
	    return PyString_FromStringAndSize(buf, bpp);
	} else if (PySlice_Check(y))
	    if (PySlice_GetIndices((PySliceObject *)y,
				   pr->y + pr->h, &y1, &y2, &ys) ||
		(y1 != 0 && pr->y > y1) ||
		pr->y > y2 || ys != 1) {
		PyErr_SetString(PyExc_IndexError,
				"invalid y slice");
		return NULL;
	    } else {
		gchar *buf = g_new(gchar, bpp * (y2 - y1));
		PyObject *ret;
				
		if (y1 == 0) y1 = pr->y;
		gimp_pixel_rgn_get_col(pr, buf, x1, y1, y2-y1);
		ret = PyString_FromStringAndSize(buf, bpp * (y2 - y1));
		g_free(buf);
		return ret;
	    }
	else {
	    PyErr_SetString(PyExc_TypeError,"invalid y subscript");
	    return NULL;
	}
    } else if (PySlice_Check(x)) {
	if (PySlice_GetIndices((PySliceObject *)x, pr->x + pr->w,
			       &x1, &x2, &xs) || (x1 != 0 && pr->x > x1) ||
	    pr->x > x2 || xs != 1) {
	    PyErr_SetString(PyExc_IndexError, "invalid x slice");
	    return NULL;
	}
	if (x1 == 0) x1 = pr->x;
	if (PyInt_Check(y)) {
	    char *buf;
	    PyObject *ret;
			
	    y1 = PyInt_AsLong(y);
	    if (pr->y > y1 || y1 >= pr->y + pr->h) {
		PyErr_SetString(PyExc_IndexError,
				"y subscript out of range");
		return NULL;
	    }
	    buf = g_new(gchar, bpp * (x2 - x1));
	    gimp_pixel_rgn_get_row(pr, buf, x1, y1, x2 - x1);
	    ret = PyString_FromStringAndSize(buf, bpp * (x2-x1));
	    g_free(buf);
	    return ret;
	} else if (PySlice_Check(y))
	    if (PySlice_GetIndices((PySliceObject *)y,
				   pr->y + pr->h, &y1, &y2, &ys) ||
		(y1 != 0 && pr->y) > y1 ||
		pr->y > y2 || ys != 1) {
		PyErr_SetString(PyExc_IndexError,
				"invalid y slice");
		return NULL;
	    } else {
		gchar *buf = g_new(gchar, bpp * (x2 - x1) * (y2 - y1));
		PyObject *ret;
				
		if (y1 == 0) y1 = pr->y;
		gimp_pixel_rgn_get_rect(pr, buf, x1, y1,
					x2 - x1, y2 - y1);
		ret = PyString_FromStringAndSize(buf, bpp * (x2-x1) * (y2-y1));
		g_free(buf);
		return ret;
	    }
	else {
	    PyErr_SetString(PyExc_TypeError,"invalid y subscript");
	    return NULL;
	}
    } else {
	PyErr_SetString(PyExc_TypeError, "invalid x subscript");
	return NULL;
    }
}

static int
pr_ass_sub(self, v, w)
     probject *self;
     PyObject *v, *w;
{
    GPixelRgn *pr = &(self->pr);
    int bpp = pr->bpp;
    PyObject *x, *y;
    char *buf;
    int len, x1, x2, xs, y1, y2, ys;
	
    if (w == NULL) {
	PyErr_SetString(PyExc_TypeError, "can't delete subscripts.");
	return -1;
    }
    if (!PyString_Check(w)) {
	PyErr_SetString(PyExc_TypeError,
			"must assign string to subscript");
	return -1;
    }
    if (!PyTuple_Check(v) || PyTuple_Size(v) != 2) {
	PyErr_SetString(PyExc_TypeError,"subscript must be a 2-tuple");
	return -1;
    }
    if (!PyArg_ParseTuple(v, "OO", &x, &y))
	return -1;
    buf = PyString_AsString(w);
    len = PyString_Size(w);
    if (PyInt_Check(x)) {
	x1 = PyInt_AsLong(x);
	if (pr->x > x1 || x1 >= pr->x + pr->w) {
	    PyErr_SetString(PyExc_IndexError,
			    "x subscript out of range");
	    return -1;
	}
	if (PyInt_Check(y)) {
	    y1 = PyInt_AsLong(y);
	    if (pr->y > y1 || y1 >= pr->y + pr->h) {
		PyErr_SetString(PyExc_IndexError,
				"y subscript out of range");
		return -1;
	    }
	    if (len != bpp) {
		PyErr_SetString(PyExc_TypeError,
				"string is wrong length");
		return -1;
	    }
	    gimp_pixel_rgn_set_pixel(pr, buf, x1, y1);
	    return 0;
	} else if (PySlice_Check(y)) {
	    if (PySlice_GetIndices((PySliceObject *)y,
				   pr->y + pr->h, &y1, &y2, &ys) ||
		(y1 != 0 && pr->y > y1) ||
		pr->y > y2 || ys != 1) {
		PyErr_SetString(PyExc_IndexError,
				"invalid y slice");
		return -1;
	    }
	    if (y1 == 0) y1 = pr->y;
	    if (len != bpp * (y2 - y1)) {
		PyErr_SetString(PyExc_TypeError,
				"string is wrong length");
		return -1;
	    }
	    gimp_pixel_rgn_set_col(pr, buf, x1, y1, y2 - y1);
	    return 0;
	} else {
	    PyErr_SetString(PyExc_IndexError,"invalid y subscript");
	    return -1;
	}
    } else if (PySlice_Check(x)) {
	if (PySlice_GetIndices((PySliceObject *)x, pr->x + pr->w,
			       &x1, &x2, &xs) || (x1 != 0 && pr->x > x1) ||
	    pr->x > x2 || xs != 1) {
	    PyErr_SetString(PyExc_IndexError, "invalid x slice");
	    return -1;
	}
	if (x1 == 0) x1 = pr->x;
	if (PyInt_Check(y)) {
	    y1 = PyInt_AsLong(y);
	    if (pr->y > y1 || y1 >= pr->y + pr->h) {
		PyErr_SetString(PyExc_IndexError,
				"y subscript out of range");
		return -1;
	    }
	    if (len != bpp * (x2 - x1)) {
		PyErr_SetString(PyExc_TypeError,
				"string is wrong length");
		return -1;
	    }
	    gimp_pixel_rgn_set_row(pr, buf, x1, y1, x2 - x1);
	    return 0;
	} else if (PySlice_Check(y)) {
	    if (PySlice_GetIndices((PySliceObject *)y,
				   pr->y + pr->h, &y1, &y2, &ys) ||
		(y1 != 0 && pr->y > y1) ||
		pr->y > y2 || ys != 1) {
		PyErr_SetString(PyExc_IndexError,
				"invalid y slice");
		return -1;
	    }
	    if (y1 == 0) y1 = pr->y;
	    if (len != bpp * (x2 - x1) * (y2 - y1)) {
		PyErr_SetString(PyExc_TypeError,
				"string is wrong length");
		return -1;
	    }
	    gimp_pixel_rgn_set_rect(pr, buf, x1, y1, x2-x1, y2-y1);
	    return 0;
	} else {
	    PyErr_SetString(PyExc_TypeError,"invalid y subscript");
	    return -1;
	}
    } else {
	PyErr_SetString(PyExc_TypeError, "invalid x subscript");
	return -1;
    }
    return -1;
}

static PyMappingMethods pr_as_mapping = {
    (inquiry)pr_length,		/*mp_length*/
    (binaryfunc)pr_subscript,		/*mp_subscript*/
    (objobjargproc)pr_ass_sub,	/*mp_ass_subscript*/
};

/* -------------------------------------------------------- */

#define OFF(x) offsetof(GPixelRgn, x)

static struct memberlist pr_memberlist[] = {
    {"drawable",  T_INT,  OFF(drawable),  RO},
    {"bpp",       T_UINT, OFF(bpp),       RO},
    {"rowstride", T_UINT, OFF(rowstride), RO},
    {"x",         T_UINT, OFF(x),         RO},
    {"y",         T_UINT, OFF(y),         RO},
    {"w",         T_UINT, OFF(w),         RO},
    {"h",         T_UINT, OFF(h),         RO},
    {"dirty",     T_UINT, 0 /*bitfield*/, RO},
    {"shadow",    T_UINT, 0 /*bitfield*/, RO}
};

#undef OFF

static PyObject *
pr_getattr(self, name)
     probject *self;
     char *name;
{
    PyObject *rv;

    if (!strcmp(name, "drawable"))
	return (PyObject *)newdrwobject(self->pr.drawable, 0);
    if (!strcmp(name, "dirty"))
	return PyInt_FromLong(self->pr.dirty);
    if (!strcmp(name, "shadow"))
	return PyInt_FromLong(self->pr.shadow);
    rv = PyMember_Get((char *)&(self->pr), pr_memberlist, name);
    if (rv)
	return rv;
    PyErr_Clear();
    return Py_FindMethod(pr_methods, (PyObject *)self, name);
}

static PyObject *
pr_repr(self)
     probject *self;
{
    PyObject *s;
    s = PyString_FromString("<pixel region for drawable ");
    PyString_ConcatAndDel(&s, PyString_FromString(gimp_drawable_name(
								     self->pr.drawable->id)));
    PyString_ConcatAndDel(&s, PyString_FromString(">"));
    return s;
}

static PyTypeObject Prtype = {
    PyObject_HEAD_INIT(&PyType_Type)
    0,				/*ob_size*/
    "PixelRegion",			/*tp_name*/
    sizeof(probject),		/*tp_basicsize*/
    0,				/*tp_itemsize*/
    /* methods */
    (destructor)pr_dealloc,	/*tp_dealloc*/
    (printfunc)0,		/*tp_print*/
    (getattrfunc)pr_getattr,	/*tp_getattr*/
    (setattrfunc)0,	/*tp_setattr*/
    (cmpfunc)0,		/*tp_compare*/
    (reprfunc)pr_repr,		/*tp_repr*/
    0,			/*tp_as_number*/
    0,		/*tp_as_sequence*/
    &pr_as_mapping,		/*tp_as_mapping*/
    (hashfunc)0,		/*tp_hash*/
    (ternaryfunc)0,		/*tp_call*/
    (reprfunc)0,		/*tp_str*/

    /* Space for future expansion */
    0L,0L,0L,0L,
    NULL /* Documentation string */
};

/* End of code for PixelRegion objects */
/* -------------------------------------------------------- */

#ifdef GIMP_HAVE_PARASITES
static PyObject *
para_copy(self, args)
     paraobject *self;
     PyObject *args;
{
    if (!PyArg_ParseTuple(args, ":copy"))
	return NULL;
    return (PyObject *)newparaobject(parasite_copy(self->para));
}

static PyObject *
para_is_type(self, args)
     paraobject *self;
     PyObject *args;
{
    char *name;
    if (!PyArg_ParseTuple(args, "s:is_type", &name))
	return NULL;
    return PyInt_FromLong(parasite_is_type(self->para, name));
}

static PyObject *
para_has_flag(self, args)
     paraobject *self;
     PyObject *args;
{
    int flag;
    if (!PyArg_ParseTuple(args, "i:has_flag", &flag))
	return NULL;
    return PyInt_FromLong(parasite_has_flag(self->para, flag));
}



static struct PyMethodDef para_methods[] = {
    {"copy",	(PyCFunction)para_copy,	METH_VARARGS},
    {"is_type",	(PyCFunction)para_is_type,	METH_VARARGS},
    {"has_flag",(PyCFunction)para_has_flag,	METH_VARARGS},
 
    {NULL,		NULL}		/* sentinel */
};

static paraobject *
newparaobject(para)
     Parasite *para;
{
    paraobject *self;

    if (!para) {
	Py_INCREF(Py_None);
	return (paraobject *)Py_None;
    }
    self = PyObject_NEW(paraobject, &Paratype);
    if (self == NULL)
	return NULL;
    self->para = para;
    return self;
}


static void
para_dealloc(self)
     paraobject *self;
{
    parasite_free(self->para);
    PyMem_DEL(self);
}

static PyObject *
para_getattr(self, name)
     paraobject *self;
     char *name;
{
    if (!strcmp(name, "__members__")) {
#if GIMP_CHECK_VERSION(1,1,5)
	return Py_BuildValue("[sssss]", "data", "flags", "is_persistent",
			     "is_undoable", "name");
#else
	return Py_BuildValue("[ssss]", "data", "flags", "is_persistent",
			     "name");
#endif
    }
    if (!strcmp(name, "is_persistent"))
	return PyInt_FromLong(parasite_is_persistent(self->para));
#if GIMP_CHECK_VERSION(1,1,5)
    if (!strcmp(name, "is_undoable"))
	return PyInt_FromLong(parasite_is_undoable(self->para));
#endif
    if (!strcmp(name, "flags"))
	return PyInt_FromLong(parasite_flags(self->para));
    if (!strcmp(name, "name"))
	return PyString_FromString(parasite_name(self->para));
    if (!strcmp(name, "data"))
	return PyString_FromStringAndSize(parasite_data(self->para),
					  parasite_data_size(self->para));
    return Py_FindMethod(para_methods, (PyObject *)self, name);
}

static PyObject *
para_repr(self)
     paraobject *self;
{
    PyObject *s;
    s = PyString_FromString("<parasite ");
    PyString_ConcatAndDel(&s, PyString_FromString(parasite_name(self->para)));
    PyString_ConcatAndDel(&s, PyString_FromString(">"));
    return s;
}

static PyObject *
para_str(self)
     paraobject *self;
{
    return PyString_FromStringAndSize(parasite_data(self->para),
				      parasite_data_size(self->para));
}

static PyTypeObject Paratype = {
    PyObject_HEAD_INIT(&PyType_Type)
    0,				/*ob_size*/
    "Parasite",			/*tp_name*/
    sizeof(paraobject),		/*tp_basicsize*/
    0,				/*tp_itemsize*/
    /* methods */
    (destructor)para_dealloc,	/*tp_dealloc*/
    (printfunc)0,		/*tp_print*/
    (getattrfunc)para_getattr,	/*tp_getattr*/
    (setattrfunc)0,	/*tp_setattr*/
    (cmpfunc)0,		/*tp_compare*/
    (reprfunc)para_repr,		/*tp_repr*/
    0,			/*tp_as_number*/
    0,		/*tp_as_sequence*/
    0,		/*tp_as_mapping*/
    (hashfunc)0,		/*tp_hash*/
    (ternaryfunc)0,		/*tp_call*/
    (reprfunc)para_str,		/*tp_str*/

    /* Space for future expansion */
    0L,0L,0L,0L,
    NULL /* Documentation string */
};

/* End of code for Parasite objects */
/* -------------------------------------------------------- */
#endif

GPlugInInfo PLUG_IN_INFO = {
    NULL, /* init_proc */
    NULL, /* quit_proc */
    NULL, /* query_proc */
    NULL  /* run_proc */
};

static PyObject *callbacks[] = {
    NULL, NULL, NULL, NULL
};

static void pygimp_init_proc() {
    PyObject *r;
    r = PyObject_CallFunction(callbacks[0], "()");
    if (!r) {
	PyErr_Print();
	PyErr_Clear();
	return;
    }
    Py_DECREF(r);
}

static void pygimp_quit_proc() {
    PyObject *r;
    r = PyObject_CallFunction(callbacks[1], "()");
    if (!r) {
	PyErr_Print();
	PyErr_Clear();
	return;
    }
    Py_DECREF(r);
}

static void pygimp_query_proc() {
    PyObject *r;
    r = PyObject_CallFunction(callbacks[2], "()");
    if (!r) {
	PyErr_Print();
	PyErr_Clear();
	return;
    }
    Py_DECREF(r);
}

static void pygimp_run_proc(char *name, int nparams, GParam *params,
			    int *nreturn_vals, GParam **return_vals) {
    PyObject *args, *ret;
    GParamDef *pd, *rv;
    char *b, *h, *a, *c, *d;
    int t, np, nrv;

    gimp_query_procedure(name, &b, &h, &a, &c, &d, &t, &np, &nrv,
			 &pd, &rv);
    g_free(b); g_free(h); g_free(a); g_free(c); g_free(d); g_free(pd);

#if PG_DEBUG > 0
    fprintf(stderr, "Params for %s:", name);
    print_GParam(nparams, params);
#endif
    args = GParam_to_tuple(nparams, params);
    if (args == NULL) {
	PyErr_Clear();
	*nreturn_vals = 1;
	*return_vals = g_new(GParam, 1);
	(*return_vals)[0].type = PARAM_STATUS;
	(*return_vals)[0].data.d_status = STATUS_CALLING_ERROR;
	return;
    }
    ret = PyObject_CallFunction(callbacks[3], "(sO)", name, args);
    Py_DECREF(args);
    if (ret == NULL) {
	PyErr_Print();
	PyErr_Clear();
	*nreturn_vals = 1;
	*return_vals = g_new(GParam, 1);
	(*return_vals)[0].type = PARAM_STATUS;
	(*return_vals)[0].data.d_status = STATUS_EXECUTION_ERROR;
	return;
    }
    *return_vals = tuple_to_GParam(ret, rv, nrv);
    g_free(rv);
    if (*return_vals == NULL) {
	PyErr_Clear();
	*nreturn_vals = 1;
	*return_vals = g_new(GParam, 1);
	(*return_vals)[0].type = PARAM_STATUS;
	(*return_vals)[0].data.d_status = STATUS_EXECUTION_ERROR;
	return;
    }
    Py_DECREF(ret);
    *nreturn_vals = nrv + 1;
    (*return_vals)[0].type = PARAM_STATUS;
    (*return_vals)[0].data.d_status = STATUS_SUCCESS;
}

static PyObject *
gimp_Main(self, args)
     PyObject *self;	/* Not used */
     PyObject *args;
{
    PyObject *av;
    int argc, i;
    char **argv;
    PyObject *ip, *qp, *query, *rp;
	
    if (!PyArg_ParseTuple(args, "OOOO:main", &ip, &qp, &query, &rp))
	return NULL;

#define Arg_Check(v) (PyCallable_Check(v) || (v) == Py_None)

    if (!Arg_Check(ip) || !Arg_Check(qp) || !Arg_Check(query) ||
	!Arg_Check(rp)) {
	PyErr_SetString(ErrorObject, "arguments must be callable.");
	return NULL;
    }
    if (query == Py_None) {
	PyErr_SetString(ErrorObject, "a query procedure must be provided.");
	return NULL;
    }

    if (ip != Py_None) {
	callbacks[0] = ip;
	PLUG_IN_INFO.init_proc = pygimp_init_proc;
    }
    if (qp != Py_None) {
	callbacks[1] = qp;
	PLUG_IN_INFO.quit_proc = pygimp_quit_proc;
    }
    if (query != Py_None) {
	callbacks[2] = query;
	PLUG_IN_INFO.query_proc = pygimp_query_proc;
    }
    if (rp != Py_None) {
	callbacks[3] = rp;
	PLUG_IN_INFO.run_proc = pygimp_run_proc;
    }

    av = PySys_GetObject("argv");

    argc = PyList_Size(av);
    argv = g_new(char *, argc);

    for (i = 0; i < argc; i++)
	argv[i] = g_strdup(PyString_AsString(PyList_GetItem(av, i)));

#ifdef G_OS_WIN32
    {
	extern void set_gimp_PLUG_IN_INFO_PTR(GPlugInInfo *);
	set_gimp_PLUG_IN_INFO_PTR(&PLUG_IN_INFO);
    }
#endif

    gimp_main(argc, argv);

    if (argv != NULL) {
	for (i = 0; i < argc; i++)
	    if (argv[i] != NULL)
		g_free(argv[i]);
	g_free(argv);
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
gimp_Quit(self, args)
     PyObject *self;	/* Not used */
     PyObject *args;
{

    if (!PyArg_ParseTuple(args, ":quit"))
	return NULL;
    gimp_quit();
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
gimp_Set_data(self, args)
     PyObject *self;	/* Not used */
     PyObject *args;
{
    char *id, *data;
    int bytes, nreturn_vals;
    GParam *return_vals;

    if (!PyArg_ParseTuple(args, "ss#:set_data", &id, &data, &bytes))
	return NULL;
    return_vals = gimp_run_procedure("gimp_procedural_db_set_data",
				     &nreturn_vals, PARAM_STRING, id, PARAM_INT32, bytes,
				     PARAM_INT8ARRAY, data, PARAM_END);
    if (return_vals[0].data.d_status != STATUS_SUCCESS) {
	PyErr_SetString(ErrorObject, "error occurred while storing");
	return NULL;
    }
    gimp_destroy_params(return_vals, nreturn_vals);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
gimp_Get_data(self, args)
     PyObject *self;	/* Not used */
     PyObject *args;
{
    char *id;
    int nreturn_vals;
    GParam *return_vals;
    PyObject *s;

    if (!PyArg_ParseTuple(args, "s:get_data", &id))
	return NULL;

    return_vals = gimp_run_procedure("gimp_procedural_db_get_data",
				     &nreturn_vals, PARAM_STRING, id, PARAM_END);

    if (return_vals[0].data.d_status != STATUS_SUCCESS) {
	PyErr_SetString(ErrorObject, "no data for id");
	return NULL;
    }
    s = PyString_FromStringAndSize((char *)return_vals[2].data.d_int8array,
				   return_vals[1].data.d_int32);
    gimp_destroy_params(return_vals, nreturn_vals);
    return s;
}

static PyObject *
gimp_Progress_init(self, args)
     PyObject *self;	/* Not used */
     PyObject *args;
{
    char *msg = NULL;
    if (!PyArg_ParseTuple(args, "|s:progress_init", &msg))
	return NULL;
    gimp_progress_init(msg);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
gimp_Progress_update(self, args)
     PyObject *self;	/* Not used */
     PyObject *args;
{
    double p;
    if (!PyArg_ParseTuple(args, "d:progress_update", &p))
	return NULL;
    gimp_progress_update(p);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
gimp_Query_images(self, args)
     PyObject *self;	/* Not used */
     PyObject *args;
{
    gint32 *imgs;
    int nimgs, i;
    PyObject *ret;
    if (!PyArg_ParseTuple(args, ":query_images"))
	return NULL;
    imgs = gimp_query_images(&nimgs);
    ret = PyList_New(nimgs);
    for (i = 0; i < nimgs; i++)
	PyList_SetItem(ret, i, (PyObject *)newimgobject(imgs[i]));
    return ret;
}

static PyObject *
gimp_Install_procedure(self, args)
     PyObject *self;	/* Not used */
     PyObject *args;
{
    char *name, *blurb, *help, *author, *copyright, *date, *menu_path,
	*image_types, *n, *d;
    GParamDef *params, *return_vals;
    int type, nparams, nreturn_vals, i;
    PyObject *pars, *rets;

    if (!PyArg_ParseTuple(args, "sssssszziOO:install_procedure",
			  &name, &blurb, &help,
			  &author, &copyright, &date, &menu_path, &image_types,
			  &type, &pars, &rets))
	return NULL;
    if (!PySequence_Check(pars) || !PySequence_Check(rets)) {
	PyErr_SetString(PyExc_TypeError,
			"last two args must be sequences");
	return NULL;
    }
    nparams = PySequence_Length(pars);
    nreturn_vals = PySequence_Length(rets);
    params = g_new(GParamDef, nparams);
    for (i = 0; i < nparams; i++) {
	if (!PyArg_ParseTuple(PySequence_GetItem(pars, i), "iss",
			      &(params[i].type), &n, &d)) {
	    g_free(params);
	    return NULL;
	}
	params[i].name = g_strdup(n);
	params[i].description = g_strdup(d);
    }
    return_vals = g_new(GParamDef, nreturn_vals);
    for (i = 0; i < nreturn_vals; i++) {
	if (!PyArg_ParseTuple(PySequence_GetItem(rets, i), "iss",
			      &(return_vals[i].type), &n, &d)) {
	    g_free(params); g_free(return_vals);
	    return NULL;
	}
	return_vals[i].name = g_strdup(n);
	return_vals[i].description = g_strdup(d);
    }
    gimp_install_procedure(name, blurb, help, author, copyright, date,
			   menu_path, image_types, type, nparams, nreturn_vals, params,
			   return_vals);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
gimp_Install_temp_proc(self, args)
     PyObject *self;	/* Not used */
     PyObject *args;
{
    char *name, *blurb, *help, *author, *copyright, *date, *menu_path,
	*image_types, *n, *d;
    GParamDef *params, *return_vals;
    int type, nparams, nreturn_vals, i;
    PyObject *pars, *rets;

    if (!PyArg_ParseTuple(args, "sssssszziOO:install_temp_proc",
			  &name, &blurb, &help,
			  &author, &copyright, &date, &menu_path, &image_types,
			  &type, &pars, &rets))
	return NULL;
    if (!PySequence_Check(pars) || !PySequence_Check(rets)) {
	PyErr_SetString(PyExc_TypeError,
			"last two args must be sequences");
	return NULL;
    }
    nparams = PySequence_Length(pars);
    nreturn_vals = PySequence_Length(rets);
    params = g_new(GParamDef, nparams);
    for (i = 0; i < nparams; i++) {
	if (!PyArg_ParseTuple(PySequence_GetItem(pars, i), "iss",
			      &(params[i].type), &n, &d)) {
	    g_free(params);
	    return NULL;
	}
	params[i].name = g_strdup(n);
	params[i].description = g_strdup(d);
    }
    return_vals = g_new(GParamDef, nreturn_vals);
    for (i = 0; i < nreturn_vals; i++) {
	if (!PyArg_ParseTuple(PySequence_GetItem(rets, i), "iss",
			      &(return_vals[i].type), &n, &d)) {
	    g_free(params); g_free(return_vals);
	    return NULL;
	}
	return_vals[i].name = g_strdup(n);
	return_vals[i].description = g_strdup(d);
    }
    gimp_install_temp_proc(name, blurb, help, author, copyright, date,
			   menu_path, image_types, type, nparams, nreturn_vals, params,
			   return_vals, pygimp_run_proc);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
gimp_Uninstall_temp_proc(self, args)
     PyObject *self;	/* Not used */
     PyObject *args;
{
    char *name;

    if (!PyArg_ParseTuple(args, "s:uninstall_temp_proc", &name))
	return NULL;
    gimp_uninstall_temp_proc(name);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
gimp_Register_magic_load_handler(self, args)
     PyObject *self;	/* Not used */
     PyObject *args;
{
    char *name, *extensions, *prefixes, *magics;
    if (!PyArg_ParseTuple(args, "ssss:register_magic_load_handler",
			  &name, &extensions, &prefixes, &magics))
	return NULL;
    gimp_register_magic_load_handler(name, extensions, prefixes, magics);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
gimp_Register_load_handler(self, args)
     PyObject *self;	/* Not used */
     PyObject *args;
{
    char *name, *extensions, *prefixes;
    if (!PyArg_ParseTuple(args, "sss:register_load_handler",
			  &name, &extensions, &prefixes))
	return NULL;
    gimp_register_load_handler(name, extensions, prefixes);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
gimp_Register_save_handler(self, args)
     PyObject *self;	/* Not used */
     PyObject *args;
{
    char *name, *extensions, *prefixes;
    if (!PyArg_ParseTuple(args, "sss:register_save_handler",
			  &name, &extensions, &prefixes))
	return NULL;
    gimp_register_save_handler(name, extensions, prefixes);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
gimp_Gamma(self, args)
     PyObject *self;	/* Not used */
     PyObject *args;
{
    if (!PyArg_ParseTuple(args, ":gamma"))
	return NULL;
    return PyFloat_FromDouble(gimp_gamma());
}

static PyObject *
gimp_Install_cmap(self, args)
     PyObject *self;	/* Not used */
     PyObject *args;
{
    if (!PyArg_ParseTuple(args, ":install_cmap"))
	return NULL;
    return PyInt_FromLong(gimp_install_cmap());
}

static PyObject *
gimp_Use_xshm(self, args)
     PyObject *self;	/* Not used */
     PyObject *args;
{
    if (!PyArg_ParseTuple(args, ":use_xshm"))
	return NULL;
    return PyInt_FromLong(gimp_use_xshm());
}

static PyObject *
gimp_Color_cube(self, args)
     PyObject *self;	/* Not used */
     PyObject *args;
{
    if (!PyArg_ParseTuple(args, ":color_cube"))
	return NULL;
    return PyString_FromString(gimp_color_cube());
}

static PyObject *
gimp_Gtkrc(self, args)
     PyObject *self;	/* Not used */
     PyObject *args;
{
    if (!PyArg_ParseTuple(args, ":gtkrc"))
	return NULL;
    return PyString_FromString(gimp_gtkrc());
}

static PyObject *
gimp_Get_background(self, args)
     PyObject *self;	/* Not used */
     PyObject *args;
{
    guchar r, g, b;
    if (!PyArg_ParseTuple(args, ":get_background"))
	return NULL;
    gimp_palette_get_background(&r, &g, &b);
    return Py_BuildValue("(iii)", (int)r, (int)g, (int)b);
}

static PyObject *
gimp_Get_foreground(self, args)
     PyObject *self;	/* Not used */
     PyObject *args;
{
    guchar r, g, b;
    if (!PyArg_ParseTuple(args, ":get_foreground"))
	return NULL;
    gimp_palette_get_foreground(&r, &g, &b);
    return Py_BuildValue("(iii)", (int)r, (int)g, (int)b);
}

static PyObject *
gimp_Set_background(self, args)
     PyObject *self;	/* Not used */
     PyObject *args;
{
    int r, g, b;
    if (!PyArg_ParseTuple(args, "(iii):set_background", &r, &g, &b)) {
	PyErr_Clear();
	if (!PyArg_ParseTuple(args, "iii:set_background", &r, &g, &b))
	    return NULL;
    }
    gimp_palette_set_background(r,g,b);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
gimp_Set_foreground(self, args)
     PyObject *self;	/* Not used */
     PyObject *args;
{
    int r, g, b;
    if (!PyArg_ParseTuple(args, "(iii):set_foreground", &r, &g, &b)) {
	PyErr_Clear();
	if (!PyArg_ParseTuple(args, "iii:set_foreground", &r, &g, &b))
	    return NULL;
    }
    gimp_palette_set_foreground(r,g,b);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
gimp_Gradients_get_list(self, args)
     PyObject *self;	/* Not used */
     PyObject *args;
{
    char **list;
    int num, i;
    PyObject *ret;
    if (!PyArg_ParseTuple(args, ":gradients_get_list"))
	return NULL;
    list = gimp_gradients_get_list(&num);
    ret = PyList_New(num);
    for (i = 0; i < num; i++)
	PyList_SetItem(ret, i, PyString_FromString(list[i]));
    g_free(list);
    return ret;
}

static PyObject *
gimp_Gradients_get_active(self, args)
     PyObject *self;	/* Not used */
     PyObject *args;
{
    if (!PyArg_ParseTuple(args, ":gradients_get_active"))
	return NULL;
    return PyString_FromString(gimp_gradients_get_active());
}

static PyObject *
gimp_Gradients_set_active(self, args)
     PyObject *self;	/* Not used */
     PyObject *args;
{
    char *actv;
    if (!PyArg_ParseTuple(args, "s:gradients_set_active", &actv))
	return NULL;
    gimp_gradients_set_active(actv);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
gimp_Gradients_sample_uniform(self, args)
     PyObject *self;	/* Not used */
     PyObject *args;
{
    int num, i, j;
    double *samp;
    PyObject *ret;
    if (!PyArg_ParseTuple(args, "i:gradients_sample_uniform", &num))
	return NULL;
    samp = gimp_gradients_sample_uniform(num);
    ret = PyList_New(num);
    for (i = 0, j = 0; i < num; i++, j += 4)
	PyList_SetItem(ret, i, Py_BuildValue("(dddd)", samp[j],
					     samp[j+1], samp[j+2], samp[j+3]));
    g_free(samp);
    return ret;
}

static PyObject *
gimp_Gradients_sample_custom(self, args)
     PyObject *self;	/* Not used */
     PyObject *args;
{
    int num, i, j;
    double *pos, *samp;
    PyObject *ret, *item;
    if (!PyArg_ParseTuple(args, "O:gradients_sample_custom", &ret))
	return NULL;
    if (!PySequence_Check(ret)) {
	PyErr_SetString(PyExc_TypeError,
			"second arg must be a sequence");
	return NULL;
    }
    num = PySequence_Length(ret);
    pos = g_new(gdouble, num);
    for (i = 0; i < num; i++) {
	item = PySequence_GetItem(ret, i);
	if (!PyFloat_Check(item)) {
	    PyErr_SetString(PyExc_TypeError,
			    "second arg must be a sequence of floats");
	    g_free(pos);
	    return NULL;
	}
	pos[i] = PyFloat_AsDouble(item);
    }
    samp = gimp_gradients_sample_custom(num, pos);
    g_free(pos);
    ret = PyList_New(num);
    for (i = 0, j = 0; i < num; i++, j += 4)
	PyList_SetItem(ret, i, Py_BuildValue("(dddd)", samp[j],
					     samp[j+1], samp[j+2], samp[j+3]));
    g_free(samp);
    return ret;
}


static PyObject *
gimp_image(self, args)
     PyObject *self, *args;
{
    unsigned int width, height;
    GImageType type;

    if (!PyArg_ParseTuple(args, "iii:image", &width, &height, &type))
	return NULL;
    return (PyObject *)newimgobject(gimp_image_new(width, height, type));
}


static PyObject *
gimp_layer(self, args)
     PyObject *self, *args;
{
    imgobject *img;
    char *name;
    unsigned int width, height;
    GDrawableType type;
    double opacity;
    GLayerMode mode;
	

    if (!PyArg_ParseTuple(args, "O!siiidi:layer", &Imgtype, &img, &name,
			  &width, &height, &type, &opacity, &mode))
	return NULL;
    return (PyObject *)newlayobject(gimp_layer_new(img->ID, name, width,
						   height, type, opacity, mode));
}


static PyObject *
gimp_channel(self, args)
     PyObject *self, *args;
{
    imgobject *img;
    char *name;
    unsigned int width, height, r, g, b;
    double opacity;
    unsigned char colour[3];

    if (!PyArg_ParseTuple(args, "O!siid(iii):channel", &Imgtype, &img,
			  &name, &width, &height, &opacity, &r, &g, &b))
	return NULL;
    colour[0] = r & 0xff;
    colour[1] = g & 0xff;
    colour[2] = b & 0xff;
    return (PyObject *)newchnobject(gimp_channel_new(img->ID, name,
						     width, height, opacity, colour));
}


static PyObject *
gimp_display(self, args)
     PyObject *self, *args;
{
    imgobject *img;

    if (!PyArg_ParseTuple(args, "O!:display", &Imgtype, &img))
	return NULL;
    return (PyObject *)newdispobject(gimp_display_new(img->ID));
}


static PyObject *
gimp_delete(self, args)
     PyObject *self, *args;
{
    imgobject *img;

    if (!PyArg_ParseTuple(args, "O:delete", &img))
	return NULL;
    if (img_check(img)) gimp_image_delete(img->ID);
    else if (lay_check(img)) gimp_layer_delete(img->ID);
    else if (chn_check(img)) gimp_channel_delete(img->ID);
    else if (disp_check(img)) gimp_display_delete(img->ID);
    Py_INCREF(Py_None);
    return Py_None;
}


static PyObject *
gimp_Displays_flush(self, args)
     PyObject *self, *args;
{
    if (!PyArg_ParseTuple(args, ":flush"))
	return NULL;
    gimp_displays_flush();
    Py_INCREF(Py_None);
    return Py_None;
}


static PyObject *
gimp_Tile_cache_size(self, args)
     PyObject *self, *args;
{
    unsigned long k;
    if (!PyArg_ParseTuple(args, "l:tile_cache_size", &k))
	return NULL;
    gimp_tile_cache_size(k);
    Py_INCREF(Py_None);
    return Py_None;
}


static PyObject *
gimp_Tile_cache_ntiles(self, args)
     PyObject *self, *args;
{
    unsigned long n;
    if (!PyArg_ParseTuple(args, "l:tile_cache_ntiles", &n))
	return NULL;
    gimp_tile_cache_ntiles(n);
    Py_INCREF(Py_None);
    return Py_None;
}


static PyObject *
gimp_Tile_width(self, args)
     PyObject *self, *args;
{
    if (PyArg_ParseTuple(args, ":tile_width"))
	return NULL;
    return PyInt_FromLong(gimp_tile_width());
}


static PyObject *
gimp_Tile_height(self, args)
     PyObject *self, *args;
{
    if (PyArg_ParseTuple(args, ":tile_height"))
	return NULL;
    return PyInt_FromLong(gimp_tile_height());
}

void gimp_extension_ack     (void);
void gimp_extension_process (guint timeout);

static PyObject *
gimp_Extension_ack(self, args)
     PyObject *self, *args;
{
    if (!PyArg_ParseTuple(args, ":extension_ack"))
	return NULL;
    gimp_extension_ack();
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
gimp_Extension_process(self, args)
     PyObject *self, *args;
{
    int timeout;

    if (!PyArg_ParseTuple(args, "i:extension_process", &timeout))
	return NULL;
    gimp_extension_process(timeout);
    Py_INCREF(Py_None);
    return Py_None;
}

#ifdef GIMP_HAVE_PARASITES
static PyObject *
new_parasite(self, args)
     PyObject *self, *args;
{
    char *name, *data;
    int flags, size;
    if (!PyArg_ParseTuple(args, "sis#:parasite", &name, &flags,
			  &data, &size))
	return NULL;
    return (PyObject *)newparaobject(parasite_new(name, flags, size, data));
}

static PyObject *
gimp_Parasite_find(self, args)
     PyObject *self, *args;
{
    char *name;
    if (!PyArg_ParseTuple(args, "s:parasite_find", &name))
	return NULL;
    return (PyObject *)newparaobject(gimp_parasite_find(name));
}

static PyObject *
gimp_Parasite_attach(self, args)
     PyObject *self, *args;
{
    paraobject *parasite;
    if (!PyArg_ParseTuple(args, "O!:parasite_attach", &Paratype, &parasite))
	return NULL;
    gimp_parasite_attach(parasite->para);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
gimp_Attach_new_parasite(self, args)
     PyObject *self, *args;
{
    char *name, *data;
    int flags, size;
    if (!PyArg_ParseTuple(args, "sis#:attach_new_parasite", &name, &flags,
			  &data, &size))
	return NULL;
    gimp_attach_new_parasite(name, flags, size, data);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
gimp_Parasite_detach(self, args)
     PyObject *self, *args;
{
    char *name;
    if (!PyArg_ParseTuple(args, "s:parasite_detach", &name))
	return NULL;
    gimp_parasite_detach(name);
    Py_INCREF(Py_None);
    return Py_None;
}
#endif

#ifdef GIMP_HAVE_DEFAULT_DISPLAY
static PyObject *
gimp_Default_display(self, args)
     PyObject *self, *args;
{
    if (!PyArg_ParseTuple(args, ":default_display"))
	return NULL;
    return (PyObject *)newdispobject(gimp_default_display());
}
#endif

static PyObject *
id2image(self, args)
     PyObject *self, *args;
{
    int id;
    if (!PyArg_ParseTuple(args, "i:_id2image", &id))
	return NULL;
    if (id >= 0)
	return (PyObject *)newimgobject(id);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
id2drawable(self, args)
     PyObject *self, *args;
{
    int id;
    if (!PyArg_ParseTuple(args, "i:_id2drawable", &id))
	return NULL;
    if (id >= 0)
	return (PyObject *)newdrwobject(NULL, id);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
id2display(self, args)
     PyObject *self, *args;
{
    int id;
    if (!PyArg_ParseTuple(args, "i:_id2display", &id))
	return NULL;
    if (id >= 0)
	return (PyObject *)newdispobject(id);
    Py_INCREF(Py_None);
    return Py_None;
}

/* List of methods defined in the module */

static struct PyMethodDef gimp_methods[] = {
    {"main",	(PyCFunction)gimp_Main,	METH_VARARGS},
    {"quit",	(PyCFunction)gimp_Quit,	METH_VARARGS},
    {"set_data",	(PyCFunction)gimp_Set_data,	METH_VARARGS},
    {"get_data",	(PyCFunction)gimp_Get_data,	METH_VARARGS},
    {"progress_init",	(PyCFunction)gimp_Progress_init,	METH_VARARGS},
    {"progress_update",	(PyCFunction)gimp_Progress_update,	METH_VARARGS},
    {"query_images",	(PyCFunction)gimp_Query_images,	METH_VARARGS},
    {"install_procedure",	(PyCFunction)gimp_Install_procedure,	METH_VARARGS},
    {"install_temp_proc",	(PyCFunction)gimp_Install_temp_proc,	METH_VARARGS},
    {"uninstall_temp_proc",	(PyCFunction)gimp_Uninstall_temp_proc,	METH_VARARGS},
    {"register_magic_load_handler",	(PyCFunction)gimp_Register_magic_load_handler,	METH_VARARGS},
    {"register_load_handler",	(PyCFunction)gimp_Register_load_handler,	METH_VARARGS},
    {"register_save_handler",	(PyCFunction)gimp_Register_save_handler,	METH_VARARGS},
    {"gamma",	(PyCFunction)gimp_Gamma,	METH_VARARGS},
    {"install_cmap",	(PyCFunction)gimp_Install_cmap,	METH_VARARGS},
    {"use_xshm",	(PyCFunction)gimp_Use_xshm,	METH_VARARGS},
    {"color_cube",	(PyCFunction)gimp_Color_cube,	METH_VARARGS},
    {"colour_cube", (PyCFunction)gimp_Color_cube,  METH_VARARGS},
    {"gtkrc",	(PyCFunction)gimp_Gtkrc,	METH_VARARGS},
    {"get_background",	(PyCFunction)gimp_Get_background,	METH_VARARGS},
    {"get_foreground",	(PyCFunction)gimp_Get_foreground,	METH_VARARGS},
    {"set_background",	(PyCFunction)gimp_Set_background,	METH_VARARGS},
    {"set_foreground",	(PyCFunction)gimp_Set_foreground,	METH_VARARGS},
    {"gradients_get_list",	(PyCFunction)gimp_Gradients_get_list,	METH_VARARGS},
    {"gradients_get_active",	(PyCFunction)gimp_Gradients_get_active,	METH_VARARGS},
    {"gradients_set_active",	(PyCFunction)gimp_Gradients_set_active,	METH_VARARGS},
    {"gradients_sample_uniform",	(PyCFunction)gimp_Gradients_sample_uniform,	METH_VARARGS},
    {"gradients_sample_custom",	(PyCFunction)gimp_Gradients_sample_custom,	METH_VARARGS},
    {"image", (PyCFunction)gimp_image, METH_VARARGS},
    {"layer", (PyCFunction)gimp_layer, METH_VARARGS},
    {"channel", (PyCFunction)gimp_channel, METH_VARARGS},
    {"display", (PyCFunction)gimp_display, METH_VARARGS},
    {"delete", (PyCFunction)gimp_delete, METH_VARARGS},
    {"displays_flush", (PyCFunction)gimp_Displays_flush, METH_VARARGS},
    {"tile_cache_size", (PyCFunction)gimp_Tile_cache_size, METH_VARARGS},
    {"tile_cache_ntiles", (PyCFunction)gimp_Tile_cache_ntiles, METH_VARARGS},
    {"tile_width", (PyCFunction)gimp_Tile_width, METH_VARARGS},
    {"tile_height", (PyCFunction)gimp_Tile_height, METH_VARARGS},
    {"extension_ack", (PyCFunction)gimp_Extension_ack, METH_VARARGS},
    {"extension_process", (PyCFunction)gimp_Extension_process, METH_VARARGS},
#ifdef GIMP_HAVE_PARASITES
    {"parasite",           (PyCFunction)new_parasite,            METH_VARARGS},
    {"parasite_find",      (PyCFunction)gimp_Parasite_find,      METH_VARARGS},
    {"parasite_attach",    (PyCFunction)gimp_Parasite_attach,    METH_VARARGS},
    {"attach_new_parasite",(PyCFunction)gimp_Attach_new_parasite,METH_VARARGS},
    {"parasite_detach",    (PyCFunction)gimp_Parasite_detach,    METH_VARARGS},
#endif
#ifdef GIMP_HAVE_DEFAULT_DISPLAY
    {"default_display",  (PyCFunction)gimp_Default_display,  METH_VARARGS},
#endif
    {"_id2image", (PyCFunction)id2image, METH_VARARGS},
    {"_id2drawable", (PyCFunction)id2drawable, METH_VARARGS},
    {"_id2display", (PyCFunction)id2display, METH_VARARGS},
    {NULL,	 (PyCFunction)NULL, 0, NULL}		/* sentinel */
};


/* Initialization function for the module (*must* be called initgimp) */

static char gimp_module_documentation[] = 
"This module provides interfaces to allow you to write gimp plugins"
;

void
initgimp()
{
    PyObject *m, *d;
    PyObject *i;

#if defined (_MSC_VER)
    /* see: Python FAQ 3.24 "Initializer not a constant." */
    Pdbtype.ob_type = &PyType_Type;
    Pftype.ob_type = &PyType_Type;
    Imgtype.ob_type = &PyType_Type;
    Disptype.ob_type = &PyType_Type;
    Laytype.ob_type = &PyType_Type;
    Chntype.ob_type = &PyType_Type;
    Tiletype.ob_type = &PyType_Type;
    Prtype.ob_type = &PyType_Type;
    Paratype.ob_type = &PyType_Type;
#endif

    /* Create the module and add the functions */
    m = Py_InitModule4("gimp", gimp_methods,
		       gimp_module_documentation,
		       (PyObject*)NULL,PYTHON_API_VERSION);

    /* Add some symbolic constants to the module */
    d = PyModule_GetDict(m);
    ErrorObject = PyString_FromString("gimp.error");
    PyDict_SetItemString(d, "error", ErrorObject);

    PyDict_SetItemString(d, "pdb", (PyObject *)newpdbobject());

    /* export the types used in gimpmodule */
    PyDict_SetItemString(d, "ImageType", (PyObject *)&Imgtype);
    PyDict_SetItemString(d, "LayerType", (PyObject *)&Laytype);
    PyDict_SetItemString(d, "ChannelType", (PyObject *)&Chntype);
    PyDict_SetItemString(d, "DisplayType", (PyObject *)&Disptype);
    PyDict_SetItemString(d, "TileType", (PyObject *)&Tiletype);
    PyDict_SetItemString(d, "PixelRegionType", (PyObject *)&Prtype);
#ifdef GIMP_HAVE_PARASITES
    PyDict_SetItemString(d, "ParasiteType", (PyObject *)&Paratype);
#endif

    PyDict_SetItemString(d, "major_version",
			 i=PyInt_FromLong(gimp_major_version));
    Py_DECREF(i);
    PyDict_SetItemString(d, "minor_version",
			 i=PyInt_FromLong(gimp_minor_version));
    Py_DECREF(i);
    PyDict_SetItemString(d, "micro_version",
			 i=PyInt_FromLong(gimp_micro_version));
    Py_DECREF(i);
	
    /* Check for errors */
    if (PyErr_Occurred())
	Py_FatalError("can't initialize module gimp");
}

