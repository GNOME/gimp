/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimpplugin_pdb.c
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
gimp_progress_init (gchar *message)
{
  GimpParam *return_vals;
  gint nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_progress_init",
				    &nreturn_vals,
				    PARAM_STRING, message,
				    PARAM_INT32, gimp_default_display (),
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}

void
gimp_progress_update (gdouble percentage)
{
  GimpParam *return_vals;
  gint nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_progress_update",
				    &nreturn_vals,
				    PARAM_FLOAT, percentage,
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}

void
gimp_plugin_domain_register (gchar *domain_name,
			     gchar *domain_path)
{
  GimpParam *return_vals;
  gint nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_plugin_domain_register",
				    &nreturn_vals,
				    PARAM_STRING, domain_name,
				    PARAM_STRING, domain_path,
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}

void
gimp_plugin_help_register (gchar *help_path)
{
  GParam *return_vals;
  gint    nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_plugin_help_register",
                                    &nreturn_vals,
                                    PARAM_STRING, help_path,
                                    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}
