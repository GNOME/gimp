/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2003 Peter Mattis and Spencer Kimball
 *
 * gimpprocedure-private.c
 * Copyright (C) 2019 Michael Natterer <mitch@gimp.org>
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
#include "libgimpbase/gimpwire.h"

#include "gimp.h"
#include "gimp-private.h"
#include "gimpgpparams.h"
#include "gimpprocedure-private.h"


/*  public functions  */

void
_gimp_procedure_register (GimpProcedure *procedure)
{
  GParamSpec   **args;
  GParamSpec   **return_vals;
  gint           n_args;
  gint           n_return_vals;
  GList         *list;
  GPProcInstall  proc_install;
  GimpIconType   icon_type;
  const guint8  *icon_data;
  gint           icon_data_length;
  gint           i;

  g_return_if_fail (GIMP_IS_PROCEDURE (procedure));

  args        = gimp_procedure_get_arguments (procedure, &n_args);
  return_vals = gimp_procedure_get_return_values (procedure, &n_return_vals);

  proc_install.name         = (gchar *) gimp_procedure_get_name (procedure);
  proc_install.blurb        = (gchar *) gimp_procedure_get_blurb (procedure);
  proc_install.help         = (gchar *) gimp_procedure_get_help (procedure);
  proc_install.authors      = (gchar *) gimp_procedure_get_authors (procedure);
  proc_install.copyright    = (gchar *) gimp_procedure_get_copyright (procedure);
  proc_install.date         = (gchar *) gimp_procedure_get_date (procedure);
  proc_install.menu_label   = (gchar *) gimp_procedure_get_menu_label (procedure);
  proc_install.image_types  = (gchar *) gimp_procedure_get_image_types (procedure);
  proc_install.type         = gimp_procedure_get_proc_type (procedure);
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

  if (! gp_proc_install_write (_gimp_writechannel, &proc_install, NULL))
    gimp_quit ();

  icon_type = gimp_procedure_get_icon (procedure,
                                       &icon_data, &icon_data_length);
  if (icon_data)
    _gimp_plugin_icon_register (gimp_procedure_get_name (procedure),
                                icon_type, icon_data_length, icon_data);

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

void
_gimp_procedure_unregister (GimpProcedure *procedure)
{
  GPProcUninstall proc_uninstall;

  proc_uninstall.name = (gchar *) gimp_procedure_get_name (procedure);

  if (! gp_proc_uninstall_write (_gimp_writechannel, &proc_uninstall, NULL))
    gimp_quit ();
}

void
_gimp_procedure_extension_ready (GimpProcedure *procedure)
{
  if (! gp_extension_ack_write (_gimp_writechannel, NULL))
    gimp_quit ();
}
