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

#include <gegl.h>

#include "core-types.h"

#include "base/temp-buf.h"

#include "gegl/gimp-gegl-utils.h"

#include "gimp.h"
#include "gimpbuffer.h"
#include "gimpbrushclipboard.h"
#include "gimpimage.h"

#include "gimp-intl.h"


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

static void     gimp_brush_clipboard_buffer_changed (Gimp         *gimp,
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
  brush->gimp = NULL;
}

static void
gimp_brush_clipboard_constructed (GObject *object)
{
  GimpBrushClipboard *brush = GIMP_BRUSH_CLIPBOARD (object);

  if (G_OBJECT_CLASS (parent_class)->constructed)
    G_OBJECT_CLASS (parent_class)->constructed (object);

  g_assert (GIMP_IS_GIMP (brush->gimp));

  g_signal_connect_object (brush->gimp, "buffer-changed",
                           G_CALLBACK (gimp_brush_clipboard_buffer_changed),
                           brush, 0);

  gimp_brush_clipboard_buffer_changed (brush->gimp, GIMP_BRUSH (brush));
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
gimp_brush_clipboard_buffer_changed (Gimp      *gimp,
                                     GimpBrush *brush)
{
  gint width;
  gint height;

  if (brush->mask)
    {
      temp_buf_free (brush->mask);
      brush->mask = NULL;
    }

  if (brush->pixmap)
    {
      temp_buf_free (brush->pixmap);
      brush->pixmap = NULL;
    }

  if (gimp->global_buffer)
    {
      GeglBuffer    *buffer = gimp_buffer_get_buffer (gimp->global_buffer);
      GeglBuffer    *dest_buffer;
      GeglRectangle  rect   = { 0, };
      GimpImageType  type   = gimp_buffer_get_image_type (gimp->global_buffer);

      width  = MIN (gimp_buffer_get_width  (gimp->global_buffer), 512);
      height = MIN (gimp_buffer_get_height (gimp->global_buffer), 512);

      rect.width  = width;
      rect.height = height;

      brush->mask   = temp_buf_new (width, height, 1, 0, 0, NULL);
      brush->pixmap = temp_buf_new (width, height, 3, 0, 0, NULL);

      /*  copy the alpha channel into the brush's mask  */
      if (GIMP_IMAGE_TYPE_HAS_ALPHA (type))
        {
          dest_buffer =
            gegl_buffer_linear_new_from_data (temp_buf_get_data (brush->mask),
                                              babl_format ("A u8"),
                                              &rect,
                                              width * brush->mask->bytes,
                                              NULL, NULL);

          gegl_buffer_copy (buffer, NULL, dest_buffer, NULL);

          g_object_unref (dest_buffer);
        }
      else
        {
          memset (temp_buf_get_data (brush->mask), OPAQUE_OPACITY,
                  width * height);
        }

      /*  copy the color channels into the brush's pixmap  */
      dest_buffer =
        gegl_buffer_linear_new_from_data (temp_buf_get_data (brush->pixmap),
                                          babl_format ("RGB u8"),
                                          &rect,
                                          width * brush->pixmap->bytes,
                                          NULL, NULL);

      gegl_buffer_copy (buffer, NULL, dest_buffer, NULL);

      g_object_unref (dest_buffer);
    }
  else
    {
      guchar color = 0;

      width  = 17;
      height = 17;

      brush->mask = temp_buf_new (width, height, 1, 0, 0, &color);
    }

  brush->x_axis.x = width / 2;
  brush->x_axis.y = 0;
  brush->y_axis.x = 0;
  brush->y_axis.y = height / 2;

  gimp_data_dirty (GIMP_DATA (brush));
}
