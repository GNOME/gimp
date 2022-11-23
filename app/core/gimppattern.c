/* LIGMA - The GNU Image Manipulation Program
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

#include "libligmabase/ligmabase.h"

#include "core-types.h"

#include "gegl/ligma-gegl-loops.h"

#include "ligmapattern.h"
#include "ligmapattern-load.h"
#include "ligmapattern-save.h"
#include "ligmatagged.h"
#include "ligmatempbuf.h"

#include "ligma-intl.h"


static void          ligma_pattern_tagged_iface_init (LigmaTaggedInterface  *iface);
static void          ligma_pattern_finalize          (GObject              *object);

static gint64        ligma_pattern_get_memsize       (LigmaObject           *object,
                                                     gint64               *gui_size);

static gboolean      ligma_pattern_get_size          (LigmaViewable         *viewable,
                                                     gint                 *width,
                                                     gint                 *height);
static LigmaTempBuf * ligma_pattern_get_new_preview   (LigmaViewable         *viewable,
                                                     LigmaContext          *context,
                                                     gint                  width,
                                                     gint                  height);
static gchar       * ligma_pattern_get_description   (LigmaViewable         *viewable,
                                                     gchar               **tooltip);

static const gchar * ligma_pattern_get_extension     (LigmaData             *data);
static void          ligma_pattern_copy              (LigmaData             *data,
                                                     LigmaData             *src_data);

static gchar       * ligma_pattern_get_checksum      (LigmaTagged           *tagged);


G_DEFINE_TYPE_WITH_CODE (LigmaPattern, ligma_pattern, LIGMA_TYPE_DATA,
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_TAGGED,
                                                ligma_pattern_tagged_iface_init))

#define parent_class ligma_pattern_parent_class


static void
ligma_pattern_class_init (LigmaPatternClass *klass)
{
  GObjectClass      *object_class      = G_OBJECT_CLASS (klass);
  LigmaObjectClass   *ligma_object_class = LIGMA_OBJECT_CLASS (klass);
  LigmaViewableClass *viewable_class    = LIGMA_VIEWABLE_CLASS (klass);
  LigmaDataClass     *data_class        = LIGMA_DATA_CLASS (klass);

  object_class->finalize            = ligma_pattern_finalize;

  ligma_object_class->get_memsize    = ligma_pattern_get_memsize;

  viewable_class->default_icon_name = "ligma-tool-bucket-fill";
  viewable_class->get_size          = ligma_pattern_get_size;
  viewable_class->get_new_preview   = ligma_pattern_get_new_preview;
  viewable_class->get_description   = ligma_pattern_get_description;

  data_class->save                  = ligma_pattern_save;
  data_class->get_extension         = ligma_pattern_get_extension;
  data_class->copy                  = ligma_pattern_copy;
}

static void
ligma_pattern_tagged_iface_init (LigmaTaggedInterface *iface)
{
  iface->get_checksum = ligma_pattern_get_checksum;
}

static void
ligma_pattern_init (LigmaPattern *pattern)
{
  pattern->mask = NULL;
}

static void
ligma_pattern_finalize (GObject *object)
{
  LigmaPattern *pattern = LIGMA_PATTERN (object);

  g_clear_pointer (&pattern->mask, ligma_temp_buf_unref);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gint64
ligma_pattern_get_memsize (LigmaObject *object,
                          gint64     *gui_size)
{
  LigmaPattern *pattern = LIGMA_PATTERN (object);
  gint64       memsize = 0;

  memsize += ligma_temp_buf_get_memsize (pattern->mask);

  return memsize + LIGMA_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static gboolean
ligma_pattern_get_size (LigmaViewable *viewable,
                       gint         *width,
                       gint         *height)
{
  LigmaPattern *pattern = LIGMA_PATTERN (viewable);

  *width  = ligma_temp_buf_get_width  (pattern->mask);
  *height = ligma_temp_buf_get_height (pattern->mask);

  return TRUE;
}

static LigmaTempBuf *
ligma_pattern_get_new_preview (LigmaViewable *viewable,
                              LigmaContext  *context,
                              gint          width,
                              gint          height)
{
  LigmaPattern *pattern = LIGMA_PATTERN (viewable);
  LigmaTempBuf *temp_buf;
  GeglBuffer  *src_buffer;
  GeglBuffer  *dest_buffer;
  gint         copy_width;
  gint         copy_height;

  copy_width  = MIN (width,  ligma_temp_buf_get_width  (pattern->mask));
  copy_height = MIN (height, ligma_temp_buf_get_height (pattern->mask));

  temp_buf = ligma_temp_buf_new (copy_width, copy_height,
                                ligma_temp_buf_get_format (pattern->mask));

  src_buffer  = ligma_temp_buf_create_buffer (pattern->mask);
  dest_buffer = ligma_temp_buf_create_buffer (temp_buf);

  ligma_gegl_buffer_copy (src_buffer,
                         GEGL_RECTANGLE (0, 0, copy_width, copy_height),
                         GEGL_ABYSS_NONE,
                         dest_buffer, GEGL_RECTANGLE (0, 0, 0, 0));

  g_object_unref (src_buffer);
  g_object_unref (dest_buffer);

  return temp_buf;
}

static gchar *
ligma_pattern_get_description (LigmaViewable  *viewable,
                              gchar        **tooltip)
{
  LigmaPattern *pattern = LIGMA_PATTERN (viewable);

  return g_strdup_printf ("%s (%d × %d)",
                          ligma_object_get_name (pattern),
                          ligma_temp_buf_get_width  (pattern->mask),
                          ligma_temp_buf_get_height (pattern->mask));
}

static const gchar *
ligma_pattern_get_extension (LigmaData *data)
{
  return LIGMA_PATTERN_FILE_EXTENSION;
}

static void
ligma_pattern_copy (LigmaData *data,
                   LigmaData *src_data)
{
  LigmaPattern *pattern     = LIGMA_PATTERN (data);
  LigmaPattern *src_pattern = LIGMA_PATTERN (src_data);

  g_clear_pointer (&pattern->mask, ligma_temp_buf_unref);
  pattern->mask = ligma_temp_buf_copy (src_pattern->mask);

  ligma_data_dirty (data);
}

static gchar *
ligma_pattern_get_checksum (LigmaTagged *tagged)
{
  LigmaPattern *pattern         = LIGMA_PATTERN (tagged);
  gchar       *checksum_string = NULL;

  if (pattern->mask)
    {
      GChecksum *checksum = g_checksum_new (G_CHECKSUM_MD5);

      g_checksum_update (checksum, ligma_temp_buf_get_data (pattern->mask),
                         ligma_temp_buf_get_data_size (pattern->mask));

      checksum_string = g_strdup (g_checksum_get_string (checksum));

      g_checksum_free (checksum);
    }

  return checksum_string;
}

LigmaData *
ligma_pattern_new (LigmaContext *context,
                  const gchar *name)
{
  LigmaPattern *pattern;
  guchar      *data;
  gint         row, col;

  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (name[0] != '\n', NULL);

  pattern = g_object_new (LIGMA_TYPE_PATTERN,
                          "name", name,
                          NULL);

  pattern->mask = ligma_temp_buf_new (32, 32, babl_format ("R'G'B' u8"));

  data = ligma_temp_buf_get_data (pattern->mask);

  for (row = 0; row < ligma_temp_buf_get_height (pattern->mask); row++)
    for (col = 0; col < ligma_temp_buf_get_width (pattern->mask); col++)
      {
        memset (data, (col % 2) && (row % 2) ? 255 : 0, 3);
        data += 3;
      }

  return LIGMA_DATA (pattern);
}

LigmaData *
ligma_pattern_get_standard (LigmaContext *context)
{
  static LigmaData *standard_pattern = NULL;

  if (! standard_pattern)
    {
      standard_pattern = ligma_pattern_new (context, "Standard");

      ligma_data_clean (standard_pattern);
      ligma_data_make_internal (standard_pattern, "ligma-pattern-standard");

      g_object_add_weak_pointer (G_OBJECT (standard_pattern),
                                 (gpointer *) &standard_pattern);
    }

  return standard_pattern;
}

LigmaTempBuf *
ligma_pattern_get_mask (LigmaPattern *pattern)
{
  g_return_val_if_fail (LIGMA_IS_PATTERN (pattern), NULL);

  return pattern->mask;
}

GeglBuffer *
ligma_pattern_create_buffer (LigmaPattern *pattern)
{
  g_return_val_if_fail (LIGMA_IS_PATTERN (pattern), NULL);

  return ligma_temp_buf_create_buffer (pattern->mask);
}
