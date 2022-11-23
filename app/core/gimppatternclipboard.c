/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmapatternclipboard.c
 * Copyright (C) 2006 Michael Natterer <mitch@ligma.org>
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

#include "libligmabase/ligmabase.h"

#include "core-types.h"

#include "ligma.h"
#include "ligmabuffer.h"
#include "ligmapatternclipboard.h"
#include "ligmaimage.h"
#include "ligmapickable.h"
#include "ligmatempbuf.h"

#include "ligma-intl.h"


#define PATTERN_MAX_SIZE 1024

enum
{
  PROP_0,
  PROP_LIGMA
};


/*  local function prototypes  */

static void       ligma_pattern_clipboard_constructed  (GObject      *object);
static void       ligma_pattern_clipboard_set_property (GObject      *object,
                                                       guint         property_id,
                                                       const GValue *value,
                                                       GParamSpec   *pspec);
static void       ligma_pattern_clipboard_get_property (GObject      *object,
                                                       guint         property_id,
                                                       GValue       *value,
                                                       GParamSpec   *pspec);
static LigmaData * ligma_pattern_clipboard_duplicate    (LigmaData     *data);

static void       ligma_pattern_clipboard_changed      (Ligma         *ligma,
                                                       LigmaPattern  *pattern);


G_DEFINE_TYPE (LigmaPatternClipboard, ligma_pattern_clipboard, LIGMA_TYPE_PATTERN)

#define parent_class ligma_pattern_clipboard_parent_class


static void
ligma_pattern_clipboard_class_init (LigmaPatternClipboardClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  LigmaDataClass *data_class   = LIGMA_DATA_CLASS (klass);

  object_class->constructed  = ligma_pattern_clipboard_constructed;
  object_class->set_property = ligma_pattern_clipboard_set_property;
  object_class->get_property = ligma_pattern_clipboard_get_property;

  data_class->duplicate      = ligma_pattern_clipboard_duplicate;

  g_object_class_install_property (object_class, PROP_LIGMA,
                                   g_param_spec_object ("ligma", NULL, NULL,
                                                        LIGMA_TYPE_LIGMA,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
ligma_pattern_clipboard_init (LigmaPatternClipboard *pattern)
{
}

static void
ligma_pattern_clipboard_constructed (GObject *object)
{
  LigmaPatternClipboard *pattern = LIGMA_PATTERN_CLIPBOARD (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  ligma_assert (LIGMA_IS_LIGMA (pattern->ligma));

  g_signal_connect_object (pattern->ligma, "clipboard-changed",
                           G_CALLBACK (ligma_pattern_clipboard_changed),
                           pattern, 0);

  ligma_pattern_clipboard_changed (pattern->ligma, LIGMA_PATTERN (pattern));
}

static void
ligma_pattern_clipboard_set_property (GObject      *object,
                                     guint         property_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  LigmaPatternClipboard *pattern = LIGMA_PATTERN_CLIPBOARD (object);

  switch (property_id)
    {
    case PROP_LIGMA:
      pattern->ligma = g_value_get_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_pattern_clipboard_get_property (GObject    *object,
                                     guint       property_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  LigmaPatternClipboard *pattern = LIGMA_PATTERN_CLIPBOARD (object);

  switch (property_id)
    {
    case PROP_LIGMA:
      g_value_set_object (value, pattern->ligma);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static LigmaData *
ligma_pattern_clipboard_duplicate (LigmaData *data)
{
  LigmaData *new = g_object_new (LIGMA_TYPE_PATTERN, NULL);

  ligma_data_copy (new, data);

  return new;
}

LigmaData *
ligma_pattern_clipboard_new (Ligma *ligma)
{
  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);

  return g_object_new (LIGMA_TYPE_PATTERN_CLIPBOARD,
                       "name", _("Clipboard Image"),
                       "ligma", ligma,
                       NULL);
}


/*  private functions  */

static void
ligma_pattern_clipboard_changed (Ligma        *ligma,
                                LigmaPattern *pattern)
{
  LigmaObject *paste;
  GeglBuffer *buffer = NULL;

  g_clear_pointer (&pattern->mask, ligma_temp_buf_unref);

  paste = ligma_get_clipboard_object (ligma);

  if (LIGMA_IS_IMAGE (paste))
    {
      ligma_pickable_flush (LIGMA_PICKABLE (paste));
      buffer = ligma_pickable_get_buffer (LIGMA_PICKABLE (paste));
    }
  else if (LIGMA_IS_BUFFER (paste))
    {
      buffer = ligma_buffer_get_buffer (LIGMA_BUFFER (paste));
    }

  if (buffer)
    {
      gint width  = MIN (gegl_buffer_get_width  (buffer), PATTERN_MAX_SIZE);
      gint height = MIN (gegl_buffer_get_height (buffer), PATTERN_MAX_SIZE);

      pattern->mask = ligma_temp_buf_new (width, height,
                                         gegl_buffer_get_format (buffer));

      gegl_buffer_get (buffer,
                       GEGL_RECTANGLE (0, 0, width, height), 1.0,
                       NULL,
                       ligma_temp_buf_get_data (pattern->mask),
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
    }
  else
    {
      pattern->mask = ligma_temp_buf_new (16, 16, babl_format ("R'G'B' u8"));
      memset (ligma_temp_buf_get_data (pattern->mask), 255, 16 * 16 * 3);
    }

  ligma_data_dirty (LIGMA_DATA (pattern));
}
