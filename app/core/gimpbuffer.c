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

#include <glib-object.h>

#include "libgimpmath/gimpmath.h"

#include "core-types.h"

#include "base/pixel-region.h"
#include "base/tile-manager.h"
#include "base/tile-manager-preview.h"
#include "base/temp-buf.h"

#include "paint-funcs/paint-funcs.h"

#include "gimpbuffer.h"


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
  buffer->tiles = NULL;
}

static void
gimp_buffer_finalize (GObject *object)
{
  GimpBuffer *buffer = GIMP_BUFFER (object);

  if (buffer->tiles)
    {
      tile_manager_unref (buffer->tiles);
      buffer->tiles = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gint64
gimp_buffer_get_memsize (GimpObject *object,
                         gint64     *gui_size)
{
  GimpBuffer *buffer  = GIMP_BUFFER (object);
  gint64      memsize = 0;

  if (buffer->tiles)
    memsize += tile_manager_get_memsize (buffer->tiles, FALSE);

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

  return tile_manager_get_preview (buffer->tiles, width, height);
}

static gchar *
gimp_buffer_get_description (GimpViewable  *viewable,
                             gchar        **tooltip)
{
  GimpBuffer *buffer = GIMP_BUFFER (viewable);

  return g_strdup_printf ("%s (%d × %d)",
                          GIMP_OBJECT (buffer)->name,
                          gimp_buffer_get_width (buffer),
                          gimp_buffer_get_height (buffer));
}

GimpBuffer *
gimp_buffer_new (TileManager *tiles,
                 const gchar *name,
                 gboolean     copy_pixels)
{
  GimpBuffer *buffer;
  gint        width, height;

  g_return_val_if_fail (tiles != NULL, NULL);
  g_return_val_if_fail (name != NULL, NULL);

  width  = tile_manager_width (tiles);
  height = tile_manager_height (tiles);

  buffer = g_object_new (GIMP_TYPE_BUFFER,
                         "name", name,
                         NULL);

  if (copy_pixels)
    {
      PixelRegion srcPR, destPR;

      buffer->tiles = tile_manager_new (width, height,
                                        tile_manager_bpp (tiles));

      pixel_region_init (&srcPR, tiles, 0, 0, width, height, FALSE);
      pixel_region_init (&destPR, buffer->tiles, 0, 0, width, height, TRUE);
      copy_region (&srcPR, &destPR);
    }
  else
    {
      buffer->tiles = tiles;
    }

  return buffer;
}

GimpBuffer *
gimp_buffer_new_from_pixbuf (GdkPixbuf   *pixbuf,
                             const gchar *name)
{
  TileManager *tiles;
  guchar      *pixels;
  PixelRegion  destPR;
  gint         width;
  gint         height;
  gint         rowstride;
  gint         channels;
  gint         y;

  g_return_val_if_fail (GDK_IS_PIXBUF (pixbuf), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  width     = gdk_pixbuf_get_width (pixbuf);
  height    = gdk_pixbuf_get_height (pixbuf);
  rowstride = gdk_pixbuf_get_rowstride (pixbuf);
  channels  = gdk_pixbuf_get_n_channels (pixbuf);

  tiles = tile_manager_new (width, height, channels);

  pixel_region_init (&destPR, tiles, 0, 0, width, height, TRUE);

  for (y = 0, pixels = gdk_pixbuf_get_pixels (pixbuf);
       y < height;
       y++, pixels += rowstride)
    {
      pixel_region_set_row (&destPR, 0, y, width, pixels);
   }

  return gimp_buffer_new (tiles, name, FALSE);
}

gint
gimp_buffer_get_width (const GimpBuffer *buffer)
{
  g_return_val_if_fail (GIMP_IS_BUFFER (buffer), 0);

  return tile_manager_width (buffer->tiles);
}

gint
gimp_buffer_get_height (const GimpBuffer *buffer)
{
  g_return_val_if_fail (GIMP_IS_BUFFER (buffer), 0);

  return tile_manager_height (buffer->tiles);
}

gint
gimp_buffer_get_bytes (const GimpBuffer *buffer)
{
  g_return_val_if_fail (GIMP_IS_BUFFER (buffer), 0);

  return tile_manager_bpp (buffer->tiles);
}

GimpImageType
gimp_buffer_get_image_type (const GimpBuffer *buffer)
{
  g_return_val_if_fail (GIMP_IS_BUFFER (buffer), 0);

  switch (tile_manager_bpp (buffer->tiles))
    {
    case 1: return GIMP_GRAY_IMAGE;
    case 2: return GIMP_GRAYA_IMAGE;
    case 3: return GIMP_RGB_IMAGE;
    case 4: return GIMP_RGBA_IMAGE;
    }

  return 0;
}
