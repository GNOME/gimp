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

#include "config.h"

#include <Python.h>
#include <structseq.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include <pygobject.h>
#include <pygtk/pygtk.h>

#include "pygimp-api.h"

#include "pygimp-util.h"

#include "pygimp-intl.h"

typedef struct
{
    PyObject *func;
    PyObject *data;
} ProxyData;


static void
proxy_response (GtkWidget *widget,
                gint       response_id,
                ProxyData *proxy_data)
{
    PyObject        *pdb_func, *ret;
    gchar           *name;
    gchar           *blurb;
    gchar           *help;
    gchar           *author;
    gchar           *copyright;
    gchar           *date;
    GimpPDBProcType  proc_type;
    gint             n_params;
    gint             n_return_vals;
    GimpParamDef    *params;
    GimpParamDef    *return_vals;

    if (response_id != GTK_RESPONSE_APPLY) {
	if (proxy_data)
	    gtk_widget_destroy (widget);
	else
	    gtk_main_quit ();

	return;
    }

    if (proxy_data == NULL)
        return;

    name = gimp_proc_browser_dialog_get_selected(GIMP_PROC_BROWSER_DIALOG (widget));

    if (name == NULL)
	return;

    gimp_procedural_db_proc_info(name, &blurb, &help, &author, &copyright,
				 &date, &proc_type,
				 &n_params, &n_return_vals,
				 &params, &return_vals);

    pdb_func = pygimp_pdb_function_new(name, blurb, help, author, copyright,
				       date, proc_type, n_params, n_return_vals,
				       params, return_vals);

    if (pdb_func == NULL) {
	PyErr_Print();

	gimp_destroy_paramdefs(params,      n_params);
	gimp_destroy_paramdefs(return_vals, n_return_vals);

	goto bail;
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

bail:
    g_free(name);
    g_free(blurb);
    g_free(help);
    g_free(author);
    g_free(copyright);
    g_free(date);
}

static void
proxy_cleanup(gpointer data, GClosure *closure)
{
    ProxyData *proxy_data = data;

    if (!data)
        return;

    Py_DECREF(proxy_data->func);
    Py_XDECREF(proxy_data->data);

    g_free(proxy_data);
}

static void
proxy_row_activated(GtkDialog *dialog)
{
    gtk_dialog_response (dialog, GTK_RESPONSE_APPLY);
}

static PyObject *
proc_browser_dialog_new(PyObject *self, PyObject *args, PyObject *kwargs)
{
    PyObject *py_func = Py_None, *py_data = Py_None;
    ProxyData *proxy_data = NULL;
    GObject *dlg;
    gboolean has_apply = FALSE;

    static char *kwlist[] = { "apply_callback", "data", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|OO:dialog_new", kwlist,
				     &py_func, &py_data))
	return NULL;

    if (py_func != Py_None) {
        if (PyCallable_Check(py_func))
	    has_apply = TRUE;
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

    dlg = G_OBJECT(gimp_proc_browser_dialog_new(_("Python Procedure Browser"),
                                                "python-fu-procedure-browser",
                                                gimp_standard_help_func, NULL,
                                                NULL));

    if (has_apply) {
        gtk_dialog_add_button(GTK_DIALOG(dlg),
                              GTK_STOCK_APPLY, GTK_RESPONSE_APPLY);
        gtk_dialog_set_default_response(GTK_DIALOG(dlg),
                                        GTK_RESPONSE_APPLY);
    }

    gtk_dialog_add_button(GTK_DIALOG(dlg),
                          GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);

    g_signal_connect_data(dlg, "response",
                          G_CALLBACK(proxy_response), proxy_data,
			  proxy_cleanup, 0);
    g_signal_connect(dlg, "row-activated",
                     G_CALLBACK(proxy_row_activated),
                     NULL);

    gtk_widget_show(GTK_WIDGET(dlg));

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

DL_EXPORT(void)
initgimpprocbrowser(void)
{
    PyObject *m;

    pygimp_init_pygobject();
    init_pygtk();
    init_pygimp();

    /* Create the module and add the functions */
    m = Py_InitModule3("gimpprocbrowser",
		       procbrowser_methods, procbrowser_doc);

    /* Check for errors */
    if (PyErr_Occurred())
	Py_FatalError("can't initialize module gimpprocbrowser");
}
