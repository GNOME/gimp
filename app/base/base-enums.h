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

#ifndef __BASE_ENUMS_H__
#define __BASE_ENUMS_H__

#if 0
   This file is parsed by two scripts, enumgen.pl in tools/pdbgen
   and glib-mkenums. All enums that are not marked with /*< pdb-skip >*/
   are exported to libgimp and the PDB. Enums that are not marked with
   /*< skip >*/ are registered with the GType system. If you want the
   enum to be skipped by both scripts, you have to use /*< pdb-skip >*/
   _before_ /*< skip >*/. 

   All enum values that are marked with /*< skip >*/ are skipped for
   both targets.
#endif


/* 
 * these enums that are registered with the type system
 */

#define GIMP_TYPE_INTERPOLATION_TYPE (gimp_interpolation_type_get_type ())

GType gimp_interpolation_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_LINEAR_INTERPOLATION,
  GIMP_CUBIC_INTERPOLATION,
  GIMP_NEAREST_NEIGHBOR_INTERPOLATION
} GimpInterpolationType;


#define GIMP_TYPE_LAYER_MODE_EFFECTS (gimp_layer_mode_effects_get_type ())

GType gimp_layer_mode_effects_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_NORMAL_MODE,
  GIMP_DISSOLVE_MODE,
  GIMP_BEHIND_MODE,
  GIMP_MULTIPLY_MODE,
  GIMP_SCREEN_MODE,
  GIMP_OVERLAY_MODE,
  GIMP_DIFFERENCE_MODE,
  GIMP_ADDITION_MODE,
  GIMP_SUBTRACT_MODE,
  GIMP_DARKEN_ONLY_MODE,
  GIMP_LIGHTEN_ONLY_MODE,
  GIMP_HUE_MODE,
  GIMP_SATURATION_MODE,
  GIMP_COLOR_MODE,
  GIMP_VALUE_MODE,
  GIMP_DIVIDE_MODE,
  GIMP_DODGE_MODE,
  GIMP_BURN_MODE,
  GIMP_HARDLIGHT_MODE,
  GIMP_COLOR_ERASE_MODE,
  GIMP_ERASE_MODE,         /*< skip >*/
  GIMP_REPLACE_MODE,       /*< skip >*/
  GIMP_ANTI_ERASE_MODE     /*< skip >*/
} GimpLayerModeEffects;


#define GIMP_TYPE_CHECK_SIZE (gimp_check_size_get_type ())

GType gimp_check_size_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  GIMP_SMALL_CHECKS  = 0,
  GIMP_MEDIUM_CHECKS = 1,
  GIMP_LARGE_CHECKS  = 2
} GimpCheckSize;


#define GIMP_TYPE_CHECK_TYPE (gimp_check_type_get_type ())

GType gimp_check_type_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  GIMP_LIGHT_CHECKS = 0,
  GIMP_GRAY_CHECKS  = 1,
  GIMP_DARK_CHECKS  = 2,
  GIMP_WHITE_ONLY   = 3,
  GIMP_GRAY_ONLY    = 4,
  GIMP_BLACK_ONLY   = 5
} GimpCheckType;


/*
 * non-registered enums; register them if needed
 */

typedef enum  /*< skip >*/ 
{
  GIMP_NORMAL_CONVOL,      /*  Negative numbers truncated  */
  GIMP_ABSOLUTE_CONVOL,    /*  Absolute value              */
  GIMP_NEGATIVE_CONVOL     /*  add 127 to values           */
} GimpConvolutionType;

typedef enum  /*< skip >*/
{
  GIMP_VALUE_LUT,
  GIMP_RED_LUT,
  GIMP_GREEN_LUT,
  GIMP_BLUE_LUT,
  GIMP_ALPHA_LUT,
  GIMP_GRAY_LUT = 0  /*< skip >*/
} GimpChannelLutType;

typedef enum  /*< pdb-skip >*/ /*< skip >*/
{
  GIMP_HISTOGRAM_VALUE = 0,
  GIMP_HISTOGRAM_RED   = 1,
  GIMP_HISTOGRAM_GREEN = 2,
  GIMP_HISTOGRAM_BLUE  = 3,
  GIMP_HISTOGRAM_ALPHA = 4
} GimpHistogramChannel;


#endif /* __BASE_ENUMS_H__ */
