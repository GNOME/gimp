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

#ifndef __LIGMA_BRUSH_PRIVATE_H__
#define __LIGMA_BRUSH_PRIVATE_H__


struct _LigmaBrushPrivate
{
  LigmaTempBuf     *mask;           /*  the actual mask                    */
  LigmaTempBuf     *blurred_mask;    /*  blurred actual mask cached          */
  LigmaTempBuf     *pixmap;         /*  optional pixmap data               */
  LigmaTempBuf     *blurred_pixmap;  /*  optional pixmap data blurred cache  */

  gdouble          blur_hardness;

  gint             n_horz_mipmaps;
  gint             n_vert_mipmaps;
  LigmaTempBuf    **mask_mipmaps;
  LigmaTempBuf    **pixmap_mipmaps;

  gint             spacing;    /*  brush's spacing                */
  LigmaVector2      x_axis;     /*  for calculating brush spacing  */
  LigmaVector2      y_axis;     /*  for calculating brush spacing  */

  gint             use_count;  /*  for keeping the caches alive   */
  LigmaBrushCache  *mask_cache;
  LigmaBrushCache  *pixmap_cache;
  LigmaBrushCache  *boundary_cache;
};


#endif /* __LIGMA_BRUSH_PRIVATE_H__ */
