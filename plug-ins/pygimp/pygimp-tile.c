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

#define GIMP_DISABLE_DEPRECATION_WARNINGS

#include "pygimp.h"

#define NO_IMPORT_PYGIMPCOLOR
#include "pygimpcolor-api.h"

#include <structmember.h>

static PyObject *
tile_flush(PyGimpTile *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ":flush"))
	return NULL;

    gimp_tile_flush(self->tile);

    Py_INCREF(Py_None);
    return Py_None;
}


static PyMethodDef tile_methods[] = {
    {"flush",	(PyCFunction)tile_flush,	METH_VARARGS},
    {NULL,		NULL}		/* sentinel */
};

/* ---------- */


PyObject *
pygimp_tile_new(GimpTile *t, PyGimpDrawable *drw)
{
    PyGimpTile *self;

    self = PyObject_NEW(PyGimpTile, &PyGimpTile_Type);

    if (self == NULL)
	return NULL;

    gimp_tile_ref(t);

    self->tile = t;

    Py_INCREF(drw);
    self->drawable = drw;

    return (PyObject *)self;
}


static void
tile_dealloc(PyGimpTile *self)
{
    gimp_tile_unref(self->tile, FALSE);

    Py_DECREF(self->drawable);
    PyObject_DEL(self);
}

static PyObject *
tile_get_uint_field(PyGimpTile *self, void *closure)
{
    gint offset = GPOINTER_TO_INT(closure);
    guint value;
    gchar *addr;

    addr = (gchar *)self->tile;
    addr += offset;
    value = *(guint *)addr;

    return PyInt_FromLong(value);
}

static PyObject *
tile_get_dirty(PyGimpTile *self, void *closure)
{
    return PyBool_FromLong(self->tile->dirty);
}

static PyObject *
tile_get_shadow(PyGimpTile *self, void *closure)
{
    return PyBool_FromLong(self->tile->shadow);
}

static PyObject *
tile_get_drawable(PyGimpTile *self, void *closure)
{
    return pygimp_drawable_new(self->tile->drawable, 0);
}


#define OFF(x) GINT_TO_POINTER(offsetof(GimpTile, x))
static PyGetSetDef tile_getsets[] = {
    { "ewidth",   (getter)tile_get_uint_field, 0, NULL, OFF(ewidth) },
    { "eheight",  (getter)tile_get_uint_field, 0, NULL, OFF(eheight) },
    { "bpp",      (getter)tile_get_uint_field, 0, NULL, OFF(bpp) },
    { "tile_num", (getter)tile_get_uint_field, 0, NULL, OFF(tile_num) },
    { "dirty",    (getter)tile_get_dirty, 0, NULL },
    { "shadow",   (getter)tile_get_shadow, 0, NULL },
    { "drawable", (getter)tile_get_drawable, 0, NULL },
    { NULL, (getter)0, (setter)0 }
};
#undef OFF

static PyObject *
tile_repr(PyGimpTile *self)
{
    PyObject *s;
    gchar *name;

    name = gimp_item_get_name(self->tile->drawable->drawable_id);

    if (self->tile->shadow)
	s = PyString_FromFormat("<gimp.Tile for drawable '%s' (shadow)>", name);
    else
	s = PyString_FromFormat("<gimp.Tile for drawable '%s'>", name);

    g_free(name);

    return s;
}

static Py_ssize_t
tile_length(PyObject *self)
{
    return ((PyGimpTile*)self)->tile->ewidth * ((PyGimpTile*)self)->tile->eheight;
}

static PyObject *
tile_subscript(PyGimpTile *self, PyObject *sub)
{
    GimpTile *tile = self->tile;
    int bpp = tile->bpp;
    long x, y;

    if (PyInt_Check(sub)) {
	x = PyInt_AsLong(sub);

	if (x < 0 || x >= tile->ewidth * tile->eheight) {
	    PyErr_SetString(PyExc_IndexError, "index out of range");
	    return NULL;
	}

	return PyString_FromStringAndSize
            ((char *)tile->data + bpp * x, bpp);
    }

    if (PyTuple_Check(sub)) {
	if (!PyArg_ParseTuple(sub, "ll", &x, &y))
	    return NULL;

	if (x < 0 || y < 0 || x >= tile->ewidth || y>=tile->eheight) {
	    PyErr_SetString(PyExc_IndexError, "index out of range");
	    return NULL;
	}

	return PyString_FromStringAndSize
            ((char *)tile->data + bpp * (x + y * tile->ewidth), bpp);
    }

    PyErr_SetString(PyExc_TypeError, "tile subscript not int or 2-tuple");
    return NULL;
}

static int
tile_ass_sub(PyGimpTile *self, PyObject *v, PyObject *w)
{
    GimpTile *tile = self->tile;
    int bpp = tile->bpp, i;
    long x, y;
    guchar *pix, *data;

    if (w == NULL) {
	PyErr_SetString(PyExc_TypeError,
			"can not delete pixels in tile");
	return -1;
    }

    if (!PyString_Check(w) && PyString_Size(w) == bpp) {
	PyErr_SetString(PyExc_TypeError, "invalid subscript");
	return -1;
    }

    pix = (guchar *)PyString_AsString(w);

    if (PyInt_Check(v)) {
	x = PyInt_AsLong(v);

	if (x < 0 || x >= tile->ewidth * tile->eheight) {
	    PyErr_SetString(PyExc_IndexError, "index out of range");
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
	    PyErr_SetString(PyExc_IndexError, "index out of range");
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
    tile_length, /*length*/
    (binaryfunc)tile_subscript, /*subscript*/
    (objobjargproc)tile_ass_sub, /*ass_sub*/
};

PyTypeObject PyGimpTile_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                                  /* ob_size */
    "gimp.Tile",                        /* tp_name */
    sizeof(PyGimpTile),                 /* tp_basicsize */
    0,                                  /* tp_itemsize */
    /* methods */
    (destructor)tile_dealloc,           /* tp_dealloc */
    (printfunc)0,                       /* tp_print */
    (getattrfunc)0,                     /* tp_getattr */
    (setattrfunc)0,                     /* tp_setattr */
    (cmpfunc)0,                         /* tp_compare */
    (reprfunc)tile_repr,                /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    &tile_as_mapping,                   /* tp_as_mapping */
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
    tile_methods,			/* tp_methods */
    0,					/* tp_members */
    tile_getsets,			/* tp_getset */
    (PyTypeObject *)0,			/* tp_base */
    (PyObject *)0,			/* tp_dict */
    0,					/* tp_descr_get */
    0,					/* tp_descr_set */
    0,					/* tp_dictoffset */
    (initproc)0,                        /* tp_init */
    (allocfunc)0,			/* tp_alloc */
    (newfunc)0,				/* tp_new */
};

/* End of code for Tile objects */
/* -------------------------------------------------------- */


static PyObject *
pr_resize(PyGimpPixelRgn *self, PyObject *args)
{
    int x, y, w, h;

    if (!PyArg_ParseTuple(args, "iiii:resize", &x, &y, &w, &h))
	return NULL;

    gimp_pixel_rgn_resize(&(self->pr), x, y, w, h);

    Py_INCREF(Py_None);
    return Py_None;
}



static PyMethodDef pr_methods[] = {
    {"resize",	(PyCFunction)pr_resize,	METH_VARARGS},

    {NULL,		NULL}		/* sentinel */
};

/* ---------- */


PyObject *
pygimp_pixel_rgn_new(PyGimpDrawable *drawable, int x, int y,
		     int width, int height, int dirty, int shadow)
{
    PyGimpPixelRgn *self;
    int drw_width;
    int drw_height;

    self = PyObject_NEW(PyGimpPixelRgn, &PyGimpPixelRgn_Type);

    if (self == NULL || drawable == NULL)
        return NULL;

    drw_width = gimp_drawable_width(drawable->ID);
    drw_height = gimp_drawable_height(drawable->ID);

    if(x < 0) x = 0;
    if(y < 0) y = 0;
    if(width < 0) width = drw_width - x;
    if(height < 0) height = drw_height - y;
    if(x >= drw_width) x = drw_width - 1;
    if(y >= drw_height) y = drw_height - 1;
    if(x + width > drw_width) width = drw_width - x;
    if(y + height > drw_height) height = drw_height - y;

    gimp_pixel_rgn_init(&(self->pr), drawable->drawable, x, y, width, height,
			dirty, shadow);

    self->drawable = drawable;
    Py_INCREF(drawable);

    return (PyObject *)self;
}


static void
pr_dealloc(PyGimpPixelRgn *self)
{
    Py_DECREF(self->drawable);
    PyObject_DEL(self);
}

/* Code to access pr objects as mappings */

static Py_ssize_t
pr_length(PyObject *self)
{
    PyErr_SetString(pygimp_error, "Can't get size of pixel region");
    return -1;
}

static PyObject *
pr_subscript(PyGimpPixelRgn *self, PyObject *key)
{
    GimpPixelRgn *pr = &(self->pr);
    PyObject *x, *y;
    Py_ssize_t x1, y1, x2, y2, xs, ys;
    PyObject *ret;

    if (!PyTuple_Check(key) || PyTuple_Size(key) != 2) {
        PyErr_SetString(PyExc_TypeError, "subscript must be a 2-tuple");
        return NULL;
    }

    if (!PyArg_ParseTuple(key, "OO", &x, &y))
        return NULL;

    if (PyInt_Check(x)) {
        x1 = PyInt_AsSsize_t(x);

        if (x1 < pr->x || x1 >= pr->x + pr->w) {
            PyErr_SetString(PyExc_IndexError, "x subscript out of range");
            return NULL;
        }

        if (PyInt_Check(y)) {
            y1 = PyInt_AsSsize_t(y);

            if (y1 < pr->y || y1 >= pr->y + pr->h) {
                PyErr_SetString(PyExc_IndexError, "y subscript out of range");
                return NULL;
            }

            ret = PyString_FromStringAndSize(NULL, pr->bpp);
            gimp_pixel_rgn_get_pixel(pr, (guchar*)PyString_AS_STRING(ret),
                                     x1, y1);

        } else if (PySlice_Check(y)) {
            if (PySlice_GetIndices((PySliceObject*)y, pr->y + pr->h,
                                   &y1, &y2, &ys) ||
                y1 >= y2 || ys != 1) {
                PyErr_SetString(PyExc_IndexError, "invalid y slice");
                return NULL;
            }

            if(y1 == 0)
                y1 = pr->y;

            if(y1 < pr->y || y2 < pr->y) {
                PyErr_SetString(PyExc_IndexError, "y subscript out of range");
                return NULL;
            }

            ret = PyString_FromStringAndSize(NULL, pr->bpp * (y2 - y1));
            gimp_pixel_rgn_get_col(pr, (guchar*)PyString_AS_STRING(ret),
                                   x1, y1, y2-y1);
	    }
        else {
            PyErr_SetString(PyExc_TypeError, "invalid y subscript");
            return NULL;
        }
    } else if (PySlice_Check(x)) {
        if (PySlice_GetIndices((PySliceObject *)x, pr->x + pr->w,
                               &x1, &x2, &xs) ||
            x1 >= x2 || xs != 1) {
            PyErr_SetString(PyExc_IndexError, "invalid x slice");
            return NULL;
        }
        if (x1 == 0)
            x1 = pr->x;
        if(x1 < pr->x || x2 < pr->x) {
            PyErr_SetString(PyExc_IndexError, "x subscript out of range");
            return NULL;
        }

        if (PyInt_Check(y)) {
            y1 = PyInt_AsSsize_t(y);
            if (y1 < pr->y || y1 >= pr->y + pr->h) {
                PyErr_SetString(PyExc_IndexError, "y subscript out of range");
                return NULL;
            }
            ret = PyString_FromStringAndSize(NULL, pr->bpp * (x2 - x1));
            gimp_pixel_rgn_get_row(pr, (guchar*)PyString_AS_STRING(ret),
                                   x1, y1, x2 - x1);

        } else if (PySlice_Check(y)) {
            if (PySlice_GetIndices((PySliceObject*)y, pr->y + pr->h,
                                   &y1, &y2, &ys) ||
                y1 >= y2 || ys != 1) {
                PyErr_SetString(PyExc_IndexError, "invalid y slice");
                return NULL;
            }

            if(y1 == 0)
                y1 = pr->y;
            if(y1 < pr->y || y2 < pr->y) {
                PyErr_SetString(PyExc_IndexError, "y subscript out of range");
                return NULL;
            }

            ret = PyString_FromStringAndSize(NULL,
                                             pr->bpp * (x2 - x1) * (y2 - y1));
            gimp_pixel_rgn_get_rect(pr, (guchar*)PyString_AS_STRING(ret),
                                    x1, y1, x2 - x1, y2 - y1);
	    }
        else {
            PyErr_SetString(PyExc_TypeError, "invalid y subscript");
            return NULL;
        }

    } else {
        PyErr_SetString(PyExc_TypeError, "invalid x subscript");
        return NULL;
    }
    return ret;
}

static int
pr_ass_sub(PyGimpPixelRgn *self, PyObject *v, PyObject *w)
{
    GimpPixelRgn *pr = &(self->pr);
    PyObject *x, *y;
    const guchar *buf;
    Py_ssize_t len, x1, x2, xs, y1, y2, ys;

    if (w == NULL) {
        PyErr_SetString(PyExc_TypeError, "can't delete subscripts");
        return -1;
    }

    if (!PyString_Check(w)) {
        PyErr_SetString(PyExc_TypeError, "must assign string to subscript");
        return -1;
    }

    if (!PyTuple_Check(v) || PyTuple_Size(v) != 2) {
        PyErr_SetString(PyExc_TypeError, "subscript must be a 2-tuple");
        return -1;
    }

    if (!PyArg_ParseTuple(v, "OO", &x, &y))
        return -1;

    buf = (const guchar *)PyString_AsString(w);
    len = PyString_Size(w);
    if (!buf || len > INT_MAX) {
        return -1;
    }

    if (PyInt_Check(x)) {
        x1 = PyInt_AsSsize_t(x);
        if (x1 < pr->x || x1 >= pr->x + pr->w) {
            PyErr_SetString(PyExc_IndexError, "x subscript out of range");
            return -1;
        }

        if (PyInt_Check(y)) {
            y1 = PyInt_AsSsize_t(y);

            if (y1 < pr->y || y1 >= pr->y + pr->h) {
                PyErr_SetString(PyExc_IndexError, "y subscript out of range");
                return -1;
            }

            if (len != pr->bpp) {
                PyErr_SetString(PyExc_TypeError, "string is wrong length");
                return -1;
            }
            gimp_pixel_rgn_set_pixel(pr, buf, x1, y1);

        } else if (PySlice_Check(y)) {
            if (PySlice_GetIndices((PySliceObject *)y, pr->y + pr->h,
                                   &y1, &y2, &ys) ||
                y1 >= y2 || ys != 1) {
                PyErr_SetString(PyExc_IndexError, "invalid y slice");
                return -1;
            }
            if (y1 == 0)
                y1 = pr->y;

            if(y1 < pr->y || y2 < pr->y) {
                PyErr_SetString(PyExc_IndexError, "y subscript out of range");
                return -1;
            }
            if (len != pr->bpp * (y2 - y1)) {
                PyErr_SetString(PyExc_TypeError, "string is wrong length");
                return -1;
            }
            gimp_pixel_rgn_set_col(pr, buf, x1, y1, y2 - y1);

        } else {
            PyErr_SetString(PyExc_IndexError,"invalid y subscript");
            return -1;
        }
    } else if (PySlice_Check(x)) {
        if (PySlice_GetIndices((PySliceObject *)x, pr->x + pr->w,
                               &x1, &x2, &xs) ||
	        x1 >= x2 || xs != 1) {
            PyErr_SetString(PyExc_IndexError, "invalid x slice");
	        return -1;
        }
        if(x1 == 0)
            x1 = pr->x;

        if(x1 < pr->x || x2 < pr->x) {
            PyErr_SetString(PyExc_IndexError, "x subscript out of range");
            return -1;
        }

        if (PyInt_Check(y)) {
            y1 = PyInt_AsSsize_t(y);

            if (y1 < pr->y || y1 >= pr->y + pr->h) {
                PyErr_SetString(PyExc_IndexError, "y subscript out of range");
                return -1;
            }

            if (len != pr->bpp * (x2 - x1)) {
                PyErr_SetString(PyExc_TypeError, "string is wrong length");
                return -1;
            }
            gimp_pixel_rgn_set_row(pr, buf, x1, y1, x2 - x1);

        } else if (PySlice_Check(y)) {
            if (PySlice_GetIndices((PySliceObject *)y, pr->y + pr->h,
                                   &y1, &y2, &ys) ||
                y1 >= y2 || ys != 1) {
                PyErr_SetString(PyExc_IndexError, "invalid y slice");
                return -1;
            }
            if (y1 == 0)
                y1 = pr->y;

            if(y1 < pr->y || y2 < pr->y) {
                PyErr_SetString(PyExc_IndexError, "y subscript out of range");
                return -1;
            }
            if (len != pr->bpp * (x2 - x1) * (y2 - y1)) {
                PyErr_SetString(PyExc_TypeError, "string is wrong length");
                return -1;
            }
            gimp_pixel_rgn_set_rect(pr, buf, x1, y1, x2 - x1, y2 - y1);

        } else {
            PyErr_SetString(PyExc_IndexError,"invalid y subscript");
            return -1;
        }
    } else {
        PyErr_SetString(PyExc_TypeError, "invalid x subscript");
        return -1;
    }
    return 0;
}

static PyMappingMethods pr_as_mapping = {
    pr_length,		/*mp_length*/
    (binaryfunc)pr_subscript,		/*mp_subscript*/
    (objobjargproc)pr_ass_sub,	/*mp_ass_subscript*/
};

/* -------------------------------------------------------- */

static PyObject *
pr_get_drawable(PyGimpPixelRgn *self, void *closure)
{
    return pygimp_drawable_new(self->pr.drawable, 0);
}

static PyObject *
pr_get_uint_field(PyGimpPixelRgn *self, void *closure)
{
    gint offset = GPOINTER_TO_INT(closure);
    guint value;
    gchar *addr;

    addr = (gchar *)&self->pr;
    addr += offset;
    value = *(guint *)addr;

    return PyInt_FromLong(value);
}

static PyObject *
pr_get_dirty(PyGimpPixelRgn *self, void *closure)
{
    return PyBool_FromLong(self->pr.dirty);
}

static PyObject *
pr_get_shadow(PyGimpPixelRgn *self, void *closure)
{
    return PyBool_FromLong(self->pr.shadow);
}

#define OFF(x) GINT_TO_POINTER(offsetof(GimpPixelRgn, x))
static PyGetSetDef pr_getsets[] = {
    { "drawable", (getter)pr_get_drawable, 0, NULL },
    { "bpp", (getter)pr_get_uint_field, 0, NULL, OFF(bpp) },
    { "rowstride", (getter)pr_get_uint_field, 0, NULL, OFF(rowstride) },
    { "x", (getter)pr_get_uint_field, 0, NULL, OFF(x) },
    { "y", (getter)pr_get_uint_field, 0, NULL, OFF(y) },
    { "w", (getter)pr_get_uint_field, 0, NULL, OFF(w) },
    { "h", (getter)pr_get_uint_field, 0, NULL, OFF(h) },
    { "dirty", (getter)pr_get_dirty, 0, NULL },
    { "shadow", (getter)pr_get_shadow, 0, NULL },
    { NULL, (getter)0, (setter)0 },
};
#undef OFF

static PyObject *
pr_repr(PyGimpPixelRgn *self)
{
    PyObject *s;
    gchar *name;

    name = gimp_item_get_name(self->drawable->drawable->drawable_id);
    s = PyString_FromFormat("<gimp.PixelRgn for drawable '%s'>", name);
    g_free(name);

    return s;
}

PyTypeObject PyGimpPixelRgn_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                                  /* ob_size */
    "gimp.PixelRgn",                    /* tp_name */
    sizeof(PyGimpPixelRgn),             /* tp_basicsize */
    0,                                  /* tp_itemsize */
    /* methods */
    (destructor)pr_dealloc,             /* tp_dealloc */
    (printfunc)0,                       /* tp_print */
    (getattrfunc)0,                     /* tp_getattr */
    (setattrfunc)0,                     /* tp_setattr */
    (cmpfunc)0,                         /* tp_compare */
    (reprfunc)pr_repr,                  /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    &pr_as_mapping,                     /* tp_as_mapping */
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
    pr_methods,				/* tp_methods */
    0,					/* tp_members */
    pr_getsets,				/* tp_getset */
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
pf_get_pixel(PyGimpPixelFetcher *self, PyObject *args, PyObject *kwargs)
{
    int x, y;
    guchar pixel[4];
    static char *kwlist[] = { "x", "y", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
                                     "ii:get_pixel", kwlist,
                                      &x, &y))
        return NULL;

    gimp_pixel_fetcher_get_pixel(self->pf, x, y, pixel);

    return PyString_FromStringAndSize((char *)pixel, self->bpp);
}

static PyObject *
pf_put_pixel(PyGimpPixelFetcher *self, PyObject *args, PyObject *kwargs)
{
    int x, y, len;
    guchar *pixel;
    static char *kwlist[] = { "x", "y", "pixel", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
                                     "iis#:put_pixel", kwlist,
                                      &x, &y, &pixel, &len))
        return NULL;

    if (len != self->bpp) {
	PyErr_Format(PyExc_TypeError, "pixel must be %d bpp", self->bpp);
        return NULL;
    }

    gimp_pixel_fetcher_put_pixel(self->pf, x, y, pixel);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyMethodDef pf_methods[] = {
    {"get_pixel", (PyCFunction)pf_get_pixel, METH_VARARGS | METH_KEYWORDS},
    {"put_pixel", (PyCFunction)pf_put_pixel, METH_VARARGS | METH_KEYWORDS},
    {NULL, NULL}
};

static int
pf_length(PyGimpPixelFetcher *self)
{
    PyErr_SetString(pygimp_error, "Can't get size of pixel fetcher");
    return -1;
}

static PyObject *
pf_subscript(PyGimpPixelFetcher *self, PyObject *key)
{
    PyObject *py_x, *py_y;
    int x, y;
    guchar pixel[4];

    if (!PyTuple_Check(key) || PyTuple_Size(key) != 2) {
        PyErr_SetString(PyExc_TypeError, "subscript must be a 2-tuple");
	return NULL;
    }

    if (!PyArg_ParseTuple(key, "OO", &py_x, &py_y))
        return NULL;

    if (PyInt_Check(py_x)) {
        if (PyInt_Check(py_y)) {
	    x = PyInt_AsLong(py_x);
            y = PyInt_AsLong(py_y);

            gimp_pixel_fetcher_get_pixel(self->pf, x, y, pixel);
            return PyString_FromStringAndSize((char *)pixel, self->bpp);
        } else {
            PyErr_SetString(PyExc_TypeError, "invalid y subscript");
            return NULL;
        }
    } else {
        PyErr_SetString(PyExc_TypeError, "invalid x subscript");
        return NULL;
    }
}

static int
pf_ass_sub(PyGimpPixelFetcher *self, PyObject *v, PyObject *w)
{
    PyObject *py_x, *py_y;
    int x, y, len;
    guchar *pixel;

    if (w == NULL) {
        PyErr_SetString(PyExc_TypeError, "can't delete subscripts");
        return -1;
    }

    if (!PyString_Check(w)) {
        PyErr_SetString(PyExc_TypeError, "must assign string to subscript");
        return -1;
    }

    if (!PyTuple_Check(v) || PyTuple_Size(v) != 2) {
        PyErr_SetString(PyExc_TypeError, "subscript must be a 2-tuple");
        return -1;
    }

    if (!PyArg_ParseTuple(v, "OO", &py_x, &py_y))
        return -1;

    pixel = (guchar *)PyString_AsString(w);
    len = PyString_Size(w);

    if (len != self->bpp) {
	PyErr_Format(PyExc_TypeError, "pixel must be %d bpp", self->bpp);
        return -1;
    }

    if (PyInt_Check(py_x)) {
        if (PyInt_Check(py_y)) {
	    x = PyInt_AsLong(py_x);
            y = PyInt_AsLong(py_y);

            gimp_pixel_fetcher_put_pixel(self->pf, x, y, pixel);
            return 0;
        } else {
            PyErr_SetString(PyExc_TypeError, "invalid y subscript");
            return -1;
        }
    } else {
        PyErr_SetString(PyExc_TypeError, "invalid x subscript");
        return -1;
    }
}

static PyMappingMethods pf_as_mapping = {
    (lenfunc)pf_length,
    (binaryfunc)pf_subscript,
    (objobjargproc)pf_ass_sub,
};

static PyObject *
pf_get_bg_color(PyGimpPixelFetcher *self, void *closure)
{
    return pygimp_rgb_new(&self->bg_color);
}

static int
pf_set_bg_color(PyGimpPixelFetcher *self, PyObject *value, void *closure)
{
    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError, "cannot delete bg_color");
        return -1;
    }

    if (!pygimp_rgb_from_pyobject(value, &self->bg_color))
        return -1;

    gimp_pixel_fetcher_set_bg_color(self->pf, &self->bg_color);

    return 0;
}

static PyObject *
pf_get_edge_mode(PyGimpPixelFetcher *self, void *closure)
{
    return PyInt_FromLong(self->edge_mode);
}

static int
pf_set_edge_mode(PyGimpPixelFetcher *self, PyObject *value, void *closure)
{
    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError, "cannot delete edge_mode");
        return -1;
    }

    if (!PyInt_Check(value)) {
        PyErr_SetString(PyExc_TypeError, "type mismatch");
        return -1;
    }

    self->edge_mode = PyInt_AsLong(value);

    gimp_pixel_fetcher_set_edge_mode(self->pf, self->edge_mode);

    return 0;
}

static PyGetSetDef pf_getsets[] = {
    { "bg_color", (getter)pf_get_bg_color, (setter)pf_set_bg_color },
    { "edge_mode", (getter)pf_get_edge_mode, (setter)pf_set_edge_mode },
    { NULL, (getter)0, (setter)0 }
};

static void
pf_dealloc(PyGimpPixelFetcher *self)
{
    gimp_pixel_fetcher_destroy(self->pf);

    Py_XDECREF(self->drawable);

    PyObject_DEL(self);
}

static PyObject *
pf_repr(PyGimpPixelFetcher *self)
{
    PyObject *s;
    char *name;

    name = gimp_item_get_name(self->drawable->drawable->drawable_id);

    if (self->shadow)
        s = PyString_FromFormat("<gimp.PixelFetcher for drawable '%s' (shadow)>", name);
    else
        s = PyString_FromFormat("<gimp.PixelFetcher for drawable '%s'>", name);

    g_free(name);

    return s;
}

static int
pf_init(PyGimpPixelFetcher *self, PyObject *args, PyObject *kwargs)
{
    PyGimpDrawable *drw;
    gboolean shadow = FALSE;
    GimpRGB bg_color = { 0.0, 0.0, 0.0, 1.0 };
    GimpPixelFetcherEdgeMode edge_mode = GIMP_PIXEL_FETCHER_EDGE_NONE;
    static char *kwlist[] = { "drawable", "shadow", "bg_color", "edge_mode",
	                      NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
                                     "O!|iO&i:gimp.PixelFetcher.__init__",
                                     kwlist,
                                     &PyGimpDrawable_Type, &drw, &shadow,
                                     pygimp_rgb_from_pyobject, &bg_color,
                                     &edge_mode))
        return -1;

    if(!drw->drawable)
        drw->drawable = gimp_drawable_get(drw->ID);

    self->pf = gimp_pixel_fetcher_new(drw->drawable, shadow);

    Py_INCREF(drw);
    self->drawable = drw;

    self->shadow = shadow;
    self->bg_color = bg_color;
    self->edge_mode = edge_mode;

    self->bpp = gimp_drawable_bpp(drw->drawable->drawable_id);

    gimp_pixel_fetcher_set_bg_color(self->pf, &bg_color);
    gimp_pixel_fetcher_set_edge_mode(self->pf, edge_mode);

    return 0;
}

PyTypeObject PyGimpPixelFetcher_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                                  /* ob_size */
    "gimp.PixelFetcher",                /* tp_name */
    sizeof(PyGimpPixelFetcher),         /* tp_basicsize */
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
    &pf_as_mapping,                     /* tp_as_mapping */
    (hashfunc)0,                        /* tp_hash */
    (ternaryfunc)0,                     /* tp_call */
    (reprfunc)0,                        /* tp_str */
    (getattrofunc)0,                    /* tp_getattro */
    (setattrofunc)0,                    /* tp_setattro */
    0,                                  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                 /* tp_flags */
    NULL, /* Documentation string */
    (traverseproc)0,                    /* tp_traverse */
    (inquiry)0,                         /* tp_clear */
    (richcmpfunc)0,                     /* tp_richcompare */
    0,                                  /* tp_weaklistoffset */
    (getiterfunc)0,                     /* tp_iter */
    (iternextfunc)0,                    /* tp_iternext */
    pf_methods,                         /* tp_methods */
    0,                                  /* tp_members */
    pf_getsets,                         /* tp_getset */
    (PyTypeObject *)0,                  /* tp_base */
    (PyObject *)0,                      /* tp_dict */
    0,                                  /* tp_descr_get */
    0,                                  /* tp_descr_set */
    0,                                  /* tp_dictoffset */
    (initproc)pf_init,                  /* tp_init */
    (allocfunc)0,                       /* tp_alloc */
    (newfunc)0,                         /* tp_new */
};
