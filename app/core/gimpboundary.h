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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef  __BOUNDARY_H__
#define  __BOUNDARY_H__


/* half intensity for mask */
#define BOUNDARY_HALF_WAY 127


typedef enum
{
  BOUNDARY_WITHIN_BOUNDS,
  BOUNDARY_IGNORE_BOUNDS
} BoundaryType;


struct _BoundSeg
{
  gint   x1;
  gint   y1;
  gint   x2;
  gint   y2;
  guint  open    : 1;
  guint  visited : 1;
};


BoundSeg * boundary_find      (PixelRegion    *maskPR,
                               BoundaryType    type,
                               gint            x1,
                               gint            y1,
                               gint            x2,
                               gint            y2,
                               guchar          threshold,
                               gint           *num_segs);
BoundSeg * boundary_sort      (const BoundSeg *segs,
                               gint            num_segs,
                               gint           *num_groups);
BoundSeg * boundary_simplify  (BoundSeg       *sorted_segs,
                               gint            num_groups,
                               gint           *num_segs);

/* offsets in-place */
void       boundary_offset    (BoundSeg       *segs,
                               gint            num_segs,
                               gint            off_x,
                               gint            off_y);


#endif  /*  __BOUNDARY_H__  */
