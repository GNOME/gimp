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

#ifndef __APPENUMS_H__
#define __APPENUMS_H__


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

typedef enum  /*< chop=ADD_ >*/
{
  ADD_WHITE_MASK,
  ADD_BLACK_MASK,
  ADD_ALPHA_MASK,
  ADD_SELECTION_MASK,
  ADD_INV_SELECTION_MASK
} AddMaskType;

/* gradient paint modes */
typedef enum
{
  ONCE_FORWARD,    /* paint through once, then stop */
  ONCE_BACKWARDS,  /* paint once, then stop, but run the gradient the other way */
  LOOP_SAWTOOTH,   /* keep painting, looping through the grad start->end,start->end /|/|/| */
  LOOP_TRIANGLE,   /* keep paiting, looping though the grad start->end,end->start /\/\/\/  */
  ONCE_END_COLOR   /* paint once, but keep painting with the end color */
} GradientPaintMode;

typedef enum /*< skip >*/
{
  ORIENTATION_UNKNOWN,
  ORIENTATION_HORIZONTAL,
  ORIENTATION_VERTICAL
} InternalOrientationType;

typedef enum
{
  HORIZONTAL,
  VERTICAL,
  UNKNOWN
} OrientationType;

typedef enum /*< skip >*/
{
  GIMP_ZOOM_IN,
  GIMP_ZOOM_OUT
} GimpZoomType;

/*  Selection Boolean operations  */
typedef enum /*< chop=CHANNEL_OP_ >*/
{
  CHANNEL_OP_ADD,
  CHANNEL_OP_SUB,
  CHANNEL_OP_REPLACE,
  CHANNEL_OP_INTERSECT
} ChannelOps;

typedef enum /*< skip >*/
{
  SELECTION_ADD       = CHANNEL_OP_ADD,
  SELECTION_SUB       = CHANNEL_OP_SUB,
  SELECTION_REPLACE   = CHANNEL_OP_REPLACE,
  SELECTION_INTERSECT = CHANNEL_OP_INTERSECT,
  SELECTION_MOVE_MASK,
  SELECTION_MOVE,
  SELECTION_ANCHOR
} SelectOps;

/*  Plug-In run modes  */
typedef enum
{
  RUN_INTERACTIVE    = 0,
  RUN_NONINTERACTIVE = 1,
  RUN_WITH_LAST_VALS = 2
} RunModeType;

typedef enum /*< skip >*/
{
  CURSOR_MODE_TOOL_ICON,
  CURSOR_MODE_TOOL_CROSSHAIR,
  CURSOR_MODE_CROSSHAIR
} CursorMode;


#endif /* __APPENUMS_H__ */
