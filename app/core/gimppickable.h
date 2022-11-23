/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * ligmapickable.h
 * Copyright (C) 2004  Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_PICKABLE_H__
#define __LIGMA_PICKABLE_H__


#define LIGMA_TYPE_PICKABLE (ligma_pickable_get_type ())
G_DECLARE_INTERFACE (LigmaPickable, ligma_pickable, LIGMA, PICKABLE, GObject)

struct _LigmaPickableInterface
{
  GTypeInterface base_iface;

  /*  virtual functions  */
  void            (* flush)                 (LigmaPickable        *pickable);
  LigmaImage     * (* get_image)             (LigmaPickable        *pickable);
  const Babl    * (* get_format)            (LigmaPickable        *pickable);
  const Babl    * (* get_format_with_alpha) (LigmaPickable        *pickable);
  GeglBuffer    * (* get_buffer)            (LigmaPickable        *pickable);
  gboolean        (* get_pixel_at)          (LigmaPickable        *pickable,
                                             gint                 x,
                                             gint                 y,
                                             const Babl          *format,
                                             gpointer             pixel);
  gdouble         (* get_opacity_at)        (LigmaPickable        *pickable,
                                             gint                 x,
                                             gint                 y);
  void            (* get_pixel_average)     (LigmaPickable        *pickable,
                                             const GeglRectangle *rect,
                                             const Babl          *format,
                                             gpointer             pixel);
  void            (* pixel_to_srgb)         (LigmaPickable        *pickable,
                                             const Babl          *format,
                                             gpointer             pixel,
                                             LigmaRGB             *color);
  void            (* srgb_to_pixel)         (LigmaPickable        *pickable,
                                             const LigmaRGB       *color,
                                             const Babl          *format,
                                             gpointer             pixel);
};


void            ligma_pickable_flush                 (LigmaPickable        *pickable);
LigmaImage     * ligma_pickable_get_image             (LigmaPickable        *pickable);
const Babl    * ligma_pickable_get_format            (LigmaPickable        *pickable);
const Babl    * ligma_pickable_get_format_with_alpha (LigmaPickable        *pickable);
GeglBuffer    * ligma_pickable_get_buffer            (LigmaPickable        *pickable);
gboolean        ligma_pickable_get_pixel_at          (LigmaPickable        *pickable,
                                                     gint                 x,
                                                     gint                 y,
                                                     const Babl          *format,
                                                     gpointer             pixel);
gboolean        ligma_pickable_get_color_at          (LigmaPickable        *pickable,
                                                     gint                 x,
                                                     gint                 y,
                                                     LigmaRGB             *color);
gdouble         ligma_pickable_get_opacity_at        (LigmaPickable        *pickable,
                                                     gint                 x,
                                                     gint                 y);
void            ligma_pickable_get_pixel_average     (LigmaPickable        *pickable,
                                                     const GeglRectangle *rect,
                                                     const Babl          *format,
                                                     gpointer             pixel);
void            ligma_pickable_pixel_to_srgb         (LigmaPickable        *pickable,
                                                     const Babl          *format,
                                                     gpointer             pixel,
                                                     LigmaRGB             *color);
void            ligma_pickable_srgb_to_pixel         (LigmaPickable        *pickable,
                                                     const LigmaRGB       *color,
                                                     const Babl          *format,
                                                     gpointer             pixel);
void            ligma_pickable_srgb_to_image_color   (LigmaPickable        *pickable,
                                                     const LigmaRGB       *color,
                                                     LigmaRGB             *image_color);

gboolean        ligma_pickable_pick_color            (LigmaPickable        *pickable,
                                                     gint                 x,
                                                     gint                 y,
                                                     gboolean             sample_average,
                                                     gdouble              average_radius,
                                                     gpointer             pixel,
                                                     LigmaRGB             *color);


#endif  /* __LIGMA_PICKABLE_H__ */
