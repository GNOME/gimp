/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimppickable.c
 * Copyright (C) 2004  Michael Natterer <mitch@gimp.org>
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

/* This file contains an interface for pixel objects that their color at
 * a given position can be picked. Also included is a utility for
 * sampling an average area (which uses the implemented picking
 * functions).
 */

#include "config.h"

#include <string.h>

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"

#include "core-types.h"

#include "gimpobject.h"
#include "gimpimage.h"
#include "gimppickable.h"


/*  local function prototypes  */

static void   gimp_pickable_real_get_pixel_average (GimpPickable          *pickable,
                                                    const GeglRectangle   *rect,
                                                    const Babl            *format,
                                                    gpointer               pixel);


G_DEFINE_INTERFACE (GimpPickable, gimp_pickable, GIMP_TYPE_OBJECT)


/*  private functions  */


static void
gimp_pickable_default_init (GimpPickableInterface *iface)
{
  iface->get_pixel_average = gimp_pickable_real_get_pixel_average;

  g_object_interface_install_property (iface,
                                       g_param_spec_object ("buffer",
                                                            NULL, NULL,
                                                            GEGL_TYPE_BUFFER,
                                                            GIMP_PARAM_READABLE));
}

static void
gimp_pickable_real_get_pixel_average (GimpPickable        *pickable,
                                      const GeglRectangle *rect,
                                      const Babl          *format,
                                      gpointer             pixel)
{
  const Babl *average_format = babl_format ("RaGaBaA double");
  gdouble     average[4]     = {};
  gint        n              = 0;
  gint        x;
  gint        y;
  gint        c;

  for (y = rect->y; y < rect->y + rect->height; y++)
    {
      for (x = rect->x; x < rect->x + rect->width; x++)
        {
          gdouble sample[4];

          if (gimp_pickable_get_pixel_at (pickable,
                                          x, y, average_format, sample))
            {
              for (c = 0; c < 4; c++)
                average[c] += sample[c];

              n++;
            }
        }
    }

  if (n > 0)
    {
      for (c = 0; c < 4; c++)
        average[c] /= n;
    }

  babl_process (babl_fish (average_format, format), average, pixel, 1);
}


/*  public functions  */


void
gimp_pickable_flush (GimpPickable *pickable)
{
  GimpPickableInterface *pickable_iface;

  g_return_if_fail (GIMP_IS_PICKABLE (pickable));

  pickable_iface = GIMP_PICKABLE_GET_INTERFACE (pickable);

  if (pickable_iface->flush)
    pickable_iface->flush (pickable);
}

GimpImage *
gimp_pickable_get_image (GimpPickable *pickable)
{
  GimpPickableInterface *pickable_iface;

  g_return_val_if_fail (GIMP_IS_PICKABLE (pickable), NULL);

  pickable_iface = GIMP_PICKABLE_GET_INTERFACE (pickable);

  if (pickable_iface->get_image)
    return pickable_iface->get_image (pickable);

  return NULL;
}

const Babl *
gimp_pickable_get_format (GimpPickable *pickable)
{
  GimpPickableInterface *pickable_iface;

  g_return_val_if_fail (GIMP_IS_PICKABLE (pickable), NULL);

  pickable_iface = GIMP_PICKABLE_GET_INTERFACE (pickable);

  if (pickable_iface->get_format)
    return pickable_iface->get_format (pickable);

  return NULL;
}

const Babl *
gimp_pickable_get_format_with_alpha (GimpPickable *pickable)
{
  GimpPickableInterface *pickable_iface;

  g_return_val_if_fail (GIMP_IS_PICKABLE (pickable), NULL);

  pickable_iface = GIMP_PICKABLE_GET_INTERFACE (pickable);

  if (pickable_iface->get_format_with_alpha)
    return pickable_iface->get_format_with_alpha (pickable);

  return NULL;
}

GeglBuffer *
gimp_pickable_get_buffer (GimpPickable *pickable)
{
  GimpPickableInterface *pickable_iface;

  g_return_val_if_fail (GIMP_IS_PICKABLE (pickable), NULL);

  pickable_iface = GIMP_PICKABLE_GET_INTERFACE (pickable);

  if (pickable_iface->get_buffer)
    return pickable_iface->get_buffer (pickable);

  return NULL;
}

gboolean
gimp_pickable_get_pixel_at (GimpPickable *pickable,
                            gint          x,
                            gint          y,
                            const Babl   *format,
                            gpointer      pixel)
{
  GimpPickableInterface *pickable_iface;

  g_return_val_if_fail (GIMP_IS_PICKABLE (pickable), FALSE);
  g_return_val_if_fail (pixel != NULL, FALSE);

  if (! format)
    format = gimp_pickable_get_format (pickable);

  pickable_iface = GIMP_PICKABLE_GET_INTERFACE (pickable);

  if (pickable_iface->get_pixel_at)
    return pickable_iface->get_pixel_at (pickable, x, y, format, pixel);

  return FALSE;
}

void
gimp_pickable_get_pixel_average (GimpPickable        *pickable,
                                 const GeglRectangle *rect,
                                 const Babl          *format,
                                 gpointer             pixel)
{
  GimpPickableInterface *pickable_iface;

  g_return_if_fail (GIMP_IS_PICKABLE (pickable));
  g_return_if_fail (rect != NULL);
  g_return_if_fail (pixel != NULL);

  if (! format)
    format = gimp_pickable_get_format (pickable);

  pickable_iface = GIMP_PICKABLE_GET_INTERFACE (pickable);

  if (pickable_iface->get_pixel_average)
    pickable_iface->get_pixel_average (pickable, rect, format, pixel);
  else
    memset (pixel, 0, babl_format_get_bytes_per_pixel (format));
}

gboolean
gimp_pickable_get_color_at (GimpPickable *pickable,
                            gint          x,
                            gint          y,
                            GimpRGB      *color)
{
  guchar pixel[32];

  g_return_val_if_fail (GIMP_IS_PICKABLE (pickable), FALSE);
  g_return_val_if_fail (color != NULL, FALSE);

  if (! gimp_pickable_get_pixel_at (pickable, x, y, NULL, pixel))
    return FALSE;

  gimp_pickable_pixel_to_srgb (pickable, NULL, pixel, color);

  return TRUE;
}

gdouble
gimp_pickable_get_opacity_at (GimpPickable *pickable,
                              gint          x,
                              gint          y)
{
  GimpPickableInterface *pickable_iface;

  g_return_val_if_fail (GIMP_IS_PICKABLE (pickable), GIMP_OPACITY_TRANSPARENT);

  pickable_iface = GIMP_PICKABLE_GET_INTERFACE (pickable);

  if (pickable_iface->get_opacity_at)
    return pickable_iface->get_opacity_at (pickable, x, y);

  return GIMP_OPACITY_TRANSPARENT;
}

void
gimp_pickable_pixel_to_srgb (GimpPickable *pickable,
                             const Babl   *format,
                             gpointer      pixel,
                             GimpRGB      *color)
{
  GimpPickableInterface *pickable_iface;

  g_return_if_fail (GIMP_IS_PICKABLE (pickable));
  g_return_if_fail (pixel != NULL);
  g_return_if_fail (color != NULL);

  if (! format)
    format = gimp_pickable_get_format (pickable);

  pickable_iface = GIMP_PICKABLE_GET_INTERFACE (pickable);

  if (pickable_iface->pixel_to_srgb)
    {
      pickable_iface->pixel_to_srgb (pickable, format, pixel, color);
    }
  else
    {
      gimp_rgba_set_pixel (color, format, pixel);
    }
}

void
gimp_pickable_srgb_to_pixel (GimpPickable  *pickable,
                             const GimpRGB *color,
                             const Babl    *format,
                             gpointer       pixel)
{
  GimpPickableInterface *pickable_iface;

  g_return_if_fail (GIMP_IS_PICKABLE (pickable));
  g_return_if_fail (color != NULL);
  g_return_if_fail (pixel != NULL);

  if (! format)
    format = gimp_pickable_get_format (pickable);

  pickable_iface = GIMP_PICKABLE_GET_INTERFACE (pickable);

  if (pickable_iface->srgb_to_pixel)
    {
      pickable_iface->srgb_to_pixel (pickable, color, format, pixel);
    }
  else
    {
      gimp_rgba_get_pixel (color, format, pixel);
    }
}

void
gimp_pickable_srgb_to_image_color (GimpPickable  *pickable,
                                   const GimpRGB *color,
                                   GimpRGB       *image_color)
{
  g_return_if_fail (GIMP_IS_PICKABLE (pickable));
  g_return_if_fail (color != NULL);
  g_return_if_fail (image_color != NULL);

  gimp_pickable_srgb_to_pixel (pickable,
                               color,
                               babl_format ("R'G'B'A double"),
                               image_color);
}

gboolean
gimp_pickable_pick_color (GimpPickable *pickable,
                          gint          x,
                          gint          y,
                          gboolean      sample_average,
                          gdouble       average_radius,
                          gpointer      pixel,
                          GimpRGB      *color)
{
  const Babl *format;
  gdouble     sample[4];

  g_return_val_if_fail (GIMP_IS_PICKABLE (pickable), FALSE);
  g_return_val_if_fail (color != NULL, FALSE);

  format = gimp_pickable_get_format (pickable);

  if (! gimp_pickable_get_pixel_at (pickable, x, y, format, sample))
    return FALSE;

  if (pixel)
    memcpy (pixel, sample, babl_format_get_bytes_per_pixel (format));

  if (sample_average)
    {
      gint radius = floor (average_radius);

      format = babl_format ("RaGaBaA double");

      gimp_pickable_get_pixel_average (pickable,
                                       GEGL_RECTANGLE (x - radius,
                                                       y - radius,
                                                       2 * radius + 1,
                                                       2 * radius + 1),
                                       format, sample);
    }

  gimp_pickable_pixel_to_srgb (pickable, format, sample, color);

  return TRUE;
}
