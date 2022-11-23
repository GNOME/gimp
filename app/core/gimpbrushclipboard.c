/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmabrushclipboard.c
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
#include "ligmabrush-private.h"
#include "ligmabrushclipboard.h"
#include "ligmaimage.h"
#include "ligmapickable.h"
#include "ligmatempbuf.h"

#include "ligma-intl.h"


#define BRUSH_MAX_SIZE 1024

enum
{
  PROP_0,
  PROP_LIGMA,
  PROP_MASK_ONLY
};


/*  local function prototypes  */

static void       ligma_brush_clipboard_constructed  (GObject      *object);
static void       ligma_brush_clipboard_set_property (GObject      *object,
                                                     guint         property_id,
                                                     const GValue *value,
                                                     GParamSpec   *pspec);
static void       ligma_brush_clipboard_get_property (GObject      *object,
                                                     guint         property_id,
                                                     GValue       *value,
                                                     GParamSpec   *pspec);

static LigmaData * ligma_brush_clipboard_duplicate    (LigmaData     *data);

static void       ligma_brush_clipboard_changed      (Ligma         *ligma,
                                                     LigmaBrush    *brush);


G_DEFINE_TYPE (LigmaBrushClipboard, ligma_brush_clipboard, LIGMA_TYPE_BRUSH)

#define parent_class ligma_brush_clipboard_parent_class


static void
ligma_brush_clipboard_class_init (LigmaBrushClipboardClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  LigmaDataClass *data_class   = LIGMA_DATA_CLASS (klass);

  object_class->constructed  = ligma_brush_clipboard_constructed;
  object_class->set_property = ligma_brush_clipboard_set_property;
  object_class->get_property = ligma_brush_clipboard_get_property;

  data_class->duplicate      = ligma_brush_clipboard_duplicate;

  g_object_class_install_property (object_class, PROP_LIGMA,
                                   g_param_spec_object ("ligma", NULL, NULL,
                                                        LIGMA_TYPE_LIGMA,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_MASK_ONLY,
                                   g_param_spec_boolean ("mask-only", NULL, NULL,
                                                         FALSE,
                                                         LIGMA_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));
}

static void
ligma_brush_clipboard_init (LigmaBrushClipboard *brush)
{
}

static void
ligma_brush_clipboard_constructed (GObject *object)
{
  LigmaBrushClipboard *brush = LIGMA_BRUSH_CLIPBOARD (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  ligma_assert (LIGMA_IS_LIGMA (brush->ligma));

  g_signal_connect_object (brush->ligma, "clipboard-changed",
                           G_CALLBACK (ligma_brush_clipboard_changed),
                           brush, 0);

  ligma_brush_clipboard_changed (brush->ligma, LIGMA_BRUSH (brush));
}

static void
ligma_brush_clipboard_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  LigmaBrushClipboard *brush = LIGMA_BRUSH_CLIPBOARD (object);

  switch (property_id)
    {
    case PROP_LIGMA:
      brush->ligma = g_value_get_object (value);
      break;

    case PROP_MASK_ONLY:
      brush->mask_only = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_brush_clipboard_get_property (GObject    *object,
                                   guint       property_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  LigmaBrushClipboard *brush = LIGMA_BRUSH_CLIPBOARD (object);

  switch (property_id)
    {
    case PROP_LIGMA:
      g_value_set_object (value, brush->ligma);
      break;

    case PROP_MASK_ONLY:
      g_value_set_boolean (value, brush->mask_only);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static LigmaData *
ligma_brush_clipboard_duplicate (LigmaData *data)
{
  LigmaData *new = g_object_new (LIGMA_TYPE_BRUSH, NULL);

  ligma_data_copy (new, data);

  return new;
}

LigmaData *
ligma_brush_clipboard_new (Ligma     *ligma,
                          gboolean  mask_only)
{
  const gchar *name;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);

  if (mask_only)
    name = _("Clipboard Mask");
  else
    name = _("Clipboard Image");

  return g_object_new (LIGMA_TYPE_BRUSH_CLIPBOARD,
                       "name",      name,
                       "ligma",      ligma,
                       "mask-only", mask_only,
                       NULL);
}


/*  private functions  */

static void
ligma_brush_clipboard_changed (Ligma      *ligma,
                              LigmaBrush *brush)
{
  LigmaObject *paste;
  GeglBuffer *buffer = NULL;
  gint        width;
  gint        height;

  g_clear_pointer (&brush->priv->mask,   ligma_temp_buf_unref);
  g_clear_pointer (&brush->priv->pixmap, ligma_temp_buf_unref);

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
      const Babl *format = gegl_buffer_get_format (buffer);

      width  = MIN (gegl_buffer_get_width  (buffer), BRUSH_MAX_SIZE);
      height = MIN (gegl_buffer_get_height (buffer), BRUSH_MAX_SIZE);

      brush->priv->mask = ligma_temp_buf_new (width, height,
                                             babl_format ("Y u8"));

      if (LIGMA_BRUSH_CLIPBOARD (brush)->mask_only)
        {
          guchar *p;
          gint    i;

          gegl_buffer_get (buffer,
                           GEGL_RECTANGLE (0, 0, width, height), 1.0,
                           babl_format ("Y u8"),
                           ligma_temp_buf_get_data (brush->priv->mask),
                           GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

          /*  invert the mask, it's more intuitive to think
           *  "black on white" than the other way around
           */
          for (i = 0, p = ligma_temp_buf_get_data (brush->priv->mask);
               i < width * height;
               i++, p++)
            {
              *p = 255 - *p;
            }
        }
      else
        {
          brush->priv->pixmap = ligma_temp_buf_new (width, height,
                                                   babl_format ("R'G'B' u8"));

          /*  copy the alpha channel into the brush's mask  */
          if (babl_format_has_alpha (format))
            {
              gegl_buffer_get (buffer,
                               GEGL_RECTANGLE (0, 0, width, height), 1.0,
                               babl_format ("A u8"),
                               ligma_temp_buf_get_data (brush->priv->mask),
                               GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
            }
          else
            {
              memset (ligma_temp_buf_get_data (brush->priv->mask), 255,
                      width * height);
            }

          /*  copy the color channels into the brush's pixmap  */
          gegl_buffer_get (buffer,
                           GEGL_RECTANGLE (0, 0, width, height), 1.0,
                           babl_format ("R'G'B' u8"),
                           ligma_temp_buf_get_data (brush->priv->pixmap),
                           GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
        }
    }
  else
    {
      width  = 17;
      height = 17;

      brush->priv->mask = ligma_temp_buf_new (width, height,
                                             babl_format ("Y u8"));
      ligma_temp_buf_data_clear (brush->priv->mask);
    }

  brush->priv->x_axis.x = width / 2;
  brush->priv->x_axis.y = 0;
  brush->priv->y_axis.x = 0;
  brush->priv->y_axis.y = height / 2;

  ligma_data_dirty (LIGMA_DATA (brush));
}
