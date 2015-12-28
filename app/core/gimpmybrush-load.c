/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpmybrush-load.c
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "gimpmybrush.h"
#include "gimpmybrush-load.h"
#include "gimpmybrush-private.h"

#include "gimp-intl.h"


/*  public functions  */

GList *
gimp_mybrush_load (GimpContext   *context,
                   GFile         *file,
                   GInputStream  *input,
                   GError       **error)
{
  GimpBrush *brush = NULL;
  GdkPixbuf *pixbuf;
  gchar     *path;
  gchar     *basename;
  gchar     *preview_filename;

  g_return_val_if_fail (G_IS_FILE (file), NULL);
  g_return_val_if_fail (G_IS_INPUT_STREAM (input), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  path = g_file_get_path (file);
  basename = g_strndup (path, strlen (path) - 4);
  g_free (path);

  preview_filename = g_strconcat (basename, "_prev.png", NULL);
  g_free (basename);

  pixbuf = gdk_pixbuf_new_from_file_at_size (preview_filename,
                                             48, 48, error);
  g_free (preview_filename);

  if (pixbuf)
    {
      basename = g_file_get_basename (file);

      brush = g_object_new (GIMP_TYPE_MYBRUSH,
                            "name",        gimp_filename_to_utf8 (basename),
                            "mime-type",   "image/x-gimp-myb",
                            "icon-pixbuf", pixbuf,
                            NULL);

      g_free (basename);
      g_object_unref (pixbuf);
    }

  if (! brush)
    return NULL;

  return g_list_prepend (NULL, brush);
}
