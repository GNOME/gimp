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

#include <glib-object.h>

static PyObject *
item_from_id(PyObject *not_used, PyObject *args)
{
    gint32 ID;

    if (!PyArg_ParseTuple(args, "i", &ID))
        return NULL;
    return pygimp_item_new(ID);
}

static PyMethodDef item_methods[] = {
    {"from_id",   (PyCFunction)item_from_id,  METH_VARARGS | METH_STATIC},
    {NULL,              NULL}           /* sentinel */
};

static PyObject *
item_get_parent(PyGimpLayer *self, void *closure)
{
    gint32 id = gimp_item_get_parent(self->ID);

    if (id == -1) {
        Py_INCREF(Py_None);
        return Py_None;
    }

    return pygimp_item_new(id);
}

static PyObject *
item_get_children(PyGimpLayer *self, void *closure)
{
    gint32 *children;
    gint n_children, i;
    PyObject *ret;

    children = gimp_item_get_children(self->ID, &n_children);

    ret = PyList_New(n_children);

    for (i = 0; i < n_children; i++)
        PyList_SetItem(ret, i, pygimp_item_new(children[i]));

    g_free(children);

    return ret;
}

static PyGetSetDef item_getsets[] = {
    { "parent", (getter)item_get_parent, (setter)0 },
    { "children", (getter) item_get_children, (setter)0 },
    { NULL, (getter)0, (setter)0 }
};


static void
item_dealloc(PyGimpItem *self)
{
    PyObject_DEL(self);
}

static PyObject *
item_repr(PyGimpItem *self)
{
    PyObject *s;

    s = PyString_FromFormat("<gimp.Item '%d'>", self->ID);

    return s;
}

static int
item_init(PyGimpLayer *self, PyObject *args, PyObject *kwargs)
{
    return -1;
}



PyTypeObject PyGimpItem_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                                  /* ob_size */
    "gimp.Item",                        /* tp_name */
    sizeof(PyGimpItem),                 /* tp_basicsize */
    0,                                  /* tp_itemsize */
    /* methods */
    (destructor)item_dealloc,            /* tp_dealloc */
    (printfunc)0,                       /* tp_print */
    (getattrfunc)0,                     /* tp_getattr */
    (setattrfunc)0,                     /* tp_setattr */
    (cmpfunc)0,                         /* tp_compare */
    (reprfunc)item_repr,                /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
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
    item_methods,                       /* tp_methods */
    0,                                  /* tp_members */
    item_getsets,                       /* tp_getset */
    (PyTypeObject *)0,                   /* tp_base */
    (PyObject *)0,                      /* tp_dict */
    0,                                  /* tp_descr_get */
    0,                                  /* tp_descr_set */
    0,                                  /* tp_dictoffset */
    (initproc)item_init,                /* tp_init */
    (allocfunc)0,                       /* tp_alloc */
    (newfunc)0,                         /* tp_new */
};


PyObject *
pygimp_item_new(gint32 ID)
{
    PyObject *self;

    if (!gimp_item_is_valid(ID)) {
        Py_INCREF(Py_None);
        return Py_None;
    }

    /* create the appropriate object type */
    if (gimp_item_is_drawable(ID)) {
        if (gimp_item_is_group(ID)) {
            self = pygimp_group_layer_new(ID);
        }
        else {
            self = pygimp_drawable_new(NULL, ID);
        }
    }
    else /* Vectors */
        self = pygimp_vectors_new(ID);

    if (self == NULL)
        return NULL;

    return self;
}
