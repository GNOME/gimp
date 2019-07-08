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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "print.h"
#include "print-draw-page.h"

#include "libgimp/stdplugins-intl.h"


static cairo_surface_t * print_surface_from_drawable (gint32        drawable_ID,
                                                      GError      **error);

static void              print_draw_crop_marks       (GtkPrintContext *context,
                                                      gdouble          x,
                                                      gdouble          y,
                                                      gdouble          w,
                                                      gdouble          h);

gboolean
print_draw_page (GtkPrintContext *context,
                 PrintData       *data,
                 GError         **error)
{
  cairo_t         *cr = gtk_print_context_get_cairo_context (context);
  cairo_surface_t *surface;

  surface = print_surface_from_drawable (data->drawable_id, error);

  if (surface)
    {
      gint    width;
      gint    height;
      gdouble scale_x;
      gdouble scale_y;

      /*  create a white rectangle covering the entire page, just
       *  to be safe; see bug #777233.
       */
      cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 1.0);
      cairo_paint (cr);

      width  = cairo_image_surface_get_width (surface);
      height = cairo_image_surface_get_height (surface);

      scale_x = gtk_print_context_get_dpi_x (context) / data->xres;
      scale_y = gtk_print_context_get_dpi_y (context) / data->yres;

      cairo_translate (cr, data->offset_x, data->offset_y);

      if (data->draw_crop_marks)
        print_draw_crop_marks (context,
                               0, 0, width * scale_x, height * scale_y);

      cairo_scale (cr, scale_x, scale_y);
      cairo_rectangle (cr, 0, 0, width, height);
      cairo_set_source_surface (cr, surface, 0, 0);
      cairo_fill (cr);

      cairo_surface_destroy (surface);

      return TRUE;
    }

  return FALSE;
}

static cairo_surface_t *
print_surface_from_drawable (gint32   drawable_ID,
                             GError **error)
{
  GeglBuffer         *buffer   = gimp_drawable_get_buffer (drawable_ID);
  const Babl         *format;
  cairo_surface_t    *surface;
  cairo_status_t      status;
  const gint          width    = gimp_drawable_width  (drawable_ID);
  const gint          height   = gimp_drawable_height (drawable_ID);
  GeglBufferIterator *iter;
  guchar             *pixels;
  gint                stride;
  guint               count    = 0;
  guint               done     = 0;

  if (gimp_drawable_has_alpha (drawable_ID))
    format = babl_format ("cairo-ARGB32");
  else
    format = babl_format ("cairo-RGB24");

  surface = cairo_image_surface_create (gimp_drawable_has_alpha (drawable_ID) ?
                                        CAIRO_FORMAT_ARGB32 :
                                        CAIRO_FORMAT_RGB24,
                                        width, height);

  status = cairo_surface_status (surface);
  if (status != CAIRO_STATUS_SUCCESS)
    {
      switch (status)
        {
        case CAIRO_STATUS_INVALID_SIZE:
          g_set_error_literal (error,
                               GIMP_PLUGIN_PRINT_ERROR,
                               GIMP_PLUGIN_PRINT_ERROR_FAILED,
                               _("Cannot handle the size (either width or height) of the image."));
          break;
        default:
          g_set_error (error,
                       GIMP_PLUGIN_PRINT_ERROR,
                       GIMP_PLUGIN_PRINT_ERROR_FAILED,
                       "Cairo error: %s",
                       cairo_status_to_string (status));
          break;
        }

      return NULL;
    }

  pixels = cairo_image_surface_get_data (surface);
  stride = cairo_image_surface_get_stride (surface);

  iter = gegl_buffer_iterator_new (buffer,
                                   GEGL_RECTANGLE (0, 0, width, height), 0,
                                   format,
                                   GEGL_ACCESS_READ, GEGL_ABYSS_NONE, 1);

  while (gegl_buffer_iterator_next (iter))
    {
      const guchar *src  = iter->items[0].data;
      guchar       *dest = pixels + iter->items[0].roi.y * stride + iter->items[0].roi.x * 4;
      gint          y;

      for (y = 0; y < iter->items[0].roi.height; y++)
        {
          memcpy (dest, src, iter->items[0].roi.width * 4);

          src  += iter->items[0].roi.width * 4;
          dest += stride;
        }

      done += iter->items[0].roi.height * iter->items[0].roi.width;

      if (count++ % 16 == 0)
        gimp_progress_update ((gdouble) done / (width * height));
    }

  g_object_unref (buffer);

  cairo_surface_mark_dirty (surface);

  gimp_progress_update (1.0);

  return surface;
}

static void
print_draw_crop_marks (GtkPrintContext *context,
                       gdouble          x,
                       gdouble          y,
                       gdouble          w,
                       gdouble          h)
{
  cairo_t *cr  = gtk_print_context_get_cairo_context (context);
  gdouble  len = MIN (gtk_print_context_get_width (context),
                      gtk_print_context_get_height (context)) / 20.0;

  /*  upper left  */

  cairo_move_to (cr, x - len,     y);
  cairo_line_to (cr, x - len / 5, y);

  cairo_move_to (cr, x, y - len);
  cairo_line_to (cr, x, y - len / 5);

  /*  upper right  */

  cairo_move_to (cr, x + w + len / 5, y);
  cairo_line_to (cr, x + w + len,     y);

  cairo_move_to (cr, x + w, y - len);
  cairo_line_to (cr, x + w, y - len / 5);

  /*  lower left  */

  cairo_move_to (cr, x - len,     y + h);
  cairo_line_to (cr, x - len / 5, y + h);

  cairo_move_to (cr, x, y + h + len);
  cairo_line_to (cr, x, y + h + len / 5);

  /*  lower right  */

  cairo_move_to (cr, x + w + len / 5, y + h);
  cairo_line_to (cr, x + w + len,     y + h);

  cairo_move_to (cr, x + w, y + h + len);
  cairo_line_to (cr, x + w, y + h + len / 5);

  cairo_set_source_rgb (cr, 0.5, 0.5, 0.5);
  cairo_set_line_width (cr, 2);
  cairo_stroke (cr);
}
