/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-2003 Spencer Kimball, Peter Mattis, and others
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

#ifndef __GIMP_TRANSFORM_REGION_H__
#define __GIMP_TRANSFORM_REGION_H__


void   gimp_transform_region (GimpPickable          *pickable,
                              GimpContext           *context,
                              TileManager           *orig_tiles,
                              PixelRegion           *destPR,
                              gint                   dest_x1,
                              gint                   dest_y1,
                              gint                   dest_x2,
                              gint                   dest_y2,
                              const GimpMatrix3     *matrix,
                              GimpInterpolationType  interpolation_type,
                              gint                   recursion_level,
                              GimpProgress          *progress);


#endif /* __GIMP_TRANSFORM_REGION_H__ */
