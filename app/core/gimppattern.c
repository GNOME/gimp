/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include "core-types.h"

#include "gegl/gimp-gegl-loops.h"

#include "gimppattern.h"
#include "gimppattern-load.h"
#include "gimppattern-save.h"
#include "gimptagged.h"
#include "gimptempbuf.h"

#include "gimp-intl.h"


static void          gimp_pattern_tagged_iface_init (GimpTaggedInterface  *iface);
static void          gimp_pattern_finalize          (GObject              *object);

static gint64        gimp_pattern_get_memsize       (GimpObject           *object,
                                                     gint64               *gui_size);

static gboolean      gimp_pattern_get_size          (GimpViewable         *viewable,
                                                     gint                 *width,
                                                     gint                 *height);
static GimpTempBuf * gimp_pattern_get_new_preview   (GimpViewable         *viewable,
                                                     GimpContext          *context,
                                                     gint                  width,
                                                     gint                  height);
static gchar       * gimp_pattern_get_description   (GimpViewable         *viewable,
                                                     gchar               **tooltip);

static const gchar * gimp_pattern_get_extension     (GimpData             *data);
static void          gimp_pattern_copy              (GimpData             *data,
                                                     GimpData             *src_data);

static gchar       * gimp_pattern_get_checksum      (GimpTagged           *tagged);


G_DEFINE_TYPE_WITH_CODE (GimpPattern, gimp_pattern, GIMP_TYPE_DATA,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_TAGGED,
                                                gimp_pattern_tagged_iface_init))

#define parent_class gimp_pattern_parent_class


static void
gimp_pattern_class_init (GimpPatternClass *klass)
{
  GObjectClass      *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass   *gimp_object_class = GIMP_OBJECT_CLASS (klass);
  GimpViewableClass *viewable_class    = GIMP_VIEWABLE_CLASS (klass);
  GimpDataClass     *data_class        = GIMP_DATA_CLASS (klass);

  object_class->finalize            = gimp_pattern_finalize;

  gimp_object_class->get_memsize    = gimp_pattern_get_memsize;

  viewable_class->default_icon_name = "gimp-tool-bucket-fill";
  viewable_class->get_size          = gimp_pattern_get_size;
  viewable_class->get_new_preview   = gimp_pattern_get_new_preview;
  viewable_class->get_description   = gimp_pattern_get_description;

  data_class->save                  = gimp_pattern_save;
  data_class->get_extension         = gimp_pattern_get_extension;
  data_class->copy                  = gimp_pattern_copy;
}

static void
gimp_pattern_tagged_iface_init (GimpTaggedInterface *iface)
{
  iface->get_checksum = gimp_pattern_get_checksum;
}

static void
gimp_pattern_init (GimpPattern *pattern)
{
  pattern->mask = NULL;
}

static void
gimp_pattern_finalize (GObject *object)
{
  GimpPattern *pattern = GIMP_PATTERN (object);

  g_clear_pointer (&pattern->mask, gimp_temp_buf_unref);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gint64
gimp_pattern_get_memsize (GimpObject *object,
                          gint64     *gui_size)
{
  GimpPattern *pattern = GIMP_PATTERN (object);
  gint64       memsize = 0;

  memsize += gimp_temp_buf_get_memsize (pattern->mask);

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static gboolean
gimp_pattern_get_size (GimpViewable *viewable,
                       gint         *width,
                       gint         *height)
{
  GimpPattern *pattern = GIMP_PATTERN (viewable);

  *width  = gimp_temp_buf_get_width  (pattern->mask);
  *height = gimp_temp_buf_get_height (pattern->mask);

  return TRUE;
}

static GimpTempBuf *
gimp_pattern_get_new_preview (GimpViewable *viewable,
                              GimpContext  *context,
                              gint          width,
                              gint          height)
{
  GimpPattern *pattern = GIMP_PATTERN (viewable);
  GimpTempBuf *temp_buf;
  GeglBuffer  *src_buffer;
  GeglBuffer  *dest_buffer;
  gint         copy_width;
  gint         copy_height;

  copy_width  = MIN (width,  gimp_temp_buf_get_width  (pattern->mask));
  copy_height = MIN (height, gimp_temp_buf_get_height (pattern->mask));

  temp_buf = gimp_temp_buf_new (copy_width, copy_height,
                                gimp_temp_buf_get_format (pattern->mask));

  src_buffer  = gimp_temp_buf_create_buffer (pattern->mask);
  dest_buffer = gimp_temp_buf_create_buffer (temp_buf);

  gimp_gegl_buffer_copy (src_buffer,
                         GEGL_RECTANGLE (0, 0, copy_width, copy_height),
                         GEGL_ABYSS_NONE,
                         dest_buffer, GEGL_RECTANGLE (0, 0, 0, 0));

  g_object_unref (src_buffer);
  g_object_unref (dest_buffer);

  return temp_buf;
}

static gchar *
gimp_pattern_get_description (GimpViewable  *viewable,
                              gchar        **tooltip)
{
  GimpPattern *pattern = GIMP_PATTERN (viewable);

  return g_strdup_printf ("%s (%d × %d)",
                          gimp_object_get_name (pattern),
                          gimp_temp_buf_get_width  (pattern->mask),
                          gimp_temp_buf_get_height (pattern->mask));
}

static const gchar *
gimp_pattern_get_extension (GimpData *data)
{
  return GIMP_PATTERN_FILE_EXTENSION;
}

static void
gimp_pattern_copy (GimpData *data,
                   GimpData *src_data)
{
  GimpPattern *pattern     = GIMP_PATTERN (data);
  GimpPattern *src_pattern = GIMP_PATTERN (src_data);

  g_clear_pointer (&pattern->mask, gimp_temp_buf_unref);
  pattern->mask = gimp_temp_buf_copy (src_pattern->mask);

  gimp_data_dirty (data);
}

static gchar *
gimp_pattern_get_checksum (GimpTagged *tagged)
{
  GimpPattern *pattern         = GIMP_PATTERN (tagged);
  gchar       *checksum_string = NULL;

  if (pattern->mask)
    {
      GChecksum *checksum = g_checksum_new (G_CHECKSUM_MD5);

      g_checksum_update (checksum, gimp_temp_buf_get_data (pattern->mask),
                         gimp_temp_buf_get_data_size (pattern->mask));

      checksum_string = g_strdup (g_checksum_get_string (checksum));

      g_checksum_free (checksum);
    }

  return checksum_string;
}

GimpData *
gimp_pattern_new (GimpContext *context,
                  const gchar *name)
{
  GimpPattern *pattern;
  guchar      *data;
  gint         row, col;

  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (name[0] != '\n', NULL);

  pattern = g_object_new (GIMP_TYPE_PATTERN,
                          "name", name,
                          NULL);

  pattern->mask = gimp_temp_buf_new (32, 32, babl_format ("R'G'B' u8"));

  data = gimp_temp_buf_get_data (pattern->mask);

  for (row = 0; row < gimp_temp_buf_get_height (pattern->mask); row++)
    for (col = 0; col < gimp_temp_buf_get_width (pattern->mask); col++)
      {
        memset (data, (col % 2) && (row % 2) ? 255 : 0, 3);
        data += 3;
      }

  return GIMP_DATA (pattern);
}

GimpData *
gimp_pattern_get_standard (GimpContext *context)
{
  static GimpData *standard_pattern = NULL;

  if (! standard_pattern)
    {
      standard_pattern = gimp_pattern_new (context, "Standard");

      gimp_data_clean (standard_pattern);
      gimp_data_make_internal (standard_pattern, "gimp-pattern-standard");

      g_object_add_weak_pointer (G_OBJECT (standard_pattern),
                                 (gpointer *) &standard_pattern);
    }

  return standard_pattern;
}

GimpTempBuf *
gimp_pattern_get_mask (GimpPattern *pattern)
{
  g_return_val_if_fail (GIMP_IS_PATTERN (pattern), NULL);

  return pattern->mask;
}

GeglBuffer *
gimp_pattern_create_buffer (GimpPattern *pattern)
{
  g_return_val_if_fail (GIMP_IS_PATTERN (pattern), NULL);

  return gimp_temp_buf_create_buffer (pattern->mask);
}
