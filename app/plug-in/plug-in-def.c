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

#include <gtk/gtk.h>

#include "plug-in-types.h"

#include "plug-in.h"
#include "plug-in-def.h"
#include "plug-in-proc.h"


PlugInDef *
plug_in_def_new (const gchar *prog)
{
  PlugInDef *plug_in_def;

  g_return_val_if_fail (prog != NULL, NULL);

  plug_in_def = g_new0 (PlugInDef, 1);

  plug_in_def->prog = g_strdup (prog);
  
  return plug_in_def;
}

void
plug_in_def_free (PlugInDef *plug_in_def,
		  gboolean   free_proc_defs)
{
  GSList *list;

  g_free (plug_in_def->prog);
  g_free (plug_in_def->locale_domain);
  g_free (plug_in_def->locale_path);
  g_free (plug_in_def->help_path);

  if (free_proc_defs)
    {
      for (list = plug_in_def->proc_defs; list; list = list->next)
	plug_in_proc_def_destroy ((PlugInProcDef *)(list->data), FALSE);
    }

  if (plug_in_def->proc_defs)
    g_slist_free (plug_in_def->proc_defs);

  g_free (plug_in_def);
}

void
plug_in_def_add_proc_def (PlugInDef     *plug_in_def,
                          PlugInProcDef *proc_def)
{
  proc_def->mtime = plug_in_def->mtime;
  proc_def->prog = g_strdup (plug_in_def->prog);

  plug_in_def->proc_defs = g_slist_append (plug_in_def->proc_defs,
                                           proc_def);
}

void
plug_in_def_set_mtime (PlugInDef *plug_in_def,
                       time_t     mtime)
{
  plug_in_def->mtime = mtime;
} 

void
plug_in_def_set_locale_domain_name (PlugInDef   *plug_in_def,
                                    const gchar *domain_name)
{
  if (plug_in_def->locale_domain)
    g_free (plug_in_def->locale_domain);      
  plug_in_def->locale_domain = g_strdup (domain_name);
}
  
void
plug_in_def_set_locale_domain_path (PlugInDef   *plug_in_def,
                                    const gchar *domain_path)
{
  if (plug_in_def->locale_path)
    g_free (plug_in_def->locale_path);
  plug_in_def->locale_path = g_strdup (domain_path);
}

void
plug_in_def_set_help_path (PlugInDef   *plug_in_def,
                           const gchar *help_path)
{
  if (plug_in_def->help_path)
    g_free (plug_in_def->help_path);
  plug_in_def->help_path = g_strdup (help_path);
}
