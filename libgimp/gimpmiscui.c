/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpmiscui.c
 * Contains all kinds of miscellaneous routines factored out from different
 * plug-ins. They stay here until their API has crystalized a bit and we can
 * put them into the file where they belong (Maurits Rijk
 * <lpeek.mrijk@consunet.nl> if you want to blame someone for this mess)
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

#include "config.h"

#include <string.h>

#ifdef __GNUC__
#warning GTK_DISABLE_DEPRECATED
#endif
#undef GTK_DISABLE_DEPRECATED
#include <gtk/gtk.h>

#include "gimp.h"
#include "gimpmiscui.h"

#include "libgimp-intl.h"

gchar *
gimp_plug_in_get_path (const gchar *path_name,
                       const gchar *dir_name)
{
  gchar *path;

  g_return_val_if_fail (path_name != NULL, NULL);
  g_return_val_if_fail (dir_name != NULL, NULL);

  path = gimp_gimprc_query (path_name);

  if (! path)
    {
      gchar *gimprc = gimp_personal_rc_file ("gimprc");
      gchar *full_path;
      gchar *esc_path;

      full_path =
        g_strconcat ("${gimp_dir}", G_DIR_SEPARATOR_S, dir_name,
                     G_SEARCHPATH_SEPARATOR_S,
                     "${gimp_data_dir}", G_DIR_SEPARATOR_S, dir_name,
                     NULL);
      esc_path = g_strescape (full_path, NULL);
      g_free (full_path);

      g_message (_("No %s in gimprc:\n"
                   "You need to add an entry like\n"
                   "(%s \"%s\")\n"
                   "to your %s file."),
                 path_name, path_name, esc_path, gimprc);

      g_free (gimprc);
      g_free (esc_path);
    }

  return path;
}

