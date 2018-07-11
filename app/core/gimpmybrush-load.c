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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include <mypaint-brush.h>

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
  GimpMybrush  *brush = NULL;
  MyPaintBrush *mypaint_brush;
  GdkPixbuf    *pixbuf;
  GFileInfo    *info;
  guint64       size;
  guchar       *buffer;
  gchar        *path;
  gchar        *basename;
  gchar        *preview_filename;
  gchar        *p;

  g_return_val_if_fail (G_IS_FILE (file), NULL);
  g_return_val_if_fail (G_IS_INPUT_STREAM (input), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  info = g_file_query_info (file,
                            G_FILE_ATTRIBUTE_STANDARD_SIZE,
                            G_FILE_QUERY_INFO_NONE,
                            NULL, error);
  if (! info)
    return NULL;

  size = g_file_info_get_attribute_uint64 (info,
                                           G_FILE_ATTRIBUTE_STANDARD_SIZE);
  g_object_unref (info);

  if (size > 32768)
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("MyPaint brush file is unreasonably large, skipping."));
      return NULL;
    }

  buffer = g_new0 (guchar, size + 1);

  if (! g_input_stream_read_all (input, buffer, size, NULL, NULL, error))
    {
      g_free (buffer);
      return NULL;
    }

  mypaint_brush = mypaint_brush_new ();
  mypaint_brush_from_defaults (mypaint_brush);

  if (! mypaint_brush_from_string (mypaint_brush, (const gchar *) buffer))
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("Failed to deserialize MyPaint brush."));
      mypaint_brush_unref (mypaint_brush);
      g_free (buffer);
      return NULL;
    }

  path = g_file_get_path (file);
  basename = g_strndup (path, strlen (path) - 4);
  g_free (path);

  preview_filename = g_strconcat (basename, "_prev.png", NULL);
  g_free (basename);

  pixbuf = gdk_pixbuf_new_from_file_at_size (preview_filename,
                                             48, 48, error);
  g_free (preview_filename);

  basename = g_path_get_basename (gimp_file_get_utf8_name (file));

  basename[strlen (basename) - 4] = '\0';
  for (p = basename; *p; p++)
    if (*p == '_' || *p == '-')
      *p = ' ';

  brush = g_object_new (GIMP_TYPE_MYBRUSH,
                        "name",        basename,
                        "mime-type",   "image/x-gimp-myb",
                        "icon-pixbuf", pixbuf,
                        NULL);

  g_free (basename);

  if (pixbuf)
    g_object_unref (pixbuf);

  brush->priv->brush_json = (gchar *) buffer;

  brush->priv->radius =
    mypaint_brush_get_base_value (mypaint_brush,
                                  MYPAINT_BRUSH_SETTING_RADIUS_LOGARITHMIC);

  brush->priv->opaque =
    mypaint_brush_get_base_value (mypaint_brush,
                                  MYPAINT_BRUSH_SETTING_OPAQUE);

  brush->priv->hardness =
    mypaint_brush_get_base_value (mypaint_brush,
                                  MYPAINT_BRUSH_SETTING_HARDNESS);

  brush->priv->eraser =
    mypaint_brush_get_base_value (mypaint_brush,
                                  MYPAINT_BRUSH_SETTING_ERASER) > 0.5f;

  brush->priv->offset_by_random =
    mypaint_brush_get_base_value (mypaint_brush,
                                  MYPAINT_BRUSH_SETTING_OFFSET_BY_RANDOM);

  mypaint_brush_unref (mypaint_brush);

  return g_list_prepend (NULL, brush);
}
