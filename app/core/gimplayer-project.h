/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimplayer-project.h
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

#ifndef __GIMP_LAYER_PROJECT_H__
#define __GIMP_LAYER_PROJECT_H__


void   gimp_layer_project_region (GimpDrawable *drawable,
                                  gint          x,
                                  gint          y,
                                  gint          width,
                                  gint          height,
                                  PixelRegion  *projPR,
                                  gboolean      combine);


#endif /* __GIMP_LAYER_PROJECT_H__ */
