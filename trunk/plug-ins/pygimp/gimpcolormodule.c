/* -*- Mode: C; c-basic-offset: 4 -*-
    Gimp-Python - allows the writing of Gimp plugins in Python.
    Copyright (C) 2005-2006  Manish Singh <yosh@gimp.org>

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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "pygimpcolor.h"

#define _INSIDE_PYGIMPCOLOR_
#include "pygimpcolor-api.h"

#include "pygimp-util.h"


static PyObject *
pygimp_rgb_parse_name(PyObject *self, PyObject *args, PyObject *kwargs)
{
    char *name;
    int len;
    GimpRGB rgb;
    gboolean success;
    static char *kwlist[] = { "name", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s#:rgb_parse_name", kwlist,
				     &name, &len))
        return NULL;

    rgb.a = 1.0;
    success = gimp_rgb_parse_name(&rgb, name, len);
    PyMem_Free(name);

    if (!success) {
	PyErr_SetString(PyExc_ValueError, "unable to parse color name");
	return NULL;
    }

    return pygimp_rgb_new(&rgb);
}

static PyObject *
pygimp_rgb_parse_hex(PyObject *self, PyObject *args, PyObject *kwargs)
{
    char *hex;
    int len;
    GimpRGB rgb;
    gboolean success;
    static char *kwlist[] = { "hex", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s#:rgb_parse_hex", kwlist,
				     &hex, &len))
        return NULL;

    rgb.a = 1.0;
    success = gimp_rgb_parse_hex(&rgb, hex, len);
    PyMem_Free(hex);

    if (!success) {
	PyErr_SetString(PyExc_ValueError, "unable to parse hex value");
	return NULL;
    }

    return pygimp_rgb_new(&rgb);
}

static PyObject *
pygimp_rgb_parse_css(PyObject *self, PyObject *args, PyObject *kwargs)
{
    char *css;
    int len;
    GimpRGB rgb;
    gboolean success, with_alpha = FALSE;
    static char *kwlist[] = { "css", "with_alpha", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
				     "s#|i:rgb_parse_css", kwlist,
				     &css, &len, &with_alpha))
        return NULL;

    if (with_alpha)
	success = gimp_rgba_parse_css(&rgb, css, len);
    else {
	rgb.a = 1.0;
	success = gimp_rgb_parse_css(&rgb, css, len);
    }

    if (!success) {
	PyErr_SetString(PyExc_ValueError, "unable to parse CSS color");
	return NULL;
    }

    return pygimp_rgb_new(&rgb);
}

static PyObject *
pygimp_rgb_list_names(PyObject *self)
{
    int num_names, i;
    const char **names;
    GimpRGB *colors;
    PyObject *dict, *color;

    num_names = gimp_rgb_list_names(&names, &colors);

    dict = PyDict_New();
    if (!dict)
        goto cleanup;

    for (i = 0; i < num_names; i++) {
        color = pygimp_rgb_new(&colors[i]);

	if (!color)
	    goto bail;

	if (PyDict_SetItemString(dict, names[i], color) < 0) {
	    Py_DECREF(color);
	    goto bail;
	}

	Py_DECREF(color);
    }

    goto cleanup;

bail:
    Py_DECREF(dict);
    dict = NULL;

cleanup:
    g_free(names);
    g_free(colors);

    return dict;
}

#if 0
static PyObject *
pygimp_bilinear(PyObject *self, PyObject *args, PyObject *kwargs)
{
    gdouble x, y;
    gdouble values[4];
    static char *kwlist[] = { "x", "y", "values", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
				     "dd(dddd):bilinear", kwlist,
				     &x, &y,
				     &values[0], &values[1],
				     &values[2], &values[3]))
  	return NULL;

    return PyFloat_FromDouble(gimp_bilinear(x, y, values));
}

static PyObject *
pygimp_bilinear_8(PyObject *self, PyObject *args, PyObject *kwargs)
{
    gdouble x, y;
    char *values;
    int len;
    guchar r;
    static char *kwlist[] = { "x", "y", "values", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
				     "dds#:bilinear_8", kwlist,
				     &x, &y, &values, &len))
  	return NULL;

    if (len != 4) {
	PyErr_SetString(PyExc_ValueError,
			"string must be exactly 4 bytes long");
	return NULL;
    }

    r = gimp_bilinear_8(x, y, values);

    return PyString_FromStringAndSize(&r, 1);
}

static PyObject *
pygimp_bilinear_32(PyObject *self, PyObject *args, PyObject *kwargs)
{
    gdouble x, y;
    guint32 values[4];
    static char *kwlist[] = { "x", "y", "values", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
				     "dd(IIII):bilinear_32",
				     kwlist,
				     &x, &y,
				     &values[0], &values[1],
				     &values[2], &values[3]))
  	return NULL;

    return PyInt_FromLong(gimp_bilinear_32(x, y, values));
}

static PyObject *
pygimp_bilinear_pixels_8(PyObject *self, PyObject *args, PyObject *kwargs)
{
    Py_INCREF(Py_None);
    return Py_None;
}

typedef struct
{
    PyObject *func;
    PyObject *data;
} ProxyData;

static void
proxy_render(gdouble x, gdouble y, GimpRGB *color, gpointer pdata)
{
    ProxyData *data = pdata;

    if (data->data)
	PyObject_CallFunction(data->func, "ddO&O", x, y, pygimp_rgb_new, color,
			      data->data);
    else
	PyObject_CallFunction(data->func, "ddO&", x, y, pygimp_rgb_new, color);
}

static void
proxy_put_pixel(gint x, gint y, GimpRGB *color, gpointer pdata)
{
    ProxyData *data = pdata;

    if (data->data)
	PyObject_CallFunction(data->func, "iiO&O", x, y, pygimp_rgb_new, color,
			      data->data);
    else
	PyObject_CallFunction(data->func, "iiO&", x, y, pygimp_rgb_new, color);
}

static void
proxy_progress(gint min, gint max, gint current, gpointer pdata)
{
    ProxyData *data = pdata;

    if (data->data)
	PyObject_CallFunction(data->func, "iiiO", min, max, current,
			      data->data);
    else
	PyObject_CallFunction(data->func, "iii", min, max, current);
}

static PyObject *
pygimp_adaptive_supersample_area(PyObject *self, PyObject *args, PyObject *kwargs)
{
    gulong r;

    gint x1, y1, x2, y2, max_depth;
    gdouble threshold;
    PyObject *py_func_render = NULL, *py_data_render = NULL;
    PyObject *py_func_put_pixel = NULL, *py_data_put_pixel = NULL;
    PyObject *py_func_progress = NULL, *py_data_progress = NULL;

    GimpRenderFunc proxy_func_render = NULL;
    GimpPutPixelFunc proxy_func_put_pixel = NULL;
    GimpProgressFunc proxy_func_progress = NULL;

    ProxyData proxy_data_render, proxy_data_put_pixel, proxy_data_progress;

    static char *kwlist[] = {
	"x1", "y1", "x2", "y2", "max_depth", "threshold",
	"render_func", "render_data",
        "put_pixel_func", "put_pixel_data",
        "progress_func", "progress_data",
        NULL
    };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
				     "iiiiid|OOOOOO"
				     ":adaptive_supersample_area",
				     kwlist,
				     &x1, &y1, &x2, &y2, &max_depth, &threshold,
				     &py_func_render, &py_data_render,
				     &py_func_put_pixel, &py_data_put_pixel,
				     &py_func_progress, &py_data_progress))
	return NULL;

#define PROCESS_FUNC(n)	G_STMT_START {					\
    if (py_func_##n != NULL) {						\
        if (!PyCallable_Check(py_func_##n)) {				\
	    PyErr_SetString(PyExc_TypeError, #n "_func "		\
			    "must be callable");			\
	    return NULL;						\
	}								\
									\
	proxy_func_##n = proxy_##n;					\
									\
	proxy_data_##n.func = py_func_##n;				\
	proxy_data_##n.data = py_data_##n;				\
    }									\
} G_STMT_END

    PROCESS_FUNC(render);
    PROCESS_FUNC(put_pixel);
    PROCESS_FUNC(progress);

#undef PROCESS_FUNC

#define PASS_FUNC(n) proxy_func_##n, &proxy_data_##n

    r = gimp_adaptive_supersample_area (x1, y1, x2, y2, max_depth, threshold,
					PASS_FUNC(render),
					PASS_FUNC(put_pixel),
					PASS_FUNC(progress));

#undef PASS_FUNC

    return PyInt_FromLong(r);
}
#endif

/* List of methods defined in the module */

static struct PyMethodDef gimpcolor_methods[] = {
    {"rgb_parse_name", (PyCFunction)pygimp_rgb_parse_name, METH_VARARGS | METH_KEYWORDS},
    {"rgb_parse_hex", (PyCFunction)pygimp_rgb_parse_hex, METH_VARARGS | METH_KEYWORDS},
    {"rgb_parse_css", (PyCFunction)pygimp_rgb_parse_css, METH_VARARGS | METH_KEYWORDS},
    {"rgb_names", (PyCFunction)pygimp_rgb_list_names, METH_NOARGS},
#if 0
    {"bilinear", (PyCFunction)pygimp_bilinear, METH_VARARGS | METH_KEYWORDS},
    {"bilinear_8", (PyCFunction)pygimp_bilinear_8, METH_VARARGS | METH_KEYWORDS},
    {"bilinear_32", (PyCFunction)pygimp_bilinear_32, METH_VARARGS | METH_KEYWORDS},
    //{"bilinear_pixels_8", (PyCFunction)pygimp_bilinear_pixels_8, METH_VARARGS | METH_KEYWORDS},
    {"adaptive_supersample_area", (PyCFunction)pygimp_adaptive_supersample_area, METH_VARARGS | METH_KEYWORDS},
#endif
    {NULL,	 (PyCFunction)NULL, 0, NULL}		/* sentinel */
};


static struct _PyGimpColor_Functions pygimpcolor_api_functions = {
    &PyGimpRGB_Type,
    pygimp_rgb_new,
    &PyGimpHSV_Type,
    pygimp_hsv_new,
    &PyGimpHSL_Type,
    pygimp_hsl_new,
    &PyGimpCMYK_Type,
    pygimp_cmyk_new
};


/* Initialization function for the module (*must* be called initgimpcolor) */

static char gimpcolor_doc[] =
"This module provides interfaces to allow you to write gimp plugins"
;

void initgimpcolor(void);

PyMODINIT_FUNC
initgimpcolor(void)
{
    PyObject *m, *d;

    pygimp_init_pygobject();

    /* Create the module and add the functions */
    m = Py_InitModule3("gimpcolor", gimpcolor_methods, gimpcolor_doc);

    d = PyModule_GetDict(m);

    pyg_register_boxed(d, "RGB", GIMP_TYPE_RGB, &PyGimpRGB_Type);
    pyg_register_boxed(d, "HSV", GIMP_TYPE_HSV, &PyGimpHSV_Type);
    pyg_register_boxed(d, "HSL", GIMP_TYPE_HSL, &PyGimpHSL_Type);
    pyg_register_boxed(d, "CMYK", GIMP_TYPE_CMYK, &PyGimpCMYK_Type);

    PyModule_AddObject(m, "RGB_COMPOSITE_NONE",
		       PyInt_FromLong(GIMP_RGB_COMPOSITE_NONE));
    PyModule_AddObject(m, "RGB_COMPOSITE_NORMAL",
		       PyInt_FromLong(GIMP_RGB_COMPOSITE_NORMAL));
    PyModule_AddObject(m, "RGB_COMPOSITE_BEHIND",
		       PyInt_FromLong(GIMP_RGB_COMPOSITE_BEHIND));

    PyModule_AddObject(m, "RGB_LUMINANCE_RED",
		       PyFloat_FromDouble(GIMP_RGB_LUMINANCE_RED));
    PyModule_AddObject(m, "RGB_LUMINANCE_GREEN",
		       PyFloat_FromDouble(GIMP_RGB_LUMINANCE_GREEN));
    PyModule_AddObject(m, "RGB_LUMINANCE_BLUE",
		       PyFloat_FromDouble(GIMP_RGB_LUMINANCE_BLUE));

    /* for other modules */
    PyModule_AddObject(m, "_PyGimpColor_API",
                       PyCObject_FromVoidPtr(&pygimpcolor_api_functions, NULL));

    /* Check for errors */
    if (PyErr_Occurred())
	Py_FatalError("can't initialize module gimpcolor");
}
