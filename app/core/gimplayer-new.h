/* LIGMA - The GNU Image Manipulation Program
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

#ifndef __LIGMA_LAYER_NEW_H__
#define __LIGMA_LAYER_NEW_H__


LigmaLayer * ligma_layer_new                  (LigmaImage        *image,
                                             gint              width,
                                             gint              height,
                                             const Babl       *format,
                                             const gchar      *name,
                                             gdouble           opacity,
                                             LigmaLayerMode     mode);

LigmaLayer * ligma_layer_new_from_buffer      (LigmaBuffer       *buffer,
                                             LigmaImage        *dest_image,
                                             const Babl       *format,
                                             const gchar      *name,
                                             gdouble           opacity,
                                             LigmaLayerMode     mode);
LigmaLayer * ligma_layer_new_from_gegl_buffer (GeglBuffer       *buffer,
                                             LigmaImage        *dest_image,
                                             const Babl       *format,
                                             const gchar      *name,
                                             gdouble           opacity,
                                             LigmaLayerMode     mode,
                                             LigmaColorProfile *buffer_profile);
LigmaLayer * ligma_layer_new_from_pixbuf      (GdkPixbuf        *pixbuf,
                                             LigmaImage        *dest_image,
                                             const Babl       *format,
                                             const gchar      *name,
                                             gdouble           opacity,
                                             LigmaLayerMode     mode);


#endif /* __LIGMA_LAYER_NEW_H__ */
