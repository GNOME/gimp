/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpcairo.c
 * Copyright (C) 2007      Sven Neumann <sven@gimp.org>
 *               2010-2012 Michael Natterer <mitch@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <cairo.h>
#include <gio/gio.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"

#include "gimpcolortypes.h"

#include "gimpcairo.h"
#include "gimpcolor-private.h"


/**
 * SECTION: gimpcairo
 * @title: GimpCairo
 * @short_description: Color utility functions for cairo
 *
 * Utility functions that make cairo easier to use with GIMP color
 * data types.
 **/


static void gimp_cairo_surface_buffer_changed (GeglBuffer          *buffer,
                                               const GeglRectangle *rect,
                                               cairo_surface_t     *surface);


/**
 * gimp_cairo_checkerboard_create:
 * @cr:    Cairo context
 * @size:  check size
 * @light: light check color or %NULL to use the default light gray
 * @dark:  dark check color or %NULL to use the default dark gray
 *
 * Create a repeating checkerboard pattern.
 *
 * Returns: a new Cairo pattern that can be used as a source on @cr.
 *
 * Since: 2.6
 **/
cairo_pattern_t *
gimp_cairo_checkerboard_create (cairo_t         *cr,
                                gint             size,
                                const GeglColor *light,
                                const GeglColor *dark)
{
  cairo_t         *context;
  cairo_surface_t *surface;
  cairo_pattern_t *pattern;

  g_return_val_if_fail (cr != NULL, NULL);
  g_return_val_if_fail (size > 0, NULL);

  surface = cairo_surface_create_similar (cairo_get_target (cr),
                                          CAIRO_CONTENT_COLOR,
                                          2 * size, 2 * size);
  context = cairo_create (surface);

  if (light)
    {
      gdouble rgb[3];

      gegl_color_get_pixel (GEGL_COLOR (light), babl_format ("R'G'B' double"), rgb);
      cairo_set_source_rgb (context, rgb[0], rgb[1], rgb[2]);
    }
  else
    {
      cairo_set_source_rgb (context,
                            GIMP_CHECK_LIGHT, GIMP_CHECK_LIGHT, GIMP_CHECK_LIGHT);
    }

  cairo_rectangle (context, 0,    0,    size, size);
  cairo_rectangle (context, size, size, size, size);
  cairo_fill (context);

  if (dark)
    {
      gdouble rgb[3];

      gegl_color_get_pixel (GEGL_COLOR (dark), babl_format ("R'G'B' double"), rgb);
      cairo_set_source_rgb (context, rgb[0], rgb[1], rgb[2]);
    }
  else
    {
      cairo_set_source_rgb (context,
                            GIMP_CHECK_DARK, GIMP_CHECK_DARK, GIMP_CHECK_DARK);
    }

  cairo_rectangle (context, 0,    size, size, size);
  cairo_rectangle (context, size, 0,    size, size);
  cairo_fill (context);

  cairo_destroy (context);

  pattern = cairo_pattern_create_for_surface (surface);
  cairo_pattern_set_extend (pattern, CAIRO_EXTEND_REPEAT);

  cairo_surface_destroy (surface);

  return pattern;
}

/**
 * gimp_cairo_surface_get_format:
 * @surface: a Cairo surface
 *
 * This function returns a #Babl format that corresponds to @surface's
 * pixel format.
 *
 * Returns: the #Babl format of @surface.
 *
 * Since: 2.10
 **/
const Babl *
gimp_cairo_surface_get_format (cairo_surface_t *surface)
{
  g_return_val_if_fail (surface != NULL, NULL);
  g_return_val_if_fail (cairo_surface_get_type (surface) ==
                        CAIRO_SURFACE_TYPE_IMAGE, NULL);

  switch (cairo_image_surface_get_format (surface))
    {
    case CAIRO_FORMAT_RGB24:    return babl_format ("cairo-RGB24");
    case CAIRO_FORMAT_ARGB32:   return babl_format ("cairo-ARGB32");
    case CAIRO_FORMAT_A8:       return babl_format ("cairo-A8");
#if CAIRO_VERSION >= CAIRO_VERSION_ENCODE(1, 17, 2)
    /* Since Cairo 1.17.2 */
    case CAIRO_FORMAT_RGB96F:   return babl_format ("R'G'B' float");
    case CAIRO_FORMAT_RGBA128F: return babl_format ("R'aG'aB'aA float");
#endif

    default:
      break;
    }

  g_return_val_if_reached (NULL);
}

/**
 * gimp_cairo_surface_create_buffer:
 * @surface: a Cairo surface
 * @format:  a Babl format.
 *
 * This function returns a #GeglBuffer which wraps @surface's pixels.
 * It must only be called on image surfaces, calling it on other surface
 * types is an error.
 *
 * If @format is set, the returned [class@Gegl.Buffer] will use it. It has to
 * map with @surface Cairo format. If unset, the buffer format will be
 * determined from @surface. The main difference is that automatically
 * determined format has sRGB space and TRC by default.
 *
 * Returns: (transfer full): a #GeglBuffer
 *
 * Since: 2.10
 *
 * Deprecated: 3.0.8: Use gimp_cairo_surface_get_buffer().
 **/
GeglBuffer *
gimp_cairo_surface_create_buffer (cairo_surface_t *surface,
                                  const Babl      *format)
{
  return gimp_cairo_surface_get_buffer (surface, format, TRUE);
}

/**
 * gimp_cairo_surface_get_buffer:
 * @surface: a Cairo surface
 * @format:  a Babl format.
 * @sync_back: whether changes on the returned buffer should be synced
 *             back to @surface.
 *
 * This function returns a [class@Gegl.Buffer] containing @surface's
 * pixels. It must only be called on image surfaces, calling it on other
 * surface types is an error.
 *
 * If @format is set, the returned buffer will use it. It has to map
 * with @surface Cairo format. If unset, the buffer format will be
 * determined from @surface. The main difference is that automatically
 * determined format has sRGB space and TRC by default.
 *
 * If you want the changes to the returned buffer to be synced back to
 * @surface data, set @sync_back to %TRUE. If you don't need this and
 * only want a copy of @surface at a given time, %FALSE will be less
 * costly.
 *
 * When @sync_back is %TRUE, [method@Gegl.Buffer.freeze_changed] and
 * [method@Gegl.Buffer.thaw_changed] may be useful to block intermediate
 * syncing.
 *
 * Returns: (transfer full): a #GeglBuffer
 *
 * Since: 3.0.8
 **/
GeglBuffer *
gimp_cairo_surface_get_buffer (cairo_surface_t *surface,
                               const Babl      *format,
                               gboolean         sync_back)
{
  const Babl    *surface_format;
  GeglBuffer    *buffer;
  GeglRectangle  extent = {0};
  gint           rowstride;
  gint           bpp;

  g_return_val_if_fail (surface != NULL, NULL);
  g_return_val_if_fail (cairo_surface_get_type (surface) == CAIRO_SURFACE_TYPE_IMAGE, NULL);

  surface_format = gimp_cairo_surface_get_format (surface);
  rowstride      = cairo_image_surface_get_stride (surface);
  bpp            = babl_format_get_bytes_per_pixel (surface_format);

  g_return_val_if_fail (format == NULL || babl_format_get_bytes_per_pixel (format) == bpp, NULL);

  if (format == NULL)
    format = surface_format;

  extent.width  = cairo_image_surface_get_width  (surface);
  extent.height = cairo_image_surface_get_height (surface);

  if ( ! sync_back || rowstride % bpp != 0 ||
      extent.width * extent.height > GIMP_LINEAR_BUFFER_MAX_SIZE)
    buffer = gegl_buffer_new (&extent, format);
  else
    return gegl_buffer_linear_new_from_data (cairo_image_surface_get_data (surface),
                                             format, &extent, rowstride,
                                             (GDestroyNotify) cairo_surface_destroy,
                                             cairo_surface_reference (surface));

  gegl_buffer_set (buffer, &extent, 0, format,
                   cairo_image_surface_get_data (surface),
                   rowstride);

  if (sync_back)
    {
      /* Making sure we don't work on a destroyed surface. */
      g_object_set_data_full (G_OBJECT (buffer),
                              "gimp-cairo-surface-get-buffer-surface",
                              cairo_surface_reference (surface),
                              (GDestroyNotify) cairo_surface_destroy);
      gegl_buffer_signal_connect (buffer, "changed",
                                  G_CALLBACK (gimp_cairo_surface_buffer_changed),
                                  surface);
    }

  return buffer;
}


/* Private functions */

static void
gimp_cairo_surface_buffer_changed (GeglBuffer          *buffer,
                                   const GeglRectangle *rect,
                                   cairo_surface_t     *surface)
{
  unsigned char *data;
  gint           stride;
  gint           bpp;

  data   = cairo_image_surface_get_data (surface);
  stride = cairo_image_surface_get_stride (surface);
  bpp    = babl_format_get_bytes_per_pixel (gegl_buffer_get_format (buffer));

  data += stride * rect->y + rect->x * bpp;

  if ((rect->x == 0 && rect->width == gegl_buffer_get_width (buffer)) || rect->height == 1)
    {
      gegl_buffer_get (buffer, rect, 1.0, NULL,
                       data, stride, GEGL_ABYSS_NONE);
    }
  else
    {
      GeglRectangle extent;

      extent.x      = rect->x;
      extent.width  = rect->width;
      extent.height = 1;

      for (gint y = rect->y; y < rect->y + rect->height; y++)
        {
          extent.y = y;
          gegl_buffer_get (buffer, &extent, 1.0, NULL,
                           data, stride, GEGL_ABYSS_NONE);
          data += stride;
        }
    }

  cairo_surface_mark_dirty (surface);
}
