/* -*- Mode: C; c-basic-offset: 4 -*- 
    Gimp-Python - allows the writing of Gimp plugins in Python.
    Copyright (C) 1997-2002  James Henstridge <james@daa.com.au>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published 
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
drw_flush(PyGimpDrawable *self)
{
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
drw_offset(PyGimpDrawable *self, PyObject *args)
{
    gboolean wrap_around;
    GimpOffsetType fill_type;
    gint offset_x, offset_y;

    if (!PyArg_ParseTuple(args, "iiii:offset", &wrap_around, &fill_type,
			   &offset_x, &offset_y))
	return NULL;
    return PyInt_FromLong(gimp_drawable_offset(self->ID, wrap_around,
					       fill_type, offset_x, offset_y));
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

static PyObject *
drw_parasite_list(PyGimpImage *self)
{
    gint num_parasites;
    gchar **parasites;

    if (gimp_drawable_parasite_list(self->ID, &num_parasites, &parasites)) {
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
    PyErr_SetString(pygimp_error, "could not list parasites");
    return NULL;
}

/* for inclusion with the methods of layer and channel objects */
static PyMethodDef drw_methods[] = {
    {"flush",	(PyCFunction)drw_flush,	METH_NOARGS},
    {"update",	(PyCFunction)drw_update,	METH_VARARGS},
    {"merge_shadow",	(PyCFunction)drw_merge_shadow,	METH_VARARGS},
    {"fill",	(PyCFunction)drw_fill,	METH_VARARGS},
    {"get_tile",	(PyCFunction)drw_get_tile,	METH_VARARGS},
    {"get_tile2",	(PyCFunction)drw_get_tile2,	METH_VARARGS},
    {"get_pixel_rgn", (PyCFunction)drw_get_pixel_rgn, METH_VARARGS},
    {"offset", (PyCFunction)drw_offset, METH_VARARGS},
    {"parasite_find",       (PyCFunction)drw_parasite_find, METH_VARARGS},
    {"parasite_attach",     (PyCFunction)drw_parasite_attach, METH_VARARGS},
    {"attach_new_parasite",(PyCFunction)drw_attach_new_parasite,METH_VARARGS},
    {"parasite_detach",     (PyCFunction)drw_parasite_detach, METH_VARARGS},
    {"parasite_list",     (PyCFunction)drw_parasite_list, METH_VARARGS},
    {NULL, NULL, 0}
};

static PyObject *
drw_get_ID(PyGimpDrawable *self, void *closure)
{
    return PyInt_FromLong(self->ID);
}

static PyObject *
drw_get_name(PyGimpDrawable *self, void *closure)
{
    return PyString_FromString(gimp_drawable_get_name(self->ID));
}

static int
drw_set_name(PyGimpDrawable *self, PyObject *value, void *closure)
{
    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError, "cannot delete name");
        return -1;
    }
    if (!PyString_Check(value)) {
        PyErr_SetString(PyExc_TypeError, "type mismatch");
        return -1;
    }
    gimp_drawable_set_name(self->ID, PyString_AsString(value));
    return 0;
}

static PyObject *
drw_get_bpp(PyGimpDrawable *self, void *closure)
{
    return PyInt_FromLong(gimp_drawable_bpp(self->ID));
}

static PyObject *
drw_get_has_alpha(PyGimpDrawable *self, void *closure)
{
    return PyInt_FromLong(gimp_drawable_has_alpha(self->ID));
}

static PyObject *
drw_get_height(PyGimpDrawable *self, void *closure)
{
    return PyInt_FromLong(gimp_drawable_height(self->ID));
}

static PyObject *
drw_get_image(PyGimpDrawable *self, void *closure)
{
    return pygimp_image_new(gimp_drawable_get_image(self->ID));
}

static PyObject *
drw_get_is_rgb(PyGimpDrawable *self, void *closure)
{
    return PyInt_FromLong(gimp_drawable_is_rgb(self->ID));
}

static PyObject *
drw_get_is_gray(PyGimpDrawable *self, void *closure)
{
    return PyInt_FromLong(gimp_drawable_is_gray(self->ID));
}

static PyObject *
drw_get_is_indexed(PyGimpDrawable *self, void *closure)
{
    return PyInt_FromLong(gimp_drawable_is_indexed(self->ID));
}

static PyObject *
drw_get_is_layer_mask(PyGimpDrawable *self, void *closure)
{
    return PyInt_FromLong(gimp_drawable_is_layer_mask(self->ID));
}

static PyObject *
drw_get_mask_bounds(PyGimpDrawable *self, void *closure)
{
    gint x1, y1, x2, y2;

    gimp_drawable_mask_bounds(self->ID, &x1, &y1, &x2, &y2);
    return Py_BuildValue("(iiii)", x1, y1, x2, y2);
}

static PyObject *
drw_get_offsets(PyGimpDrawable *self, void *closure)
{
    gint x, y;

    gimp_drawable_offsets(self->ID, &x, &y);
    return Py_BuildValue("(ii)", x, y);
}

static PyObject *
drw_get_type(PyGimpDrawable *self, void *closure)
{
    return PyInt_FromLong(gimp_drawable_type(self->ID));
}

static PyObject *
drw_get_type_with_alpha(PyGimpDrawable *self, void *closure)
{
    return PyInt_FromLong(gimp_drawable_type_with_alpha(self->ID));
}

static PyObject *
drw_get_width(PyGimpDrawable *self, void *closure)
{
    return PyInt_FromLong(gimp_drawable_width(self->ID));
}

static PyObject *
drw_get_linked(PyGimpDrawable *self, void *closure)
{
    return PyInt_FromLong(gimp_drawable_get_linked(self->ID));
}

static int
drw_set_linked(PyGimpDrawable *self, PyObject *value, void *closure)
{
    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError, "cannot delete linked");
        return -1;
    }
    if (!PyInt_Check(value)) {
	PyErr_SetString(PyExc_TypeError, "type mismatch");
	return -1;
    }
    gimp_drawable_set_linked(self->ID, PyInt_AsLong(value));
    return 0;
}

static PyObject *
drw_get_tattoo(PyGimpDrawable *self, void *closure)
{
    return PyInt_FromLong(gimp_drawable_get_tattoo(self->ID));
}

static int
drw_set_tattoo(PyGimpDrawable *self, PyObject *value, void *closure)
{
    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError, "cannot delete tattoo");
        return -1;
    }
    if (!PyInt_Check(value)) {
        PyErr_SetString(PyExc_TypeError, "type mismatch");
        return -1;
    }
    gimp_drawable_set_tattoo(self->ID, PyInt_AsLong(value));
    return 0;
}

static PyObject *
drw_get_visible(PyGimpDrawable *self, void *closure)
{
    return PyInt_FromLong(gimp_drawable_get_visible(self->ID));
}
                                                                                
static int
drw_set_visible(PyGimpDrawable *self, PyObject *value, void *closure)
{
    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError, "cannot delete visible");
        return -1;
    }
    if (!PyInt_Check(value)) {
        PyErr_SetString(PyExc_TypeError, "type mismatch");
        return -1;
    }
    gimp_drawable_set_visible(self->ID, PyInt_AsLong(value));
    return 0;
}

static  PyGetSetDef drw_getsets[] = {
    { "ID", (getter)drw_get_ID, (setter)0 },
    { "name", (getter)drw_get_name, (setter)drw_set_name },
    { "bpp", (getter)drw_get_bpp, (setter)0 },
    { "has_alpha", (getter)drw_get_has_alpha, (setter)0 },
    { "height", (getter)drw_get_height, (setter)0 },
    { "image", (getter)drw_get_image, (setter)0 },
    { "is_rgb", (getter)drw_get_is_rgb, (setter)0 },
    { "is_gray", (getter)drw_get_is_gray, (setter)0 },
    { "is_grey", (getter)drw_get_is_gray, (setter)0 },
    { "is_indexed", (getter)drw_get_is_indexed, (setter)0 },
    { "is_layer_mask", (getter)drw_get_is_layer_mask, (setter)0 },
    { "mask_bounds", (getter)drw_get_mask_bounds, (setter)0 },
    { "offsets", (getter)drw_get_offsets, (setter)0 },
    { "type", (getter)drw_get_type, (setter)0 },
    { "type_with_alpha", (getter)drw_get_type_with_alpha, (setter)0 },
    { "width", (getter)drw_get_width, (setter)0 },
    { "linked", (getter)drw_get_linked, (setter)drw_set_linked },
    { "tattoo", (getter)drw_get_tattoo, (setter)drw_set_tattoo },
    { "visible", (getter)drw_get_visible, (setter)drw_set_visible },
    { NULL, (getter)0, (setter)0 }
};

static void
drw_dealloc(PyGimpDrawable *self)
{
    if (self->drawable)
	gimp_drawable_detach(self->drawable);
    PyObject_DEL(self);
}

static PyObject *
drw_repr(PyGimpDrawable *self)
{
    PyObject *s;
    gchar *name;

    name = gimp_drawable_get_name(self->ID);
    s = PyString_FromFormat("<gimp.Drawable '%s'>", name?name:"(null)");
    g_free(name);
    return s;
}

static int
drw_cmp(PyGimpDrawable *self, PyGimpDrawable *other)
{
    if (self->ID == other->ID) return 0;
    if (self->ID > other->ID) return -1;
    return 1;
}

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
    drw_getsets,			/* tp_getset */
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
lay_add_alpha(PyGimpLayer *self)
{
    gimp_layer_add_alpha(self->ID);
    Py_INCREF(Py_None);
    return Py_None;
}


static PyObject *
lay_add_mask(PyGimpLayer *self, PyObject *args)
{
    PyGimpChannel *mask;

    if (!PyArg_ParseTuple(args, "O!:add_mask", &PyGimpChannel_Type, &mask))
	return NULL;
    return PyInt_FromLong(gimp_layer_add_mask(self->ID, mask->ID));
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
lay_remove_mask(PyGimpLayer *self, PyObject *args)
{
    int mode;

    if (!PyArg_ParseTuple(args, "i:remove_mask", &mode))
	return NULL;
    return PyInt_FromLong(gimp_layer_remove_mask(self->ID, mode));
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

static PyMethodDef lay_methods[] = {
    {"copy",	(PyCFunction)lay_copy,	METH_VARARGS},
    {"add_alpha",	(PyCFunction)lay_add_alpha,	METH_NOARGS},
    {"add_mask",        (PyCFunction)lay_add_mask,      METH_VARARGS},
    {"create_mask",	(PyCFunction)lay_create_mask,	METH_VARARGS},
    {"remove_mask",     (PyCFunction)lay_remove_mask,   METH_VARARGS},
    {"resize",	(PyCFunction)lay_resize,	METH_VARARGS},
    {"scale",	(PyCFunction)lay_scale,	METH_VARARGS},
    {"translate",	(PyCFunction)lay_translate,	METH_VARARGS},
    {"set_offsets",	(PyCFunction)lay_set_offsets,	METH_VARARGS},
    {NULL,		NULL}		/* sentinel */
};

static PyObject *
lay_get_is_floating_sel(PyGimpLayer *self, void *closure)
{
    return PyInt_FromLong(gimp_layer_is_floating_sel(self->ID));
}

static PyObject *
lay_get_mask(PyGimpLayer *self, void *closure)
{
    gint32 id = gimp_layer_get_mask(self->ID);
    if (id == -1) {
	Py_INCREF(Py_None);
	return Py_None;
    }
    return pygimp_channel_new(id);
}

static PyObject *
lay_get_apply_mask(PyGimpLayer *self, void *closure)
{
    return PyInt_FromLong(gimp_layer_get_apply_mask(self->ID));
}

static int
lay_set_apply_mask(PyGimpLayer *self, PyObject *value, void *closure)
{
    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError, "cannot delete apply_mask");
        return -1;
    }
    if (!PyInt_Check(value)) {
	PyErr_SetString(PyExc_TypeError, "type mismatch");
	return -1;
    }
    gimp_layer_set_apply_mask(self->ID, PyInt_AsLong(value));
    return 0;
}

static PyObject *
lay_get_edit_mask(PyGimpLayer *self, void *closure)
{
    return PyInt_FromLong(gimp_layer_get_edit_mask(self->ID));}

static int
lay_set_edit_mask(PyGimpLayer *self, PyObject *value, void *closure)
{
    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError, "cannot delete edit_mask");
        return -1;
    }
    if (!PyInt_Check(value)) {
	PyErr_SetString(PyExc_TypeError, "type mismatch");
	return -1;
    }
    gimp_layer_set_edit_mask(self->ID, PyInt_AsLong(value));
    return 0;
}

static PyObject *
lay_get_mode(PyGimpLayer *self, void *closure)
{
    return PyInt_FromLong(gimp_layer_get_mode(self->ID));
}

static int
lay_set_mode(PyGimpLayer *self, PyObject *value, void *closure)
{
    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError, "cannot delete mode");
        return -1;
    }
    if (!PyInt_Check(value)) {
	PyErr_SetString(PyExc_TypeError, "type mismatch");
	return -1;
    }
    gimp_layer_set_mode(self->ID, (GimpLayerModeEffects)PyInt_AsLong(value));
    return 0;
}

static PyObject *
lay_get_opacity(PyGimpLayer *self, void *closure)
{
    return PyFloat_FromDouble(gimp_layer_get_opacity(self->ID));
}

static int
lay_set_opacity(PyGimpLayer *self, PyObject *value, void *closure)
{
    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError, "cannot delete opacity");
        return -1;
    }
    if (!PyFloat_Check(value)) {
	PyErr_SetString(PyExc_TypeError, "type mismatch");
	return -1;
    }
    gimp_layer_set_opacity(self->ID, PyFloat_AsDouble(value));
    return 0;
}

static PyObject *
lay_get_preserve_trans(PyGimpLayer *self, void *closure)
{
    return PyInt_FromLong(gimp_layer_get_preserve_trans(self->ID));
}

static int
lay_set_preserve_trans(PyGimpLayer *self, PyObject *value, void *closure)
{
    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError,
			"cannot delete preserve_transparency");
        return -1;
    }
    if (!PyInt_Check(value)) {
	PyErr_SetString(PyExc_TypeError, "type mismatch");
	return -1;
    }
    gimp_layer_set_preserve_trans(self->ID, PyInt_AsLong(value));
    return 0;
}

static PyObject *
lay_get_show_mask(PyGimpLayer *self, void *closure)
{
    return PyInt_FromLong(gimp_layer_get_show_mask(self->ID));
}

static int
lay_set_show_mask(PyGimpLayer *self, PyObject *value, void *closure)
{
    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError, "cannot delete show_mask");
        return -1;
    }
    if (!PyInt_Check(value)) {
	PyErr_SetString(PyExc_TypeError, "type mismatch");
	return -1;
    }
    gimp_layer_set_show_mask(self->ID, PyInt_AsLong(value));
    return 0;
}

static PyGetSetDef lay_getsets[] = {
    { "is_floating_sel", (getter)lay_get_is_floating_sel, (setter)0 },
    { "mask", (getter)lay_get_mask, (setter)0 },
    { "apply_mask", (getter)lay_get_apply_mask, (setter)lay_set_apply_mask },
    { "edit_mask", (getter)lay_get_edit_mask, (setter)lay_set_edit_mask },
    { "mode", (getter)lay_get_mode, (setter)lay_set_mode },
    { "opacity", (getter)lay_get_opacity, (setter)lay_set_opacity },
    { "preserve_trans", (getter)lay_get_preserve_trans,
      (setter)lay_set_preserve_trans },
    { "show_mask", (getter)lay_get_show_mask, (setter)lay_set_show_mask },
    { NULL, (getter)0, (setter)0 }
};

static PyObject *
lay_repr(PyGimpLayer *self)
{
    PyObject *s;
    gchar *name;

    name = gimp_drawable_get_name(self->ID);
    s = PyString_FromFormat("<gimp.Layer '%s'>", name);
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
    (getattrfunc)0,                     /* tp_getattr */
    (setattrfunc)0,                     /* tp_setattr */
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
    lay_getsets,			/* tp_getset */
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
chn_copy(PyGimpChannel *self)
{
    gint32 id;
	
    id = gimp_channel_copy(self->ID);
    if (id == -1) {
	PyErr_SetString(pygimp_error, "can't copy channel");
	return NULL;
    }
    return pygimp_channel_new(id);
}

static PyObject *
chn_combine_masks(PyGimpChannel *self, PyObject *args)
{
    PyGimpChannel *channel2;
    GimpChannelOps operation;
    gint offx, offy;

    if (!PyArg_ParseTuple(args, "O!iii:combine_masks", &PyGimpChannel_Type,
			  &channel2, &operation, &offx, &offy))
	return NULL;
    return PyInt_FromLong(gimp_channel_combine_masks(self->ID, channel2->ID,
						     operation, offx, offy));
}

static PyMethodDef chn_methods[] = {
    {"copy",	(PyCFunction)chn_copy,	METH_NOARGS},
    {"combine_masks",	(PyCFunction)chn_combine_masks,	METH_VARARGS},
    {NULL,		NULL}		/* sentinel */
};

static PyObject *
chn_get_color(PyGimpChannel *self, void *closure)
{
    GimpRGB colour;
    guchar r, g, b;

    gimp_channel_get_color(self->ID, &colour);
    gimp_rgb_get_uchar(&colour, &r, &g, &b);
    return Py_BuildValue("(iii)", (long)r, (long)g, (long)b);
}

static int
chn_set_color(PyGimpChannel *self, PyObject *value, void *closure)
{
    guchar r, g, b;
    GimpRGB colour;

    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError, "cannot delete colour");
        return -1;
    }
    if (!PyTuple_Check(value) || !PyArg_ParseTuple(value, "(BBB)", &r,&g,&b)) {
	PyErr_Clear();
	PyErr_SetString(PyExc_TypeError, "type mismatch");
	return -1;
    }
    gimp_rgb_set_uchar(&colour, r, g, b);
    gimp_channel_set_color(self->ID, &colour);
    return 0;
}

static PyObject *
chn_get_opacity(PyGimpLayer *self, void *closure)
{
    return PyFloat_FromDouble(gimp_channel_get_opacity(self->ID));
}

static int
chn_set_opacity(PyGimpLayer *self, PyObject *value, void *closure)
{
    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError, "cannot delete opacity");
        return -1;
    }
    if (!PyFloat_Check(value)) {
	PyErr_SetString(PyExc_TypeError, "type mismatch");
	return -1;
    }
    gimp_channel_set_opacity(self->ID, PyFloat_AsDouble(value));
    return 0;
}

static PyObject *
chn_get_show_masked(PyGimpLayer *self, void *closure)
{
    return PyInt_FromLong(gimp_channel_get_show_masked(self->ID));
}

static int
chn_set_show_masked(PyGimpLayer *self, PyObject *value, void *closure)
{
    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError, "cannot delete show_masked");
        return -1;
    }
    if (!PyInt_Check(value)) {
	PyErr_SetString(PyExc_TypeError, "type mismatch");
	return -1;
    }
    gimp_channel_set_show_masked(self->ID, PyInt_AsLong(value));
    return 0;
}

static PyGetSetDef chn_getsets[] = {
    { "color", (getter)chn_get_color, (setter)chn_set_color },
    { "colour", (getter)chn_get_color, (setter)chn_set_color },
    { "opacity", (getter)chn_get_opacity, (setter)chn_set_opacity },
    { "show_masked", (getter)chn_get_show_masked, (setter)chn_set_show_masked},
    { NULL, (getter)0, (setter)0 }
};

static PyObject *
chn_repr(PyGimpChannel *self)
{
    PyObject *s;
    gchar *name;

    name = gimp_drawable_get_name(self->ID);
    s = PyString_FromFormat("<gimp.Channel '%s'>", name);
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
    (getattrfunc)0,                     /* tp_getattr */
    (setattrfunc)0,                     /* tp_setattr */
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
    chn_getsets,			/* tp_getset */
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
