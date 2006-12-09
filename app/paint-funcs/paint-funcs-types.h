/* GIMP - The GNU Image Manipulation Program
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

#ifndef __PAINT_FUNCS_TYPES_H__
#define __PAINT_FUNCS_TYPES_H__


#include "base/base-types.h"


/* Opacities */
#define TRANSPARENT_OPACITY        0
#define OPAQUE_OPACITY             255


/*  Lay down the groundwork for layer construction...
 *  This includes background images for indexed or non-alpha
 *  images, floating selections, selective display of intensity
 *  channels, and display of arbitrary mask channels
 */
typedef enum
{
  INITIAL_CHANNEL_MASK = 0,
  INITIAL_CHANNEL_SELECTION,
  INITIAL_INDEXED,
  INITIAL_INDEXED_ALPHA,
  INITIAL_INTENSITY,
  INITIAL_INTENSITY_ALPHA
} InitialMode;

/*  Combine two source regions with the help of an optional mask
 *  region into a destination region.  This is used for constructing
 *  layer projections, and for applying image patches to an image
 */
typedef enum
{
  NO_COMBINATION = 0,
  COMBINE_INDEXED_INDEXED,
  COMBINE_INDEXED_INDEXED_A,
  COMBINE_INDEXED_A_INDEXED_A,
  COMBINE_INTEN_A_INDEXED,
  COMBINE_INTEN_A_INDEXED_A,
  COMBINE_INTEN_A_CHANNEL_MASK,
  COMBINE_INTEN_A_CHANNEL_SELECTION,
  COMBINE_INTEN_INTEN,
  COMBINE_INTEN_INTEN_A,
  COMBINE_INTEN_A_INTEN,
  COMBINE_INTEN_A_INTEN_A,

  /*  Non-conventional combination modes  */
  BEHIND_INTEN,
  BEHIND_INDEXED,
  REPLACE_INTEN,
  REPLACE_INDEXED,
  ERASE_INTEN,
  ERASE_INDEXED,
  ANTI_ERASE_INTEN,
  ANTI_ERASE_INDEXED,
  COLOR_ERASE_INTEN
} CombinationMode;


#endif  /*  __PAINT_FUNCS_TYPES_H__  */
