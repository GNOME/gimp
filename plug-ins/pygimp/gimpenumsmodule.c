/* -*- Mode: C; c-basic-offset: 4 -*-
 * Gimp-Python - allows the writing of Gimp plugins in Python.
 * Copyright (C) 2005  Manish Singh <yosh@gimp.org>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <Python.h>

#include <glib-object.h>

#include <pygobject.h>

#include "pygimp-api.h"
#include "pygimp-util.h"


static void
add_misc_enums(PyObject *m)
{
    PyModule_AddIntConstant(m, "PARASITE_PERSISTENT",
			    GIMP_PARASITE_PERSISTENT);
    PyModule_AddIntConstant(m, "PARASITE_UNDOABLE",
			    GIMP_PARASITE_UNDOABLE);
    PyModule_AddIntConstant(m, "PARASITE_ATTACH_PARENT",
			    GIMP_PARASITE_ATTACH_PARENT);
    PyModule_AddIntConstant(m, "PARASITE_PARENT_PERSISTENT",
			    GIMP_PARASITE_PARENT_PERSISTENT);
    PyModule_AddIntConstant(m, "PARASITE_PARENT_UNDOABLE",
			    GIMP_PARASITE_PARENT_UNDOABLE);
    PyModule_AddIntConstant(m, "PARASITE_ATTACH_GRANDPARENT",
			    GIMP_PARASITE_ATTACH_GRANDPARENT);
    PyModule_AddIntConstant(m, "PARASITE_GRANDPARENT_PERSISTENT",
			    GIMP_PARASITE_GRANDPARENT_PERSISTENT);
    PyModule_AddIntConstant(m, "PARASITE_GRANDPARENT_UNDOABLE",
			    GIMP_PARASITE_GRANDPARENT_UNDOABLE);

    PyModule_AddIntConstant(m, "UNIT_PIXEL",
			    GIMP_UNIT_PIXEL);
    PyModule_AddIntConstant(m, "UNIT_INCH",
			    GIMP_UNIT_INCH);
    PyModule_AddIntConstant(m, "UNIT_MM",
			    GIMP_UNIT_MM);
    PyModule_AddIntConstant(m, "UNIT_POINT",
			    GIMP_UNIT_POINT);
    PyModule_AddIntConstant(m, "UNIT_PICA",
			    GIMP_UNIT_PICA);

    PyModule_AddIntConstant(m, "MIN_IMAGE_SIZE",
			    GIMP_MIN_IMAGE_SIZE);
    PyModule_AddIntConstant(m, "MAX_IMAGE_SIZE",
			    GIMP_MAX_IMAGE_SIZE);

    PyModule_AddObject(m, "MIN_RESOLUTION",
		       PyFloat_FromDouble(GIMP_MIN_RESOLUTION));
    PyModule_AddObject(m, "MAX_RESOLUTION",
		       PyFloat_FromDouble(GIMP_MAX_RESOLUTION));

    PyModule_AddObject(m, "MAX_MEMSIZE",
		       PyLong_FromUnsignedLongLong(GIMP_MAX_MEMSIZE));

    PyModule_AddIntConstant(m, "PIXEL_FETCHER_EDGE_NONE",
                            GIMP_PIXEL_FETCHER_EDGE_NONE);
    PyModule_AddIntConstant(m, "PIXEL_FETCHER_EDGE_WRAP",
                            GIMP_PIXEL_FETCHER_EDGE_WRAP);
    PyModule_AddIntConstant(m, "PIXEL_FETCHER_EDGE_SMEAR",
                            GIMP_PIXEL_FETCHER_EDGE_SMEAR);
    PyModule_AddIntConstant(m, "PIXEL_FETCHER_EDGE_BLACK",
                            GIMP_PIXEL_FETCHER_EDGE_BLACK);
    PyModule_AddIntConstant(m, "PIXEL_FETCHER_EDGE_BACKGROUND",
                            GIMP_PIXEL_FETCHER_EDGE_BACKGROUND);
}

static void
add_compat_enums(PyObject *m)
{
    PyModule_AddIntConstant(m, "ADD_WHITE_MASK",
			    GIMP_ADD_MASK_WHITE);
    PyModule_AddIntConstant(m, "ADD_BLACK_MASK",
			    GIMP_ADD_MASK_BLACK);
    PyModule_AddIntConstant(m, "ADD_ALPHA_MASK",
			    GIMP_ADD_MASK_ALPHA);
    PyModule_AddIntConstant(m, "ADD_ALPHA_TRANSFER_MASK",
			    GIMP_ADD_MASK_ALPHA_TRANSFER);
    PyModule_AddIntConstant(m, "ADD_SELECTION_MASK",
			    GIMP_ADD_MASK_SELECTION);
    PyModule_AddIntConstant(m, "ADD_COPY_MASK",
			    GIMP_ADD_MASK_COPY);
    PyModule_AddIntConstant(m, "ADD_CHANNEL_MASK",
			    GIMP_ADD_MASK_CHANNEL);

    PyModule_AddIntConstant(m, "FG_BG_RGB_MODE",
			    GIMP_BLEND_FG_BG_RGB);
    PyModule_AddIntConstant(m, "FG_BG_HSV_MODE",
			    GIMP_BLEND_FG_BG_HSV);
    PyModule_AddIntConstant(m, "FG_TRANSPARENT_MODE",
			    GIMP_BLEND_FG_TRANSPARENT);
    PyModule_AddIntConstant(m, "CUSTOM_MODE",
			    GIMP_BLEND_CUSTOM);

    PyModule_AddIntConstant(m, "FG_BUCKET_FILL",
			    GIMP_BUCKET_FILL_FG);
    PyModule_AddIntConstant(m, "BG_BUCKET_FILL",
			    GIMP_BUCKET_FILL_BG);
    PyModule_AddIntConstant(m, "PATTERN_BUCKET_FILL",
			    GIMP_BUCKET_FILL_PATTERN);

    PyModule_AddIntConstant(m, "BLUR_CONVOLVE",
			    GIMP_CONVOLVE_BLUR);
    PyModule_AddIntConstant(m, "SHARPEN_CONVOLVE",
			    GIMP_CONVOLVE_SHARPEN);

    PyModule_AddIntConstant(m, "IMAGE_CLONE",
			    GIMP_CLONE_IMAGE);
    PyModule_AddIntConstant(m, "PATTERN_CLONE",
			    GIMP_CLONE_PATTERN);

    PyModule_AddIntConstant(m, "DODGE",
			    GIMP_DODGE_BURN_TYPE_DODGE);
    PyModule_AddIntConstant(m, "BURN",
			    GIMP_DODGE_BURN_TYPE_BURN);

    PyModule_AddIntConstant(m, "SHADOWS",
			    GIMP_TRANSFER_SHADOWS);
    PyModule_AddIntConstant(m, "MIDTONES",
			    GIMP_TRANSFER_MIDTONES);
    PyModule_AddIntConstant(m, "HIGHLIGHTS",
			    GIMP_TRANSFER_HIGHLIGHTS);
}

static void
add_registered_enums(PyObject *m)
{
    int num_names, i;
    const char **names;

    names = gimp_enums_get_type_names(&num_names);

    pyg_enum_add_constants(m, GIMP_TYPE_CHECK_SIZE, "GIMP_");
    pyg_enum_add_constants(m, GIMP_TYPE_CHECK_TYPE, "GIMP_");

    for (i = 0; i < num_names; i++)
	pyg_enum_add_constants(m, g_type_from_name(names[i]), "GIMP_");
}


/* Initialization function for the module (*must* be called initgimpenums) */

static char gimpenums_doc[] =
"This module provides interfaces to allow you to write gimp plugins"
;

void init_gimpenums(void);

PyMODINIT_FUNC
init_gimpenums(void)
{
    PyObject *m;

    pygimp_init_pygobject();

    init_pygimp();

    gimp_enums_init();

    /* Create the module and add the functions */
    m = Py_InitModule3("_gimpenums", NULL, gimpenums_doc);

    add_misc_enums(m);
    add_compat_enums(m);
    add_registered_enums(m);

    /* Check for errors */
    if (PyErr_Occurred())
	Py_FatalError("can't initialize module _gimpenums");
}
