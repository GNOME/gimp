/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpcolorselect.h
 * Copyright (C) 2002 Michael Natterer <mitch@gimp.org>
 *
 * based on color_notebook module
 * Copyright (C) 1998 Austin Donnelly <austin@greenend.org.uk>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_COLOR_SELECT_H__
#define __GIMP_COLOR_SELECT_H__

G_BEGIN_DECLS


#define GIMP_TYPE_COLOR_SELECT (gimp_color_select_get_type ())
G_DECLARE_FINAL_TYPE (GimpColorSelect, gimp_color_select, GIMP, COLOR_SELECT, GimpColorSelector)


G_END_DECLS

#endif /* __GIMP_COLOR_SELECT_H__ */
