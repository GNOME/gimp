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

#ifndef __SCRIPT_FU_COLOR_H__
#define __SCRIPT_FU_COLOR_H__


typedef GeglColor * SFColorType;


/* Methods on SFColorType. */
GeglColor* sf_color_get_gegl_color           (SFColorType  arg_value);
void       sf_color_set_from_gegl_color      (SFColorType *arg_value,
                                              GeglColor   *color);

gchar*     sf_color_get_repr                 (SFColorType  arg_value);

/* Other conversions. */
gchar*     sf_color_get_repr_from_gegl_color (GeglColor   *color);
GeglColor* sf_color_get_color_from_name      (gchar       *color_name);

#endif /*  __SCRIPT_FU_COLOR__  */
