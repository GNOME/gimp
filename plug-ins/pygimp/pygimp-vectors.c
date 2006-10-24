/* -*- Mode: C; c-basic-offset: 4 -*-
    Gimp-Python - allows the writing of Gimp plugins in Python.
    Copyright (C) 2006  Manish Singh <yosh@gimp.org>

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

#include "pygimp.h"


typedef struct {
    PyObject_HEAD
    gint32 vectors_ID;
    gint stroke;
} PyGimpVectorsStroke;

static PyObject *
vs_get_length(PyGimpVectorsStroke *self, PyObject *args, PyObject *kwargs)
{
    double precision;
    double length;

    static char *kwlist[] = { "precision", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "d:get_length", kwlist,
	                             &precision))
        return NULL;

    length = gimp_vectors_stroke_get_length(self->vectors_ID, self->stroke,
                                            precision);

    return PyFloat_FromDouble(length);
}

static PyObject *
vs_get_point_at_dist(PyGimpVectorsStroke *self, PyObject *args, PyObject *kwargs)
{
    double dist, precision;
    double x, y, slope;
    gboolean valid;
    PyObject *ret;

    static char *kwlist[] = { "dist", "precision", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
                                     "dd:get_point_at_dist", kwlist,
				     &dist, &precision))
        return NULL;

    gimp_vectors_stroke_get_point_at_dist(self->vectors_ID, self->stroke,
	                                  dist, precision,
					  &x, &y, &slope, &valid);

    ret = PyTuple_New(4);
    if (ret == NULL)
        return NULL;

    PyTuple_SetItem(ret, 0, PyFloat_FromDouble(x));
    PyTuple_SetItem(ret, 1, PyFloat_FromDouble(y));
    PyTuple_SetItem(ret, 2, PyFloat_FromDouble(slope));
    PyTuple_SetItem(ret, 3, PyBool_FromLong(valid));

    return ret;
}

static PyObject *
vs_remove(PyGimpVectorsStroke *self)
{
    gimp_vectors_stroke_remove(self->vectors_ID, self->stroke);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
vs_close(PyGimpVectorsStroke *self)
{
    gimp_vectors_stroke_close(self->vectors_ID, self->stroke);
    Py_INCREF(Py_None);
    return Py_None;
}


static PyObject *
vs_translate(PyGimpVectorsStroke *self, PyObject *args, PyObject *kwargs)
{
    double off_x, off_y;

    static char *kwlist[] = { "off_x", "off_y", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "dd:translate", kwlist,
				     &off_x, &off_y))
        return NULL;

    gimp_vectors_stroke_translate(self->vectors_ID, self->stroke, off_x, off_y);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
vs_scale(PyGimpVectorsStroke *self, PyObject *args, PyObject *kwargs)
{
    double scale_x, scale_y;

    static char *kwlist[] = { "scale_x", "scale_y", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "dd:scale", kwlist,
				     &scale_x, &scale_y))
        return NULL;

    gimp_vectors_stroke_scale(self->vectors_ID, self->stroke, scale_x, scale_y);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
vs_interpolate(PyGimpVectorsStroke *self, PyObject *args, PyObject *kwargs)
{
    double precision;
    double *coords;
    int i, num_coords;
    gboolean closed;
    PyObject *ret, *ret_coords;

    static char *kwlist[] = { "precision", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "d:interpolate", kwlist,
				     &precision))
        return NULL;

    coords = gimp_vectors_stroke_interpolate(self->vectors_ID, self->stroke,
                                             precision, &num_coords, &closed);

    ret = PyTuple_New(2);
    if (ret == NULL)
        return NULL;

    ret_coords = PyList_New(num_coords);
    if (ret_coords == NULL) {
	Py_DECREF(ret);
        return NULL;
    }

    for (i = 0; i < num_coords; i++)
        PyList_SetItem(ret_coords, i, PyFloat_FromDouble(coords[i]));

    PyTuple_SetItem(ret, 0, ret_coords);
    PyTuple_SetItem(ret, 1, PyBool_FromLong(closed));

    return ret;
}

static PyMethodDef vs_methods[] = {
    { "get_length", (PyCFunction)vs_get_length, METH_VARARGS | METH_KEYWORDS },
    { "get_point_at_dist", (PyCFunction)vs_get_point_at_dist, METH_VARARGS | METH_KEYWORDS },
    { "remove", (PyCFunction)vs_remove, METH_NOARGS },
    { "close", (PyCFunction)vs_close, METH_NOARGS },
    { "translate", (PyCFunction)vs_translate, METH_VARARGS | METH_KEYWORDS },
    { "scale", (PyCFunction)vs_scale, METH_VARARGS | METH_KEYWORDS },
    { "interpolate", (PyCFunction)vs_interpolate, METH_VARARGS | METH_KEYWORDS },
    { NULL, NULL, 0 }
};

static void
vs_dealloc(PyGimpVectorsStroke *self)
{
    PyObject_DEL(self);
}

static PyObject *
vs_repr(PyGimpVectorsStroke *self)
{
    PyObject *s;
    char *name;

    name = gimp_vectors_get_name(self->vectors_ID);
    s = PyString_FromFormat("<gimp.VectorsStroke %d of gimp.Vectors '%s'>",
	                    self->stroke, name ? name : "(null)");
    g_free(name);

    return s;
}

static int
vs_cmp(PyGimpVectorsStroke *self, PyGimpVectorsStroke *other)
{
    if (self->vectors_ID == other->vectors_ID) {
        if (self->stroke == other->stroke)
            return 0;
        if (self->stroke > other->stroke)
            return -1;
        return 1;
    }
    if (self->vectors_ID > other->vectors_ID)
        return -1;
    return 1;
}

PyTypeObject PyGimpVectorsStroke_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                                  /* ob_size */
    "gimp.VectorsStroke",               /* tp_name */
    sizeof(PyGimpVectorsStroke),        /* tp_basicsize */
    0,                                  /* tp_itemsize */
    /* methods */
    (destructor)vs_dealloc,             /* tp_dealloc */
    (printfunc)0,                       /* tp_print */
    (getattrfunc)0,                     /* tp_getattr */
    (setattrfunc)0,                     /* tp_setattr */
    (cmpfunc)vs_cmp,                    /* tp_compare */
    (reprfunc)vs_repr,                  /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    (hashfunc)0,                        /* tp_hash */
    (ternaryfunc)0,                     /* tp_call */
    (reprfunc)0,                        /* tp_str */
    (getattrofunc)0,                    /* tp_getattro */
    (setattrofunc)0,                    /* tp_setattro */
    0,					/* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
    NULL, /* Documentation string */
    (traverseproc)0,			/* tp_traverse */
    (inquiry)0,				/* tp_clear */
    (richcmpfunc)0,			/* tp_richcompare */
    0,					/* tp_weaklistoffset */
    (getiterfunc)0,			/* tp_iter */
    (iternextfunc)0,			/* tp_iternext */
    vs_methods,				/* tp_methods */
    0,					/* tp_members */
    0,					/* tp_getset */
    (PyTypeObject *)0,			/* tp_base */
    (PyObject *)0,			/* tp_dict */
    0,					/* tp_descr_get */
    0,					/* tp_descr_set */
    0,					/* tp_dictoffset */
    (initproc)0,                        /* tp_init */
    (allocfunc)0,			/* tp_alloc */
    (newfunc)0,				/* tp_new */
};

static PyObject *
vectors_stroke_new(PyGimpVectors *vectors, gint stroke)
{
    PyGimpVectorsStroke *self;

    self = PyObject_NEW(PyGimpVectorsStroke, &PyGimpVectorsStroke_Type);

    if (self == NULL)
	return NULL;

    self->vectors_ID = vectors->ID;
    self->stroke = stroke;

    return (PyObject *)self;
}


static PyObject *
vbs_lineto(PyGimpVectorsStroke *self, PyObject *args, PyObject *kwargs)
{
    double x0, y0;

    static char *kwlist[] = { "x0", "y0", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
	                             "dd:lineto", kwlist,
	                             &x0, &y0))
        return NULL;

    gimp_vectors_bezier_stroke_lineto(self->vectors_ID, self->stroke, x0, y0);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
vbs_conicto(PyGimpVectorsStroke *self, PyObject *args, PyObject *kwargs)
{
    double x0, y0, x1, y1;

    static char *kwlist[] = { "x0", "y0", "x1", "y1", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
	                             "dddd:conicto", kwlist,
	                             &x0, &y0, &x1, &y1))
        return NULL;

    gimp_vectors_bezier_stroke_conicto(self->vectors_ID, self->stroke,
	                               x0, y0, x1, y1);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
vbs_cubicto(PyGimpVectorsStroke *self, PyObject *args, PyObject *kwargs)
{
    double x0, y0, x1, y1, x2, y2;

    static char *kwlist[] = { "x0", "y0", "x1", "y1", "x2", "y2", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
	                             "dddddd:conicto", kwlist,
	                             &x0, &y0, &x1, &y1, &x2, &y2))
        return NULL;

    gimp_vectors_bezier_stroke_cubicto(self->vectors_ID, self->stroke,
	                               x0, y0, x1, y1, x2, y2);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyMethodDef vbs_methods[] = {
    { "lineto", (PyCFunction)vbs_lineto, METH_VARARGS | METH_KEYWORDS },
    { "conicto", (PyCFunction)vbs_conicto, METH_VARARGS | METH_KEYWORDS },
    { "cubicto", (PyCFunction)vbs_cubicto, METH_VARARGS | METH_KEYWORDS },
    { NULL, NULL, 0 }
};

static PyObject *
vbs_repr(PyGimpVectorsStroke *self)
{
    PyObject *s;
    char *name;

    name = gimp_vectors_get_name(self->vectors_ID);
    s = PyString_FromFormat("<gimp.VectorsBezierStroke %d of gimp.Vectors '%s'>",
	                    self->stroke, name ? name : "(null)");
    g_free(name);

    return s;
}

PyTypeObject PyGimpVectorsBezierStroke_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                                  /* ob_size */
    "gimp.VectorsBezierStroke",         /* tp_name */
    sizeof(PyGimpVectorsStroke),        /* tp_basicsize */
    0,                                  /* tp_itemsize */
    /* methods */
    (destructor)vs_dealloc,             /* tp_dealloc */
    (printfunc)0,                       /* tp_print */
    (getattrfunc)0,                     /* tp_getattr */
    (setattrfunc)0,                     /* tp_setattr */
    (cmpfunc)vs_cmp,                    /* tp_compare */
    (reprfunc)vbs_repr,                 /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    (hashfunc)0,                        /* tp_hash */
    (ternaryfunc)0,                     /* tp_call */
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
    vbs_methods,			/* tp_methods */
    0,					/* tp_members */
    0,					/* tp_getset */
    &PyGimpVectorsStroke_Type,		/* tp_base */
    (PyObject *)0,			/* tp_dict */
    0,					/* tp_descr_get */
    0,					/* tp_descr_set */
    0,					/* tp_dictoffset */
    (initproc)0,                        /* tp_init */
    (allocfunc)0,			/* tp_alloc */
    (newfunc)0,				/* tp_new */
};

static PyObject *
vectors_bezier_stroke_new(PyGimpVectors *vectors, gint stroke)
{
    PyGimpVectorsStroke *self;

    self = PyObject_NEW(PyGimpVectorsStroke, &PyGimpVectorsBezierStroke_Type);

    if (self == NULL)
	return NULL;

    self->vectors_ID = vectors->ID;
    self->stroke = stroke;

    return (PyObject *)self;
}


static PyObject *
vectors_bezier_stroke_new_moveto(PyGimpVectors *self, PyObject *args, PyObject *kwargs)
{
    double x0, y0;
    gint stroke;

    static char *kwlist[] = { "x0", "y0", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
	                             "dd:bezier_stroke_new_moveto", kwlist,
	                             &x0, &y0))
        return NULL;

    stroke = gimp_vectors_bezier_stroke_new_moveto(self->ID, x0, y0);

    return vectors_bezier_stroke_new(self, stroke);
}

static PyObject *
vectors_bezier_stroke_new_ellipse(PyGimpVectors *self, PyObject *args, PyObject *kwargs)
{
    double x0, y0, radius_x, radius_y, angle;
    gint stroke;

    static char *kwlist[] = { "x0", "y0", "radius_x", "radius_y", "angle", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
	                             "ddddd:bezier_stroke_new_ellipse", kwlist,
	                             &x0, &y0, &radius_x, &radius_y, &angle))
        return NULL;

    stroke = gimp_vectors_bezier_stroke_new_ellipse(self->ID, x0, y0,
	                                            radius_x, radius_y, angle);

    return vectors_bezier_stroke_new(self, stroke);
}

static PyObject *
vectors_to_selection(PyGimpVectors *self, PyObject *args, PyObject *kwargs)
{
    GimpChannelOps operation = GIMP_CHANNEL_OP_REPLACE;
    gboolean antialias = TRUE, feather = FALSE;
    double feather_radius_x = 0.0, feather_radius_y = 0.0;

    static char *kwlist[] = { "operation", "antialias", "feather",
	                      "feather_radius_x", "feather_radius_y", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
                                     "|iiidd:to_selection", kwlist,
                                     &operation, &antialias, &feather,
				     &feather_radius_x, &feather_radius_y))
        return NULL;

    gimp_vectors_to_selection(self->ID, operation, antialias, feather,
                              feather_radius_x, feather_radius_y);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
vectors_parasite_find(PyGimpVectors *self, PyObject *args)
{
    char *name;

    if (!PyArg_ParseTuple(args, "s:parasite_find", &name))
        return NULL;

    return pygimp_parasite_new(gimp_vectors_parasite_find(self->ID, name));
}

static PyObject *
vectors_parasite_attach(PyGimpVectors *self, PyObject *args)
{
    PyGimpParasite *parasite;

    if (!PyArg_ParseTuple(args, "O!:parasite_attach", &PyGimpParasite_Type,
                          &parasite))
        return NULL;

    if (!gimp_vectors_parasite_attach(self->ID, parasite->para)) {
        PyErr_Format(pygimp_error,
                     "could not attach parasite '%s' to vectors (ID %d)",
                     parasite->para->name, self->ID);
        return NULL;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
vectors_parasite_detach(PyGimpVectors *self, PyObject *args)
{
    char *name;

    if (!PyArg_ParseTuple(args, "s:parasite_detach", &name))
        return NULL;

    if (!gimp_vectors_parasite_detach(self->ID, name)) {
        PyErr_Format(pygimp_error,
                     "could not detach parasite '%s' from vectors (ID %d)",
                     name, self->ID);
        return NULL;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
vectors_parasite_list(PyGimpVectors *self)
{
    gint num_parasites;
    gchar **parasites;

    if (gimp_vectors_parasite_list(self->ID, &num_parasites, &parasites)) {
        PyObject *ret;
        gint i;

        ret = PyTuple_New(num_parasites);

        for (i = 0; i < num_parasites; i++) {
            PyTuple_SetItem(ret, i, PyString_FromString(parasites[i]));
            g_free(parasites[i]);
        }

        g_free(parasites);
        return ret;
    }

    PyErr_Format(pygimp_error, "could not list parasites on vectors (ID %d)",
                 self->ID);
    return NULL;
}

static PyMethodDef vectors_methods[] = {
    { "bezier_stroke_new_moveto",
      (PyCFunction)vectors_bezier_stroke_new_moveto,
      METH_VARARGS | METH_KEYWORDS },
    { "bezier_stroke_new_ellipse",
      (PyCFunction)vectors_bezier_stroke_new_ellipse,
      METH_VARARGS | METH_KEYWORDS },
    { "to_selection",
      (PyCFunction)vectors_to_selection,
      METH_VARARGS | METH_KEYWORDS },
    { "parasite_find",
      (PyCFunction)vectors_parasite_find,
      METH_VARARGS },
    { "parasite_attach",
      (PyCFunction)vectors_parasite_attach,
      METH_VARARGS },
    { "parasite_detach",
      (PyCFunction)vectors_parasite_detach,
      METH_VARARGS },
    { "parasite_list",
      (PyCFunction)vectors_parasite_list,
      METH_NOARGS },
    { NULL, NULL, 0 }
};

static PyObject *
vectors_get_image(PyGimpVectors *self, void *closure)
{
    return pygimp_image_new(gimp_vectors_get_image(self->ID));
}

static PyObject *
vectors_get_ID(PyGimpVectors *self, void *closure)
{
    return PyInt_FromLong(self->ID);
}

static PyObject *
vectors_get_name(PyGimpVectors *self, void *closure)
{
    return PyString_FromString(gimp_vectors_get_name(self->ID));
}

static int
vectors_set_name(PyGimpVectors *self, PyObject *value, void *closure)
{
    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError, "cannot delete name");
        return -1;
    }

    if (!PyString_Check(value) && !PyUnicode_Check(value)) {
        PyErr_SetString(PyExc_TypeError, "type mismatch");
        return -1;
    }

    gimp_vectors_set_name(self->ID, PyString_AsString(value));

    return 0;
}

static PyObject *
vectors_get_visible(PyGimpVectors *self, void *closure)
{
    return PyBool_FromLong(gimp_vectors_get_visible(self->ID));
}

static int
vectors_set_visible(PyGimpVectors *self, PyObject *value, void *closure)
{
    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError, "cannot delete visible");
        return -1;
    }

    if (!PyInt_Check(value)) {
        PyErr_SetString(PyExc_TypeError, "type mismatch");
        return -1;
    }

    gimp_vectors_set_visible(self->ID, PyInt_AsLong(value));

    return 0;
}

static PyObject *
vectors_get_linked(PyGimpVectors *self, void *closure)
{
    return PyBool_FromLong(gimp_vectors_get_linked(self->ID));
}

static int
vectors_set_linked(PyGimpVectors *self, PyObject *value, void *closure)
{
    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError, "cannot delete linked");
        return -1;
    }

    if (!PyInt_Check(value)) {
        PyErr_SetString(PyExc_TypeError, "type mismatch");
        return -1;
    }

    gimp_vectors_set_linked(self->ID, PyInt_AsLong(value));

    return 0;
}

static PyObject *
vectors_get_tattoo(PyGimpVectors *self, void *closure)
{
    return PyInt_FromLong(gimp_vectors_get_tattoo(self->ID));
}

static int
vectors_set_tattoo(PyGimpVectors *self, PyObject *value, void *closure)
{
    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError, "cannot delete tattoo");
        return -1;
    }

    if (!PyInt_Check(value)) {
        PyErr_SetString(PyExc_TypeError, "type mismatch");
        return -1;
    }

    gimp_vectors_set_tattoo(self->ID, PyInt_AsLong(value));

    return 0;
}

static PyObject *
vectors_get_strokes(PyGimpVectors *self, void *closure)
{
    int *strokes;
    int i, num_strokes;
    PyObject *ret;

    strokes = gimp_vectors_get_strokes(self->ID, &num_strokes);

    ret = PyList_New(num_strokes);
    if (ret == NULL)
        return NULL;

    for (i = 0; i < num_strokes; i++)
        PyList_SetItem(ret, i, vectors_bezier_stroke_new(self, strokes[i]));

    g_free(strokes);

    return ret;
}

static PyGetSetDef vectors_getsets[] = {
    { "ID", (getter)vectors_get_ID, (setter)0 },
    { "image", (getter)vectors_get_image, (setter)0 },
    { "name", (getter)vectors_get_name, (setter)vectors_set_name },
    { "visible", (getter)vectors_get_visible, (setter)vectors_set_visible },
    { "linked", (getter)vectors_get_linked, (setter)vectors_set_linked },
    { "tattoo", (getter)vectors_get_tattoo, (setter)vectors_set_tattoo },
    { "strokes", (getter)vectors_get_strokes, (setter)0 },
    { NULL, (getter)0, (setter)0 }
};

static void
vectors_dealloc(PyGimpVectors *self)
{
    PyObject_DEL(self);
}

static PyObject *
vectors_repr(PyGimpVectors *self)
{
    PyObject *s;
    char *name;

    name = gimp_vectors_get_name(self->ID);
    s = PyString_FromFormat("<gimp.Vectors '%s'>", name ? name : "(null)");
    g_free(name);

    return s;
}

static int
vectors_cmp(PyGimpVectors *self, PyGimpVectors *other)
{
    if (self->ID == other->ID)
        return 0;
    if (self->ID > other->ID)
        return -1;
    return 1;
}

static int
vectors_init(PyGimpVectors *self, PyObject *args, PyObject *kwargs)
{
    PyGimpImage *img;
    char *name;

    static char *kwlist[] = { "image", "name", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
                                     "O!s:gimp.Vectors.__init__",
                                     kwlist,
                                     &PyGimpImage_Type, &img, &name))
        return -1;

    self->ID = gimp_vectors_new(img->ID, name);

    if (self->ID < 0) {
        PyErr_Format(pygimp_error,
                     "could not create vectors '%s' on image (ID %d)",
                     name, img->ID);
        return -1;
    }

    return 0;
}

PyTypeObject PyGimpVectors_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                                  /* ob_size */
    "gimp.Vectors",                     /* tp_name */
    sizeof(PyGimpVectors),              /* tp_basicsize */
    0,                                  /* tp_itemsize */
    /* methods */
    (destructor)vectors_dealloc,        /* tp_dealloc */
    (printfunc)0,                       /* tp_print */
    (getattrfunc)0,                     /* tp_getattr */
    (setattrfunc)0,                     /* tp_setattr */
    (cmpfunc)vectors_cmp,               /* tp_compare */
    (reprfunc)vectors_repr,             /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    (hashfunc)0,                        /* tp_hash */
    (ternaryfunc)0,                     /* tp_call */
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
    vectors_methods,			/* tp_methods */
    0,					/* tp_members */
    vectors_getsets,			/* tp_getset */
    (PyTypeObject *)0,			/* tp_base */
    (PyObject *)0,			/* tp_dict */
    0,					/* tp_descr_get */
    0,					/* tp_descr_set */
    0,					/* tp_dictoffset */
    (initproc)vectors_init,             /* tp_init */
    (allocfunc)0,			/* tp_alloc */
    (newfunc)0,				/* tp_new */
};

PyObject *
pygimp_vectors_new(gint32 ID)
{
    PyGimpVectors *self;

    if (ID == -1) {
	Py_INCREF(Py_None);
	return Py_None;
    }

    self = PyObject_NEW(PyGimpVectors, &PyGimpVectors_Type);

    if (self == NULL)
	return NULL;

    self->ID = ID;

    return (PyObject *)self;
}
