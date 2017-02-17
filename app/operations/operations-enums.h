/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * operations-enums.h
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __OPERATIONS_ENUMS_H__
#define __OPERATIONS_ENUMS_H__


#define GIMP_TYPE_LAYER_COLOR_SPACE (gimp_layer_color_space_get_type ())

GType gimp_layer_color_space_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  GIMP_LAYER_COLOR_SPACE_AUTO,           /*< desc="Auto"             >*/
  GIMP_LAYER_COLOR_SPACE_RGB_LINEAR,     /*< desc="RGB (linear)"     >*/
  GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL, /*< desc="RGB (perceptual)" >*/
  GIMP_LAYER_COLOR_SPACE_LAB,            /*< desc="LAB"              >*/
} GimpLayerColorSpace;


#define GIMP_TYPE_LAYER_COMPOSITE_MODE (gimp_layer_composite_mode_get_type ())

GType gimp_layer_composite_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_LAYER_COMPOSITE_AUTO,      /*< desc="Auto"             >*/
  GIMP_LAYER_COMPOSITE_SRC_OVER,  /*< desc="Source over"      >*/
  GIMP_LAYER_COMPOSITE_SRC_ATOP,  /*< desc="Source atop"      >*/
  GIMP_LAYER_COMPOSITE_SRC_IN,    /*< desc="Source in"        >*/
  GIMP_LAYER_COMPOSITE_DST_ATOP   /*< desc="Destination atop" >*/
} GimpLayerCompositeMode;


#define GIMP_TYPE_LAYER_MODE (gimp_layer_mode_get_type ())

GType gimp_layer_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
  /*  Modes that exist since ancient times  */
  GIMP_LAYER_MODE_NORMAL,                /*< desc="Normal"                   >*/
  GIMP_LAYER_MODE_DISSOLVE,              /*< desc="Dissolve"                 >*/
  GIMP_LAYER_MODE_BEHIND,                /*< desc="Behind"                   >*/
  GIMP_LAYER_MODE_MULTIPLY_LEGACY,       /*< desc="Multiply (legacy)"        >*/
  GIMP_LAYER_MODE_SCREEN_LEGACY,         /*< desc="Screen (legacy)"          >*/
  GIMP_LAYER_MODE_OVERLAY_LEGACY,        /*< desc="Old broken Overlay"       >*/
  GIMP_LAYER_MODE_DIFFERENCE_LEGACY,     /*< desc="Difference (legacy)"      >*/
  GIMP_LAYER_MODE_ADDITION_LEGACY,       /*< desc="Addition (legacy)"        >*/
  GIMP_LAYER_MODE_SUBTRACT_LEGACY,       /*< desc="Subtract (legacy)"        >*/
  GIMP_LAYER_MODE_DARKEN_ONLY_LEGACY,    /*< desc="Darken only (legacy)"     >*/
  GIMP_LAYER_MODE_LIGHTEN_ONLY_LEGACY,   /*< desc="Lighten only (legacy)"    >*/
  GIMP_LAYER_MODE_HSV_HUE_LEGACY,        /*< desc="Hue (HSV) (legacy)"       >*/
  GIMP_LAYER_MODE_HSV_SATURATION_LEGACY, /*< desc="Saturation (HSV) (legacy)">*/
  GIMP_LAYER_MODE_HSV_COLOR_LEGACY,      /*< desc="Color (HSV) (legacy)"     >*/
  GIMP_LAYER_MODE_HSV_VALUE_LEGACY,      /*< desc="Value (HSV) (legacy)"     >*/
  GIMP_LAYER_MODE_DIVIDE_LEGACY,         /*< desc="Divide (legacy)"          >*/
  GIMP_LAYER_MODE_DODGE_LEGACY,          /*< desc="Dodge (legacy)"           >*/
  GIMP_LAYER_MODE_BURN_LEGACY,           /*< desc="Burn (legacy)"            >*/
  GIMP_LAYER_MODE_HARDLIGHT_LEGACY,      /*< desc="Hard light (legacy)"      >*/
  GIMP_LAYER_MODE_SOFTLIGHT_LEGACY,      /*< desc="Soft light (legacy)"      >*/
  GIMP_LAYER_MODE_GRAIN_EXTRACT_LEGACY,  /*< desc="Grain extract (legacy)"   >*/
  GIMP_LAYER_MODE_GRAIN_MERGE_LEGACY,    /*< desc="Grain merge (legacy)"     >*/
  GIMP_LAYER_MODE_COLOR_ERASE,           /*< desc="Color erase"              >*/

  /*  Since 2.8  */
  GIMP_LAYER_MODE_OVERLAY,               /*< desc="Overlay"                  >*/
  GIMP_LAYER_MODE_LCH_HUE,               /*< desc="Hue (LCH)"                >*/
  GIMP_LAYER_MODE_LCH_CHROMA,            /*< desc="Chroma (LCH)"             >*/
  GIMP_LAYER_MODE_LCH_COLOR,             /*< desc="Color (LCH)"              >*/
  GIMP_LAYER_MODE_LCH_LIGHTNESS,         /*< desc="Lightness (LCH)"          >*/

  /*  Since 2.10  */
  GIMP_LAYER_MODE_NORMAL_LINEAR,         /*< desc="Normal (linear)"          >*/
  GIMP_LAYER_MODE_BEHIND_LINEAR,         /*< desc="Behind (linear)"          >*/
  GIMP_LAYER_MODE_MULTIPLY,              /*< desc="Multiply"                 >*/
  GIMP_LAYER_MODE_MULTIPLY_LINEAR,       /*< desc="Multiply (linear)"        >*/
  GIMP_LAYER_MODE_SCREEN,                /*< desc="Screen"                   >*/
  GIMP_LAYER_MODE_SCREEN_LINEAR,         /*< desc="Screen (linear)"          >*/
  GIMP_LAYER_MODE_OVERLAY_LINEAR,        /*< desc="Overlay (linear)"         >*/
  GIMP_LAYER_MODE_DIFFERENCE,            /*< desc="Difference"               >*/
  GIMP_LAYER_MODE_DIFFERENCE_LINEAR,     /*< desc="Difference (linear)"      >*/
  GIMP_LAYER_MODE_ADDITION,              /*< desc="Addition"                 >*/
  GIMP_LAYER_MODE_ADDITION_LINEAR,       /*< desc="Addition (linear)"        >*/
  GIMP_LAYER_MODE_SUBTRACT,              /*< desc="Subtract"                 >*/
  GIMP_LAYER_MODE_SUBTRACT_LINEAR,       /*< desc="Subtract (linear)"        >*/
  GIMP_LAYER_MODE_DARKEN_ONLY,           /*< desc="Darken only"              >*/
  GIMP_LAYER_MODE_LIGHTEN_ONLY,          /*< desc="Lighten only"             >*/
  GIMP_LAYER_MODE_HSV_HUE,               /*< desc="Hue (HSV)"                >*/
  GIMP_LAYER_MODE_HSV_SATURATION,        /*< desc="Saturation (HSV)"         >*/
  GIMP_LAYER_MODE_HSV_COLOR,             /*< desc="Color (HSV)"              >*/
  GIMP_LAYER_MODE_HSV_VALUE,             /*< desc="Value (HSV)"              >*/
  GIMP_LAYER_MODE_DIVIDE,                /*< desc="Divide"                   >*/
  GIMP_LAYER_MODE_DIVIDE_LINEAR,         /*< desc="Divide (linear)"          >*/
  GIMP_LAYER_MODE_DODGE,                 /*< desc="Dodge"                    >*/
  GIMP_LAYER_MODE_DODGE_LINEAR,          /*< desc="Dodge (linear)"           >*/
  GIMP_LAYER_MODE_BURN,                  /*< desc="Burn"                     >*/
  GIMP_LAYER_MODE_BURN_LINEAR,           /*< desc="Burn (linear)"            >*/
  GIMP_LAYER_MODE_HARDLIGHT,             /*< desc="Hard light"               >*/
  GIMP_LAYER_MODE_HARDLIGHT_LINEAR,      /*< desc="Hard light (linear)"      >*/
  GIMP_LAYER_MODE_SOFTLIGHT,             /*< desc="Soft light"               >*/
  GIMP_LAYER_MODE_SOFTLIGHT_LINEAR,      /*< desc="Soft light (linear)"      >*/
  GIMP_LAYER_MODE_GRAIN_EXTRACT,         /*< desc="Grain extract"            >*/
  GIMP_LAYER_MODE_GRAIN_EXTRACT_LINEAR,  /*< desc="Grain extract (linear)"   >*/
  GIMP_LAYER_MODE_GRAIN_MERGE,           /*< desc="Grain merge"              >*/
  GIMP_LAYER_MODE_GRAIN_MERGE_LINEAR,    /*< desc="Grain merge (linear)"     >*/
  GIMP_LAYER_MODE_VIVID_LIGHT,           /*< desc="Vivid light"              >*/
  GIMP_LAYER_MODE_VIVID_LIGHT_LINEAR,    /*< desc="Vivid light (linear)"     >*/
  GIMP_LAYER_MODE_PIN_LIGHT,             /*< desc="Pin light"                >*/
  GIMP_LAYER_MODE_PIN_LIGHT_LINEAR,      /*< desc="Pin light (linear)"       >*/
  GIMP_LAYER_MODE_LINEAR_LIGHT,          /*< desc="Linear light"             >*/
  GIMP_LAYER_MODE_LINEAR_LIGHT_LINEAR,   /*< desc="Linear light (linear)"    >*/
  GIMP_LAYER_MODE_HARD_MIX,              /*< desc="Hard mix"                 >*/
  GIMP_LAYER_MODE_HARD_MIX_LINEAR,       /*< desc="Hard mix (linear)"        >*/
  GIMP_LAYER_MODE_EXCLUSION,             /*< desc="Exclusion"                >*/
  GIMP_LAYER_MODE_EXCLUSION_LINEAR,      /*< desc="Exclusion (linear)"       >*/
  GIMP_LAYER_MODE_LINEAR_BURN,           /*< desc="Linear burn"              >*/
  GIMP_LAYER_MODE_LINEAR_BURN_LINEAR,    /*< desc="Linear burn (linear)"     >*/
  GIMP_LAYER_MODE_LUMA_DARKEN_ONLY,      /*< desc="Luma darken only"         >*/
  GIMP_LAYER_MODE_LUMINANCE_DARKEN_ONLY, /*< desc="Luminance darken only"    >*/
  GIMP_LAYER_MODE_LUMA_LIGHTEN_ONLY,     /*< desc="Luma lighten only"        >*/
  GIMP_LAYER_MODE_LUMINANCE_LIGHTEN_ONLY,/*< desc="Luminance lighten only"   >*/

  /*  Internal modes, not available to the PDB, must be kept at the end  */
  GIMP_LAYER_MODE_ERASE,                 /*< pdb-skip, desc="Erase"          >*/
  GIMP_LAYER_MODE_REPLACE,               /*< pdb-skip, desc="Replace"        >*/
  GIMP_LAYER_MODE_ANTI_ERASE,            /*< pdb-skip, desc="Anti erase"     >*/

  /*  Layer mode menu separator  */
  GIMP_LAYER_MODE_SEPARATOR = -1         /*< pdb-skip, skip                  >*/
} GimpLayerMode;


#define GIMP_TYPE_LAYER_MODE_GROUP (gimp_layer_mode_group_get_type ())

GType gimp_layer_mode_group_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  GIMP_LAYER_MODE_GROUP_DEFAULT,     /*< desc="Default"      >*/
  GIMP_LAYER_MODE_GROUP_LINEAR,      /*< desc="Linear light" >*/
  GIMP_LAYER_MODE_GROUP_PERCEPTUAL,  /*< desc="Perceptual"   >*/
  GIMP_LAYER_MODE_GROUP_LEGACY,      /*< desc="Legacy"       >*/
} GimpLayerModeGroup;


#define GIMP_TYPE_LAYER_MODE_CONTEXT (gimp_layer_mode_context_get_type ())

GType gimp_layer_mode_context_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  GIMP_LAYER_MODE_CONTEXT_LAYER = 1 << 0,
  GIMP_LAYER_MODE_CONTEXT_GROUP = 1 << 1,
  GIMP_LAYER_MODE_CONTEXT_PAINT = 1 << 2,
  GIMP_LAYER_MODE_CONTEXT_FADE  = 1 << 3,

  GIMP_LAYER_MODE_CONTEXT_ALL = (GIMP_LAYER_MODE_CONTEXT_LAYER | GIMP_LAYER_MODE_CONTEXT_GROUP | GIMP_LAYER_MODE_CONTEXT_PAINT | GIMP_LAYER_MODE_CONTEXT_FADE)
} GimpLayerModeContext;


/*
 * non-registered enums; register them if needed
 */

typedef enum  /*< pdb-skip, skip >*/
{
  GIMP_LAYER_MODE_AFFECT_NONE = 0,
  GIMP_LAYER_MODE_AFFECT_DST  = 1 << 0,
  GIMP_LAYER_MODE_AFFECT_SRC  = 1 << 1
} GimpLayerModeAffectMask;

typedef enum  /*< pdb-skip, skip >*/
{
  GIMP_LAYER_MODE_FLAG_LEGACY                    =  1 << 0,
  GIMP_LAYER_MODE_FLAG_WANTS_LINEAR_DATA         =  1 << 1,
  GIMP_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     =  1 << 2,
  GIMP_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE =  1 << 3,
  GIMP_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE  =  1 << 4,
} GimpLayerModeFlags;


#endif /* __OPERATIONS_ENUMS_H__ */
