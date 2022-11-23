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

#ifndef  __LIGMA_IMAGE_PICK_COLOR_H__
#define  __LIGMA_IMAGE_PICK_COLOR_H__


gboolean   ligma_image_pick_color (LigmaImage     *image,
                                  GList         *drawables,
                                  gint           x,
                                  gint           y,
                                  gboolean       show_all,
                                  gboolean       sample_merged,
                                  gboolean       sample_average,
                                  gdouble        average_radius,
                                  const Babl   **sample_format,
                                  gpointer       pixel,
                                  LigmaRGB       *color);


#endif  /* __LIGMA_IMAGE_PICK_COLOR_H__ */
