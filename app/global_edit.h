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

#include "gimage.h"

TileManager * crop_buffer              (TileManager  *tiles,
					gint          border);

TileManager * edit_cut                 (GImage       *gimage,
					GimpDrawable *drawable);
TileManager * edit_copy                (GImage       *gimage,
					GimpDrawable *drawable);
GimpLayer   * edit_paste               (GImage       *gimage,
					GimpDrawable *drawable,
					TileManager  *paste,
					gboolean      paste_into);
gboolean      edit_paste_as_new        (GImage       *gimage,
					TileManager  *tiles);
gboolean      edit_clear               (GImage       *gimage,
					GimpDrawable *drawable);
gboolean      edit_fill                (GImage       *gimage,
					GimpDrawable *drawable,
					GimpFillType  fill_type);

gboolean      global_edit_cut          (GDisplay    *gdisp);
gboolean      global_edit_copy         (GDisplay    *gdisp);
gboolean      global_edit_paste        (GDisplay    *gdisp,
					gboolean     paste_into);
gboolean      global_edit_paste_as_new (GDisplay    *gdisp);
void          global_edit_free         (void);

gboolean      named_edit_cut           (GDisplay    *gdisp);
gboolean      named_edit_copy          (GDisplay    *gdisp);
gboolean      named_edit_paste         (GDisplay    *gdisp);
void          named_buffers_free       (void);

#endif  /*  __GLOBAL_EDIT_H__  */
