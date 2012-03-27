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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <glib-object.h>

#include "core-types.h"

#include "base/temp-buf.h"

#include "gegl/gimp-gegl-utils.h"

#include "gimp-utils.h"
#include "gimpbuffer.h"
#include "gimpimage.h"


static void      gimp_buffer_finalize         (GObject       *object);

static gint64    gimp_buffer_get_memsize      (GimpObject    *object,
                                               gint64        *gui_size);

static gboolean  gimp_buffer_get_size         (GimpViewable  *viewable,
                                               gint          *width,
                                               gint          *height);
static void      gimp_buffer_get_preview_size (GimpViewable  *viewable,
                                               gint           size,
                                               gboolean       is_popup,
                                               gboolean       dot_for_dot,
                                               gint          *popup_width,
                                               gint          *popup_height);
static gboolean  gimp_buffer_get_popup_size   (GimpViewable  *viewable,
                                               gint           width,
                                               gint           height,
                                               gboolean       dot_for_dot,
                                               gint          *popup_width,
                                               gint          *popup_height);
static TempBuf * gimp_buffer_get_new_preview  (GimpViewable  *viewable,
                                               GimpContext   *context,
                                               gint           width,
                                               gint           height);
static gchar   * gimp_buffer_get_description  (GimpViewable  *viewable,
                                               gchar        **tooltip);


G_DEFINE_TYPE (GimpBuffer, gimp_buffer, GIMP_TYPE_VIEWABLE)

#define parent_class gimp_buffer_parent_class


static void
gimp_buffer_class_init (GimpBufferClass *klass)
{
  GObjectClass      *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass   *gimp_object_class = GIMP_OBJECT_CLASS (klass);
  GimpViewableClass *viewable_class    = GIMP_VIEWABLE_CLASS (klass);

  object_class->finalize           = gimp_buffer_finalize;

  gimp_object_class->get_memsize   = gimp_buffer_get_memsize;

  viewable_class->default_stock_id = "gtk-paste";
  viewable_class->get_size         = gimp_buffer_get_size;
  viewable_class->get_preview_size = gimp_buffer_get_preview_size;
  viewable_class->get_popup_size   = gimp_buffer_get_popup_size;
  viewable_class->get_new_preview  = gimp_buffer_get_new_preview;
  viewable_class->get_description  = gimp_buffer_get_description;
}

static void
gimp_buffer_init (GimpBuffer *buffer)
{
}

static void
gimp_buffer_finalize (GObject *object)
{
  GimpBuffer *buffer = GIMP_BUFFER (object);

  if (buffer->buffer)
    {
      g_object_unref (buffer->buffer);
      buffer->buffer = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gint64
gimp_buffer_get_memsize (GimpObject *object,
                         gint64     *gui_size)
{
  GimpBuffer *buffer  = GIMP_BUFFER (object);
  gint64      memsize = 0;

  memsize += gimp_gegl_buffer_get_memsize (buffer->buffer);

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static gboolean
gimp_buffer_get_size (GimpViewable *viewable,
                      gint         *width,
                      gint         *height)
{
  GimpBuffer *buffer = GIMP_BUFFER (viewable);

  *width  = gimp_buffer_get_width (buffer);
  *height = gimp_buffer_get_height (buffer);

  return TRUE;
}

static void
gimp_buffer_get_preview_size (GimpViewable *viewable,
                              gint          size,
                              gboolean      is_popup,
                              gboolean      dot_for_dot,
                              gint         *width,
                              gint         *height)
{
  GimpBuffer *buffer = GIMP_BUFFER (viewable);

  gimp_viewable_calc_preview_size (gimp_buffer_get_width (buffer),
                                   gimp_buffer_get_height (buffer),
                                   size,
                                   size,
                                   dot_for_dot, 1.0, 1.0,
                                   width,
                                   height,
                                   NULL);
}

static gboolean
gimp_buffer_get_popup_size (GimpViewable *viewable,
                            gint          width,
                            gint          height,
                            gboolean      dot_for_dot,
                            gint         *popup_width,
                            gint         *popup_height)
{
  GimpBuffer *buffer;
  gint        buffer_width;
  gint        buffer_height;

  buffer        = GIMP_BUFFER (viewable);
  buffer_width  = gimp_buffer_get_width (buffer);
  buffer_height = gimp_buffer_get_height (buffer);

  if (buffer_width > width || buffer_height > height)
    {
      gboolean scaling_up;

      gimp_viewable_calc_preview_size (buffer_width,
                                       buffer_height,
                                       width  * 2,
                                       height * 2,
                                       dot_for_dot, 1.0, 1.0,
                                       popup_width,
                                       popup_height,
                                       &scaling_up);

      if (scaling_up)
        {
          *popup_width  = buffer_width;
          *popup_height = buffer_height;
        }

      return TRUE;
    }

  return FALSE;
}

static TempBuf *
gimp_buffer_get_new_preview (GimpViewable *viewable,
                             GimpContext  *context,
                             gint          width,
                             gint          height)
{
  GimpBuffer *buffer = GIMP_BUFFER (viewable);
  TempBuf    *preview;

  preview = temp_buf_new (width, height, gimp_buffer_get_bytes (buffer),
                          0, 0, NULL);

  gegl_buffer_get (buffer->buffer,
                   NULL,
                   MIN ((gdouble) width / (gdouble) gimp_buffer_get_width (buffer),
                        (gdouble) height / (gdouble) gimp_buffer_get_height (buffer)),
                   gimp_bpp_to_babl_format (gimp_buffer_get_bytes (buffer)),
                   temp_buf_get_data (preview),
                   width * gimp_buffer_get_bytes (buffer),
                   GEGL_ABYSS_NONE);

  return preview;
}

static gchar *
gimp_buffer_get_description (GimpViewable  *viewable,
                             gchar        **tooltip)
{
  GimpBuffer *buffer = GIMP_BUFFER (viewable);

  return g_strdup_printf ("%s (%d × %d)",
                          gimp_object_get_name (buffer),
                          gimp_buffer_get_width (buffer),
                          gimp_buffer_get_height (buffer));
}

GimpBuffer *
gimp_buffer_new (GeglBuffer    *buffer,
                 const gchar   *name,
                 GimpImageType  image_type,
                 gint           offset_x,
                 gint           offset_y,
                 gboolean       copy_pixels)
{
  GimpBuffer *gimp_buffer;

  g_return_val_if_fail (GEGL_IS_BUFFER (buffer), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  gimp_buffer = g_object_new (GIMP_TYPE_BUFFER,
                              "name", name,
                              NULL);

  if (TRUE /* FIXME copy_pixels */)
    gimp_buffer->buffer = gegl_buffer_dup (buffer);
  else
    gimp_buffer->buffer = g_object_ref (buffer);

  gimp_buffer->image_type = image_type;
  gimp_buffer->offset_x   = offset_x;
  gimp_buffer->offset_y   = offset_y;

  return gimp_buffer;
}

GimpBuffer *
gimp_buffer_new_from_pixbuf (GdkPixbuf   *pixbuf,
                             const gchar *name,
                             gint         offset_x,
                             gint         offset_y)
{
  GimpBuffer *gimp_buffer;
  GeglBuffer *buffer;
  gint        channels;

  g_return_val_if_fail (GDK_IS_PIXBUF (pixbuf), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  channels = gdk_pixbuf_get_n_channels (pixbuf);

  buffer = gimp_pixbuf_create_buffer (pixbuf);

  gimp_buffer = gimp_buffer_new (buffer, name,
                                 GIMP_IMAGE_TYPE_FROM_BYTES (channels),
                                 offset_x, offset_y, FALSE);

  g_object_unref (buffer);

  return gimp_buffer;
}

gint
gimp_buffer_get_width (const GimpBuffer *buffer)
{
  g_return_val_if_fail (GIMP_IS_BUFFER (buffer), 0);

  return gegl_buffer_get_width (buffer->buffer);
}

gint
gimp_buffer_get_height (const GimpBuffer *buffer)
{
  g_return_val_if_fail (GIMP_IS_BUFFER (buffer), 0);

  return gegl_buffer_get_height (buffer->buffer);
}

gint
gimp_buffer_get_bytes (const GimpBuffer *buffer)
{
  g_return_val_if_fail (GIMP_IS_BUFFER (buffer), 0);

  return GIMP_IMAGE_TYPE_BYTES (buffer->image_type);
}

GimpImageType
gimp_buffer_get_image_type (const GimpBuffer *buffer)
{
  g_return_val_if_fail (GIMP_IS_BUFFER (buffer), 0);

  return buffer->image_type;
}

GeglBuffer *
gimp_buffer_get_buffer (const GimpBuffer *buffer)
{
  g_return_val_if_fail (GIMP_IS_BUFFER (buffer), NULL);

  return buffer->buffer;
}
