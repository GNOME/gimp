/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2003 Peter Mattis and Spencer Kimball
 *
 * gimpprocedure-private.c
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <cairo.h>
#include <gio/gio.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpbase/gimpprotocol.h"

#include "gimp.h"
#include "gimpgpparams.h"
#include "gimpprocedure-private.h"


/*  public functions  */

void
_gimp_procedure_register (GimpProcedure *procedure)
{
  extern GIOChannel *_writechannel;

  GParamSpec   **args;
  GParamSpec   **return_vals;
  gint           n_args;
  gint           n_return_vals;
  GList         *list;
  GPProcInstall  proc_install;
  gint           i;

  g_return_if_fail (GIMP_IS_PROCEDURE (procedure));

  args        = gimp_procedure_get_arguments (procedure, &n_args);
  return_vals = gimp_procedure_get_return_values (procedure, &n_return_vals);

  proc_install.name         = (gchar *) gimp_procedure_get_name (procedure);
  proc_install.blurb        = (gchar *) gimp_procedure_get_blurb (procedure);
  proc_install.help         = (gchar *) gimp_procedure_get_help (procedure);
  proc_install.author       = (gchar *) gimp_procedure_get_author (procedure);
  proc_install.copyright    = (gchar *) gimp_procedure_get_copyright (procedure);
  proc_install.date         = (gchar *) gimp_procedure_get_date (procedure);
  proc_install.menu_label   = (gchar *) gimp_procedure_get_menu_label (procedure);
  proc_install.image_types  = (gchar *) gimp_procedure_get_image_types (procedure);
  proc_install.type         = GIMP_PLUGIN;
  proc_install.nparams      = n_args;
  proc_install.nreturn_vals = n_return_vals;
  proc_install.params       = g_new0 (GPParamDef, n_args);
  proc_install.return_vals  = g_new0 (GPParamDef, n_return_vals);

  for (i = 0; i < n_args; i++)
    {
      _gimp_param_spec_to_gp_param_def (args[i],
                                        &proc_install.params[i]);
    }

  for (i = 0; i < n_return_vals; i++)
    {
      _gimp_param_spec_to_gp_param_def (return_vals[i],
                                        &proc_install.return_vals[i]);
    }

  if (! gp_proc_install_write (_writechannel, &proc_install, NULL))
    gimp_quit ();

  g_free (proc_install.params);
  g_free (proc_install.return_vals);

  for (list = gimp_procedure_get_menu_paths (procedure);
       list;
       list = g_list_next (list))
    {
      gimp_plugin_menu_register (gimp_procedure_get_name (procedure),
                                 list->data);
    }
}
