/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * plug-in-def.c
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

#include "gimppluginprocedure.h"
#include "plug-in-def.h"


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
plug_in_def_free (PlugInDef *plug_in_def)
{
  g_return_if_fail (plug_in_def != NULL);

  g_free (plug_in_def->prog);
  g_free (plug_in_def->locale_domain_name);
  g_free (plug_in_def->locale_domain_path);
  g_free (plug_in_def->help_domain_name);
  g_free (plug_in_def->help_domain_uri);

  g_slist_foreach (plug_in_def->procedures, (GFunc) g_object_unref, NULL);
  g_slist_free (plug_in_def->procedures);

  g_free (plug_in_def);
}

void
plug_in_def_add_procedure (PlugInDef           *plug_in_def,
                           GimpPlugInProcedure *proc)
{
  g_return_if_fail (plug_in_def != NULL);
  g_return_if_fail (GIMP_IS_PLUG_IN_PROCEDURE (proc));

  proc->mtime = plug_in_def->mtime;

  plug_in_def->procedures = g_slist_append (plug_in_def->procedures,
                                            g_object_ref (proc));
}

void
plug_in_def_remove_procedure (PlugInDef           *plug_in_def,
                              GimpPlugInProcedure *proc)
{
  g_return_if_fail (plug_in_def != NULL);
  g_return_if_fail (GIMP_IS_PLUG_IN_PROCEDURE (proc));

  plug_in_def->procedures = g_slist_remove (plug_in_def->procedures, proc);
  g_object_unref (proc);
}

void
plug_in_def_set_locale_domain_name (PlugInDef   *plug_in_def,
                                    const gchar *domain_name)
{
  g_return_if_fail (plug_in_def != NULL);

  if (plug_in_def->locale_domain_name)
    g_free (plug_in_def->locale_domain_name);
  plug_in_def->locale_domain_name = g_strdup (domain_name);
}

void
plug_in_def_set_locale_domain_path (PlugInDef   *plug_in_def,
                                    const gchar *domain_path)
{
  g_return_if_fail (plug_in_def != NULL);

  if (plug_in_def->locale_domain_path)
    g_free (plug_in_def->locale_domain_path);
  plug_in_def->locale_domain_path = g_strdup (domain_path);
}

void
plug_in_def_set_help_domain_name (PlugInDef   *plug_in_def,
                                  const gchar *domain_name)
{
  g_return_if_fail (plug_in_def != NULL);

  if (plug_in_def->help_domain_name)
    g_free (plug_in_def->help_domain_name);
  plug_in_def->help_domain_name = g_strdup (domain_name);
}

void
plug_in_def_set_help_domain_uri (PlugInDef   *plug_in_def,
                                 const gchar *domain_uri)
{
  g_return_if_fail (plug_in_def != NULL);

  if (plug_in_def->help_domain_uri)
    g_free (plug_in_def->help_domain_uri);
  plug_in_def->help_domain_uri = g_strdup (domain_uri);
}

void
plug_in_def_set_mtime (PlugInDef *plug_in_def,
                       time_t     mtime)
{
  GSList *list;

  g_return_if_fail (plug_in_def != NULL);

  plug_in_def->mtime = mtime;

  for (list = plug_in_def->procedures; list; list = g_slist_next (list))
    {
      GimpPlugInProcedure *proc = list->data;

      proc->mtime = plug_in_def->mtime;
    }
}

void
plug_in_def_set_needs_query (PlugInDef *plug_in_def,
                             gboolean   needs_query)
{
  g_return_if_fail (plug_in_def != NULL);

  plug_in_def->needs_query = needs_query ? TRUE : FALSE;
}

void
plug_in_def_set_has_init (PlugInDef *plug_in_def,
                          gboolean   has_init)
{
  g_return_if_fail (plug_in_def != NULL);

  plug_in_def->has_init = has_init ? TRUE : FALSE;
}
