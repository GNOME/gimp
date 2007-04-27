/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimppickable.c
 * Copyright (C) 2004  Michael Natterer <mitch@gimp.org>
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

#include "libgimpcolor/gimpcolor.h"

#include "core-types.h"

#include "gimpobject.h"
#include "gimppickable.h"


GType
gimp_pickable_interface_get_type (void)
{
  static GType pickable_iface_type = 0;

  if (! pickable_iface_type)
    {
      const GTypeInfo pickable_iface_info =
      {
        sizeof (GimpPickableInterface),
        (GBaseInitFunc)     NULL,
        (GBaseFinalizeFunc) NULL,
      };

      pickable_iface_type = g_type_register_static (G_TYPE_INTERFACE,
                                                    "GimpPickableInterface",
                                                    &pickable_iface_info,
                                                    0);

      g_type_interface_add_prerequisite (pickable_iface_type, GIMP_TYPE_OBJECT);
    }

  return pickable_iface_type;
}

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

GimpImageType
gimp_pickable_get_image_type (GimpPickable *pickable)
{
  GimpPickableInterface *pickable_iface;

  g_return_val_if_fail (GIMP_IS_PICKABLE (pickable), -1);

  pickable_iface = GIMP_PICKABLE_GET_INTERFACE (pickable);

  if (pickable_iface->get_image_type)
    return pickable_iface->get_image_type (pickable);

  return -1;
}

gint
gimp_pickable_get_bytes (GimpPickable *pickable)
{
  GimpPickableInterface *pickable_iface;

  g_return_val_if_fail (GIMP_IS_PICKABLE (pickable), 0);

  pickable_iface = GIMP_PICKABLE_GET_INTERFACE (pickable);

  if (pickable_iface->get_bytes)
    return pickable_iface->get_bytes (pickable);

  return 0;
}

TileManager *
gimp_pickable_get_tiles (GimpPickable *pickable)
{
  GimpPickableInterface *pickable_iface;

  g_return_val_if_fail (GIMP_IS_PICKABLE (pickable), NULL);

  pickable_iface = GIMP_PICKABLE_GET_INTERFACE (pickable);

  if (pickable_iface->get_tiles)
    return pickable_iface->get_tiles (pickable);

  return NULL;
}

gboolean
gimp_pickable_get_pixel_at (GimpPickable *pickable,
                            gint          x,
                            gint          y,
                            guchar       *pixel)
{
  GimpPickableInterface *pickable_iface;

  g_return_val_if_fail (GIMP_IS_PICKABLE (pickable), FALSE);
  g_return_val_if_fail (pixel != NULL, FALSE);

  pickable_iface = GIMP_PICKABLE_GET_INTERFACE (pickable);

  if (pickable_iface->get_pixel_at)
    return pickable_iface->get_pixel_at (pickable, x, y, pixel);

  return FALSE;
}

guchar *
gimp_pickable_get_color_at (GimpPickable *pickable,
                            gint          x,
                            gint          y)
{
  GimpPickableInterface *pickable_iface;

  g_return_val_if_fail (GIMP_IS_PICKABLE (pickable), NULL);

  pickable_iface = GIMP_PICKABLE_GET_INTERFACE (pickable);

  if (pickable_iface->get_color_at)
    return pickable_iface->get_color_at (pickable, x, y);

  return NULL;
}

gint
gimp_pickable_get_opacity_at (GimpPickable *pickable,
                              gint          x,
                              gint          y)
{
  GimpPickableInterface *pickable_iface;

  g_return_val_if_fail (GIMP_IS_PICKABLE (pickable), 0);

  pickable_iface = GIMP_PICKABLE_GET_INTERFACE (pickable);

  if (pickable_iface->get_opacity_at)
    return pickable_iface->get_opacity_at (pickable, x, y);

  return 0;
}

gboolean
gimp_pickable_pick_color (GimpPickable *pickable,
                          gint          x,
                          gint          y,
                          gboolean      sample_average,
                          gdouble       average_radius,
                          GimpRGB      *color,
                          gint         *color_index)
{
  guchar *col;

  g_return_val_if_fail (GIMP_IS_PICKABLE (pickable), FALSE);

  if (! (col = gimp_pickable_get_color_at (pickable, x, y)))
    return FALSE;

  if (sample_average)
    {
      gint    i, j;
      gint    count        = 0;
      gint    color_avg[4] = { 0, 0, 0, 0 };
      guchar *tmp_col;
      gint    radius       = (gint) average_radius;

      for (i = x - radius; i <= x + radius; i++)
        for (j = y - radius; j <= y + radius; j++)
          if ((tmp_col = gimp_pickable_get_color_at (pickable, i, j)))
            {
              count++;

              color_avg[RED_PIX]   += tmp_col[RED_PIX];
              color_avg[GREEN_PIX] += tmp_col[GREEN_PIX];
              color_avg[BLUE_PIX]  += tmp_col[BLUE_PIX];
              color_avg[ALPHA_PIX] += tmp_col[ALPHA_PIX];

              g_free (tmp_col);
            }

      col[RED_PIX]   = (guchar) (color_avg[RED_PIX]   / count);
      col[GREEN_PIX] = (guchar) (color_avg[GREEN_PIX] / count);
      col[BLUE_PIX]  = (guchar) (color_avg[BLUE_PIX]  / count);
      col[ALPHA_PIX] = (guchar) (color_avg[ALPHA_PIX] / count);
    }

  if (color)
    gimp_rgba_set_uchar (color,
                         col[RED_PIX],
                         col[GREEN_PIX],
                         col[BLUE_PIX],
                         col[ALPHA_PIX]);

  if (color_index)
    *color_index = sample_average ? -1 : col[4];

  g_free (col);

  return TRUE;
}
