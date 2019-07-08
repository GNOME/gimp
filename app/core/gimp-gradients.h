/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-2002 Spencer Kimball, Peter Mattis, and others
 *
 * gimp-gradients.h
 * Copyright (C) 2002 Michael Natterer  <mitch@gimp.org>
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

#ifndef __GIMP_GRADIENTS__
#define __GIMP_GRADIENTS__


void           gimp_gradients_init               (Gimp *gimp);

GimpGradient * gimp_gradients_get_custom         (Gimp *gimp);
GimpGradient * gimp_gradients_get_fg_bg_rgb      (Gimp *gimp);
GimpGradient * gimp_gradients_get_fg_bg_hsv_ccw  (Gimp *gimp);
GimpGradient * gimp_gradients_get_fg_bg_hsv_cw   (Gimp *gimp);
GimpGradient * gimp_gradients_get_fg_transparent (Gimp *gimp);


#endif /* __GIMP_GRADIENTS__ */
