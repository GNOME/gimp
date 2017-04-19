/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpbrushclipboard.c
 * Copyright (C) 2006 Michael Natterer <mitch@gimp.org>
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

#include "core-types.h"

#include "gimp.h"
#include "gimpbuffer.h"
#include "gimpbrush-private.h"
#include "gimpbrushclipboard.h"
#include "gimpimage.h"
#include "gimppickable.h"
#include "gimptempbuf.h"

#include "gimp-intl.h"


#define BRUSH_MAX_SIZE 1024

enum
{
  PROP_0,
  PROP_GIMP
};


/*  local function prototypes  */

static void       gimp_brush_clipboard_constructed  (GObject      *object);
static void       gimp_brush_clipboard_set_property (GObject      *object,
                                                     guint         property_id,
                                                     const GValue *value,
                                                     GParamSpec   *pspec);
static void       gimp_brush_clipboard_get_property (GObject      *object,
                                                     guint         property_id,
                                                     GValue       *value,
                                                     GParamSpec   *pspec);
#if 0
static GimpData * gimp_brush_clipboard_duplicate    (GimpData     *data);
#endif

static void       gimp_brush_clipboard_changed      (Gimp         *gimp,
                                                     GimpBrush    *brush);


G_DEFINE_TYPE (GimpBrushClipboard, gimp_brush_clipboard, GIMP_TYPE_BRUSH)

#define parent_class gimp_brush_clipboard_parent_class


static void
gimp_brush_clipboard_class_init (GimpBrushClipboardClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
#if 0
  GimpDataClass *data_class   = GIMP_DATA_CLASS (klass);
#endif

  object_class->constructed  = gimp_brush_clipboard_constructed;
  object_class->set_property = gimp_brush_clipboard_set_property;
  object_class->get_property = gimp_brush_clipboard_get_property;

#if 0
  data_class->duplicate      = gimp_brush_clipboard_duplicate;
#endif

  g_object_class_install_property (object_class, PROP_GIMP,
                                   g_param_spec_object ("gimp", NULL, NULL,
                                                        GIMP_TYPE_GIMP,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
gimp_brush_clipboard_init (GimpBrushClipboard *brush)
{
}

static void
gimp_brush_clipboard_constructed (GObject *object)
{
  GimpBrushClipboard *brush = GIMP_BRUSH_CLIPBOARD (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  g_assert (GIMP_IS_GIMP (brush->gimp));

  g_signal_connect_object (brush->gimp, "clipboard-changed",
                           G_CALLBACK (gimp_brush_clipboard_changed),
                           brush, 0);

  gimp_brush_clipboard_changed (brush->gimp, GIMP_BRUSH (brush));
}

static void
gimp_brush_clipboard_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  GimpBrushClipboard *brush = GIMP_BRUSH_CLIPBOARD (object);

  switch (property_id)
    {
    case PROP_GIMP:
      brush->gimp = g_value_get_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_brush_clipboard_get_property (GObject    *object,
                                   guint       property_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  GimpBrushClipboard *brush = GIMP_BRUSH_CLIPBOARD (object);

  switch (property_id)
    {
    case PROP_GIMP:
      g_value_set_object (value, brush->gimp);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

#if 0
static GimpData *
gimp_brush_clipboard_duplicate (GimpData *data)
{
  GimpBrushClipboard *brush = GIMP_BRUSH_CLIPBOARD (data);

  return gimp_brush_clipboard_new (brush->gimp);
}
#endif

GimpData *
gimp_brush_clipboard_new (Gimp *gimp)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  return g_object_new (GIMP_TYPE_BRUSH_CLIPBOARD,
                       "name", _("Clipboard"),
                       "gimp", gimp,
                       NULL);
}


/*  private functions  */

static void
gimp_brush_clipboard_changed (Gimp      *gimp,
                              GimpBrush *brush)
{
  GimpObject *paste;
  GeglBuffer *buffer = NULL;
  gint        width;
  gint        height;

  if (brush->priv->mask)
    {
      gimp_temp_buf_unref (brush->priv->mask);
      brush->priv->mask = NULL;
    }

  if (brush->priv->pixmap)
    {
      gimp_temp_buf_unref (brush->priv->pixmap);
      brush->priv->pixmap = NULL;
    }

  paste = gimp_get_clipboard_object (gimp);

  if (GIMP_IS_IMAGE (paste))
    {
      gimp_pickable_flush (GIMP_PICKABLE (paste));
      buffer = gimp_pickable_get_buffer (GIMP_PICKABLE (paste));
    }
  else if (GIMP_IS_BUFFER (paste))
    {
      buffer = gimp_buffer_get_buffer (GIMP_BUFFER (paste));
    }

  if (buffer)
    {
      const Babl *format = gegl_buffer_get_format (buffer);
      GeglBuffer *dest_buffer;

      width  = MIN (gegl_buffer_get_width  (buffer), BRUSH_MAX_SIZE);
      height = MIN (gegl_buffer_get_height (buffer), BRUSH_MAX_SIZE);

      brush->priv->mask   = gimp_temp_buf_new (width, height,
                                               babl_format ("Y u8"));
      brush->priv->pixmap = gimp_temp_buf_new (width, height,
                                               babl_format ("R'G'B' u8"));

      /*  copy the alpha channel into the brush's mask  */
      if (babl_format_has_alpha (format))
        {
          dest_buffer = gimp_temp_buf_create_buffer (brush->priv->mask);

          gegl_buffer_set_format (dest_buffer, babl_format ("A u8"));
          gegl_buffer_copy (buffer, NULL, GEGL_ABYSS_NONE,
                            dest_buffer, NULL);

          g_object_unref (dest_buffer);
        }
      else
        {
          memset (gimp_temp_buf_get_data (brush->priv->mask), 255,
                  width * height);
        }

      /*  copy the color channels into the brush's pixmap  */
      dest_buffer = gimp_temp_buf_create_buffer (brush->priv->pixmap);

      gegl_buffer_copy (buffer, NULL, GEGL_ABYSS_NONE,
                        dest_buffer, NULL);

      g_object_unref (dest_buffer);
    }
  else
    {
      width  = 17;
      height = 17;

      brush->priv->mask = gimp_temp_buf_new (width, height,
                                             babl_format ("Y u8"));
      gimp_temp_buf_data_clear (brush->priv->mask);
    }

  brush->priv->x_axis.x = width / 2;
  brush->priv->x_axis.y = 0;
  brush->priv->y_axis.x = 0;
  brush->priv->y_axis.y = height / 2;

  gimp_data_dirty (GIMP_DATA (brush));
}
