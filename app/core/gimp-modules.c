/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpmodules.c
 * (C) 1999 Austin Donnelly <austin@gimp.org>
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

#include <stdio.h>
#include <string.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <time.h>

#include <glib-object.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmodule/gimpmodule.h"

#include "core-types.h"

#include "config/gimpcoreconfig.h"

#include "gimp.h"
#include "gimplist.h"
#include "gimpmodules.h"

#include "libgimp/gimpintl.h"


void
gimp_modules_init (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  gimp->module_db = gimp_module_db_new (gimp->be_verbose);
  gimp->write_modulerc = FALSE;
}

void
gimp_modules_exit (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  if (gimp->module_db)
    {
      g_object_unref (G_OBJECT (gimp->module_db));
      gimp->module_db = NULL;
    }
}

void
gimp_modules_load (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

#if 0
  gchar *filename = gimp_personal_rc_file ("modulerc");
  gimprc_parse_file (filename);
  g_free (filename);
#endif

  gimp_module_db_set_load_inhibit (gimp->module_db,
                                   gimp->config->module_load_inhibit);

  gimp_module_db_load (gimp->module_db,
                       gimp->config->module_path);
}

static void
add_to_inhibit_string (gpointer data, 
		       gpointer user_data)
{
  GimpModule *module = data;
  GString    *str    = user_data;

  if (module->load_inhibit)
    {
      str = g_string_append_c (str, G_SEARCHPATH_SEPARATOR);
      str = g_string_append (str, module->filename);
    }
}

void
gimp_modules_unload (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  if (gimp->write_modulerc)
    {
      GString *str;
      gchar   *p;
      gchar   *filename;
      FILE    *fp;

      str = g_string_new (NULL);
      g_list_foreach (gimp->module_db->modules, add_to_inhibit_string, str);
      if (str->len > 0)
        p = str->str + 1;
      else
        p = "";

      filename = gimp_personal_rc_file ("modulerc");
      fp = fopen (filename, "wt");
      g_free (filename);

      if (fp)
        {
          fprintf (fp, "(module-load-inhibit \"%s\")\n", p);
          fclose (fp);

          gimp->write_modulerc = FALSE;
        }

      g_string_free (str, TRUE);
    }
}

void
gimp_modules_refresh (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  gimp_module_db_set_load_inhibit (gimp->module_db,
                                   gimp->config->module_load_inhibit);

  gimp_module_db_refresh (gimp->module_db,
                          gimp->config->module_path);
}
