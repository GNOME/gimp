/* -*- Mode: C; c-basic-offset: 4 -*-
 * Gimp-Python - allows the writing of Gimp plugins in Python.
 * Copyright (C) 1997-2002  James Henstridge <james@daa.com.au>
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

#define NO_IMPORT_PYGOBJECT
#include <pygobject.h>

#include "pygimp.h"

#define NO_IMPORT_PYGIMPCOLOR
#include "pygimpcolor-api.h"

#include <structmember.h>

#include <glib-object.h>
#include <glib/gprintf.h>

#ifndef PG_DEBUG
# define PG_DEBUG 0
#endif

/* ----------------------------------------------------- */

/* Declarations for objects of type pdb */

typedef struct {
    PyObject_HEAD
} PyGimpPDB;


/* ---------------------------------------------------------------- */

/* Declarations for objects of type pdbFunc */

typedef struct {
    PyObject_HEAD
    char *name;
    PyObject *proc_name, *proc_blurb, *proc_help, *proc_author,
	*proc_copyright, *proc_date, *proc_type, *py_params,
	*py_return_vals;
    int nparams, nreturn_vals;
    GimpParamDef *params, *return_vals;
} PyGimpPDBFunction;

static PyObject *pygimp_pdb_function_new_from_proc_db(char *name);

/* ---------------------------------------------------------------- */

/* routines to convert between Python tuples and gimp GimpParam's */

#if PG_DEBUG > 0

static void
pygimp_param_print(int nparams, GimpParam *params)
{
    int i;

    for (i = 0; i < nparams; i++) {
	g_printf("param_print: type: %d, PDB_ITEM: %d\n",  params[i].type, GIMP_PDB_ITEM);
	switch(params[i].type) {
	case GIMP_PDB_INT32:
	    g_printerr("%i. int %i\n", i,
                       params[i].data.d_int32);
	    break;
	case GIMP_PDB_INT16:
	    g_printerr("%i. int %i\n", i,
                       params[i].data.d_int16);
	    break;
	case GIMP_PDB_INT8:
	    g_printerr("%i. int %u\n", i,
                       params[i].data.d_int8);
	    break;
	case GIMP_PDB_FLOAT:
	    g_printerr("%i. float %f\n", i,
                       params[i].data.d_float);
	    break;
	case GIMP_PDB_STRING:
	    g_printerr("%i. string %s\n", i,
                       params[i].data.d_string);
	    break;
	case GIMP_PDB_INT32ARRAY:
	case GIMP_PDB_INT16ARRAY:
	case GIMP_PDB_INT8ARRAY:
	case GIMP_PDB_FLOATARRAY:
	case GIMP_PDB_STRINGARRAY:
	    g_printerr("%i. array of type %i %s\n", i,
                       params[i].type,
                       params[i].data.d_int32array == NULL ? "(null)":"");
	    break;
	case GIMP_PDB_STATUS:
	    g_printerr("%i. status %i\n", i,
                       params[i].data.d_status);
	    break;
	default:
	    g_printerr("%i. other %i\n", i,
                       params[i].data.d_int32);
	    break;
	}
    }
}

#endif

PyObject *
pygimp_param_to_tuple(int nparams, const GimpParam *params)
{
    PyObject *args, *tmp;
    int i, j, n;

    args = PyTuple_New(nparams);
    for (i = 0; i < nparams && params[i].type != GIMP_PDB_END; i++) {
	PyObject *value = NULL;

#if PG_DEBUG > 1
	g_printf("param_to_tuple: type: %d, PDB_ITEM: %d\n",  params[i].type, GIMP_PDB_ITEM);
#endif

	switch(params[i].type) {
	case GIMP_PDB_INT32:
	    value = PyInt_FromLong(params[i].data.d_int32);
	    break;
	case GIMP_PDB_INT16:
	    value = PyInt_FromLong(params[i].data.d_int16);
	    break;
	case GIMP_PDB_INT8:
	    value = PyInt_FromLong(params[i].data.d_int8);
	    break;
	case GIMP_PDB_FLOAT:
	    value = PyFloat_FromDouble(params[i].data.d_float);
	    break;
	case GIMP_PDB_STRING:
	    if (params[i].data.d_string == NULL) {
		Py_INCREF(Py_None);
		value = Py_None;
	    } else
		value = PyString_FromString(params[i].data.d_string);
	    break;

	    /* For these to work, the previous argument must have
	     * been an integer
	     */
	case GIMP_PDB_INT32ARRAY:
	    if (params[i].data.d_int32array == NULL) {
		value = PyTuple_New(0);
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
	    value = PyTuple_New(n);
	    for (j = 0; j < n; j++)
		PyTuple_SetItem(value, j,
			PyInt_FromLong(params[i].data.d_int32array[j]));
	    break;
	case GIMP_PDB_INT16ARRAY:
	    if (params[i].data.d_int16array == NULL) {
		value = PyTuple_New(0);
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
	    value = PyTuple_New(n);
	    for (j = 0; j < n; j++)
		PyTuple_SetItem(value, j,
			PyInt_FromLong(params[i].data.d_int16array[j]));
	    break;
	case GIMP_PDB_INT8ARRAY:
	    if (params[i].data.d_int8array == NULL) {
		value = PyTuple_New(0);
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
	    value = PyTuple_New(n);
	    for (j = 0; j < n; j++)
		PyTuple_SetItem(value, j,
			PyInt_FromLong(params[i].data.d_int8array[j]));
	    break;
	case GIMP_PDB_FLOATARRAY:
	    if (params[i].data.d_floatarray == NULL) {
		value = PyTuple_New(0);
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
	    value = PyTuple_New(n);
	    for (j = 0; j < n; j++)
		PyTuple_SetItem(value, j,
			PyFloat_FromDouble(params[i].data.d_floatarray[j]));
	    break;
	case GIMP_PDB_STRINGARRAY:
	    if (params[i].data.d_stringarray == NULL) {
		value = PyTuple_New(0);
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
	    value = PyTuple_New(n);
	    for (j = 0; j < n; j++)
		PyTuple_SetItem(value, j,
			params[i].data.d_stringarray[j] ?
			PyString_FromString(params[i].data.d_stringarray[j]) :
			PyString_FromString(""));
	    break;
	case GIMP_PDB_COLOR:
	    value = pygimp_rgb_new(&params[i].data.d_color);
	    break;
	/*
	GIMP_PDB_REGION is deprecated in libgimpbase/gimpbaseenums.h
	and conflicts with GIMP_PDB_ITEM
	case GIMP_PDB_REGION:
	    value = Py_BuildValue("(iiii)",
				  (int) params[i].data.d_region.x,
				  (int) params[i].data.d_region.y,
				  (int) params[i].data.d_region.width,
				  (int) params[i].data.d_region.height);
	    break;
	*/
	case GIMP_PDB_DISPLAY:
	    value = pygimp_display_new(params[i].data.d_display);
	    break;
	case GIMP_PDB_IMAGE:
	    value = pygimp_image_new(params[i].data.d_image);
	    break;
	case GIMP_PDB_LAYER:
	    value = pygimp_layer_new(params[i].data.d_layer);
	    break;
	case GIMP_PDB_CHANNEL:
	    value = pygimp_channel_new(params[i].data.d_channel);
	    break;
	case GIMP_PDB_ITEM:
	    value = pygimp_item_new(params[i].data.d_item);
	    break;
	case GIMP_PDB_DRAWABLE:
	    value = pygimp_drawable_new(NULL, params[i].data.d_drawable);
	    break;
	case GIMP_PDB_SELECTION:
	    value = pygimp_channel_new(params[i].data.d_selection);
	    break;
	case GIMP_PDB_COLORARRAY:
	    if (params[i].data.d_colorarray == NULL) {
		value = PyTuple_New(0);
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
	    value = PyTuple_New(n);
	    for (j = 0; j < n; j++)
		PyTuple_SetItem(value, j,
                                pygimp_rgb_new(&params[i].data.d_colorarray[j]));
	    break;
	case GIMP_PDB_VECTORS:
	    value = pygimp_vectors_new(params[i].data.d_vectors);
	    break;
	case GIMP_PDB_PARASITE:
	    value = pygimp_parasite_new(gimp_parasite_copy(
					&(params[i].data.d_parasite)));
	    break;
	case GIMP_PDB_STATUS:
	    value = PyInt_FromLong(params[i].data.d_status);
	    break;
	case GIMP_PDB_END:
	    break;
	}
	PyTuple_SetItem(args, i, value);
    }
    return args;
}

GimpParam *
pygimp_param_from_tuple(PyObject *args, const GimpParamDef *ptype, int nparams)
{
    PyObject *tuple, *item, *x, *y;
    GimpParam *ret;
    int i, j, len;
    gint32 *i32a; gint16 *i16a; guint8 *i8a; gdouble *fa; gchar **sa;

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
        Py_DECREF(tuple);
	return NULL;
    }

    if (PyTuple_Size(tuple) != nparams) {
	PyErr_SetString(PyExc_TypeError, "wrong number of parameters");
        Py_DECREF(tuple);
	return NULL;
    }

    ret = g_new(GimpParam, nparams+1);
    for (i = 0; i <= nparams; i++)
	ret[i].type = GIMP_PDB_STATUS;
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
#if PG_DEBUG > 1
	g_printf("param_from_tuple: type: %d, PDB_ITEM: %d\n",  ptype[i-1].type, GIMP_PDB_ITEM);
#endif
	switch (ptype[i-1].type) {
	case GIMP_PDB_INT32:
	    check((x = PyNumber_Int(item)) == NULL);
	    ret[i].data.d_int32 = (gint32)PyInt_AsLong(x);
	    Py_DECREF(x);
	    break;
	case GIMP_PDB_INT16:
	    check((x = PyNumber_Int(item)) == NULL);
	    ret[i].data.d_int16 = (gint16)PyInt_AsLong(x);
	    Py_DECREF(x);
	    break;
	case GIMP_PDB_INT8:
	    check((x = PyNumber_Int(item)) == NULL);
	    ret[i].data.d_int8 = (guint8)PyInt_AsLong(x);
	    Py_DECREF(x);
	    break;
	case GIMP_PDB_FLOAT:
	    check((x = PyNumber_Float(item)) == NULL);
	    ret[i].data.d_float = PyFloat_AsDouble(x);
	    Py_DECREF(x);
	    break;
	case GIMP_PDB_STRING:
	    if (item == Py_None) {
		ret[i].data.d_string = NULL;
		break;
	    }
	    check((x = PyObject_Str(item)) == NULL);
	    ret[i].data.d_string = g_strdup(PyString_AsString(x));
	    Py_DECREF(x);
	    break;
	case GIMP_PDB_INT32ARRAY:
	    check(!PySequence_Check(item));
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
	case GIMP_PDB_INT16ARRAY:
	    check(!PySequence_Check(item));
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
	case GIMP_PDB_INT8ARRAY:
	    check(!PySequence_Check(item));
	    len = PySequence_Length(item);
	    i8a = g_new(guint8, len);
	    for (j = 0; j < len; j++) {
		x = PySequence_GetItem(item, j);
		arraycheck((y=PyNumber_Int(x))==NULL,
			   i8a);
		i8a[j] = PyInt_AsLong(y);
		Py_DECREF(y);
	    }
	    ret[i].data.d_int8array = i8a;
	    break;
	case GIMP_PDB_FLOATARRAY:
	    check(!PySequence_Check(item));
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
	case GIMP_PDB_STRINGARRAY:
	    check(!PySequence_Check(item));
	    len = PySequence_Length(item);
	    sa = g_new(gchar *, len);
	    for (j = 0; j < len; j++) {
		x = PySequence_GetItem(item, j);
		if (x == Py_None) {
		    sa[j] = NULL;
		    continue;
		}
		arraycheck((y=PyObject_Str(x))==NULL,
			   sa);
		sa[j] = g_strdup(PyString_AsString(y));
		Py_DECREF(y);
	    }
	    ret[i].data.d_stringarray = sa;
	    break;
	case GIMP_PDB_COLOR:
	    {
                GimpRGB rgb;

                if (!pygimp_rgb_from_pyobject(item, &rgb)) {
                    Py_DECREF(tuple);
                    gimp_destroy_params(ret, nparams);
                    return NULL;
		}

                ret[i].data.d_color = rgb;
	    }
	    break;
/*
	case GIMP_PDB_REGION:
	    check(!PySequence_Check(item) ||
		  PySequence_Length(item) < 4);
	    x = PySequence_GetItem(item, 0);
	    y = PySequence_GetItem(item, 1);
	    w = PySequence_GetItem(item, 2);
	    h = PySequence_GetItem(item, 3);
	    check(!PyInt_Check(x) || !PyInt_Check(y) ||
		  !PyInt_Check(w) || !PyInt_Check(h));
	    ret[i].data.d_region.x = PyInt_AsLong(x);
	    ret[i].data.d_region.y = PyInt_AsLong(y);
	    ret[i].data.d_region.width = PyInt_AsLong(w);
	    ret[i].data.d_region.height = PyInt_AsLong(h);
	    break;
*/
	case GIMP_PDB_DISPLAY:
            if (item == Py_None) {
                ret[i].data.d_display = -1;
                break;
            }
	    check(!pygimp_display_check(item));
	    ret[i].data.d_display = ((PyGimpDisplay *)item)->ID;
	    break;
	case GIMP_PDB_IMAGE:
	    if (item == Py_None) {
		ret[i].data.d_image = -1;
		break;
	    }
	    check(!pygimp_image_check(item));
	    ret[i].data.d_image = ((PyGimpImage *)item)->ID;
	    break;
	case GIMP_PDB_LAYER:
	    if (item == Py_None) {
		ret[i].data.d_layer = -1;
		break;
	    }
	    check(!pygimp_layer_check(item));
	    ret[i].data.d_layer = ((PyGimpLayer *)item)->ID;
	    break;
	case GIMP_PDB_CHANNEL:
	    if (item == Py_None) {
		ret[i].data.d_channel = -1;
		break;
	    }
	    check(!pygimp_channel_check(item));
	    ret[i].data.d_channel = ((PyGimpChannel *)item)->ID;
	    break;
	case GIMP_PDB_ITEM:
	    if (item == Py_None) {
		ret[i].data.d_channel = -1;
		break;
	    }
	    check(!pygimp_item_check(item));
	    ret[i].data.d_item = ((PyGimpItem *)item)->ID;
	    break;
	case GIMP_PDB_DRAWABLE:
	    if (item == Py_None) {
		ret[i].data.d_channel = -1;
		break;
	    }
	    check(!pygimp_drawable_check(item));
	    ret[i].data.d_channel = ((PyGimpDrawable *)item)->ID;
	    break;
	case GIMP_PDB_SELECTION:
	    if (item == Py_None) {
		ret[i].data.d_channel = -1;
		break;
	    }
	    check(!pygimp_channel_check(item));
	    ret[i].data.d_selection = ((PyGimpChannel *)item)->ID;
	    break;
	case GIMP_PDB_COLORARRAY:
	    {
                GimpRGB *rgb;

		check(!PySequence_Check(item));
		len = PySequence_Length(item);
		rgb = g_new(GimpRGB, len);
		for (j = 0; j < len; j++) {
		    if (!pygimp_rgb_from_pyobject(item, &rgb[j])) {
			Py_DECREF(tuple);
			g_free(rgb);
			gimp_destroy_params(ret, nparams);
			return NULL;
		    }
		}
                ret[i].data.d_colorarray = rgb;
	    }
	    break;
	case GIMP_PDB_VECTORS:
	    if (item == Py_None) {
		ret[i].data.d_vectors = -1;
		break;
	    }
	    check(!pygimp_vectors_check(item));
	    ret[i].data.d_vectors = ((PyGimpVectors *)item)->ID;
	    break;
	case GIMP_PDB_PARASITE:
	    /* can't do anything, since size of GimpParasite is not known */
	    break;
	case GIMP_PDB_STATUS:
	    check(!PyInt_Check(item));
	    ret[i].data.d_status = PyInt_AsLong(item);
	    break;
	case GIMP_PDB_END:
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

static PyObject *
pdb_query(PyGimpPDB *self, PyObject *args)
{
    char *n=".*", *b=".*", *h=".*", *a=".*", *c=".*", *d=".*", *t=".*";
    int num, i;
    char **names;
    PyObject *ret;

    if (!PyArg_ParseTuple(args, "|zzzzzzz:gimp.pdb.query", &n, &b, &h, &a,
			  &c, &d, &t))
	return NULL;

    gimp_procedural_db_query(n, b, h, a, c, d, t, &num, &names);

    ret = PyList_New(num);

    for (i = 0; i < num; i++)
	PyList_SetItem(ret, i, PyString_FromString(names[i]));

    g_strfreev(names);

    return ret;
}

static PyMethodDef pdb_methods[] = {
    {"query", (PyCFunction)pdb_query, METH_VARARGS},
    {NULL,		NULL}		/* sentinel */
};

/* ---------- */


PyObject *
pygimp_pdb_new(void)
{
    PyGimpPDB *self = PyObject_NEW(PyGimpPDB, &PyGimpPDB_Type);

    if (self == NULL)
	return NULL;

    return (PyObject *)self;
}


static void
pdb_dealloc(PyGimpPDB *self)
{
    PyObject_DEL(self);
}

static PyObject *
pdb_repr(PyGimpPDB *self)
{
    return PyString_FromString("<gimp procedural database>");
}

/* Code to access pdb objects as mappings */

static PyObject *
pdb_subscript(PyGimpPDB *self, PyObject *key)
{
    PyObject *r;

    if (!PyString_Check(key)) {
	PyErr_SetString(PyExc_TypeError, "Subscript must be a string");
	return NULL;
    }

    r = (PyObject *)pygimp_pdb_function_new_from_proc_db(PyString_AsString(key));

    if (r == NULL) {
	PyErr_Clear();
	PyErr_SetObject(PyExc_KeyError, key);
    }

    return r;
}

static PyMappingMethods pdb_as_mapping = {
    (lenfunc)0,			/*mp_length*/
    (binaryfunc)pdb_subscript,	/*mp_subscript*/
    (objobjargproc)0,		/*mp_ass_subscript*/
};

/* -------------------------------------------------------- */

static PyObject *
build_procedure_list(void)
{
    int num, i;
    char **names, *name, *p;
    PyObject *ret;

    gimp_procedural_db_query(".*", ".*", ".*", ".*", ".*", ".*", ".*",
                             &num, &names);

    ret = PyList_New(num);

    for (i = 0; i < num; i++) {
        name = g_strdup(names[i]);
        for (p = name; *p != '\0'; p++) {
            if (*p == '-')
                *p = '_';
	}
        PyList_SetItem(ret, i, PyString_FromString(name));
    }

    g_strfreev(names);

    return ret;
}

static PyObject *
pdb_getattro(PyGimpPDB *self, PyObject *attr)
{
    char *attr_name;
    PyObject *ret;

    attr_name = PyString_AsString(attr);
    if (!attr_name) {
         PyErr_Clear();
         return PyObject_GenericGetAttr((PyObject *)self, attr);
    }

    if (attr_name[0] == '_') {
        if (!strcmp(attr_name, "__members__")) {
            return build_procedure_list();
        } else {
            return PyObject_GenericGetAttr((PyObject *)self, attr);
        }
    }

    ret = PyObject_GenericGetAttr((PyObject *)self, attr);
    if (ret)
        return ret;

    PyErr_Clear();

    return pygimp_pdb_function_new_from_proc_db(attr_name);
}

PyTypeObject PyGimpPDB_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                                  /* ob_size */
    "gimp.PDB",                         /* tp_name */
    sizeof(PyGimpPDB),                  /* tp_basicsize */
    0,                                  /* tp_itemsize */
    /* methods */
    (destructor)pdb_dealloc,           /* tp_dealloc */
    (printfunc)0,                       /* tp_print */
    (getattrfunc)0,                     /* tp_getattr */
    (setattrfunc)0,                     /* tp_setattr */
    (cmpfunc)0,                         /* tp_compare */
    (reprfunc)pdb_repr,                 /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    &pdb_as_mapping,                     /* tp_as_mapping */
    (hashfunc)0,                        /* tp_hash */
    (ternaryfunc)0,                     /* tp_call */
    (reprfunc)0,                        /* tp_str */
    (getattrofunc)pdb_getattro,         /* tp_getattro */
    (setattrofunc)0,                    /* tp_setattro */
    0,					/* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,	                /* tp_flags */
    NULL, /* Documentation string */
    (traverseproc)0,			/* tp_traverse */
    (inquiry)0,				/* tp_clear */
    (richcmpfunc)0,			/* tp_richcompare */
    0,					/* tp_weaklistoffset */
    (getiterfunc)0,			/* tp_iter */
    (iternextfunc)0,			/* tp_iternext */
    pdb_methods,			/* tp_methods */
    0,					/* tp_members */
    0,					/* tp_getset */
    (PyTypeObject *)0,			/* tp_base */
    (PyObject *)0,			/* tp_dict */
    0,					/* tp_descr_get */
    0,					/* tp_descr_set */
    0,					/* tp_dictoffset */
    (initproc)0,	                /* tp_init */
    (allocfunc)0,			/* tp_alloc */
    (newfunc)0,				/* tp_new */
};

/* End of code for pdb objects */
/* -------------------------------------------------------- */


static PyObject *
pygimp_pdb_function_new_from_proc_db(char *name)
{
    PyObject *ret;
    char *b,*h,*a,*c,*d;
    int np, nr;
    GimpPDBProcType pt;
    GimpParamDef *p, *r;

    if (!gimp_procedural_db_proc_info (name, &b, &h, &a, &c, &d, &pt,
				       &np, &nr, &p, &r)) {
	PyErr_SetString(pygimp_error, "procedure not found");
	return NULL;
    }

    ret = pygimp_pdb_function_new(name, b, h, a, c, d, pt, np, nr, p, r);

    g_free(b); g_free(h); g_free(a); g_free(c); g_free(d);

    return ret;
}

static void
pf_dealloc(PyGimpPDBFunction *self)
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

    PyObject_DEL(self);
}

#define OFF(x) offsetof(PyGimpPDBFunction, x)
static struct PyMemberDef pf_members[] = {
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
pf_repr(PyGimpPDBFunction *self)
{
    return PyString_FromFormat("<pdb function '%s'>",
			       PyString_AsString(self->proc_name));
}

static PyObject *
pf_call(PyGimpPDBFunction *self, PyObject *args, PyObject *kwargs)
{
    GimpParam *params, *ret;
    int nret;
    PyObject *t = NULL, *r;
    GimpRunMode run_mode = GIMP_RUN_NONINTERACTIVE;

#if PG_DEBUG > 0
    g_printerr("--- %s --- ", PyString_AsString(self->proc_name));
#endif

    if (kwargs) {
        Py_ssize_t len, pos;
        PyObject *key, *val;

        len = PyDict_Size(kwargs);

        if (len == 1) {
            pos = 0;
            PyDict_Next(kwargs, &pos, &key, &val);

            if (!PyString_Check(key)) {
                PyErr_SetString(PyExc_TypeError,
                                "keyword argument name is not a string");
                return NULL;
            }

            if (strcmp(PyString_AsString(key), "run_mode") != 0) {
                PyErr_SetString(PyExc_TypeError,
                                "only 'run_mode' keyword argument accepted");
                return NULL;
            }

            if (pyg_enum_get_value(GIMP_TYPE_RUN_MODE, val, (gpointer)&run_mode))
                return NULL;
        } else if (len != 0) {
            PyErr_SetString(PyExc_TypeError,
                            "expecting at most one keyword argument");
            return NULL;
        }
    }

    if (self->nparams > 0 && !strcmp(self->params[0].name, "run-mode")) {
	params = pygimp_param_from_tuple(args, self->params + 1,
					 self->nparams - 1);

	if (params == NULL)
	    return NULL;

	params[0].type = self->params[0].type;
	params[0].data.d_int32 = run_mode;

#if PG_DEBUG > 1
	pygimp_param_print(self->nparams, params);
#endif

	ret = gimp_run_procedure2(self->name, &nret, self->nparams, params);
    } else {
	params = pygimp_param_from_tuple(args, self->params, self->nparams);

	if (params == NULL)
	    return NULL;

#if PG_DEBUG > 1
	pygimp_param_print(self->nparams, params+1);
#endif

	ret = gimp_run_procedure2(self->name, &nret, self->nparams, params + 1);
    }

    gimp_destroy_params(params, self->nparams);

    if (!ret) {
	PyErr_SetString(pygimp_error, "no status returned");
#if PG_DEBUG >= 1
	g_printerr("ret == NULL\n");
#endif
	return NULL;
    }

    switch(ret[0].data.d_status) {
    case GIMP_PDB_SUCCESS:
#if PG_DEBUG > 0
	g_printerr("success\n");
#endif
	t = pygimp_param_to_tuple(nret-1, ret+1);
	gimp_destroy_params(ret, nret);

	if (t == NULL) {
	    PyErr_SetString(pygimp_error, "could not make return value");
	    return NULL;
	}
	break;

    case GIMP_PDB_EXECUTION_ERROR:
#if PG_DEBUG > 0
	g_printerr("execution error\n");
#endif
        PyErr_SetString(PyExc_RuntimeError, gimp_get_pdb_error());
	gimp_destroy_params(ret, nret);
	return NULL;

    case GIMP_PDB_CALLING_ERROR:
#if PG_DEBUG > 0
	g_printerr("calling error\n");
#endif
        PyErr_SetString(PyExc_RuntimeError, gimp_get_pdb_error());
	gimp_destroy_params(ret, nret);
	return NULL;

    case GIMP_PDB_CANCEL:
#if PG_DEBUG > 0
	g_printerr("cancel\n");
#endif
        PyErr_SetString(PyExc_RuntimeError, gimp_get_pdb_error());
	gimp_destroy_params(ret, nret);
	return NULL;

    default:
#if PG_DEBUG > 0
	g_printerr("unknown - %i (type %i)\n",
                   ret[0].data.d_status, ret[0].type);
#endif
	PyErr_SetString(pygimp_error, "unknown return code");
	return NULL;
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


PyTypeObject PyGimpPDBFunction_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                                  /* ob_size */
    "gimp.PDBFunction",                 /* tp_name */
    sizeof(PyGimpPDBFunction),          /* tp_basicsize */
    0,                                  /* tp_itemsize */
    /* methods */
    (destructor)pf_dealloc,             /* tp_dealloc */
    (printfunc)0,                       /* tp_print */
    (getattrfunc)0,                     /* tp_getattr */
    (setattrfunc)0,                     /* tp_setattr */
    (cmpfunc)0,                         /* tp_compare */
    (reprfunc)pf_repr,                  /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    (hashfunc)0,                        /* tp_hash */
    (ternaryfunc)pf_call,               /* tp_call */
    (reprfunc)0,                        /* tp_str */
    (getattrofunc)0,                    /* tp_getattro */
    (setattrofunc)0,                    /* tp_setattro */
    0,					/* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,	                /* tp_flags */
    NULL, /* Documentation string */
    (traverseproc)0,			/* tp_traverse */
    (inquiry)0,				/* tp_clear */
    (richcmpfunc)0,			/* tp_richcompare */
    0,					/* tp_weaklistoffset */
    (getiterfunc)0,			/* tp_iter */
    (iternextfunc)0,			/* tp_iternext */
    0,					/* tp_methods */
    pf_members,				/* tp_members */
    0,					/* tp_getset */
    (PyTypeObject *)0,			/* tp_base */
    (PyObject *)0,			/* tp_dict */
    0,					/* tp_descr_get */
    0,					/* tp_descr_set */
    0,					/* tp_dictoffset */
    (initproc)0,	                /* tp_init */
    (allocfunc)0,			/* tp_alloc */
    (newfunc)0,				/* tp_new */
};

PyObject *
pygimp_pdb_function_new(const char *name, const char *blurb, const char *help,
			const char *author, const char *copyright,
			const char *date, GimpPDBProcType proc_type,
			int n_params, int n_return_vals,
			GimpParamDef *params, GimpParamDef *return_vals)
{
    PyGimpPDBFunction *self;
    int i;

    self = PyObject_NEW(PyGimpPDBFunction, &PyGimpPDBFunction_Type);

    if (self == NULL)
	return NULL;

    self->name = g_strdup(name);
    self->proc_name = PyString_FromString(name ? name : "");
    self->proc_blurb = PyString_FromString(blurb ? blurb : "");
    self->proc_help = PyString_FromString(help ? help : "");
    self->proc_author = PyString_FromString(author ? author : "");
    self->proc_copyright = PyString_FromString(copyright ? copyright : "");
    self->proc_date = PyString_FromString(date ? date : "");
    self->proc_type = PyInt_FromLong(proc_type);
    self->nparams = n_params;
    self->nreturn_vals = n_return_vals;
    self->params = params;
    self->return_vals = return_vals;

    self->py_params = PyTuple_New(n_params);
    for (i = 0; i < n_params; i++)
	PyTuple_SetItem(self->py_params, i,
			Py_BuildValue("(iss)",
				      params[i].type,
				      params[i].name,
				      params[i].description));

    self->py_return_vals = PyTuple_New(n_return_vals);
    for (i = 0; i < n_return_vals; i++)
	PyTuple_SetItem(self->py_return_vals, i,
			Py_BuildValue("(iss)",
				      return_vals[i].type,
				      return_vals[i].name,
				      return_vals[i].description));

    return (PyObject *)self;
}
