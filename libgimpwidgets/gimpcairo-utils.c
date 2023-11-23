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
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpconfig/gimpconfig.h"

#include "gimpcairo-utils.h"
#include "gimpwidgetsutils.h"


/**
 * SECTION: gimpcairo-utils
 * @title: GimpCairo-utils
 * @short_description: Utility functions for cairo
 *
 * Utility functions that make cairo easier to use with common
 * GIMP data types.
 **/


/**
 * gimp_cairo_set_focus_line_pattern:
 * @cr:     Cairo context
 * @widget: widget to draw the focus indicator on
 *
 * Sets color and dash pattern for stroking a focus line on the given
 * @cr. The line pattern is taken from @widget.
 *
 * Returns: %TRUE if the widget style has a focus line pattern,
 *               %FALSE otherwise
 *
 * Since: 2.6
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
 * Since: 2.6
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

/**
 * gimp_cairo_set_source_color:
 * @cr:     Cairo context.
 * @color:  the [class@Gegl.Color] to use as source pattern within @cr.
 * @config: the color management settings.
 * @softproof: whether the color must also be soft-proofed.
 * @widget: (nullable): [class@Gtk.Widget] to draw the focus indicator on.
 *
 * Sets @color as the source pattern within @cr, taking into account the profile
 * of the [class@Gdk.Monitor] which @widget is displayed on.
 *
 * If @config is set, the color configuration as set by the user will be used,
 * in particular using any custom monitor profile set in preferences (overriding
 * system-set profile). If no such custom profile is set, it will use the
 * profile of the monitor @widget is displayed on and will default to sRGB if
 * @widget is %NULL.
 *
 * Use [func@Gimp.get_color_configuration] to retrieve the user
 * [class@Gimp.ColorConfig].
 *
 * TODO: @softproof is currently unused.
 *
 * Since: 3.0
 **/
void
gimp_cairo_set_source_color (cairo_t         *cr,
                             GeglColor       *color,
                             GimpColorConfig *config,
                             gboolean         softproof,
                             GtkWidget       *widget)
{
  const Babl *space;
  gdouble     rgba[4];

  g_return_if_fail (GEGL_IS_COLOR (color));
  g_return_if_fail (widget == NULL || GTK_IS_WIDGET (widget));

  space = gimp_widget_get_render_space (widget, config);
  gegl_color_get_pixel (color, babl_format_with_space ("R'G'B'A double", space), rgba);
  cairo_set_source_rgba (cr, rgba[0], rgba[1], rgba[2], rgba[3]);
}
