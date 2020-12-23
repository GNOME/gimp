/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-2003 Spencer Kimball and Peter Mattis
 *
 * plug-ins-query.c
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"

#include "plug-in-types.h"

#include "gimppluginmanager.h"
#include "gimppluginmanager-query.h"
#include "gimppluginprocedure.h"


static gboolean
match_string (GRegex *regex,
              gchar  *string)
{
  return g_regex_match (regex, string, 0, NULL);
}

gint
gimp_plug_in_manager_query (GimpPlugInManager   *manager,
                            const gchar         *search_str,
                            gchar             ***procedure_strs,
                            gchar             ***accel_strs,
                            gchar             ***prog_strs,
                            gint32             **time_ints)
{
  gint     num_plugins = 0;
  GRegex  *sregex      = NULL;
  GSList  *matched     = NULL;
  GSList  *list;
  gint     i;

  g_return_val_if_fail (GIMP_IS_PLUG_IN_MANAGER (manager), 0);
  g_return_val_if_fail (procedure_strs != NULL, 0);
  g_return_val_if_fail (accel_strs != NULL, 0);
  g_return_val_if_fail (prog_strs != NULL, 0);
  g_return_val_if_fail (time_ints != NULL, 0);

  *procedure_strs = NULL;
  *accel_strs     = NULL;
  *prog_strs      = NULL;
  *time_ints      = NULL;

  if (search_str && ! strlen (search_str))
    search_str = NULL;

  if (search_str)
    {
      sregex = g_regex_new (search_str, G_REGEX_CASELESS | G_REGEX_OPTIMIZE, 0,
                            NULL);
      if (! sregex)
        return 0;
    }

  /* count number of plugin entries, then allocate arrays of correct size
   * where we can store the strings.
   */

  for (list = manager->plug_in_procedures; list; list = g_slist_next (list))
    {
      GimpPlugInProcedure *proc = list->data;

      if (proc->file && proc->menu_paths)
        {
          gchar *name = proc->menu_label;

          name = gimp_strip_uline (name);

          if (! search_str || match_string (sregex, name))
            {
              num_plugins++;
              matched = g_slist_prepend (matched, proc);
            }

          g_free (name);
        }
    }

  *procedure_strs = g_new0 (gchar *, num_plugins + 1);
  *accel_strs     = g_new0 (gchar *, num_plugins + 1);
  *prog_strs      = g_new0 (gchar *, num_plugins + 1);
  *time_ints      = g_new (gint,    num_plugins);

  matched = g_slist_reverse (matched);

  for (list = matched, i = 0;
       list;
       list = g_slist_next (list), i++)
    {
      GimpPlugInProcedure *proc = list->data;

      (*procedure_strs)[i] = g_strdup (gimp_object_get_name (proc));
      (*accel_strs)[i]     = g_strdup ("");
      (*prog_strs)[i]      = g_file_get_path (proc->file);
      (*time_ints)[i]      = proc->mtime;
    }

  g_slist_free (matched);

  if (sregex)
    g_regex_unref (sregex);

  return num_plugins;
}
