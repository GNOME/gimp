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
#ifndef __BRUSH_SCALE_H__
#define __BRUSH_SCALE_H__

#include "temp_buf.h"

/*  the pixmap for the brush_scale_indicator  */
#define brush_scale_indicator_width 7
#define brush_scale_indicator_height 7

#define WHT {255,255,255}
#define BLK {  0,  0,  0}

static unsigned char brush_scale_indicator_bits[7][7][3] = 
{
  { WHT, WHT, WHT, WHT, WHT, WHT, WHT },
  { WHT, WHT, WHT, BLK, WHT, WHT, WHT },
  { WHT, WHT, WHT, BLK, WHT, WHT, WHT },
  { WHT, BLK, BLK, BLK, BLK, BLK, WHT },
  { WHT, WHT, WHT, BLK, WHT, WHT, WHT },
  { WHT, WHT, WHT, BLK, WHT, WHT, WHT },
  { WHT, WHT, WHT, WHT, WHT, WHT, WHT }
};

#undef WHT
#undef BLK

/*  functions   */

MaskBuf * brush_scale_mask   (MaskBuf *, int, int);
MaskBuf * brush_scale_pixmap (MaskBuf *, int, int);

#endif  /*  __BRUSH_SCALE_H__  */







