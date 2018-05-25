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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_PICKABLE_H__
#define __GIMP_PICKABLE_H__


#define GIMP_TYPE_PICKABLE               (gimp_pickable_interface_get_type ())
#define GIMP_IS_PICKABLE(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_PICKABLE))
#define GIMP_PICKABLE(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_PICKABLE, GimpPickable))
#define GIMP_PICKABLE_GET_INTERFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), GIMP_TYPE_PICKABLE, GimpPickableInterface))


typedef struct _GimpPickableInterface GimpPickableInterface;

struct _GimpPickableInterface
{
  GTypeInterface base_iface;

  /*  virtual functions  */
  void            (* flush)                 (GimpPickable        *pickable);
  GimpImage     * (* get_image)             (GimpPickable        *pickable);
  const Babl    * (* get_format)            (GimpPickable        *pickable);
  const Babl    * (* get_format_with_alpha) (GimpPickable        *pickable);
  GeglBuffer    * (* get_buffer)            (GimpPickable        *pickable);
  gboolean        (* get_pixel_at)          (GimpPickable        *pickable,
                                             gint                 x,
                                             gint                 y,
                                             const Babl          *format,
                                             gpointer             pixel);
  gdouble         (* get_opacity_at)        (GimpPickable        *pickable,
                                             gint                 x,
                                             gint                 y);
  void            (* get_pixel_average)     (GimpPickable        *pickable,
                                             const GeglRectangle *rect,
                                             const Babl          *format,
                                             gpointer             pixel);
  void            (* pixel_to_srgb)         (GimpPickable        *pickable,
                                             const Babl          *format,
                                             gpointer             pixel,
                                             GimpRGB             *color);
  void            (* srgb_to_pixel)         (GimpPickable        *pickable,
                                             const GimpRGB       *color,
                                             const Babl          *format,
                                             gpointer             pixel);
};


GType           gimp_pickable_interface_get_type    (void) G_GNUC_CONST;

void            gimp_pickable_flush                 (GimpPickable        *pickable);
GimpImage     * gimp_pickable_get_image             (GimpPickable        *pickable);
const Babl    * gimp_pickable_get_format            (GimpPickable        *pickable);
const Babl    * gimp_pickable_get_format_with_alpha (GimpPickable        *pickable);
GeglBuffer    * gimp_pickable_get_buffer            (GimpPickable        *pickable);
gboolean        gimp_pickable_get_pixel_at          (GimpPickable        *pickable,
                                                     gint                 x,
                                                     gint                 y,
                                                     const Babl          *format,
                                                     gpointer             pixel);
gboolean        gimp_pickable_get_color_at          (GimpPickable        *pickable,
                                                     gint                 x,
                                                     gint                 y,
                                                     GimpRGB             *color);
gdouble         gimp_pickable_get_opacity_at        (GimpPickable        *pickable,
                                                     gint                 x,
                                                     gint                 y);
void            gimp_pickable_get_pixel_average     (GimpPickable        *pickable,
                                                     const GeglRectangle *rect,
                                                     const Babl          *format,
                                                     gpointer             pixel);
void            gimp_pickable_pixel_to_srgb         (GimpPickable        *pickable,
                                                     const Babl          *format,
                                                     gpointer             pixel,
                                                     GimpRGB             *color);
void            gimp_pickable_srgb_to_pixel         (GimpPickable        *pickable,
                                                     const GimpRGB       *color,
                                                     const Babl          *format,
                                                     gpointer             pixel);
void            gimp_pickable_srgb_to_image_color   (GimpPickable        *pickable,
                                                     const GimpRGB       *color,
                                                     GimpRGB             *image_color);

gboolean        gimp_pickable_pick_color            (GimpPickable        *pickable,
                                                     gint                 x,
                                                     gint                 y,
                                                     gboolean             sample_average,
                                                     gdouble              average_radius,
                                                     gpointer             pixel,
                                                     GimpRGB             *color);


#endif  /* __GIMP_PICKABLE_H__ */
