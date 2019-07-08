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

#ifndef __GIMP_DRAWABLE_FLOATING_SELECTION_H__
#define __GIMP_DRAWABLE_FLOATING_SELECTION_H__


GimpLayer  * gimp_drawable_get_floating_sel          (GimpDrawable *drawable);
void         gimp_drawable_attach_floating_sel       (GimpDrawable *drawable,
                                                      GimpLayer    *floating_sel);
void         gimp_drawable_detach_floating_sel       (GimpDrawable *drawable);
GimpFilter * gimp_drawable_get_floating_sel_filter   (GimpDrawable *drawable);

void         _gimp_drawable_add_floating_sel_filter  (GimpDrawable *drawable);


#endif /* __GIMP_DRAWABLE_FLOATING_SELECTION_H__ */
