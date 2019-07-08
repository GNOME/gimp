/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpitem-exclusive.h
 * Copyright (C) 2011 Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_ITEM_EXCLUSIVE_H__
#define __GIMP_ITEM_EXCLUSIVE_H__


void   gimp_item_toggle_exclusive_visible (GimpItem    *item,
                                           GimpContext *context);
void   gimp_item_toggle_exclusive_linked  (GimpItem    *item,
                                           GimpContext *context);


#endif /* __GIMP_ITEM_EXCLUSIVE_H__ */
