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
#ifndef __ELLIPSE_SELECT_H__
#define __ELLIPSE_SELECT_H__

#include "gimpimageF.h"
#include "tools.h"

/*  ellipse select functions  */

void          ellipse_select_draw (Tool *);
void          ellipse_select      (GimpImage *, int, int, int, int, int,
    				   int, int, double);

Tool *        tools_new_ellipse_select    (void);
void          tools_free_ellipse_select   (Tool *);

#endif  /*  __ELLIPSE_SELECT_H__  */
