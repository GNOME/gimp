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

static PyObject *
img_add_channel(PyGimpImage *self, PyObject *args)
{
    PyGimpChannel *chn;
    int pos;
	
    if (!PyArg_ParseTuple(args, "O!i:add_channel", &PyGimpChannel_Type, &chn, &pos))
	return NULL;
    gimp_image_add_channel(self->ID, chn->ID, pos);
    Py_INCREF(Py_None);
    return Py_None;
}


static PyObject *
img_add_layer(PyGimpImage *self, PyObject *args)
{
    PyGimpLayer *lay;
    int pos;
	
    if (!PyArg_ParseTuple(args, "O!i:add_layer", &PyGimpLayer_Type, &lay, &pos))
	return NULL;
    gimp_image_add_layer(self->ID, lay->ID, pos);
    Py_INCREF(Py_None);
    return Py_None;
}


static PyObject *
img_add_layer_mask(PyGimpImage *self, PyObject *args)
{
    PyGimpLayer *lay;
    PyGimpChannel *mask;
    if (!PyArg_ParseTuple(args, "O!O!:add_layer_mask", &PyGimpLayer_Type, &lay,
			  &PyGimpChannel_Type, &mask))
	return NULL;
    gimp_image_add_layer_mask(self->ID, lay->ID, mask->ID);
    Py_INCREF(Py_None);
    return Py_None;
}


static PyObject *
img_clean_all(PyGimpImage *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ":clean_all"))
	return NULL;
    gimp_image_clean_all(self->ID);
    Py_INCREF(Py_None);
    return Py_None;
}


static PyObject *
img_disable_undo(PyGimpImage *self, PyObject *args)
{
    /*GimpParam *return_vals;
    int nreturn_vals;*/

    if (!PyArg_ParseTuple(args, ":disable_undo"))
	return NULL;
    /*return_vals = gimp_run_procedure("gimp_undo_push_group_start",
				     &nreturn_vals, GIMP_PDB_IMAGE, self->ID,
				     GIMP_PDB_END);
    gimp_destroy_params(return_vals, nreturn_vals);*/
    gimp_image_undo_disable(self->ID);
    Py_INCREF(Py_None);
    return Py_None;
}


static PyObject *
img_enable_undo(PyGimpImage *self, PyObject *args)
{
    /*GimpParam *return_vals;
    int nreturn_vals;*/

    if (!PyArg_ParseTuple(args, ":enable_undo"))
	return NULL;
    /*return_vals = gimp_run_procedure("gimp_undo_push_group_start",
				     &nreturn_vals, GIMP_PDB_IMAGE, self->ID,
				     GIMP_PDB_END);
    gimp_destroy_params(return_vals, nreturn_vals);*/
    gimp_image_undo_enable(self->ID);
    Py_INCREF(Py_None);
    return Py_None;
}


static PyObject *
img_flatten(PyGimpImage *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ":flatten"))
	return NULL;
    gimp_image_flatten(self->ID);
    Py_INCREF(Py_None);
    return Py_None;
}


static PyObject *
img_lower_channel(PyGimpImage *self, PyObject *args)
{
    PyGimpChannel *chn;
    if (!PyArg_ParseTuple(args, "O!:lower_channel", &PyGimpChannel_Type, &chn))
	return NULL;
    gimp_image_lower_channel(self->ID, chn->ID);
    Py_INCREF(Py_None);
    return Py_None;
}


static PyObject *
img_lower_layer(PyGimpImage *self, PyObject *args)
{
    PyGimpLayer *lay;
    if (!PyArg_ParseTuple(args, "O!:lower_layer", &PyGimpLayer_Type, &lay))
	return NULL;
    gimp_image_lower_layer(self->ID, lay->ID);
    Py_INCREF(Py_None);
    return Py_None;
}


static PyObject *
img_merge_visible_layers(PyGimpImage *self, PyObject *args)
{
    gint32 id;
    int merge;
    if (!PyArg_ParseTuple(args, "i:merge_visible_layers", &merge))
	return NULL;
    id = gimp_image_merge_visible_layers(self->ID, merge);
    if (id == -1) {
	PyErr_SetString(pygimp_error, "Can't merge layers.");
	return NULL;
    }
    return pygimp_layer_new(id);
}


static PyObject *
img_pick_correlate_layer(PyGimpImage *self, PyObject *args)
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
    return pygimp_layer_new(id);
}


static PyObject *
img_raise_channel(PyGimpImage *self, PyObject *args)
{
    PyGimpChannel *chn;
	
    if (!PyArg_ParseTuple(args, "O!:raise_channel", &PyGimpChannel_Type, &chn))
	return NULL;
    gimp_image_raise_channel(self->ID, chn->ID);
    Py_INCREF(Py_None);
    return Py_None;
}


static PyObject *
img_raise_layer(PyGimpImage *self, PyObject *args)
{
    PyGimpLayer *lay;
    if (!PyArg_ParseTuple(args, "O!:raise_layer", &PyGimpLayer_Type, &lay))
	return NULL;
    gimp_image_raise_layer(self->ID, lay->ID);
    Py_INCREF(Py_None);
    return Py_None;
}


static PyObject *
img_remove_channel(PyGimpImage *self, PyObject *args)
{
    PyGimpChannel *chn;
    if (!PyArg_ParseTuple(args, "O!:remove_channel", &PyGimpChannel_Type, &chn))
	return NULL;
    gimp_image_remove_channel(self->ID, chn->ID);
    Py_INCREF(Py_None);
    return Py_None;
}


static PyObject *
img_remove_layer(PyGimpImage *self, PyObject *args)
{
    PyGimpLayer *lay;
    if (!PyArg_ParseTuple(args, "O!:remove_layer", &PyGimpLayer_Type, &lay))
	return NULL;
    gimp_image_remove_layer(self->ID, lay->ID);
    Py_INCREF(Py_None);
    return Py_None;
}


static PyObject *
img_remove_layer_mask(PyGimpImage *self, PyObject *args)
{
    PyGimpLayer *lay;
    int mode;
    if (!PyArg_ParseTuple(args, "O!i:remove_layer_mask", &PyGimpLayer_Type, &lay,
			  &mode))
	return NULL;
    gimp_image_remove_layer_mask(self->ID, lay->ID, mode);
    Py_INCREF(Py_None);
    return Py_None;
}


static PyObject *
img_resize(PyGimpImage *self, PyObject *args)
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
img_get_component_active(PyGimpImage *self, PyObject *args)
{
    int comp;
    if (!PyArg_ParseTuple(args, "i:get_component_active", &comp))
	return NULL;
    return PyInt_FromLong((long)
			  gimp_image_get_component_active(self->ID, comp));
}


static PyObject *
img_get_component_visible(PyGimpImage *self, PyObject *args)
{
    int comp;
	
    if (!PyArg_ParseTuple(args, "i:get_component_visible", &comp))
	return NULL;
	
    return PyInt_FromLong((long)
			  gimp_image_get_component_visible(self->ID, comp));
}


static PyObject *
img_set_component_active(PyGimpImage *self, PyObject *args)
{
    int comp, a;
    if (!PyArg_ParseTuple(args, "ii:set_component_active", &comp, &a))
	return NULL;
    gimp_image_set_component_active(self->ID, comp, a);
    Py_INCREF(Py_None);
    return Py_None;
}


static PyObject *
img_set_component_visible(PyGimpImage *self, PyObject *args)
{
    int comp, v;
    if (!PyArg_ParseTuple(args, "ii:set_component_visible", &comp, &v))
	return NULL;
    gimp_image_set_component_visible(self->ID, comp, v);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
img_parasite_find(PyGimpImage *self, PyObject *args)
{
    char *name;
    if (!PyArg_ParseTuple(args, "s:parasite_find", &name))
	return NULL;
    return pygimp_parasite_new(gimp_image_parasite_find(self->ID, name));
}

static PyObject *
img_parasite_attach(PyGimpImage *self, PyObject *args)
{
    PyGimpParasite *parasite;
    if (!PyArg_ParseTuple(args, "O!:parasite_attach", &PyGimpParasite_Type,
			  &parasite))
	return NULL;
    gimp_image_parasite_attach(self->ID, parasite->para);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
img_attach_new_parasite(PyGimpImage *self, PyObject *args)
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
img_parasite_detach(PyGimpImage *self, PyObject *args)
{
    char *name;
    if (!PyArg_ParseTuple(args, "s:parasite_detach", &name))
	return NULL;
    gimp_image_parasite_detach(self->ID, name);
    Py_INCREF(Py_None);
    return Py_None;
}

/* gimp_image_set_resolution
 * gimp_image_get_resolution
 * gimp_set_unit
 * gimp_get_unit
 */
static PyObject *
img_get_layer_by_tattoo(PyGimpImage *self, PyObject *args)
{
    int tattoo;
    if (!PyArg_ParseTuple(args, "i:get_layer_by_tattoo", &tattoo))
	return NULL;
    return pygimp_layer_new(gimp_image_get_layer_by_tattoo(self->ID, tattoo));
}

static PyObject *
img_get_channel_by_tattoo(PyGimpImage *self, PyObject *args)
{
    int tattoo;
    if (!PyArg_ParseTuple(args, "i:get_channel_by_tattoo", &tattoo))
	return NULL;
    return pygimp_channel_new(gimp_image_get_channel_by_tattoo(self->ID,
							       tattoo));
}

static PyObject *
img_add_hguide(PyGimpImage *self, PyObject *args)
{
    int ypos;
    if (!PyArg_ParseTuple(args, "i:add_hguide", &ypos))
	return NULL;
    gimp_image_add_hguide(self->ID, ypos);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
img_add_vguide(PyGimpImage *self, PyObject *args)
{
    int xpos;
    if (!PyArg_ParseTuple(args, "i:add_vguide", &xpos))
	return NULL;
    gimp_image_add_vguide(self->ID, xpos);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
img_delete_guide(PyGimpImage *self, PyObject *args)
{
    int guide;
    if (!PyArg_ParseTuple(args, "i:delete_guide", &guide))
	return NULL;
    gimp_image_delete_guide(self->ID, guide);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
img_find_next_guide(PyGimpImage *self, PyObject *args)
{
    int guide;
    if (!PyArg_ParseTuple(args, "i:find_next_guide", &guide))
	return NULL;
    return PyInt_FromLong(gimp_image_find_next_guide(self->ID, guide));
}

static PyObject *
img_get_guide_orientation(PyGimpImage *self, PyObject *args)
{
    int guide;
    if (!PyArg_ParseTuple(args, "i:get_guide_orientation", &guide))
	return NULL;
    return PyInt_FromLong(gimp_image_get_guide_orientation(self->ID, guide));
}

static PyObject *
img_get_guide_position(PyGimpImage *self, PyObject *args)
{
    int guide;
    if (!PyArg_ParseTuple(args, "i:get_guide_position", &guide))
	return NULL;
    return PyInt_FromLong(gimp_image_get_guide_position(self->ID, guide));
}

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
    {"parasite_find",       (PyCFunction)img_parasite_find,      METH_VARARGS},
    {"parasite_attach",     (PyCFunction)img_parasite_attach,    METH_VARARGS},
    {"attach_new_parasite", (PyCFunction)img_attach_new_parasite,METH_VARARGS},
    {"parasite_detach",     (PyCFunction)img_parasite_detach,    METH_VARARGS},
    {"get_layer_by_tattoo",(PyCFunction)img_get_layer_by_tattoo,METH_VARARGS},
    {"get_channel_by_tattoo",(PyCFunction)img_get_channel_by_tattoo,METH_VARARGS},
    {"add_hguide", (PyCFunction)img_add_hguide, METH_VARARGS},
    {"add_vguide", (PyCFunction)img_add_vguide, METH_VARARGS},
    {"delete_guide", (PyCFunction)img_delete_guide, METH_VARARGS},
    {"find_next_guide", (PyCFunction)img_find_next_guide, METH_VARARGS},
    {"get_guide_orientation",(PyCFunction)img_get_guide_orientation,METH_VARARGS},
    {"get_guide_position", (PyCFunction)img_get_guide_position, METH_VARARGS},
    {NULL,		NULL}		/* sentinel */
};

/* ---------- */


PyObject *
pygimp_image_new(gint32 ID)
{
    PyGimpImage *self;

    if (ID == -1) {
	Py_INCREF(Py_None);
	return Py_None;
    }
    self = PyObject_NEW(PyGimpImage, &PyGimpImage_Type);
    if (self == NULL)
	return NULL;
    self->ID = ID;
    return (PyObject *)self;
}


static void
img_dealloc(PyGimpImage *self)
{
    /* XXXX Add your own cleanup code here */
    PyObject_DEL(self);
}

static PyObject *
img_getattr(PyGimpImage *self, char *attr)
{
    gint32 *a, id;
    guchar *c;
    int n, i;
    PyObject *ret;
    if (!strcmp(attr, "__members__"))
	return Py_BuildValue(
			     "[ssssssssssss]",
			     "ID", "active_channel",
			     "active_layer", "base_type", "channels", "cmap",
			     "filename", "floating_selection",
			     "height", "layers",
			     "resolution",
			     "selection",
			     "unit",
			     "width");
    if (!strcmp(attr, "ID"))
	return PyInt_FromLong(self->ID);
    if (!strcmp(attr, "active_channel")) {
	id = gimp_image_get_active_channel(self->ID);
	if (id == -1) {
	    Py_INCREF(Py_None);
	    return Py_None;
	}
	return pygimp_channel_new(id);
    }
    if (!strcmp(attr, "active_layer")) {
	id = gimp_image_get_active_layer(self->ID);
	if (id == -1) {
	    Py_INCREF(Py_None);
	    return Py_None;
	}
	return pygimp_layer_new(id);
    }
    if (!strcmp(attr, "base_type"))
	return PyInt_FromLong(gimp_image_base_type(self->ID));
    if (!strcmp(attr, "channels")) {
	a = gimp_image_get_channels(self->ID, &n);
	ret = PyList_New(n);
	for (i = 0; i < n; i++)
	    PyList_SetItem(ret, i, pygimp_channel_new(a[i]));
	return ret;
    }
    if (!strcmp(attr, "cmap")) {
	c = gimp_image_get_cmap(self->ID, &n);
	return PyString_FromStringAndSize(c, n * 3);
    }
    if (!strcmp(attr, "filename"))
	return PyString_FromString(gimp_image_get_filename(self->ID));
    if (!strcmp(attr, "floating_selection")) {
	id = gimp_image_floating_selection(self->ID);
	if (id == -1) {
	    Py_INCREF(Py_None);
	    return Py_None;
	}
	return pygimp_layer_new(id);
    }
    if (!strcmp(attr, "layers")) {
	a = gimp_image_get_layers(self->ID, &n);
	ret = PyList_New(n);
	for (i = 0; i < n; i++)
	    PyList_SetItem(ret, i, pygimp_layer_new(a[i]));
	return ret;
    }
    if (!strcmp(attr, "selection"))
	return pygimp_channel_new(gimp_image_get_selection(self->ID));
    if (!strcmp(attr, "height"))
	return PyInt_FromLong(gimp_image_height(self->ID));
    if (!strcmp(attr, "width"))
	return PyInt_FromLong(gimp_image_width(self->ID));

    if (!strcmp(attr, "resolution")) {
	double xres, yres;
	gimp_image_get_resolution(self->ID, &xres, &yres);
	return Py_BuildValue("(dd)", xres, yres);
    }
    if (!strcmp(attr, "unit"))
	return PyInt_FromLong(gimp_image_get_unit(self->ID));

    {
	PyObject *name = PyString_FromString(attr);
	PyObject *ret = PyObject_GenericGetAttr((PyObject *)self, name);
	Py_DECREF(name);
	return ret;
    }
}

static int
img_setattr(PyGimpImage *self, char *attr, PyObject *v)
{
    if (v == NULL) {
	PyErr_SetString(PyExc_TypeError, "can not delete attributes.");
	return -1;
    }
    if (!strcmp(attr, "active_channel")) {
	if (!pygimp_channel_check(v)) {
	    PyErr_SetString(PyExc_TypeError, "type mis-match.");
	    return -1;
	}
	gimp_image_set_active_channel(self->ID, ((PyGimpChannel *)v)->ID);
	return 0;
    }
    if (!strcmp(attr, "active_layer")) {
	if (!pygimp_layer_check(v)) {
	    PyErr_SetString(PyExc_TypeError, "type mis-match.");
	    return -1;
	}
	gimp_image_set_active_layer(self->ID, ((PyGimpLayer *)v)->ID);
	return 0;
    }
    if (!strcmp(attr, "cmap")) {
	if (!PyString_Check(v)) {
	    PyErr_SetString(PyExc_TypeError, "type mis-match.");
	    return -1;
	}
	gimp_image_set_cmap(self->ID, PyString_AsString(v),
			    PyString_Size(v) / 3);
	return 0;
    }
    if (!strcmp(attr, "filename")) {
	if (!PyString_Check(v)) {
	    PyErr_SetString(PyExc_TypeError, "type mis-match.");
	    return -1;
	}
	gimp_image_set_filename(self->ID, PyString_AsString(v));
	return 0;
    }
    if (!strcmp(attr, "resolution")) {
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
    if (!strcmp(attr, "unit")) {
	if (!PyInt_Check(v)) {
	    PyErr_SetString(PyExc_TypeError, "type mis-match.");
	    return -1;
	}
	gimp_image_set_unit(self->ID, PyInt_AsLong(v));
    }
    if (!strcmp(attr, "channels") || !strcmp(attr, "layers") ||
	!strcmp(attr, "selection") || !strcmp(attr, "height") ||
	!strcmp(attr, "base_type") || !strcmp(attr, "width") ||
	!strcmp(attr, "floating_selection") || !strcmp(attr, "ID")) {
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
img_repr(PyGimpImage *self)
{
    PyObject *s;
    gchar *name;

    name = gimp_image_get_filename(self->ID);
    s = PyString_FromFormat("<gimp.image %s>", name);
    g_free(name);
    return s;
}

static int
img_cmp(PyGimpImage *self, PyGimpImage *other)
{
    if (self->ID == other->ID) return 0;
    if (self->ID > other->ID) return -1;
    return 1;
}

static int
img_init(PyGimpImage *self, PyObject *args, PyObject *kwargs)
{
    guint width, height;
    GimpImageBaseType type;

    if (!PyArg_ParseTuple(args, "iii:gimp.Image.__init__",
			  &width, &height, &type))
        return -1;
    self->ID = gimp_image_new(width, height, type);
    if (self->ID < 0) {
	PyErr_SetString(pygimp_error, "could not create image");
	return -1;
    }
    return 0;
}

PyTypeObject PyGimpImage_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                                  /* ob_size */
    "gimp.Image",                       /* tp_name */
    sizeof(PyGimpImage),                /* tp_basicsize */
    0,                                  /* tp_itemsize */
    /* methods */
    (destructor)img_dealloc,            /* tp_dealloc */
    (printfunc)0,                       /* tp_print */
    (getattrfunc)img_getattr,           /* tp_getattr */
    (setattrfunc)img_setattr,           /* tp_setattr */
    (cmpfunc)img_cmp,                   /* tp_compare */
    (reprfunc)img_repr,                 /* tp_repr */
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
    img_methods,			/* tp_methods */
    0,					/* tp_members */
    0,					/* tp_getset */
    (PyTypeObject *)0,			/* tp_base */
    (PyObject *)0,			/* tp_dict */
    0,					/* tp_descr_get */
    0,					/* tp_descr_set */
    0,					/* tp_dictoffset */
    (initproc)img_init,                 /* tp_init */
    (allocfunc)0,			/* tp_alloc */
    (newfunc)0,				/* tp_new */
};
