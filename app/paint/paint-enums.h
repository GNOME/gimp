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

#ifndef __PAINT_ENUMS_H__
#define __PAINT_ENUMS_H__

#if 0
   This file is parsed by two scripts, enumgen.pl in tools/pdbgen
   and gimp-mkenums. All enums that are not marked with /*< pdb-skip >*/
   are exported to libgimp and the PDB. Enums that are not marked with
   /*< skip >*/ are registered with the GType system. If you want the
   enum to be skipped by both scripts, you have to use /*< pdb-skip >*/
   _before_ /*< skip >*/. 

   All enum values that are marked with /*< skip >*/ are skipped for
   both targets.
#endif


/*
 * non-registered enums; register them if needed
 */

typedef enum  /*< skip >*/
{
  GIMP_BRUSH_HARD,       /* pencil */
  GIMP_BRUSH_SOFT,       /* paintbrush */
  GIMP_BRUSH_PRESSURE    /* paintbrush with variable pressure */
} GimpBrushApplicationMode;

typedef enum  /*< skip >*/
{
  GIMP_PAINT_CONSTANT,   /* pencil, paintbrush, airbrush, clone */
  GIMP_PAINT_INCREMENTAL /* convolve, smudge */
} GimpPaintApplicationMode;

typedef enum  /*< skip >*/
{
  GIMP_GRADIENT_ONCE_FORWARD,
  GIMP_GRADIENT_ONCE_BACKWARD,
  GIMP_GRADIENT_LOOP_SAWTOOTH,
  GIMP_GRADIENT_LOOP_TRIANGLE
} GimpGradientPaintMode;

typedef enum  /*< skip >*/
{
  GIMP_DODGE,
  GIMP_BURN
} GimpDodgeBurnType;

typedef enum  /*< skip >*/
{
  GIMP_BLUR_CONVOLVE,
  GIMP_SHARPEN_CONVOLVE,
  GIMP_CUSTOM_CONVOLVE
} GimpConvolveType;

typedef enum  /*< skip >*/
{
  GIMP_IMAGE_CLONE,
  GIMP_PATTERN_CLONE
} GimpCloneType;


#endif /* __PAINT_ENUMS_H__ */
