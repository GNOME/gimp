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
#ifndef  __BLEND_H__
#define  __BLEND_H__

#include "tools.h"
#include "procedural_db.h"

typedef enum
{
  Linear,
  BiLinear,
  Radial,
  Square,
  ConicalSymmetric,
  ConicalAsymmetric,
  ShapeburstAngular,
  ShapeburstSpherical,
  ShapeburstDimpled,
  SpiralClockwise,
  SpiralAntiClockwise,
  GradientTypeLast /*< skip >*/
} GradientType;

typedef enum
{
  FG_BG_RGB_MODE,
  FG_BG_HSV_MODE,
  FG_TRANS_MODE,
  CUSTOM_MODE,
  BLEND_MODE_LAST /*< skip >*/
} BlendMode;

typedef enum
{
  REPEAT_NONE,
  REPEAT_SAWTOOTH,
  REPEAT_TRIANGULAR,
  REPEAT_LAST /*< skip >*/
} RepeatMode;

Tool *      tools_new_blend   (void);
void        tools_free_blend  (Tool *);

/*  Procedure definition and marshalling function  */
extern ProcRecord blend_proc;

#endif  /*  __BLEND_H__  */
