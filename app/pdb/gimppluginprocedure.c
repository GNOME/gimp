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

#include "plug-in-proc.h"


ProcRecord * 
plug_in_proc_def_get_proc (PlugInProcDef *proc_def)
{
  g_return_val_if_fail (proc_def != NULL, NULL);

  return &proc_def->db_info;
}

void
plug_in_proc_def_destroy (PlugInProcDef *proc_def,
			  gboolean       data_only)
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

  if (!data_only)
    g_free (proc_def);
}
