/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimpfileops_pdb.c
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "gimp.h"


void
gimp_register_magic_load_handler (gchar *name,
				  gchar *extensions,
				  gchar *prefixes,
				  gchar *magics)
{
  GimpParam *return_vals;
  gint nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_register_magic_load_handler",
				    &nreturn_vals,
				    PARAM_STRING, name,
				    PARAM_STRING, extensions,
				    PARAM_STRING, prefixes,
				    PARAM_STRING, magics,
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}

void
gimp_register_load_handler (gchar *name,
			    gchar *extensions,
			    gchar *prefixes)
{
  GimpParam *return_vals;
  gint nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_register_load_handler",
				    &nreturn_vals,
				    PARAM_STRING, name,
				    PARAM_STRING, extensions,
				    PARAM_STRING, prefixes,
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}

void
gimp_register_save_handler (gchar *name,
			    gchar *extensions,
			    gchar *prefixes)
{
  GimpParam *return_vals;
  gint nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_register_save_handler",
				    &nreturn_vals,
				    PARAM_STRING, name,
				    PARAM_STRING, extensions,
				    PARAM_STRING, prefixes,
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}
