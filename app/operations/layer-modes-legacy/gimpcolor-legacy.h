/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcolor-legacy.h
 * Copyright (C) 2024
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

#ifndef __GIMP_COLOR_LEGACY_H__
#define __GIMP_COLOR_LEGACY_H__


void   gimp_rgb_to_hsv_legacy   (gdouble *rgb,
                                 gdouble *hsv);
void   gimp_hsv_to_rgb_legacy   (gdouble *hsv,
                                 gdouble *rgb);

void   gimp_rgb_to_hsl_legacy   (gdouble *rgb,
                                 gdouble *hsl);
void   gimp_hsl_to_rgb_legacy   (gdouble *hsl,
                                 gdouble *rgb);


#endif /* __GIMP_COLOR_LEGACY_H__ */
