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
#ifndef __GXIMAGE_H__
#define __GXIMAGE_H__

#define GXIMAGE_WIDTH       256
#define GXIMAGE_HEIGHT      256

void     gximage_init           (void);
void     gximage_free           (void);

void
gximage_put (GdkWindow *win, int x, int y, int w, int h, int xdith, int ydith);
guchar*  gximage_get_data       (void);
int      gximage_get_bpp        (void);
int      gximage_get_bpl        (void);
int      gximage_get_byte_order (void);

#endif  /*  __GXIMAGE_H__  */
