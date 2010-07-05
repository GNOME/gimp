/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpcairo-utils.c
 * Copyright (C) 2007 Sven Neumann <sven@gimp.org>
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
 * <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"

#include "gimpcairo-utils.h"


/**
 * SECTION: gimpcairo-utils
 * @title: GimpCairo-utils
 * @short_description: Utility functions for cairo
 *
 * Utility functions that make cairo easier to use with common
 * GIMP data types.
 **/


/**
 * gimp_cairo_set_source_rgb:
 * @cr:    Cairo context
 * @color: GimpRGB color
 *
 * Sets the source pattern within @cr to the solid opaque color
 * described by @color.
 *
 * This function calls cairo_set_source_rgb() for you.
 *
 * Since: GIMP 2.6
 **/
void
gimp_cairo_set_source_rgb (cairo_t       *cr,
                           const GimpRGB *color)
{
  cairo_set_source_rgb (cr, color->r, color->g, color->b);
}

/**
 * gimp_cairo_set_source_rgba:
 * @cr:    Cairo context
 * @color: GimpRGB color
 *
 * Sets the source pattern within @cr to the solid translucent color
 * described by @color.
 *
 * This function calls cairo_set_source_rgba() for you.
 *
 * Since: GIMP 2.6
 **/
void
gimp_cairo_set_source_rgba (cairo_t       *cr,
                            const GimpRGB *color)
{
  cairo_set_source_rgba (cr, color->r, color->g, color->b, color->a);
}

/**
 * gimp_cairo_set_focus_line_pattern:
 * @cr:     Cairo context
 * @widget: widget to draw the focus indicator on
 *
 * Sets color and dash pattern for stroking a focus line on the given
 * @cr. The line pattern is taken from @widget.
 *
 * Return value: %TRUE if the widget style has a focus line pattern,
 *               %FALSE otherwise
 *
 * Since: GIMP 2.6
 **/
gboolean
gimp_cairo_set_focus_line_pattern (cairo_t   *cr,
                                   GtkWidget *widget)
{
  gint8    *dash_list;
  gboolean  retval = FALSE;

  g_return_val_if_fail (cr != NULL, FALSE);
  g_return_val_if_fail (GTK_IS_WIDGET (widget), FALSE);

  gtk_widget_style_get (widget,
                        "focus-line-pattern", (gchar *) &dash_list,
                        NULL);

  if (dash_list[0])
    {
      /* Taken straight from gtk_default_draw_focus()
       */
      gint     n_dashes     = strlen ((const gchar *) dash_list);
      gdouble *dashes       = g_new (gdouble, n_dashes);
      gdouble  total_length = 0;
      gint     i;

      for (i = 0; i < n_dashes; i++)
        {
          dashes[i] = dash_list[i];
          total_length += dash_list[i];
        }

      cairo_set_dash (cr, dashes, n_dashes, 0.5);

      g_free (dashes);

      retval = TRUE;
    }

  g_free (dash_list);

  return retval;
}

/**
 * gimp_cairo_checkerboard_create:
 * @cr:    Cairo context
 * @size:  check size
 * @light: light check color or %NULL to use the default light gray
 * @dark:  dark check color or %NULL to use the default dark gray
 *
 * Create a repeating checkerboard pattern.
 *
 * Return value: a new Cairo pattern that can be used as a source on @cr.
 *
 * Since: GIMP 2.6
 **/
cairo_pattern_t *
gimp_cairo_checkerboard_create (cairo_t       *cr,
                                gint           size,
                                const GimpRGB *light,
                                const GimpRGB *dark)
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
    gimp_cairo_set_source_rgb (context, light);
  else
    cairo_set_source_rgb (context,
                          GIMP_CHECK_LIGHT, GIMP_CHECK_LIGHT, GIMP_CHECK_LIGHT);

  cairo_rectangle (context, 0,    0,    size, size);
  cairo_rectangle (context, size, size, size, size);
  cairo_fill (context);

  if (dark)
    gimp_cairo_set_source_rgb (context, dark);
  else
    cairo_set_source_rgb (context,
                          GIMP_CHECK_DARK, GIMP_CHECK_DARK, GIMP_CHECK_DARK);

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
 * gimp_cairo_surface_create_from_pixbuf:
 * @pixbuf: a #GdkPixbuf
 *
 * Create a Cairo image surface from a GdkPixbuf.
 *
 * You should avoid calling this function as there are probably more
 * efficient ways of achieving the result you are looking for.
 *
 * Returns: a #cairo_surface_t.
 *
 * Since: GIMP 2.6
 **/
cairo_surface_t *
gimp_cairo_surface_create_from_pixbuf (GdkPixbuf *pixbuf)
{
  cairo_surface_t *surface;
  cairo_format_t   format;
  guchar          *dest;
  const guchar    *src;
  gint             width;
  gint             height;
  gint             src_stride;
  gint             dest_stride;
  gint             y;

  g_return_val_if_fail (GDK_IS_PIXBUF (pixbuf), NULL);

  width  = gdk_pixbuf_get_width (pixbuf);
  height = gdk_pixbuf_get_height (pixbuf);

  switch (gdk_pixbuf_get_n_channels (pixbuf))
    {
    case 3:
      format = CAIRO_FORMAT_RGB24;
      break;
    case 4:
      format = CAIRO_FORMAT_ARGB32;
      break;
    default:
      g_assert_not_reached ();
      break;
    }

  surface = cairo_image_surface_create (format, width, height);

  cairo_surface_flush (surface);

  src         = gdk_pixbuf_get_pixels (pixbuf);
  src_stride  = gdk_pixbuf_get_rowstride (pixbuf);

  dest        = cairo_image_surface_get_data (surface);
  dest_stride = cairo_image_surface_get_stride (surface);

  switch (format)
    {
    case CAIRO_FORMAT_RGB24:
      for (y = 0; y < height; y++)
        {
          const guchar *s = src;
          guchar       *d = dest;
          gint          w = width;

          while (w--)
            {
              GIMP_CAIRO_RGB24_SET_PIXEL (d, s[0], s[1], s[2]);

              s += 3;
              d += 4;
            }

          src  += src_stride;
          dest += dest_stride;
        }
      break;

    case CAIRO_FORMAT_ARGB32:
      for (y = 0; y < height; y++)
        {
          const guchar *s = src;
          guchar       *d = dest;
          gint          w = width;

          while (w--)
            {
              GIMP_CAIRO_ARGB32_SET_PIXEL (d, s[0], s[1], s[2], s[3]);

              s += 4;
              d += 4;
            }

          src  += src_stride;
          dest += dest_stride;
        }
      break;

    default:
      break;
    }

  cairo_surface_mark_dirty (surface);

  return surface;
}
