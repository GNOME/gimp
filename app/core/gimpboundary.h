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

#ifndef  __GIMP_BOUNDARY_H__
#define  __GIMP_BOUNDARY_H__


/* half intensity for mask */
#define GIMP_BOUNDARY_HALF_WAY 0.5


typedef enum
{
  GIMP_BOUNDARY_WITHIN_BOUNDS,
  GIMP_BOUNDARY_IGNORE_BOUNDS
} GimpBoundaryType;


struct _GimpBoundSeg
{
  gint   x1;
  gint   y1;
  gint   x2;
  gint   y2;
  guint  open    : 1;
  guint  visited : 1;
};


GimpBoundSeg * gimp_boundary_find      (GeglBuffer          *buffer,
                                        const GeglRectangle *region,
                                        const Babl          *format,
                                        GimpBoundaryType     type,
                                        gint                 x1,
                                        gint                 y1,
                                        gint                 x2,
                                        gint                 y2,
                                        gfloat               threshold,
                                        gint                *num_segs);
GimpBoundSeg * gimp_boundary_sort      (const GimpBoundSeg  *segs,
                                        gint                 num_segs,
                                        gint                *num_groups);
GimpBoundSeg * gimp_boundary_simplify  (GimpBoundSeg        *sorted_segs,
                                        gint                 num_groups,
                                        gint                *num_segs);

/* offsets in-place */
void       gimp_boundary_offset        (GimpBoundSeg        *segs,
                                        gint                 num_segs,
                                        gint                 off_x,
                                        gint                 off_y);


#endif  /*  __GIMP_BOUNDARY_H__  */
