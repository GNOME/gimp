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
   This file is parsed by two scripts, enumgen.pl in tools/pdbgen,
   and gimp-mkenums. All enums that are not marked with
   /*< pdb-skip >*/ are exported to libgimp and the PDB. Enums that are
   not marked with /*< skip >*/ are registered with the GType system.
   If you want the enum to be skipped by both scripts, you have to use
   /*< pdb-skip, skip >*/.

   The same syntax applies to enum values.
#endif


/*
 * these enums that are registered with the type system
 */

#define GIMP_TYPE_CURVE_TYPE (gimp_curve_type_get_type ())

GType gimp_curve_type_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  GIMP_CURVE_SMOOTH,   /*< desc="Smooth"   >*/
  GIMP_CURVE_FREE      /*< desc="Freehand" >*/
} GimpCurveType;


#define GIMP_TYPE_HISTOGRAM_CHANNEL (gimp_histogram_channel_get_type ())

GType gimp_histogram_channel_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_HISTOGRAM_VALUE = 0,  /*< desc="Value" >*/
  GIMP_HISTOGRAM_RED   = 1,  /*< desc="Red"   >*/
  GIMP_HISTOGRAM_GREEN = 2,  /*< desc="Green" >*/
  GIMP_HISTOGRAM_BLUE  = 3,  /*< desc="Blue"  >*/
  GIMP_HISTOGRAM_ALPHA = 4,  /*< desc="Alpha" >*/
  GIMP_HISTOGRAM_RGB   = 5   /*< desc="RGB", pdb-skip >*/
} GimpHistogramChannel;


#define GIMP_TYPE_INTERPOLATION_TYPE (gimp_interpolation_type_get_type ())

GType gimp_interpolation_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_INTERPOLATION_NONE,   /*< desc="None (Fastest)" >*/
  GIMP_INTERPOLATION_LINEAR, /*< desc="Linear"         >*/
  GIMP_INTERPOLATION_CUBIC   /*< desc="Cubic (Best)"   >*/
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
  GIMP_SOFTLIGHT_MODE,
  GIMP_GRAIN_EXTRACT_MODE,
  GIMP_GRAIN_MERGE_MODE,
  GIMP_COLOR_ERASE_MODE,
  GIMP_ERASE_MODE,           /*< pdb-skip, skip >*/
  GIMP_REPLACE_MODE,         /*< pdb-skip, skip >*/
  GIMP_ANTI_ERASE_MODE       /*< pdb-skip, skip >*/
} GimpLayerModeEffects;


#define GIMP_TYPE_TRANSFER_MODE (gimp_transfer_mode_get_type ())

GType gimp_transfer_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_SHADOWS,     /*< desc="Shadows"    >*/
  GIMP_MIDTONES,    /*< desc="Midtones"   >*/
  GIMP_HIGHLIGHTS   /*< desc="Highlights" >*/
} GimpTransferMode;


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
  GIMP_ALL_HUES,
  GIMP_RED_HUES,
  GIMP_YELLOW_HUES,
  GIMP_GREEN_HUES,
  GIMP_CYAN_HUES,
  GIMP_BLUE_HUES,
  GIMP_MAGENTA_HUES
} GimpHueRange;


#endif /* __BASE_ENUMS_H__ */
