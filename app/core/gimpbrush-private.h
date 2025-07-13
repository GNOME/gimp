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

#pragma once


struct _GimpBrushPrivate
{
  GimpTempBuf     *mask;           /*  the actual mask                    */
  GimpTempBuf     *blurred_mask;    /*  blurred actual mask cached          */
  GimpTempBuf     *pixmap;         /*  optional pixmap data               */
  GimpTempBuf     *blurred_pixmap;  /*  optional pixmap data blurred cache  */

  gdouble          blur_hardness;

  gint             n_horz_mipmaps;
  gint             n_vert_mipmaps;
  GimpTempBuf    **mask_mipmaps;
  GimpTempBuf    **pixmap_mipmaps;

  gint             spacing;    /*  brush's spacing                */
  GimpVector2      x_axis;     /*  for calculating brush spacing  */
  GimpVector2      y_axis;     /*  for calculating brush spacing  */

  gint             use_count;  /*  for keeping the caches alive   */
  GimpBrushCache  *mask_cache;
  GimpBrushCache  *pixmap_cache;
  GimpBrushCache  *boundary_cache;
};
