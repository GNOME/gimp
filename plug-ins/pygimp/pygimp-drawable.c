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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include "pygimp.h"

static void
ensure_drawable(PyGimpDrawable *self)
{
    if (!self->drawable)
	self->drawable = gimp_drawable_get(self->ID);
}

static PyObject *
drw_flush(PyGimpDrawable *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ":flush"))
	return NULL;
    ensure_drawable(self);
    gimp_drawable_flush(self->drawable);
    Py_INCREF(Py_None);
    return Py_None;
}


static PyObject *
drw_update(PyGimpDrawable *self, PyObject *args)
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
drw_merge_shadow(PyGimpDrawable *self, PyObject *args)
{
    int u;
    if (!PyArg_ParseTuple(args, "i:merge_shadow", &u))
	return NULL;
    gimp_drawable_merge_shadow(self->ID, u);
    Py_INCREF(Py_None);
    return Py_None;
}


static PyObject *
drw_fill(PyGimpDrawable *self, PyObject *args)
{
    int f;
    if (!PyArg_ParseTuple(args, "i:fill", &f))
	return NULL;
    gimp_drawable_fill(self->ID, f);
    Py_INCREF(Py_None);
    return Py_None;
}


static PyObject *
drw_get_tile(PyGimpDrawable *self, PyObject *args)
{
    GimpTile *t;
    int shadow, r, c;
    if (!PyArg_ParseTuple(args, "iii:get_tile", &shadow, &r, &c))
	return NULL;
    ensure_drawable(self);
    t = gimp_drawable_get_tile(self->drawable, shadow, r, c);
    return pygimp_tile_new(t, self);
}

static PyObject *
drw_get_tile2(PyGimpDrawable *self, PyObject *args)
{
    GimpTile *t;
    int shadow, x, y;
    if (!PyArg_ParseTuple(args, "iii:get_tile2", &shadow, &x ,&y))
	return NULL;
    ensure_drawable(self);
    t = gimp_drawable_get_tile2(self->drawable, shadow, x, y);
    return pygimp_tile_new(t, self);
}

static PyObject *
drw_get_pixel_rgn(PyGimpDrawable *self, PyObject *args)
{
    int x, y, w, h, dirty = 1, shadow = 0;
    if (!PyArg_ParseTuple(args, "iiii|ii:get_pixel_rgn", &x,&y,
			  &w,&h, &dirty,&shadow))
	return NULL;
    ensure_drawable(self);
    return pygimp_pixel_rgn_new(self, x, y, w, h, dirty, shadow);
}

static PyObject *
drw_parasite_find(PyGimpDrawable *self, PyObject *args)
{
    char *name;
    if (!PyArg_ParseTuple(args, "s:parasite_find", &name))
	return NULL;
    return pygimp_parasite_new(gimp_drawable_parasite_find(self->ID, name));
}

static PyObject *
drw_parasite_attach(PyGimpDrawable *self, PyObject *args)
{
    PyGimpParasite *parasite;
    if (!PyArg_ParseTuple(args, "O!:parasite_attach", &PyGimpParasite_Type,
			  &parasite))
	return NULL;
    gimp_drawable_parasite_attach(self->ID, parasite->para);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
drw_attach_new_parasite(PyGimpDrawable *self, PyObject *args)
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
drw_parasite_detach(PyGimpDrawable *self, PyObject *args)
{
    char *name;
    if (!PyArg_ParseTuple(args, "s:detach_parasite", &name))
	return NULL;
    gimp_drawable_parasite_detach(self->ID, name);
    Py_INCREF(Py_None);
    return Py_None;
}

static void
drw_dealloc(PyGimpLayer *self)
{
    if (self->drawable)
	gimp_drawable_detach(self->drawable);
    PyObject_DEL(self);
}

static PyObject *
drw_repr(PyGimpLayer *self)
{
    PyObject *s;
    gchar *name;

    name = gimp_drawable_name(self->ID);
    s = PyString_FromFormat("<gimp.Drawable %s>", name);
    g_free(name);
    return s;
}

static int
drw_cmp(PyGimpLayer *self, PyGimpLayer *other)
{
    if (self->ID == other->ID) return 0;
    if (self->ID > other->ID) return -1;
    return 1;
}

/* for inclusion with the methods of layer and channel objects */
static PyMethodDef drw_methods[] = {
    {"flush",	(PyCFunction)drw_flush,	METH_VARARGS},
    {"update",	(PyCFunction)drw_update,	METH_VARARGS},
    {"merge_shadow",	(PyCFunction)drw_merge_shadow,	METH_VARARGS},
    {"fill",	(PyCFunction)drw_fill,	METH_VARARGS},
    {"get_tile",	(PyCFunction)drw_get_tile,	METH_VARARGS},
    {"get_tile2",	(PyCFunction)drw_get_tile2,	METH_VARARGS},
    {"get_pixel_rgn", (PyCFunction)drw_get_pixel_rgn, METH_VARARGS},
    {"parasite_find",       (PyCFunction)drw_parasite_find, METH_VARARGS},
    {"parasite_attach",     (PyCFunction)drw_parasite_attach, METH_VARARGS},
    {"attach_new_parasite",(PyCFunction)drw_attach_new_parasite,METH_VARARGS},
    {"parasite_detach",     (PyCFunction)drw_parasite_detach, METH_VARARGS},
    {NULL, NULL, 0}
};

PyTypeObject PyGimpDrawable_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                                  /* ob_size */
    "gimp.Drawable",                    /* tp_name */
    sizeof(PyGimpDrawable),             /* tp_basicsize */
    0,                                  /* tp_itemsize */
    /* methods */
    (destructor)drw_dealloc,            /* tp_dealloc */
    (printfunc)0,                       /* tp_print */
    (getattrfunc)0,                     /* tp_getattr */
    (setattrfunc)0,                     /* tp_setattr */
    (cmpfunc)drw_cmp,                   /* tp_compare */
    (reprfunc)drw_repr,                 /* tp_repr */
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
    drw_methods,			/* tp_methods */
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


PyObject *
pygimp_drawable_new(GimpDrawable *drawable, gint32 ID)
{
    PyObject *self;

    if (drawable == NULL && ID == -1) {
	Py_INCREF(Py_None);
	return Py_None;
    }
    if (drawable != NULL)
	ID = drawable->drawable_id;
    /* create the appropriate object type */
    if (gimp_drawable_is_layer(ID))
	self = pygimp_layer_new(ID);
    else
	self = pygimp_channel_new(ID);

    if (self == NULL)
	return NULL;

    ((PyGimpDrawable *)self)->drawable = drawable;

    return self;
}

/* End of code for Drawable objects */
/* -------------------------------------------------------- */


static PyObject *
lay_copy(PyGimpLayer *self, PyObject *args)
{
    int add_alpha = 0, nreturn_vals;
    GimpParam *return_vals;
    gint32 id;

    /* start of long convoluted (working) layer_copy */
    if (!PyArg_ParseTuple(args, "|i:copy", &add_alpha))
	return NULL;

    return_vals = gimp_run_procedure("gimp_layer_copy",
				     &nreturn_vals, 
				     GIMP_PDB_LAYER, self->ID,
				     GIMP_PDB_INT32, add_alpha,
				     GIMP_PDB_END);
    if (return_vals[0].data.d_status != GIMP_PDB_SUCCESS) {
	PyErr_SetString(pygimp_error, "can't create new layer");
	return NULL;
    }	
    id = return_vals[1].data.d_layer;
    gimp_destroy_params(return_vals, nreturn_vals);
    return pygimp_layer_new(id);

    /* This simple version of the code doesn't seem to work */
    /* return (PyObject *)newlayobject(gimp_layer_copy(self->ID));*/
}


static PyObject *
lay_add_alpha(PyGimpLayer *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ":add_alpha"))
	return NULL;
    gimp_layer_add_alpha(self->ID);
    Py_INCREF(Py_None);
    return Py_None;
}


static PyObject *
lay_create_mask(PyGimpLayer *self, PyObject *args)
{
    int type;

    if (!PyArg_ParseTuple(args, "i:create_mask", &type))
	return NULL;
    return pygimp_channel_new(gimp_layer_create_mask(self->ID,type));
}


static PyObject *
lay_resize(PyGimpLayer *self, PyObject *args)
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
lay_scale(PyGimpLayer *self, PyObject *args)
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
lay_translate(PyGimpLayer *self, PyObject *args)
{
    int offs_x, offs_y;
    if (!PyArg_ParseTuple(args, "ii:translate", &offs_x, &offs_y))
	return NULL;
    gimp_layer_translate(self->ID, offs_x, offs_y);
    Py_INCREF(Py_None);
    return Py_None;
}


static PyObject *
lay_set_offsets(PyGimpLayer *self, PyObject *args)
{
    int offs_x, offs_y;
    if (!PyArg_ParseTuple(args, "ii:set_offsets", &offs_x, &offs_y))
	return NULL;
    gimp_layer_set_offsets(self->ID, offs_x, offs_y);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
lay_get_tattoo(PyGimpLayer *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ":get_tattoo"))
	return NULL;
    return PyInt_FromLong(gimp_layer_get_tattoo(self->ID));
}

static struct PyMethodDef lay_methods[] = {
    {"copy",	(PyCFunction)lay_copy,	METH_VARARGS},
    {"add_alpha",	(PyCFunction)lay_add_alpha,	METH_VARARGS},
    {"create_mask",	(PyCFunction)lay_create_mask,	METH_VARARGS},
    {"resize",	(PyCFunction)lay_resize,	METH_VARARGS},
    {"scale",	(PyCFunction)lay_scale,	METH_VARARGS},
    {"translate",	(PyCFunction)lay_translate,	METH_VARARGS},
    {"set_offsets",	(PyCFunction)lay_set_offsets,	METH_VARARGS},
    {"get_tattoo",      (PyCFunction)lay_get_tattoo,    METH_VARARGS},
    {NULL,		NULL}		/* sentinel */
};




static PyObject *
lay_getattr(PyGimpLayer *self, char *attr)
{
    gint32 id;
	
    if (!strcmp(attr, "__members__"))
	return Py_BuildValue("[ssssssssssssssssssss]", "ID", "apply_mask",
			     "bpp", "edit_mask", "has_alpha", "height",
			     "image", "is_color", "is_colour",
			     "is_floating_selection", "is_gray", "is_grey",
			     "is_indexed", "is_rgb", "mask", "mask_bounds",
			     "mode", "name", "offsets", "opacity",
			     "preserve_transparency", "show_mask", "type",
			     "visible", "width");
    if (!strcmp(attr, "ID"))
	return PyInt_FromLong(self->ID);
    if (!strcmp(attr, "bpp"))
	return PyInt_FromLong((long) gimp_drawable_bpp(self->ID));
    if (!strcmp(attr, "has_alpha"))
	return PyInt_FromLong(gimp_drawable_has_alpha(self->ID));
    if (!strcmp(attr, "height"))
	return PyInt_FromLong((long) gimp_drawable_height(self->ID));
    if (!strcmp(attr, "image")) {
	id = gimp_layer_get_image_id(self->ID);
	if (id == -1) {
	    Py_INCREF(Py_None);
	    return Py_None;
	}
	return pygimp_image_new(id);
    }
    if (!strcmp(attr, "is_color") || !strcmp(attr, "is_colour") ||
	!strcmp(attr, "is_rgb"))
	return PyInt_FromLong(gimp_drawable_is_rgb(self->ID));
    if (!strcmp(attr, "is_floating_selection"))
	return PyInt_FromLong(
			      gimp_layer_is_floating_selection(self->ID));
    if (!strcmp(attr, "is_gray") || !strcmp(attr, "is_grey"))
	return PyInt_FromLong(gimp_drawable_is_gray(self->ID));
    if (!strcmp(attr, "is_indexed"))
	return PyInt_FromLong(gimp_drawable_is_indexed(self->ID));
    if (!strcmp(attr, "mask")) {
	id = gimp_layer_get_mask_id(self->ID);
	if (id == -1) {
	    Py_INCREF(Py_None);
	    return Py_None;
	}
	return pygimp_channel_new(id);
    }
    if (!strcmp(attr, "mask_bounds")) {
	gint x1, y1, x2, y2;
	gimp_drawable_mask_bounds(self->ID, &x1, &y1, &x2, &y2);
	return Py_BuildValue("(iiii)", x1, y1, x2, y2);
    }
    if (!strcmp(attr, "apply_mask"))
	return PyInt_FromLong((long) gimp_layer_get_apply_mask(
							       self->ID));
    if (!strcmp(attr, "edit_mask"))
	return PyInt_FromLong((long) gimp_layer_get_edit_mask(
							      self->ID));
    if (!strcmp(attr, "mode"))
	return PyInt_FromLong((long) gimp_layer_get_mode(self->ID));
    if (!strcmp(attr, "name"))
	return PyString_FromString(gimp_layer_get_name(self->ID));
    if (!strcmp(attr, "offsets")) {
	gint x, y;
	gimp_drawable_offsets(self->ID, &x, &y);
	return Py_BuildValue("(ii)", x, y);
    }
    if (!strcmp(attr, "opacity"))
	return PyFloat_FromDouble((double) gimp_layer_get_opacity(
								  self->ID));
    if (!strcmp(attr, "preserve_transparency"))
	return PyInt_FromLong((long)
			      gimp_layer_get_preserve_transparency(self->ID));
    if (!strcmp(attr, "show_mask"))
	return PyInt_FromLong((long) gimp_layer_get_show_mask(
							      self->ID));
    if (!strcmp(attr, "type"))
	return PyInt_FromLong((long) gimp_drawable_type(self->ID));
    if (!strcmp(attr, "visible"))
	return PyInt_FromLong((long) gimp_layer_get_visible(self->ID));
    if (!strcmp(attr, "width"))
	return PyInt_FromLong((long) gimp_drawable_width(self->ID));
    {
	PyObject *name = PyString_FromString(attr);
	PyObject *ret = PyObject_GenericGetAttr((PyObject *)self, name);
	Py_DECREF(name);
	return ret;
    }
}

static int
lay_setattr(PyGimpLayer *self, char *attr, PyObject *v)
{
    if (v == NULL) {
	PyErr_SetString(PyExc_TypeError, "can not delete attributes.");
	return -1;
    }
    /* Set attribute 'name' to value 'v'. v==NULL means delete */
    if (!strcmp(attr, "apply_mask")) {
	if (!PyInt_Check(v)) {
	    PyErr_SetString(PyExc_TypeError, "type mis-match.");
	    return -1;
	}
	gimp_layer_set_apply_mask(self->ID, PyInt_AsLong(v));
	return 0;
    }
    if (!strcmp(attr, "edit_mask")) {
	if (!PyInt_Check(v)) {
	    PyErr_SetString(PyExc_TypeError, "type mis-match.");
	    return -1;
	}
	gimp_layer_set_edit_mask(self->ID, PyInt_AsLong(v));
	return 0;
    }
    if (!strcmp(attr, "mode")) {
	if (!PyInt_Check(v)) {
	    PyErr_SetString(PyExc_TypeError, "type mis-match.");
	    return -1;
	}
	gimp_layer_set_mode(self->ID, (GimpLayerModeEffects)PyInt_AsLong(v));
	return 0;
    }
    if (!strcmp(attr, "name")) {
	if (!PyString_Check(v)) {
	    PyErr_SetString(PyExc_TypeError, "type mis-match.");
	    return -1;
	}
	gimp_layer_set_name(self->ID, PyString_AsString(v));
	return 0;
    }
    if (!strcmp(attr, "opacity")) {
	if (!PyFloat_Check(v)) {
	    PyErr_SetString(PyExc_TypeError, "type mis-match.");
	    return -1;
	}
	gimp_layer_set_opacity(self->ID, PyFloat_AsDouble(v));
	return 0;
    }
    if (!strcmp(attr, "preserve_transparency")) {
	if (!PyInt_Check(v)) {
	    PyErr_SetString(PyExc_TypeError, "type mis-match.");
	    return -1;
	}
	gimp_layer_set_preserve_transparency(self->ID,
					     PyInt_AsLong(v));
	return 0;
    }
    if (!strcmp(attr, "show_mask")) {
	if (!PyInt_Check(v)) {
	    PyErr_SetString(PyExc_TypeError, "type mis-match.");
	    return -1;
	}
	gimp_layer_set_show_mask(self->ID, PyInt_AsLong(v));
	return 0;
    }
    if (!strcmp(attr, "visible")) {
	if (!PyInt_Check(v)) {
	    PyErr_SetString(PyExc_TypeError, "type mis-match.");
	    return -1;
	}
	gimp_layer_set_visible(self->ID, PyInt_AsLong(v));
	return 0;
    }
    if (!strcmp(attr, "ID") ||
	!strcmp(attr, "bpp") || !strcmp(attr, "height") ||
	!strcmp(attr, "image") || !strcmp(attr, "mask") ||
	!strcmp(attr, "type") || !strcmp(attr, "width") ||
	!strcmp(attr, "is_floating_selection") ||
	!strcmp(attr, "offsets") || !strcmp(attr, "mask_bounds") ||
	!strcmp(attr, "has_alpha") || !strcmp(attr, "is_color") ||
	!strcmp(attr, "is_colour") || !strcmp(attr, "is_rgb") ||
	!strcmp(attr, "is_gray") || !strcmp(attr, "is_grey") ||
	!strcmp(attr, "is_indexed") || !strcmp(attr, "__members__")) {
	PyErr_SetString(PyExc_TypeError, "read-only attribute.");
	return -1;
    }
    {
	PyObject *name = PyString_FromString(attr);
	int ret = PyObject_GenericSetAttr((PyObject *)self, name, v);
	Py_DECREF(name);
	return ret;
    }
}

static PyObject *
lay_repr(PyGimpLayer *self)
{
    PyObject *s;
    gchar *name;

    name = gimp_layer_get_name(self->ID);
    s = PyString_FromFormat("<gimp.Layer %s>", name);
    g_free(name);
    return s;
}

static int
lay_init(PyGimpLayer *self, PyObject *args, PyObject *kwargs)
{
    PyGimpImage *img;
    char *name;
    unsigned int width, height;
    GimpImageType type;
    double opacity;
    GimpLayerModeEffects mode;
	

    if (!PyArg_ParseTuple(args, "O!siiidi:gimp.Layer.__init__",
			  &PyGimpImage_Type, &img, &name, &width, &height,
			  &type, &opacity, &mode))
	return -1;
    self->ID = gimp_layer_new(img->ID, name, width,
			      height, type, opacity, mode);
    self->drawable = NULL;
    if (self->ID < 0) {
	PyErr_SetString(pygimp_error, "could not create layer");
	return -1;
    }
    return 0;
}

PyTypeObject PyGimpLayer_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                                  /* ob_size */
    "gimp.Layer",                       /* tp_name */
    sizeof(PyGimpLayer),                /* tp_basicsize */
    0,                                  /* tp_itemsize */
    /* methods */
    (destructor)drw_dealloc,            /* tp_dealloc */
    (printfunc)0,                       /* tp_print */
    (getattrfunc)lay_getattr,           /* tp_getattr */
    (setattrfunc)lay_setattr,           /* tp_setattr */
    (cmpfunc)drw_cmp,                   /* tp_compare */
    (reprfunc)lay_repr,                 /* tp_repr */
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
    lay_methods,			/* tp_methods */
    0,					/* tp_members */
    0,					/* tp_getset */
    &PyGimpDrawable_Type,		/* tp_base */
    (PyObject *)0,			/* tp_dict */
    0,					/* tp_descr_get */
    0,					/* tp_descr_set */
    0,					/* tp_dictoffset */
    (initproc)lay_init,                 /* tp_init */
    (allocfunc)0,			/* tp_alloc */
    (newfunc)0,				/* tp_new */
};

PyObject *
pygimp_layer_new(gint32 ID)
{
    PyGimpLayer *self;

    if (ID == -1) {
	Py_INCREF(Py_None);
	return Py_None;
    }
    self = PyObject_NEW(PyGimpLayer, &PyGimpLayer_Type);
    if (self == NULL)
	return NULL;
    self->ID = ID;
    self->drawable = NULL;
    return (PyObject *)self;
}

/* End of code for Layer objects */
/* -------------------------------------------------------- */


static PyObject *
chn_copy(PyGimpChannel *self, PyObject *args)
{
    gint32 id;
	
    if (!PyArg_ParseTuple(args, ":copy"))
	return NULL;
    id = gimp_channel_copy(self->ID);
    if (id == -1) {
	PyErr_SetString(pygimp_error, "can't copy channel");
	return NULL;
    }
    return pygimp_channel_new(id);
}

static PyObject *
chn_get_tattoo(PyGimpChannel *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ":get_tattoo"))
	return NULL;
    return PyInt_FromLong(gimp_channel_get_tattoo(self->ID));
}

static struct PyMethodDef chn_methods[] = {
    {"copy",	(PyCFunction)chn_copy,	METH_VARARGS},
    {"get_tattoo", (PyCFunction)chn_get_tattoo, METH_VARARGS},
    {NULL,		NULL}		/* sentinel */
};


static PyObject *
chn_getattr(PyGimpChannel *self, char *attr)
{
    gint32 id;
	
    if (!strcmp(attr, "__members__"))
	return Py_BuildValue("[ssssssssssssssssssssssss]", "ID",
			     "bpp", "color", "colour", "has_alpha", "height",
			     "image", "is_color", "is_colour", "is_gray",
			     "is_grey", "is_indexed", "is_layer_mask",
			     "is_rgb", "layer", "layer_mask", "mask_bounds",
			     "name", "offsets", "opacity", "show_masked",
			     "type", "visible", "width");
    if (!strcmp(attr, "ID"))
	return PyInt_FromLong(self->ID);
    if (!strcmp(attr, "bpp"))
	return PyInt_FromLong(gimp_drawable_bpp(self->ID));
    if (!strcmp(attr, "color") || !strcmp(attr, "colour")) {
	GimpRGB colour;
	guchar r, g, b;

	gimp_channel_get_color(self->ID, &colour);
	gimp_rgb_get_uchar(&colour, &r, &g, &b);
	return Py_BuildValue("(iii)", (long)r, (long)g, (long)b);
    }
    if (!strcmp(attr, "has_alpha"))
	return PyInt_FromLong(gimp_drawable_has_alpha(self->ID));
    if (!strcmp(attr, "height"))
	return PyInt_FromLong((long)gimp_drawable_height(self->ID));
    if (!strcmp(attr, "image")) {
	id = gimp_channel_get_image_id(self->ID);
	if (id == -1) {
	    Py_INCREF(Py_None);
	    return Py_None;
	}
	return pygimp_image_new(id);
    }
    if (!strcmp(attr, "is_color") || !strcmp(attr, "is_colour") ||
	!strcmp(attr, "is_rgb"))
	return PyInt_FromLong(gimp_drawable_is_rgb(self->ID));
    if (!strcmp(attr, "is_gray") || !strcmp(attr, "is_grey"))
	return PyInt_FromLong(gimp_drawable_is_gray(self->ID));
    if (!strcmp(attr, "is_indexed"))
	return PyInt_FromLong(gimp_drawable_is_indexed(self->ID));
    if (!strcmp(attr, "layer")) {
	/* id = gimp_channel_get_layer_id(self->ID); */
	/* It isn't quite clear what that was supposed to achieve, but
	   the gimp_channel_get_layer_id call no longer exists, which
	   was breaking everything that tried to load gimpmodule.so.

	   With no-one apparently maintaing it, the options seem to be:
	    A) remove this "layer" attribute entirely, as it doesn't
	       seem to make much sense.
            B) Just return the channel ID.
	   
	    The latter seems more conservative, so that's what I'll do.
	    -- acapnotic@users.sourceforge.net (08/09/2000) */
	id = self->ID;
	if (id == -1) {
	    Py_INCREF(Py_None);
	    return Py_None;
	}
	return pygimp_layer_new(id);
    }
    if (!strcmp(attr, "layer_mask") || !strcmp(attr, "is_layer_mask"))
	return PyInt_FromLong(gimp_drawable_is_layer_mask(self->ID));
    if (!strcmp(attr, "mask_bounds")) {
	gint x1, y1, x2, y2;
	gimp_drawable_mask_bounds(self->ID, &x1, &y1, &x2, &y2);
	return Py_BuildValue("(iiii)", x1, y1, x2, y2);
    }
    if (!strcmp(attr, "name"))
	return PyString_FromString(gimp_channel_get_name(self->ID));
    if (!strcmp(attr, "offsets")) {
	gint x, y;
	gimp_drawable_offsets(self->ID, &x, &y);
	return Py_BuildValue("(ii)", x, y);
    }
    if (!strcmp(attr, "opacity"))
	return PyFloat_FromDouble(gimp_channel_get_opacity(self->ID));
    if (!strcmp(attr, "show_masked"))
	return PyInt_FromLong(gimp_channel_get_show_masked(self->ID));
    if (!strcmp(attr, "type"))
	return PyInt_FromLong(gimp_drawable_type(self->ID));
    if (!strcmp(attr, "visible"))
	return PyInt_FromLong(gimp_channel_get_visible(self->ID));
    if (!strcmp(attr, "width"))
	return PyInt_FromLong(gimp_drawable_width(self->ID));

    {
	PyObject *name = PyString_FromString(attr);
	PyObject *ret = PyObject_GenericGetAttr((PyObject *)self, name);
	Py_DECREF(name);
	return ret;
    }
}

static int
chn_setattr(PyGimpChannel *self, char *attr, PyObject *v)
{
    if (v == NULL) {
	PyErr_SetString(PyExc_TypeError, "can not delete attributes.");
	return -1;
    }
    if (!strcmp(attr, "color") || !strcmp(attr, "colour")) {
	PyObject *r, *g, *b;
	GimpRGB colour;

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
	gimp_rgb_set_uchar(&colour, PyInt_AsLong(r),
			   PyInt_AsLong(g), PyInt_AsLong(b));
	Py_DECREF(r); Py_DECREF(g); Py_DECREF(b);
	gimp_channel_set_color(self->ID, &colour);
	return 0;
    }
    if (!strcmp(attr, "name")) {
	if (!PyString_Check(v)) {
	    PyErr_SetString(PyExc_TypeError, "type mis-match.");
	    return -1;
	}
	gimp_channel_set_name(self->ID, PyString_AsString(v));
	return 0;
    }
    if (!strcmp(attr, "opacity")) {
	if (!PyFloat_Check(v)) {
	    PyErr_SetString(PyExc_TypeError, "type mis-match.");
	    return -1;
	}
	gimp_channel_set_opacity(self->ID, PyFloat_AsDouble(v));
	return 0;
    }
    /*	if (!strcmp(attr, "show_masked")) {
	if (!PyInt_Check(v)) {
	PyErr_SetString(PyExc_TypeError, "type mis-match.");
	return -1;
	}
	gimp_channel_set_show_masked(self->ID, PyInt_AsLong(v));
	return 0;
	}
    */	if (!strcmp(attr, "visible")) {
	if (!PyInt_Check(v)) {
	    PyErr_SetString(PyExc_TypeError, "type mis-match.");
	    return -1;
	}
	gimp_channel_set_visible(self->ID, PyInt_AsLong(v));
	return 0;
    }
    if (!strcmp(attr, "height") || !strcmp(attr, "image") ||
	!strcmp(attr, "layer") || !strcmp(attr, "width") ||
	!strcmp(attr, "ID") || !strcmp(attr, "bpp") ||
	!strcmp(attr, "layer_mask") || !strcmp(attr, "mask_bounds") ||
	!strcmp(attr, "offsets") || !strcmp(attr, "type") ||
	!strcmp(attr, "has_alpha") || !strcmp(attr, "is_color") ||
	!strcmp(attr, "is_colour") || !strcmp(attr, "is_rgb") ||
	!strcmp(attr, "is_gray") || !strcmp(attr, "is_grey") ||
	!strcmp(attr, "is_indexed") || !strcmp(attr, "is_layer_mask") ||
	!strcmp(attr, "__members__")) {
	PyErr_SetString(PyExc_TypeError, "read-only attribute.");
	return -1;
    }
    {
	PyObject *name = PyString_FromString(attr);
	int ret = PyObject_GenericSetAttr((PyObject *)self, name, v);
	Py_DECREF(name);
	return ret;
    }
}

static PyObject *
chn_repr(PyGimpChannel *self)
{
    PyObject *s;
    gchar *name;

    name = gimp_channel_get_name(self->ID);
    s = PyString_FromFormat("<gimp.Channel %s>", name);
    g_free(name);
    return s;
}

static int
chn_init(PyGimpChannel *self, PyObject *args, PyObject *kwargs)
{
    PyGimpImage *img;
    char *name;
    unsigned int width, height, r, g, b;
    double opacity;
    GimpRGB colour;

    if (!PyArg_ParseTuple(args, "O!siid(iii):gimp.Channel.__init__",
			  &PyGimpImage_Type, &img, &name, &width,
			  &height, &opacity, &r, &g, &b))
	return -1;
    gimp_rgb_set_uchar(&colour, r & 0xff, g & 0xff, b & 0xff);
    self->ID = gimp_channel_new(img->ID, name, width, height,
				opacity, &colour);
    self->drawable = NULL;
    if (self->ID < 0) {
	PyErr_SetString(pygimp_error, "could not create layer");
	return -1;
    }
    return 0;
}

PyTypeObject PyGimpChannel_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                                  /* ob_size */
    "gimp.Channel",                     /* tp_name */
    sizeof(PyGimpChannel),              /* tp_basicsize */
    0,                                  /* tp_itemsize */
    /* methods */
    (destructor)drw_dealloc,            /* tp_dealloc */
    (printfunc)0,                       /* tp_print */
    (getattrfunc)chn_getattr,           /* tp_getattr */
    (setattrfunc)chn_setattr,           /* tp_setattr */
    (cmpfunc)drw_cmp,                   /* tp_compare */
    (reprfunc)chn_repr,                 /* tp_repr */
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
    chn_methods,			/* tp_methods */
    0,					/* tp_members */
    0,					/* tp_getset */
    &PyGimpDrawable_Type,		/* tp_base */
    (PyObject *)0,			/* tp_dict */
    0,					/* tp_descr_get */
    0,					/* tp_descr_set */
    0,					/* tp_dictoffset */
    (initproc)chn_init,                 /* tp_init */
    (allocfunc)0,			/* tp_alloc */
    (newfunc)0,				/* tp_new */
};

PyObject *
pygimp_channel_new(gint32 ID)
{
    PyGimpChannel *self;

    if (ID == -1) {
	Py_INCREF(Py_None);
	return Py_None;
    }
    self = PyObject_NEW(PyGimpChannel, &PyGimpChannel_Type);
    if (self == NULL)
	return NULL;
    self->ID = ID;
    self->drawable = NULL;
    return (PyObject *)self;
}
