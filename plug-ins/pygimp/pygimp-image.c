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
    int pos = -1;
	
    if (!PyArg_ParseTuple(args, "O!|i:add_channel",
	                        &PyGimpChannel_Type, &chn, &pos))
	return NULL;

    if (!gimp_image_add_channel(self->ID, chn->ID, pos)) {
	PyErr_Format(pygimp_error,
		     "could not add channel (ID %d) to image (ID %d)",
		     chn->ID, self->ID);
	return NULL;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
img_add_layer(PyGimpImage *self, PyObject *args)
{
    PyGimpLayer *lay;
    int pos = -1;
	
    if (!PyArg_ParseTuple(args, "O!|i:add_layer", &PyGimpLayer_Type, &lay,
			  &pos))
	return NULL;

    if (!gimp_image_add_layer(self->ID, lay->ID, pos)) {
	PyErr_Format(pygimp_error,
		     "could not add layer (ID %d) to image (ID %d)",
		     lay->ID, self->ID);
	return NULL;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
img_clean_all(PyGimpImage *self)
{
    if (!gimp_image_clean_all(self->ID)) {
	PyErr_Format(pygimp_error, "could not clean all on image (ID %d)",
		     self->ID);
	return NULL;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
img_disable_undo(PyGimpImage *self)
{
    return PyBool_FromLong(gimp_image_undo_disable(self->ID));
}

static PyObject *
img_enable_undo(PyGimpImage *self)
{
    return PyBool_FromLong(gimp_image_undo_enable(self->ID));
}

static PyObject *
img_flatten(PyGimpImage *self)
{
    return pygimp_layer_new(gimp_image_flatten(self->ID));
}

static PyObject *
img_lower_channel(PyGimpImage *self, PyObject *args)
{
    PyGimpChannel *chn;

    if (!PyArg_ParseTuple(args, "O!:lower_channel", &PyGimpChannel_Type, &chn))
	return NULL;

    if (!gimp_image_lower_channel(self->ID, chn->ID)) {
	PyErr_Format(pygimp_error,
		     "could not lower channel (ID %d) on image (ID %d)",
		     chn->ID, self->ID);
	return NULL;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
img_lower_layer(PyGimpImage *self, PyObject *args)
{
    PyGimpLayer *lay;

    if (!PyArg_ParseTuple(args, "O!:lower_layer", &PyGimpLayer_Type, &lay))
	return NULL;

    if (!gimp_image_lower_layer(self->ID, lay->ID)) {
	PyErr_Format(pygimp_error,
		     "could not lower layer (ID %d) on image (ID %d)",
		     lay->ID, self->ID);
	return NULL;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
img_lower_layer_to_bottom(PyGimpImage *self, PyObject *args)
{
    PyGimpLayer *lay;

    if (!PyArg_ParseTuple(args, "O!:lower_layer_to_bottom",
			  &PyGimpLayer_Type, &lay))
	return NULL;

    if (!gimp_image_lower_layer_to_bottom(self->ID, lay->ID)) {
	PyErr_Format(pygimp_error,
		     "could not lower layer (ID %d) to bottom on image (ID %d)",
		     lay->ID, self->ID);
	return NULL;
    }

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
	PyErr_Format(pygimp_error,
		     "could not merge visible layers on image (ID %d) "
		     "with merge type %d",
		     self->ID, merge);
	return NULL;
    }

    return pygimp_layer_new(id);
}

static PyObject *
img_merge_down(PyGimpImage *self, PyObject *args)
{
    gint32 id;
    PyGimpLayer *layer;
    int merge;

    if (!PyArg_ParseTuple(args, "O!i:merge_down",
			  &PyGimpLayer_Type, &layer, &merge))
	return NULL;

    id = gimp_image_merge_down(self->ID, layer->ID, merge);

    if (id == -1) {
	PyErr_Format(pygimp_error,
		     "could not merge down layer (ID %d) on image (ID %d) "
		     "with merge type %d",
		     layer->ID, self->ID, merge);
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

    if (!gimp_image_raise_channel(self->ID, chn->ID)) {
	PyErr_Format(pygimp_error,
		     "could not raise channel (ID %d) on image (ID %d)",
		     chn->ID, self->ID);
	return NULL;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
img_raise_layer(PyGimpImage *self, PyObject *args)
{
    PyGimpLayer *lay;

    if (!PyArg_ParseTuple(args, "O!:raise_layer", &PyGimpLayer_Type, &lay))
	return NULL;

    if (!gimp_image_raise_layer(self->ID, lay->ID)) {
	PyErr_Format(pygimp_error,
		     "could not raise layer (ID %d) on image (ID %d)",
		     lay->ID, self->ID);
	return NULL;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
img_raise_layer_to_top(PyGimpImage *self, PyObject *args)
{
    PyGimpLayer *lay;

    if (!PyArg_ParseTuple(args, "O!:raise_layer_to_top",
			  &PyGimpLayer_Type, &lay))
	return NULL;

    if (!gimp_image_raise_layer_to_top(self->ID, lay->ID)) {
	PyErr_Format(pygimp_error,
		     "could not raise layer (ID %d) to top on image (ID %d)",
		     lay->ID, self->ID);
	return NULL;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
img_remove_channel(PyGimpImage *self, PyObject *args)
{
    PyGimpChannel *chn;

    if (!PyArg_ParseTuple(args, "O!:remove_channel", &PyGimpChannel_Type, &chn))
	return NULL;

    if (!gimp_image_remove_channel(self->ID, chn->ID)) {
	PyErr_Format(pygimp_error,
		     "could not remove channel (ID %d) from image (ID %d)",
		     chn->ID, self->ID);
	return NULL;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
img_remove_layer(PyGimpImage *self, PyObject *args)
{
    PyGimpLayer *lay;

    if (!PyArg_ParseTuple(args, "O!:remove_layer", &PyGimpLayer_Type, &lay))
	return NULL;

    if (!gimp_image_remove_layer(self->ID, lay->ID)) {
	PyErr_Format(pygimp_error,
		     "could not remove layer (ID %d) from image (ID %d)",
		     lay->ID, self->ID);
	return NULL;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
img_resize(PyGimpImage *self, PyObject *args, PyObject *kwargs)
{
    int new_w, new_h;
    int offs_x = 0, offs_y = 0;

    static char *kwlist[] = { "width", "height", "offset_x", "offset_y",
			      NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "ii|ii:resize", kwlist,
				     &new_w, &new_h, &offs_x, &offs_y))
	return NULL;

    if (!gimp_image_resize(self->ID, new_w, new_h, offs_x, offs_y)) {
	PyErr_Format(pygimp_error,
		     "could not resize image (ID %d) to %dx%d, offset %d, %d",
		     self->ID, new_w, new_h, offs_x, offs_y);
	return NULL;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
img_resize_to_layers(PyGimpImage *self)
{
    if (!gimp_image_resize_to_layers(self->ID)) {
	PyErr_Format(pygimp_error, "could not resize to layers on image "
	                           "(ID %d)",
		     self->ID);
	return NULL;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
img_scale(PyGimpImage *self, PyObject *args, PyObject *kwargs)
{
    int new_width, new_height;

    static char *kwlist[] = { "width", "height", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "ii:scale", kwlist,
				     &new_width, &new_height))
	return NULL;

    if (!gimp_image_scale(self->ID, new_width, new_height)) {
	PyErr_Format(pygimp_error, "could not scale image (ID %d) to %dx%d",
		     self->ID, new_width, new_height);
	return NULL;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
img_crop(PyGimpImage *self, PyObject *args, PyObject *kwargs)
{
    int new_w, new_h;
    int offs_x = 0, offs_y = 0;

    static char *kwlist[] = { "width", "height", "offset_x", "offset_y",
			      NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "ii|ii:crop", kwlist,
				     &new_w, &new_h, &offs_x, &offs_y))
	return NULL;

    if (!gimp_image_crop(self->ID, new_w, new_h, offs_x, offs_y)) {
	PyErr_Format(pygimp_error,
		     "could not crop image (ID %d) to %dx%d, offset %d, %d",
		     self->ID, new_w, new_h, offs_x, offs_y);
	return NULL;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
img_free_shadow(PyGimpImage *self)
{
    if (!gimp_image_free_shadow(self->ID)) {
	PyErr_Format(pygimp_error, "could not free shadow on image (ID %d)",
		     self->ID);
	return NULL;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
img_unset_active_channel(PyGimpImage *self)
{
    if (!gimp_image_unset_active_channel(self->ID)) {
	PyErr_Format(pygimp_error,
		     "could not unset active channel on image (ID %d)",
		     self->ID);
	return NULL;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
img_get_component_active(PyGimpImage *self, PyObject *args)
{
    int comp;

    if (!PyArg_ParseTuple(args, "i:get_component_active", &comp))
	return NULL;

    return PyBool_FromLong(gimp_image_get_component_active(self->ID, comp));
}


static PyObject *
img_get_component_visible(PyGimpImage *self, PyObject *args)
{
    int comp;
	
    if (!PyArg_ParseTuple(args, "i:get_component_visible", &comp))
	return NULL;
	
    return PyBool_FromLong(gimp_image_get_component_visible(self->ID, comp));
}


static PyObject *
img_set_component_active(PyGimpImage *self, PyObject *args)
{
    int comp, a;

    if (!PyArg_ParseTuple(args, "ii:set_component_active", &comp, &a))
	return NULL;

    if (!gimp_image_set_component_active(self->ID, comp, a)) {
	PyErr_Format(pygimp_error,
		     "could not set component (%d) %sactive on image (ID %d)",
		     comp, a ? "" : "in", self->ID);
	return NULL;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
img_set_component_visible(PyGimpImage *self, PyObject *args)
{
    int comp, v;

    if (!PyArg_ParseTuple(args, "ii:set_component_visible", &comp, &v))
	return NULL;

    if (!gimp_image_set_component_visible(self->ID, comp, v)) {
	PyErr_Format(pygimp_error,
		     "could not set component (%d) %svisible on image (ID %d)",
		     comp, v ? "" : "in", self->ID);
	return NULL;
    }

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

    if (!gimp_image_parasite_attach(self->ID, parasite->para)) {
	PyErr_Format(pygimp_error,
		     "could not attach parasite '%s' to image (ID %d)",
		     parasite->para->name, self->ID);
	return NULL;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
img_attach_new_parasite(PyGimpImage *self, PyObject *args, PyObject *kwargs)
{
    char *name;
    int flags, size;
    guint8 *data;

    static char *kwlist[] = { "name", "flags", "data", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
				     "sis#:attach_new_parasite", kwlist,
				     &name, &flags, &data, &size))
	return NULL;

    if (!gimp_image_attach_new_parasite(self->ID, name, flags, size, data)) {
	PyErr_Format(pygimp_error,
		     "could not attach new parasite '%s' to image (ID %d)",
		     name, self->ID);
	return NULL;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
img_parasite_detach(PyGimpImage *self, PyObject *args)
{
    char *name;

    if (!PyArg_ParseTuple(args, "s:parasite_detach", &name))
	return NULL;

    if (!gimp_image_parasite_detach(self->ID, name)) {
	PyErr_Format(pygimp_error,
		     "could not detach parasite '%s' from image (ID %d)",
		     name, self->ID);
	return NULL;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
img_parasite_list(PyGimpImage *self)
{
    gint num_parasites;
    gchar **parasites;

    if (gimp_image_parasite_list(self->ID, &num_parasites, &parasites)) {
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

    PyErr_Format(pygimp_error, "could not list parasites on image (ID %d)",
		 self->ID);
    return NULL;
}

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

    return PyInt_FromLong(gimp_image_add_hguide(self->ID, ypos));
}

static PyObject *
img_add_vguide(PyGimpImage *self, PyObject *args)
{
    int xpos;

    if (!PyArg_ParseTuple(args, "i:add_vguide", &xpos))
	return NULL;

    return PyInt_FromLong(gimp_image_add_vguide(self->ID, xpos));
}

static PyObject *
img_delete_guide(PyGimpImage *self, PyObject *args)
{
    int guide;

    if (!PyArg_ParseTuple(args, "i:delete_guide", &guide))
	return NULL;

    if (!gimp_image_delete_guide(self->ID, guide)) {
	PyErr_Format(pygimp_error,
		     "could not delete guide (ID %d) from image (ID %d)",
		     guide, self->ID);
	return NULL;
    }

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

static PyObject *
img_undo_is_enabled(PyGimpImage *self)
{
    return PyBool_FromLong(gimp_image_undo_is_enabled(self->ID));
}

static PyObject *
img_undo_freeze(PyGimpImage *self)
{
    return PyBool_FromLong(gimp_image_undo_freeze(self->ID));
}

static PyObject *
img_undo_thaw(PyGimpImage *self)
{
    return PyBool_FromLong(gimp_image_undo_thaw(self->ID));
}

static PyObject *
img_duplicate(PyGimpImage *self)
{
    return pygimp_image_new(gimp_image_duplicate(self->ID));
}

static PyObject *
img_undo_group_start(PyGimpImage *self)
{
    if (!gimp_image_undo_group_start(self->ID)) {
	PyErr_Format(pygimp_error,
		     "could not start undo group on image (ID %d)",
		     self->ID);
	return NULL;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
img_undo_group_end(PyGimpImage *self)
{
    if (!gimp_image_undo_group_end(self->ID)) {
	PyErr_Format(pygimp_error,
		     "could not end undo group on image (ID %d)",
		     self->ID);
	return NULL;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyMethodDef img_methods[] = {
    {"add_channel",	(PyCFunction)img_add_channel,	METH_VARARGS},
    {"add_layer",	(PyCFunction)img_add_layer,	METH_VARARGS},
    {"clean_all",	(PyCFunction)img_clean_all,	METH_NOARGS},
    {"disable_undo",	(PyCFunction)img_disable_undo,	METH_NOARGS},
    {"enable_undo",	(PyCFunction)img_enable_undo,	METH_NOARGS},
    {"flatten",	(PyCFunction)img_flatten,	METH_NOARGS},
    {"lower_channel",	(PyCFunction)img_lower_channel,	METH_VARARGS},
    {"lower_layer",	(PyCFunction)img_lower_layer,	METH_VARARGS},
    {"lower_layer_to_bottom",	(PyCFunction)img_lower_layer_to_bottom,	METH_VARARGS},
    {"merge_visible_layers",	(PyCFunction)img_merge_visible_layers,	METH_VARARGS},
    {"merge_down",	(PyCFunction)img_merge_down,	METH_VARARGS},
    {"pick_correlate_layer",	(PyCFunction)img_pick_correlate_layer,	METH_VARARGS},
    {"raise_channel",	(PyCFunction)img_raise_channel,	METH_VARARGS},
    {"raise_layer",	(PyCFunction)img_raise_layer,	METH_VARARGS},
    {"raise_layer_to_top",	(PyCFunction)img_raise_layer_to_top,	METH_VARARGS},
    {"remove_channel",	(PyCFunction)img_remove_channel,	METH_VARARGS},
    {"remove_layer",	(PyCFunction)img_remove_layer,	METH_VARARGS},
    {"resize",	(PyCFunction)img_resize,	METH_VARARGS | METH_KEYWORDS},
    {"resize_to_layers",	(PyCFunction)img_resize_to_layers,	METH_NOARGS},
    {"get_component_active",	(PyCFunction)img_get_component_active,	METH_VARARGS},
    {"get_component_visible",	(PyCFunction)img_get_component_visible,	METH_VARARGS},
    {"set_component_active",	(PyCFunction)img_set_component_active,	METH_VARARGS},
    {"set_component_visible",	(PyCFunction)img_set_component_visible,	METH_VARARGS},
    {"parasite_find",       (PyCFunction)img_parasite_find,      METH_VARARGS},
    {"parasite_attach",     (PyCFunction)img_parasite_attach,    METH_VARARGS},
    {"attach_new_parasite", (PyCFunction)img_attach_new_parasite,	METH_VARARGS | METH_KEYWORDS},
    {"parasite_detach",     (PyCFunction)img_parasite_detach,    METH_VARARGS},
    {"parasite_list", (PyCFunction)img_parasite_list, METH_NOARGS},
    {"get_layer_by_tattoo",(PyCFunction)img_get_layer_by_tattoo,METH_VARARGS},
    {"get_channel_by_tattoo",(PyCFunction)img_get_channel_by_tattoo,METH_VARARGS},
    {"add_hguide", (PyCFunction)img_add_hguide, METH_VARARGS},
    {"add_vguide", (PyCFunction)img_add_vguide, METH_VARARGS},
    {"delete_guide", (PyCFunction)img_delete_guide, METH_VARARGS},
    {"find_next_guide", (PyCFunction)img_find_next_guide, METH_VARARGS},
    {"get_guide_orientation",(PyCFunction)img_get_guide_orientation,METH_VARARGS},
    {"get_guide_position", (PyCFunction)img_get_guide_position, METH_VARARGS},
    {"scale", (PyCFunction)img_scale, METH_VARARGS | METH_KEYWORDS},
    {"crop", (PyCFunction)img_crop, METH_VARARGS | METH_KEYWORDS},
    {"free_shadow", (PyCFunction)img_free_shadow, METH_NOARGS},
    {"unset_active_channel", (PyCFunction)img_unset_active_channel, METH_NOARGS},
    {"undo_is_enabled", (PyCFunction)img_undo_is_enabled, METH_NOARGS},
    {"undo_freeze", (PyCFunction)img_undo_freeze, METH_NOARGS},
    {"undo_thaw", (PyCFunction)img_undo_thaw, METH_NOARGS},
    {"duplicate", (PyCFunction)img_duplicate, METH_NOARGS},
    {"undo_group_start", (PyCFunction)img_undo_group_start, METH_NOARGS},
    {"undo_group_end", (PyCFunction)img_undo_group_end, METH_NOARGS},
    {NULL,		NULL}		/* sentinel */
};

static PyObject *
img_get_ID(PyGimpImage *self, void *closure)
{
    return PyInt_FromLong(self->ID);
}

static PyObject *
img_get_active_channel(PyGimpImage *self, void *closure)
{
    gint32 id = gimp_image_get_active_channel(self->ID);

    if (id == -1) {
	Py_INCREF(Py_None);
	return Py_None;
    }

    return pygimp_channel_new(id);
}

static int
img_set_active_channel(PyGimpImage *self, PyObject *value, void *closure)
{
    PyGimpChannel *chn;

    if (value == NULL) {
	PyErr_SetString(PyExc_TypeError, "cannot delete active_channel");
	return -1;
    }

    if (!pygimp_channel_check(value)) {
	PyErr_SetString(PyExc_TypeError, "type mismatch");
	return -1;
    }

    chn = (PyGimpChannel *)value;

    if (!gimp_image_set_active_channel(self->ID, chn->ID)) {
	PyErr_Format(pygimp_error,
		     "could not set active channel (ID %d) on image (ID %d)",
		     chn->ID, self->ID);
        return -1;
    }

    return 0;
}

static PyObject *
img_get_active_drawable(PyGimpImage *self, void *closure)
{
    gint32 id = gimp_image_get_active_drawable(self->ID);

    if (id == -1) {
	Py_INCREF(Py_None);
	return Py_None;
    }

    return pygimp_drawable_new(NULL, id);
}

static PyObject *
img_get_active_layer(PyGimpImage *self, void *closure)
{
    gint32 id = gimp_image_get_active_layer(self->ID);

    if (id == -1) {
	Py_INCREF(Py_None);
	return Py_None;
    }

    return pygimp_layer_new(id);
}

static int
img_set_active_layer(PyGimpImage *self, PyObject *value, void *closure)
{
    PyGimpLayer *lay;

    if (value == NULL) {
	PyErr_SetString(PyExc_TypeError, "cannot delete active_layer");
	return -1;
    }

    if (!pygimp_layer_check(value)) {
	PyErr_SetString(PyExc_TypeError, "type mismatch");
	return -1;
    }

    lay = (PyGimpLayer *)value;

    if (!gimp_image_set_active_layer(self->ID, lay->ID)) {
	PyErr_Format(pygimp_error,
		     "could not set active layer (ID %d) on image (ID %d)",
		     lay->ID, self->ID);
        return -1;
    }

    return 0;
}

static PyObject *
img_get_base_type(PyGimpImage *self, void *closure)
{
    return PyInt_FromLong(gimp_image_base_type(self->ID));
}

static PyObject *
img_get_channels(PyGimpImage *self, void *closure)
{
    gint32 *channels;
    gint n_channels, i;
    PyObject *ret;

    channels = gimp_image_get_channels(self->ID, &n_channels);

    ret = PyList_New(n_channels);

    for (i = 0; i < n_channels; i++)
	PyList_SetItem(ret, i, pygimp_channel_new(channels[i]));

    g_free(channels);

    return ret;
}

static PyObject *
img_get_colormap(PyGimpImage *self, void *closure)
{
    guchar *cmap;
    gint n_colours;
    PyObject *ret;

    cmap = gimp_image_get_colormap(self->ID, &n_colours);

    if (cmap == NULL) {
	PyErr_Format(pygimp_error, "could not get colormap for image (ID %d)",
		     self->ID);
	return NULL;
    }
	
    ret = PyString_FromStringAndSize(cmap, n_colours * 3);
    g_free(cmap);

    return ret;
}

static int
img_set_colormap(PyGimpImage *self, PyObject *value, void *closure)
{
    if (value == NULL) {
	PyErr_SetString(PyExc_TypeError, "cannot delete colormap");
	return -1;
    }

    if (!PyString_Check(value)) {
	PyErr_SetString(PyExc_TypeError, "type mismatch");
	return -1;
    }

    if (!gimp_image_set_colormap(self->ID, PyString_AsString(value),
                                 PyString_Size(value) / 3)) {
	PyErr_Format(pygimp_error, "could not set colormap on image (ID %d)",
		     self->ID);
        return -1;
    }

    return 0;
}

static PyObject *
img_get_is_dirty(PyGimpImage *self, void *closure)
{
    return PyBool_FromLong(gimp_image_is_dirty(self->ID));
}

static PyObject *
img_get_filename(PyGimpImage *self, void *closure)
{
    gchar *filename;

    filename = gimp_image_get_filename(self->ID);

    if (filename) {
	PyObject *ret = PyString_FromString(filename);
	g_free(filename);
	return ret;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static int
img_set_filename(PyGimpImage *self, PyObject *value, void *closure)
{
    if (value == NULL) {
	PyErr_SetString(PyExc_TypeError, "cannot delete filename");
	return -1;
    }

    if (!PyString_Check(value)) {
	PyErr_SetString(PyExc_TypeError, "type mismatch");
	return -1;
    }

    if (!gimp_image_set_filename(self->ID, PyString_AsString(value))) {
        PyErr_SetString(PyExc_TypeError, "could not set filename "
	                                 "(possibly bad encoding)");
        return -1;
    }

    return 0;
}

static PyObject *
img_get_floating_selection(PyGimpImage *self, void *closure)
{
    gint32 id;

    id = gimp_image_get_floating_sel(self->ID);

    if (id == -1) {
	Py_INCREF(Py_None);
	return Py_None;
    }

    return pygimp_layer_new(id);
}

static PyObject *
img_get_floating_sel_attached_to(PyGimpImage *self, void *closure)
{
    gint32 id;

    id = gimp_image_floating_sel_attached_to(self->ID);

    if (id == -1) {
	Py_INCREF(Py_None);
	return Py_None;
    }

    return pygimp_layer_new(id);
}

static PyObject *
img_get_layers(PyGimpImage *self, void *closure)
{
    gint32 *layers;
    gint n_layers, i;
    PyObject *ret;

    layers = gimp_image_get_layers(self->ID, &n_layers);

    ret = PyList_New(n_layers);

    for (i = 0; i < n_layers; i++)
	PyList_SetItem(ret, i, pygimp_layer_new(layers[i]));

    g_free(layers);

    return ret;
}

static PyObject *
img_get_name(PyGimpImage *self, void *closure)
{
    gchar *name;

    name = gimp_image_get_name(self->ID);

    if (name) {
	PyObject *ret = PyString_FromString(name);
	g_free(name);
	return ret;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
img_get_selection(PyGimpImage *self, void *closure)
{
    return pygimp_channel_new(gimp_image_get_selection(self->ID));
}

static PyObject *
img_get_tattoo_state(PyGimpImage *self, void *closure)
{
    return PyInt_FromLong(gimp_image_get_tattoo_state(self->ID));
}

static int
img_set_tattoo_state(PyGimpImage *self, PyObject *value, void *closure)
{
    if (value == NULL) {
	PyErr_SetString(PyExc_TypeError, "cannot delete tattoo_state");
	return -1;
    }

    if (!PyInt_Check(value)) {
	PyErr_SetString(PyExc_TypeError, "type mismatch");
	return -1;
    }

    gimp_image_set_tattoo_state(self->ID, PyInt_AsLong(value));

    return 0;
}

static PyObject *
img_get_height(PyGimpImage *self, void *closure)
{
    return PyInt_FromLong(gimp_image_height(self->ID));
}

static PyObject *
img_get_width(PyGimpImage *self, void *closure)
{
    return PyInt_FromLong(gimp_image_width(self->ID));
}

static PyObject *
img_get_resolution(PyGimpImage *self, void *closure)
{
    double xres, yres;

    gimp_image_get_resolution(self->ID, &xres, &yres);

    return Py_BuildValue("(dd)", xres, yres);
}

static int
img_set_resolution(PyGimpImage *self, PyObject *value, void *closure)
{
    gdouble xres, yres;

    if (value == NULL) {
	PyErr_SetString(PyExc_TypeError, "cannot delete resolution");
	return -1;
    }

    if (!PySequence_Check(value) ||
	!PyArg_ParseTuple(value, "dd", &xres, &yres)) {
	PyErr_Clear();
	PyErr_SetString(PyExc_TypeError, "type mismatch");
	return -1;
    }

    if (!gimp_image_set_resolution(self->ID, xres, yres)) {
        PyErr_SetString(PyExc_TypeError, "could not set resolution");
        return -1;
    }

    return 0;
}

static PyObject *
img_get_unit(PyGimpImage *self, void *closure)
{
    return PyInt_FromLong(gimp_image_get_unit(self->ID));
}

static int
img_set_unit(PyGimpImage *self, PyObject *value, void *closure)
{
    if (value == NULL) {
	PyErr_SetString(PyExc_TypeError, "cannot delete unit");
	return -1;
    }

    if (!PyInt_Check(value)) {
	PyErr_SetString(PyExc_TypeError, "type mismatch");
	return -1;
    }

    if (!gimp_image_set_unit(self->ID, PyInt_AsLong(value))) {
        PyErr_SetString(PyExc_TypeError, "could not set unit");
        return -1;
    }

    return 0;
}

static PyObject *
img_get_vectors(PyGimpImage *self, void *closure)
{
    int *vectors;
    int i, num_vectors;
    PyObject *ret;

    vectors = gimp_image_get_vectors(self->ID, &num_vectors);

    ret = PyList_New(num_vectors);

    for (i = 0; i < num_vectors; i++)
        PyList_SetItem(ret, i, pygimp_vectors_new(vectors[i]));

    g_free(vectors);

    return ret;
}

static PyGetSetDef img_getsets[] = {
    { "ID", (getter)img_get_ID, (setter)0 },
    { "active_channel", (getter)img_get_active_channel,
      (setter)img_set_active_channel },
    { "active_drawable", (getter)img_get_active_drawable, (setter)0 },
    { "active_layer", (getter)img_get_active_layer,
      (setter)img_set_active_layer },
    { "base_type", (getter)img_get_base_type, (setter)0 },
    { "channels", (getter)img_get_channels, (setter)0 },
    { "colormap", (getter)img_get_colormap, (setter)img_set_colormap },
    { "dirty", (getter)img_get_is_dirty, (setter)0 },
    { "filename", (getter)img_get_filename, (setter)img_set_filename },
    { "floating_selection", (getter)img_get_floating_selection, (setter)0 },
    { "floating_sel_attached_to", (getter)img_get_floating_sel_attached_to,
      (setter)0 },
    { "height", (getter)img_get_height, (setter)0 },
    { "layers", (getter)img_get_layers, (setter)0 },
    { "name", (getter)img_get_name, (setter)0 },
    { "resolution", (getter)img_get_resolution, (setter)img_set_resolution },
    { "selection", (getter)img_get_selection, (setter)0 },
    { "tattoo_state", (getter)img_get_tattoo_state,
      (setter)img_set_tattoo_state },
    { "unit", (getter)img_get_unit, (setter)img_set_unit },
    { "vectors", (getter)img_get_vectors, (setter)0 },
    { "width", (getter)img_get_width, (setter)0 },
    { NULL, (getter)0, (setter)0 }
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
img_repr(PyGimpImage *self)
{
    PyObject *s;
    gchar *name;

    name = gimp_image_get_name(self->ID);
    s = PyString_FromFormat("<gimp.Image '%s'>", name ? name : "(null)");
    g_free(name);

    return s;
}

static int
img_cmp(PyGimpImage *self, PyGimpImage *other)
{
    if (self->ID == other->ID)
        return 0;

    if (self->ID > other->ID)
        return -1;

    return 1;
}

static int
img_init(PyGimpImage *self, PyObject *args, PyObject *kwargs)
{
    guint width, height;
    GimpImageBaseType type = GIMP_RGB;

    static char *kwlist[] = { "width", "height", "type", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
				     "ii|i:gimp.Image.__init__", kwlist,
				     &width, &height, &type))
        return -1;

    self->ID = gimp_image_new(width, height, type);

    if (self->ID < 0) {
	PyErr_Format(pygimp_error,
		     "could not create image (width: %d, height: %d, type: %d)",
		     width, height, type);
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
    (getattrfunc)0,                     /* tp_getattr */
    (setattrfunc)0,                     /* tp_setattr */
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
    img_getsets,			/* tp_getset */
    (PyTypeObject *)0,			/* tp_base */
    (PyObject *)0,			/* tp_dict */
    0,					/* tp_descr_get */
    0,					/* tp_descr_set */
    0,					/* tp_dictoffset */
    (initproc)img_init,                 /* tp_init */
    (allocfunc)0,			/* tp_alloc */
    (newfunc)0,				/* tp_new */
};
