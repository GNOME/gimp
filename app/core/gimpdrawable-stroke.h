/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdrawable-stroke.h
 * Copyright (C) 2003 Simon Budig  <simon@gimp.org>
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

#ifndef  __GIMP_DRAWABLE_STROKE_H__
#define  __GIMP_DRAWABLE_STROKE_H__


void   gimp_drawable_stroke_boundary (GimpDrawable      *drawable,
                                      GimpStrokeOptions *options,
                                      const BoundSeg    *bound_segs,
                                      gint               n_bound_segs,
                                      gint               offset_x,
                                      gint               offset_y);

void   gimp_drawable_stroke_vectors  (GimpDrawable      *drawable,
                                      GimpStrokeOptions *options,
                                      GimpVectors       *vectors);


#endif  /*  __GIMP_DRAWABLE_STROKE_H__  */
