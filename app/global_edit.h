/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#ifndef __GLOBAL_EDIT_H__
#define __GLOBAL_EDIT_H__


TileManager * gimp_edit_cut          (GimpImage    *gimage,
				      GimpDrawable *drawable);
TileManager * gimp_edit_copy         (GimpImage    *gimage,
				      GimpDrawable *drawable);
GimpLayer   * gimp_edit_paste        (GimpImage    *gimage,
				      GimpDrawable *drawable,
				      TileManager  *paste,
				      gboolean      paste_into);
GimpImage   * gimp_edit_paste_as_new (GimpImage    *gimage,
				      TileManager  *tiles);
gboolean      gimp_edit_clear        (GimpImage    *gimage,
				      GimpDrawable *drawable);
gboolean      gimp_edit_fill         (GimpImage    *gimage,
				      GimpDrawable *drawable,
				      GimpFillType  fill_type);


#endif  /*  __GLOBAL_EDIT_H__  */
