/* GIMP - The GNU Image Manipulation Program
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

#include <string.h>

#include <glib-object.h>

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "base/temp-buf.h"

#include "gimppattern.h"
#include "gimppattern-load.h"

#include "gimp-intl.h"


static void       gimp_pattern_finalize        (GObject       *object);

static gint64     gimp_pattern_get_memsize     (GimpObject    *object,
                                                gint64        *gui_size);

static gboolean   gimp_pattern_get_size        (GimpViewable  *viewable,
                                                gint          *width,
                                                gint          *height);
static TempBuf  * gimp_pattern_get_new_preview (GimpViewable  *viewable,
                                                GimpContext   *context,
                                                gint           width,
                                                gint           height);
static gchar    * gimp_pattern_get_description (GimpViewable  *viewable,
                                                gchar        **tooltip);
static gchar    * gimp_pattern_get_extension   (GimpData      *data);
static GimpData * gimp_pattern_duplicate       (GimpData      *data);


G_DEFINE_TYPE (GimpPattern, gimp_pattern, GIMP_TYPE_DATA)

#define parent_class gimp_pattern_parent_class


static void
gimp_pattern_class_init (GimpPatternClass *klass)
{
  GObjectClass      *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass   *gimp_object_class = GIMP_OBJECT_CLASS (klass);
  GimpViewableClass *viewable_class    = GIMP_VIEWABLE_CLASS (klass);
  GimpDataClass     *data_class        = GIMP_DATA_CLASS (klass);

  object_class->finalize           = gimp_pattern_finalize;

  gimp_object_class->get_memsize   = gimp_pattern_get_memsize;

  viewable_class->default_stock_id = "gimp-tool-bucket-fill";
  viewable_class->get_size         = gimp_pattern_get_size;
  viewable_class->get_new_preview  = gimp_pattern_get_new_preview;
  viewable_class->get_description  = gimp_pattern_get_description;

  data_class->get_extension        = gimp_pattern_get_extension;
  data_class->duplicate            = gimp_pattern_duplicate;
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

  if (pattern->mask)
    {
      temp_buf_free (pattern->mask);
      pattern->mask = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gint64
gimp_pattern_get_memsize (GimpObject *object,
                          gint64     *gui_size)
{
  GimpPattern *pattern = GIMP_PATTERN (object);
  gint64       memsize = 0;

  if (pattern->mask)
    memsize += temp_buf_get_memsize (pattern->mask);

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static gboolean
gimp_pattern_get_size (GimpViewable *viewable,
                       gint         *width,
                       gint         *height)
{
  GimpPattern *pattern = GIMP_PATTERN (viewable);

  *width  = pattern->mask->width;
  *height = pattern->mask->height;

  return TRUE;
}

static TempBuf *
gimp_pattern_get_new_preview (GimpViewable *viewable,
                              GimpContext  *context,
                              gint          width,
                              gint          height)
{
  GimpPattern *pattern = GIMP_PATTERN (viewable);
  TempBuf     *temp_buf;
  gint         copy_width;
  gint         copy_height;

  copy_width  = MIN (width,  pattern->mask->width);
  copy_height = MIN (height, pattern->mask->height);

  temp_buf = temp_buf_new (copy_width, copy_height,
                           pattern->mask->bytes,
                           0, 0, NULL);

  temp_buf_copy_area (pattern->mask, temp_buf,
                      0, 0, copy_width, copy_height, 0, 0);

  return temp_buf;
}

static gchar *
gimp_pattern_get_description (GimpViewable  *viewable,
                              gchar        **tooltip)
{
  GimpPattern *pattern = GIMP_PATTERN (viewable);

  return g_strdup_printf ("%s (%d × %d)",
                          GIMP_OBJECT (pattern)->name,
                          pattern->mask->width,
                          pattern->mask->height);
}

static gchar *
gimp_pattern_get_extension (GimpData *data)
{
  return GIMP_PATTERN_FILE_EXTENSION;
}

static GimpData *
gimp_pattern_duplicate (GimpData *data)
{
  GimpPattern *pattern = g_object_new (GIMP_TYPE_PATTERN, NULL);

  pattern->mask = temp_buf_copy (GIMP_PATTERN (data)->mask, NULL);

  return GIMP_DATA (pattern);
}

GimpData *
gimp_pattern_new (const gchar *name)
{
  GimpPattern *pattern;
  guchar      *data;
  gint         row, col;

  g_return_val_if_fail (name != NULL, NULL);

  pattern = g_object_new (GIMP_TYPE_PATTERN,
                          "name", name,
                          NULL);

  pattern->mask = temp_buf_new (32, 32, 3, 0, 0, NULL);

  data = temp_buf_data (pattern->mask);

  for (row = 0; row < pattern->mask->height; row++)
    for (col = 0; col < pattern->mask->width; col++)
      {
        memset (data, (col % 2) && (row % 2) ? 255 : 0, 3);
        data += 3;
      }

  return GIMP_DATA (pattern);
}

GimpData *
gimp_pattern_get_standard (void)
{
  static GimpData *standard_pattern = NULL;

  if (! standard_pattern)
    {
      standard_pattern = gimp_pattern_new ("Standard");

      standard_pattern->dirty = FALSE;
      gimp_data_make_internal (standard_pattern);

      /*  set ref_count to 2 --> never swap the standard pattern  */
      g_object_ref (standard_pattern);
    }

  return standard_pattern;
}

TempBuf *
gimp_pattern_get_mask (const GimpPattern *pattern)
{
  g_return_val_if_fail (GIMP_IS_PATTERN (pattern), NULL);

  return pattern->mask;
}
