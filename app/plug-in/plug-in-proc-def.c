/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <glib-object.h>

#include "plug-in-types.h"

#include "plug-in.h"
#include "plug-in-proc.h"


PlugInProcDef *
plug_in_proc_def_new (void)
{
  PlugInProcDef *proc_def;

  proc_def = g_new0 (PlugInProcDef, 1);

  return proc_def;
}

void
plug_in_proc_def_free (PlugInProcDef *proc_def)
{
  gint i;

  g_return_if_fail (proc_def != NULL);

  g_free (proc_def->db_info.name);
  g_free (proc_def->db_info.blurb);
  g_free (proc_def->db_info.help);
  g_free (proc_def->db_info.author);
  g_free (proc_def->db_info.copyright);
  g_free (proc_def->db_info.date);

  for (i = 0; i < proc_def->db_info.num_args; i++)
    {
      g_free (proc_def->db_info.args[i].name);
      g_free (proc_def->db_info.args[i].description);
    }

  for (i = 0; i < proc_def->db_info.num_values; i++)
    {
      g_free (proc_def->db_info.values[i].name);
      g_free (proc_def->db_info.values[i].description);
    }

  g_free (proc_def->db_info.args);
  g_free (proc_def->db_info.values);

  g_free (proc_def->prog);
  g_free (proc_def->menu_path);
  g_free (proc_def->accelerator);
  g_free (proc_def->extensions);
  g_free (proc_def->prefixes);
  g_free (proc_def->magics);
  g_free (proc_def->image_types);

  g_free (proc_def);
}

ProcRecord * 
plug_in_proc_def_get_proc (PlugInProcDef *proc_def)
{
  g_return_val_if_fail (proc_def != NULL, NULL);

  return &proc_def->db_info;
}

const gchar *
plug_in_proc_def_get_progname (PlugInProcDef *proc_def)
{
  g_return_val_if_fail (proc_def != NULL, NULL);

  switch (proc_def->db_info.proc_type)
    {
    case GIMP_PLUGIN:
    case GIMP_EXTENSION:
      return proc_def->prog;

    case GIMP_TEMPORARY:
      return ((PlugIn *) proc_def->db_info.exec_method.temporary.plug_in)->prog;

    default:
      break;
    }

  return NULL;
}

gchar *
plug_in_proc_def_get_help_id (PlugInProcDef *proc_def)
{
  const gchar *progname;
  gchar       *basename;
  gchar       *lowercase_basename;
  gchar       *help_id;

  g_return_val_if_fail (proc_def != NULL, NULL);

  progname = plug_in_proc_def_get_progname (proc_def);

  basename = g_path_get_basename (progname);
  lowercase_basename = g_ascii_strdown (basename, -1);
  g_free (basename);

#ifdef __GNUC__
#warning FIXME: fix plug-in menu item help
#endif
  help_id = g_strconcat (lowercase_basename, ".html", NULL);

  g_free (lowercase_basename);

  return help_id;
}
