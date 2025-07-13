/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimppickable.h
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

#pragma once


#define GIMP_TYPE_PICKABLE (gimp_pickable_get_type ())
G_DECLARE_INTERFACE (GimpPickable, gimp_pickable, GIMP, PICKABLE, GObject)

struct _GimpPickableInterface
{
  GTypeInterface base_iface;

  /*  virtual functions  */
  void            (* flush)                   (GimpPickable        *pickable);
  GimpImage     * (* get_image)               (GimpPickable        *pickable);
  const Babl    * (* get_format)              (GimpPickable        *pickable);
  const Babl    * (* get_format_with_alpha)   (GimpPickable        *pickable);
  GeglBuffer    * (* get_buffer)              (GimpPickable        *pickable);
  GeglBuffer    * (* get_buffer_with_effects) (GimpPickable        *pickable);
  gboolean        (* get_pixel_at)            (GimpPickable        *pickable,
                                               gint                 x,
                                               gint                 y,
                                               const Babl          *format,
                                               gpointer             pixel);
  gdouble         (* get_opacity_at)          (GimpPickable        *pickable,
                                               gint                 x,
                                               gint                 y);
  void            (* get_pixel_average)       (GimpPickable        *pickable,
                                               const GeglRectangle *rect,
                                               const Babl          *format,
                                               gpointer             pixel);
};


void            gimp_pickable_flush                   (GimpPickable        *pickable);
GimpImage     * gimp_pickable_get_image               (GimpPickable        *pickable);
const Babl    * gimp_pickable_get_format              (GimpPickable        *pickable);
const Babl    * gimp_pickable_get_format_with_alpha   (GimpPickable        *pickable);
GeglBuffer    * gimp_pickable_get_buffer              (GimpPickable        *pickable);
GeglBuffer    * gimp_pickable_get_buffer_with_effects (GimpPickable        *pickable);
gboolean        gimp_pickable_get_pixel_at            (GimpPickable        *pickable,
                                                       gint                 x,
                                                       gint                 y,
                                                       const Babl          *format,
                                                       gpointer             pixel);
GeglColor     * gimp_pickable_get_color_at            (GimpPickable        *pickable,
                                                       gint                 x,
                                                       gint                 y);
gdouble         gimp_pickable_get_opacity_at          (GimpPickable        *pickable,
                                                       gint                 x,
                                                       gint                 y);
void            gimp_pickable_get_pixel_average       (GimpPickable        *pickable,
                                                       const GeglRectangle *rect,
                                                       const Babl          *format,
                                                       gpointer             pixel);

gboolean        gimp_pickable_pick_color              (GimpPickable        *pickable,
                                                       gint                 x,
                                                       gint                 y,
                                                       gboolean             sample_average,
                                                       gdouble              average_radius,
                                                       gpointer             pixel,
                                                       GeglColor          **color);
