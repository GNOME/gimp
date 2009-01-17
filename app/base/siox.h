/*
 * SIOX: Simple Interactive Object Extraction
 *
 * For algorithm documentation refer to:
 * G. Friedland, K. Jantz, L. Knipping, R. Rojas:
 * "Image Segmentation by Uniform Color Clustering
 *  -- Approach and Benchmark Results",
 * Technical Report B-05-07, Department of Computer Science,
 * Freie Universitaet Berlin, June 2005.
 * http://www.inf.fu-berlin.de/inst/pubs/tr-b-05-07.pdf
 *
 * See http://www.siox.org/ for more information.
 *
 * Algorithm idea by Gerald Friedland.
 * This implementation is Copyright (C) 2005
 * by Gerald Friedland <fland@inf.fu-berlin.de>
 * and Kristian Jantz <jantz@inf.fu-berlin.de>.
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


#ifndef __SIOX_H__
#define __SIOX_H__


#define SIOX_DEFAULT_SMOOTHNESS     3

#define SIOX_DEFAULT_SENSITIVITY_L  0.64
#define SIOX_DEFAULT_SENSITIVITY_A  1.28
#define SIOX_DEFAULT_SENSITIVITY_B  2.56

/*  FIXME: turn this into an enum  */
#define SIOX_DRB_ADD                0
#define SIOX_DRB_SUBTRACT           1


typedef void (* SioxProgressFunc) (gpointer  progress_data,
                                   gdouble   fraction);

SioxState * siox_init               (TileManager        *pixels,
                                     const guchar       *colormap,
                                     gint                offset_x,
                                     gint                offset_y,
                                     gint                x,
                                     gint                y,
                                     gint                width,
                                     gint                height);
void        siox_foreground_extract (SioxState          *state,
                                     SioxRefinementType  refinement,
                                     TileManager        *mask,
                                     gint                x1,
                                     gint                y1,
                                     gint                x2,
                                     gint                y2,
                                     gint                smoothness,
                                     const gdouble       sensitivity[3],
                                     gboolean            multiblob,
                                     SioxProgressFunc    progress_callback,
                                     gpointer            progress_data);
void        siox_done               (SioxState          *state);

void        siox_drb                (SioxState          *state,
                                     TileManager        *mask,
                                     gint                x,
                                     gint                y,
                                     gint                brush_radius,
                                     gint                brush_mode,
                                     gfloat              threshold);


#endif /* __SIOX_H__ */
