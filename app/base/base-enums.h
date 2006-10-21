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


#define GIMP_TYPE_LAYER_MODE_EFFECTS (gimp_layer_mode_effects_get_type ())

GType gimp_layer_mode_effects_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_NORMAL_MODE,          /*< desc="Normal"               >*/
  GIMP_DISSOLVE_MODE,        /*< desc="Dissolve"             >*/
  GIMP_BEHIND_MODE,          /*< desc="Behind"               >*/
  GIMP_MULTIPLY_MODE,        /*< desc="Multiply"             >*/
  GIMP_SCREEN_MODE,          /*< desc="Screen"               >*/
  GIMP_OVERLAY_MODE,         /*< desc="Overlay"              >*/
  GIMP_DIFFERENCE_MODE,      /*< desc="Difference"           >*/
  GIMP_ADDITION_MODE,        /*< desc="Addition"             >*/
  GIMP_SUBTRACT_MODE,        /*< desc="Subtract"             >*/
  GIMP_DARKEN_ONLY_MODE,     /*< desc="Darken only"          >*/
  GIMP_LIGHTEN_ONLY_MODE,    /*< desc="Lighten only"         >*/
  GIMP_HUE_MODE,             /*< desc="Hue"                  >*/
  GIMP_SATURATION_MODE,      /*< desc="Saturation"           >*/
  GIMP_COLOR_MODE,           /*< desc="Color"                >*/
  GIMP_VALUE_MODE,           /*< desc="Value"                >*/
  GIMP_DIVIDE_MODE,          /*< desc="Divide"               >*/
  GIMP_DODGE_MODE,           /*< desc="Dodge"                >*/
  GIMP_BURN_MODE,            /*< desc="Burn"                 >*/
  GIMP_HARDLIGHT_MODE,       /*< desc="Hard light"           >*/
  GIMP_SOFTLIGHT_MODE,       /*< desc="Soft light"           >*/
  GIMP_GRAIN_EXTRACT_MODE,   /*< desc="Grain extract"        >*/
  GIMP_GRAIN_MERGE_MODE,     /*< desc="Grain merge"          >*/
  GIMP_COLOR_ERASE_MODE,     /*< desc="Color erase"          >*/
  GIMP_ERASE_MODE,           /*< pdb-skip, desc="Erase"      >*/
  GIMP_REPLACE_MODE,         /*< pdb-skip, desc="Replace"    >*/
  GIMP_ANTI_ERASE_MODE       /*< pdb-skip, desc="Anti erase" >*/
} GimpLayerModeEffects;


#define GIMP_TYPE_HUE_RANGE (gimp_hue_range_get_type ())

GType gimp_hue_range_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_ALL_HUES,
  GIMP_RED_HUES,
  GIMP_YELLOW_HUES,
  GIMP_GREEN_HUES,
  GIMP_CYAN_HUES,
  GIMP_BLUE_HUES,
  GIMP_MAGENTA_HUES
} GimpHueRange;


/*
 * non-registered enums; register them if needed
 */

typedef enum  /*< skip >*/
{
  GIMP_NORMAL_CONVOL,      /*  Negative numbers truncated  */
  GIMP_ABSOLUTE_CONVOL,    /*  Absolute value              */
  GIMP_NEGATIVE_CONVOL     /*  add 127 to values           */
} GimpConvolutionType;

typedef enum  /*< pdb-skip, skip >*/
{
  SIOX_REFINEMENT_NO_CHANGE          = 0,
  SIOX_REFINEMENT_ADD_FOREGROUND     = (1 << 0),
  SIOX_REFINEMENT_ADD_BACKGROUND     = (1 << 1),
  SIOX_REFINEMENT_CHANGE_SENSITIVITY = (1 << 2),
  SIOX_REFINEMENT_CHANGE_SMOOTHNESS  = (1 << 3),
  SIOX_REFINEMENT_CHANGE_MULTIBLOB   = (1 << 4),
  SIOX_REFINEMENT_RECALCULATE        = 0xFF
} SioxRefinementType;

#endif /* __BASE_ENUMS_H__ */
