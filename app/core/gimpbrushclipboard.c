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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"

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
  PROP_GIMP,
  PROP_MASK_ONLY
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

static GimpData * gimp_brush_clipboard_duplicate    (GimpData     *data);

static void       gimp_brush_clipboard_changed      (Gimp         *gimp,
                                                     GimpBrush    *brush);


G_DEFINE_TYPE (GimpBrushClipboard, gimp_brush_clipboard, GIMP_TYPE_BRUSH)

#define parent_class gimp_brush_clipboard_parent_class


static void
gimp_brush_clipboard_class_init (GimpBrushClipboardClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  GimpDataClass *data_class   = GIMP_DATA_CLASS (klass);

  object_class->constructed  = gimp_brush_clipboard_constructed;
  object_class->set_property = gimp_brush_clipboard_set_property;
  object_class->get_property = gimp_brush_clipboard_get_property;

  data_class->duplicate      = gimp_brush_clipboard_duplicate;

  g_object_class_install_property (object_class, PROP_GIMP,
                                   g_param_spec_object ("gimp", NULL, NULL,
                                                        GIMP_TYPE_GIMP,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_MASK_ONLY,
                                   g_param_spec_boolean ("mask-only", NULL, NULL,
                                                         FALSE,
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

  gimp_assert (GIMP_IS_GIMP (brush->gimp));

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

    case PROP_MASK_ONLY:
      brush->mask_only = g_value_get_boolean (value);
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

    case PROP_MASK_ONLY:
      g_value_set_boolean (value, brush->mask_only);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static GimpData *
gimp_brush_clipboard_duplicate (GimpData *data)
{
  GimpData *new = g_object_new (GIMP_TYPE_BRUSH, NULL);

  gimp_data_copy (new, data);

  return new;
}

GimpData *
gimp_brush_clipboard_new (Gimp     *gimp,
                          gboolean  mask_only)
{
  const gchar *name;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  if (mask_only)
    name = _("Clipboard Mask");
  else
    name = _("Clipboard Image");

  return g_object_new (GIMP_TYPE_BRUSH_CLIPBOARD,
                       "name",      name,
                       "gimp",      gimp,
                       "mask-only", mask_only,
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

  g_clear_pointer (&brush->priv->mask,   gimp_temp_buf_unref);
  g_clear_pointer (&brush->priv->pixmap, gimp_temp_buf_unref);

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

      width  = MIN (gegl_buffer_get_width  (buffer), BRUSH_MAX_SIZE);
      height = MIN (gegl_buffer_get_height (buffer), BRUSH_MAX_SIZE);

      brush->priv->mask = gimp_temp_buf_new (width, height,
                                             babl_format ("Y u8"));

      if (GIMP_BRUSH_CLIPBOARD (brush)->mask_only)
        {
          guchar *p;
          gint    i;

          gegl_buffer_get (buffer,
                           GEGL_RECTANGLE (0, 0, width, height), 1.0,
                           babl_format ("Y u8"),
                           gimp_temp_buf_get_data (brush->priv->mask),
                           GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

          /*  invert the mask, it's more intuitive to think
           *  "black on white" than the other way around
           */
          for (i = 0, p = gimp_temp_buf_get_data (brush->priv->mask);
               i < width * height;
               i++, p++)
            {
              *p = 255 - *p;
            }
        }
      else
        {
          brush->priv->pixmap = gimp_temp_buf_new (width, height,
                                                   babl_format ("R'G'B' u8"));

          /*  copy the alpha channel into the brush's mask  */
          if (babl_format_has_alpha (format))
            {
              gegl_buffer_get (buffer,
                               GEGL_RECTANGLE (0, 0, width, height), 1.0,
                               babl_format ("A u8"),
                               gimp_temp_buf_get_data (brush->priv->mask),
                               GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
            }
          else
            {
              memset (gimp_temp_buf_get_data (brush->priv->mask), 255,
                      width * height);
            }

          /*  copy the color channels into the brush's pixmap  */
          gegl_buffer_get (buffer,
                           GEGL_RECTANGLE (0, 0, width, height), 1.0,
                           babl_format ("R'G'B' u8"),
                           gimp_temp_buf_get_data (brush->priv->pixmap),
                           GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
        }
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
