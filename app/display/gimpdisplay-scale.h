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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifndef __SCALE_H__
#define __SCALE_H__

#include "gdisplay.h"

#define ZOOMIN  0
#define ZOOMOUT 1

/*  Functions  */

void change_scale (GDisplay *, int);
void resize_display (GDisplay *, int, int);
void shrink_wrap_display (GDisplay *);
void setup_scale (GDisplay *);
void scale_image (GDisplay *, long, long, long, long);
void bounds_checking (GDisplay *);


#endif  /*  __SCALE_H__  */
