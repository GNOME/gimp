/* -*- Mode: C; c-basic-offset: 4 -*-
    Gimp-Python - allows the writing of Gimp plugins in Python.
    Copyright (C) 2004  Manish Singh <yosh@gimp.org>

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

#include <Python.h>
#include <structseq.h>

#include <gtk/gtk.h>

#include <pygobject.h>
#include <pygtk/pygtk.h>

#include <libgimp/gimp.h>

#include <plug-ins/dbbrowser/gimpprocbrowser.h>


typedef PyObject *(*PyGimpPDBFunctionNew)(const char *name, const char *blurb,
					  const char *help, const char *author,
					  const char *copyright,
					  const char *date,
					  GimpPDBProcType proc_type,
					  int n_params, int n_return_vals,
					  GimpParamDef *params,
					  GimpParamDef *return_vals);

typedef struct
{
    PyObject *func;
    PyObject *data;
} ProxyData;


static PyTypeObject *PyGimpPDBFunction_Type;
static PyGimpPDBFunctionNew pygimp_pdb_function_new;


static GimpParamDef *
copy_paramdefs(const GimpParamDef *paramdefs, gint n_params)
{
    GimpParamDef *copy;

    copy = g_new(GimpParamDef, n_params);

    while (n_params--)
      {
	copy[n_params].type = paramdefs[n_params].type;
	copy[n_params].name = g_strdup(paramdefs[n_params].name);
	copy[n_params].description = g_strdup(paramdefs[n_params].description);
      }

    return copy;
}
            
static void
proxy_apply_callback(const gchar        *name,
		     const gchar        *blurb,
		     const gchar        *help,
		     const gchar        *author,
		     const gchar        *copyright,
		     const gchar        *date,
		     GimpPDBProcType     proc_type,
		     gint                n_params,
		     gint                n_return_vals,
		     const GimpParamDef *params,
		     const GimpParamDef *return_vals,
		     gpointer            user_data)
{
    ProxyData *proxy_data = user_data;
    GimpParamDef *params_copy, *return_vals_copy;
    PyObject *pdb_func, *ret;

    params_copy      = copy_paramdefs(params, n_params);
    return_vals_copy = copy_paramdefs(return_vals, n_return_vals);

    pdb_func = pygimp_pdb_function_new(name, blurb, help, author, copyright,
				       date, proc_type, n_params, n_return_vals,
				       params_copy, return_vals_copy);

    if (pdb_func == NULL) {
	PyErr_Print();
	return;
    }

    if (proxy_data->data)
	ret = PyObject_CallFunctionObjArgs(proxy_data->func, pdb_func,
					   proxy_data->data, NULL);
    else
	ret = PyObject_CallFunctionObjArgs(proxy_data->func, pdb_func, NULL);

    if (ret)
        Py_DECREF(ret);
    else
        PyErr_Print();

    Py_DECREF(pdb_func);
}

static void
proxy_cleanup(gpointer data, GObject *obj)
{
  ProxyData *proxy_data = data;

  Py_DECREF(proxy_data->func);
  Py_XDECREF(proxy_data->data);

  g_free(proxy_data);
}

static PyObject *
proc_browser_dialog_new(PyObject *self, PyObject *args, PyObject *kwargs)
{
    PyObject *py_func = Py_None, *py_data = Py_None;
    GimpProcBrowserApplyCallback proxy_func = NULL;
    ProxyData *proxy_data = NULL;
    GObject *dlg;

    static char *kwlist[] = { "apply_callback", "data", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|OO:dialog_new", kwlist,
				     &py_func, &py_data))
	return NULL;

    if (py_func != Py_None) {
        if (PyCallable_Check(py_func))
	    proxy_func = proxy_apply_callback;
	else {
	    PyErr_SetString(PyExc_TypeError,
			    "apply_callback must be a callable object or None");
	    return NULL;
	}

	proxy_data = g_new0(ProxyData, 1);

	proxy_data->func = py_func;
	Py_INCREF(py_func);

	if (py_data != Py_None) {
	    proxy_data->data = py_data;
	    Py_INCREF(py_data);
	}
    }

    dlg = G_OBJECT(gimp_proc_browser_dialog_new(FALSE, proxy_func, proxy_data));

    if (proxy_data)
      g_object_weak_ref(dlg, proxy_cleanup, proxy_data);

    return pygobject_new(dlg);
}

/* List of methods defined in the module */

static struct PyMethodDef procbrowser_methods[] = {
    {"dialog_new",	(PyCFunction)proc_browser_dialog_new,	METH_VARARGS | METH_KEYWORDS},
    {NULL,	 (PyCFunction)NULL, 0, NULL}		/* sentinel */
};


/* Initialization function for the module (*must* be called initgimpprocbrowser) */

static char procbrowser_doc[] = 
"This module provides a simple interface for the GIMP PDB Browser"
;

void
initgimpprocbrowser(void)
{
    PyObject *m, *pygimp;

    init_pygobject();
    init_pygtk();

    pygimp = PyImport_ImportModule("gimp");

    if (pygimp) {
	PyObject *module_dict = PyModule_GetDict(pygimp);
	PyObject *type_object, *c_object;

	type_object = PyDict_GetItemString(module_dict, "_PDBFunction");
	c_object    = PyDict_GetItemString(module_dict, "_pdb_function_new");

	if (PyType_Check(type_object) && PyCObject_Check(c_object)) {
	    PyGimpPDBFunction_Type = (PyTypeObject*)type_object;
	    pygimp_pdb_function_new = PyCObject_AsVoidPtr(c_object);
	} else {
	    PyErr_SetString(PyExc_RuntimeError,
			    "could not find compatible gimp module");
	    return;
	}
    } else {
	PyErr_SetString(PyExc_ImportError, "could not import gimp");
	return;
    }

    /* Create the module and add the functions */
    m = Py_InitModule3("gimpprocbrowser",
		       procbrowser_methods, procbrowser_doc);

    /* Check for errors */
    if (PyErr_Occurred())
	Py_FatalError("can't initialize module gimpprocbrowser");
}
