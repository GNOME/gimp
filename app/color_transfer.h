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
#ifndef __COLOR_TRANSFER_H__
#define __COLOR_TRANSFER_H__

/*************************/
/*  color transfer data  */

/*  for lightening  */
extern double  highlights_add[];
extern double  midtones_add[];
extern double  shadows_add[];

/*  for darkening  */
extern double  highlights_sub[];
extern double  midtones_sub[];
extern double  shadows_sub[];

/*  color transfer functions  */
void           color_transfer_init (void);

#endif  /*  __COLOR_TRANSFER_H__  */
