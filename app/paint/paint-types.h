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

#ifndef __PAINT_TYPES_H__
#define __PAINT_TYPES_H__


#include "core/core-types.h"


/*  objects  */

typedef struct _GimpPaintCore    GimpPaintCore;
typedef struct _GimpPaintOptions GimpPaintOptions;


/*  enums  */

/* Brush application types  */
typedef enum
{
  HARD,     /* pencil */
  SOFT,     /* paintbrush */
  PRESSURE  /* paintbrush with variable pressure */
} BrushApplicationMode;

/* Paint application modes  */
typedef enum
{
  CONSTANT,    /*< nick=CONTINUOUS >*/ /* pencil, paintbrush, airbrush, clone */
  INCREMENTAL  /* convolve, smudge */
} PaintApplicationMode;

/* gradient paint modes */
typedef enum
{
  ONCE_FORWARD,    /* paint through once, then stop */
  ONCE_BACKWARDS,  /* paint once, then stop, but run the gradient the other way */
  LOOP_SAWTOOTH,   /* keep painting, looping through the grad start->end,start->end /|/|/| */
  LOOP_TRIANGLE,   /* keep paiting, looping though the grad start->end,end->start /\/\/\/  */
  ONCE_END_COLOR   /* paint once, but keep painting with the end color */
} GradientPaintMode;

typedef enum
{
  DODGE,
  BURN
} DodgeBurnType;

typedef enum
{
  BLUR_CONVOLVE,
  SHARPEN_CONVOLVE,
  CUSTOM_CONVOLVE
} ConvolveType;

typedef enum
{
  IMAGE_CLONE,
  PATTERN_CLONE
} CloneType;


#endif /* __PAINT_TYPES_H__ */
