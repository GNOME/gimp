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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once


#define GIMP_TYPE_LAYER_COLOR_SPACE (gimp_layer_color_space_get_type ())

GType gimp_layer_color_space_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_LAYER_COLOR_SPACE_AUTO,           /*< desc="Auto"             >*/
  GIMP_LAYER_COLOR_SPACE_RGB_LINEAR,     /*< desc="RGB (linear)"     >*/
  GIMP_LAYER_COLOR_SPACE_RGB_NON_LINEAR, /*< desc="RGB (from color profile)" >*/
  GIMP_LAYER_COLOR_SPACE_LAB,            /*< desc="LAB"              >*/
  GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL, /*< desc="RGB (perceptual)" >*/
  GIMP_LAYER_COLOR_SPACE_LAST,           /*< pdb-skip, skip          >*/
} GimpLayerColorSpace;


#define GIMP_TYPE_LAYER_COMPOSITE_MODE (gimp_layer_composite_mode_get_type ())

GType gimp_layer_composite_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_LAYER_COMPOSITE_AUTO,             /*< desc="Auto"             >*/
  GIMP_LAYER_COMPOSITE_UNION,            /*< desc="Union"            >*/
  GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP, /*< desc="Clip to backdrop" >*/
  GIMP_LAYER_COMPOSITE_CLIP_TO_LAYER,    /*< desc="Clip to layer"    >*/
  GIMP_LAYER_COMPOSITE_INTERSECTION      /*< desc="Intersection"     >*/
} GimpLayerCompositeMode;


#define GIMP_TYPE_LAYER_MODE (gimp_layer_mode_get_type ())

GType gimp_layer_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
  /*  Modes that exist since ancient times  */
  GIMP_LAYER_MODE_NORMAL_LEGACY,         /*< desc="Normal (legacy)",         abbrev="Normal (l)"            >*/
  GIMP_LAYER_MODE_DISSOLVE,              /*< desc="Dissolve"                                                >*/
  GIMP_LAYER_MODE_BEHIND_LEGACY,         /*< desc="Behind (legacy)",         abbrev="Behind (l)"            >*/
  GIMP_LAYER_MODE_MULTIPLY_LEGACY,       /*< desc="Multiply (legacy)",       abbrev="Multiply (l)"          >*/
  GIMP_LAYER_MODE_SCREEN_LEGACY,         /*< desc="Screen (legacy)",         abbrev="Screen (l)"            >*/
  GIMP_LAYER_MODE_OVERLAY_LEGACY,        /*< desc="Old broken Overlay",      abbrev="Old Overlay"           >*/
  GIMP_LAYER_MODE_DIFFERENCE_LEGACY,     /*< desc="Difference (legacy)",     abbrev="Difference (l)"        >*/
  GIMP_LAYER_MODE_ADDITION_LEGACY,       /*< desc="Addition (legacy)",       abbrev="Addition (l)"          >*/
  GIMP_LAYER_MODE_SUBTRACT_LEGACY,       /*< desc="Subtract (legacy)",       abbrev="Subtract (l)"          >*/
  GIMP_LAYER_MODE_DARKEN_ONLY_LEGACY,    /*< desc="Darken only (legacy)",    abbrev="Darken only (l)"       >*/
  GIMP_LAYER_MODE_LIGHTEN_ONLY_LEGACY,   /*< desc="Lighten only (legacy)",   abbrev="Lighten only (l)"      >*/
  GIMP_LAYER_MODE_HSV_HUE_LEGACY,        /*< desc="HSV Hue (legacy)",        abbrev="HSV Hue (l)"           >*/
  GIMP_LAYER_MODE_HSV_SATURATION_LEGACY, /*< desc="HSV Saturation (legacy)", abbrev="HSV Saturation (l)"    >*/
  GIMP_LAYER_MODE_HSL_COLOR_LEGACY,      /*< desc="HSL Color (legacy)",      abbrev="HSL Color (l)"         >*/
  GIMP_LAYER_MODE_HSV_VALUE_LEGACY,      /*< desc="HSV Value (legacy)",      abbrev="HSV Value (l)"         >*/
  GIMP_LAYER_MODE_DIVIDE_LEGACY,         /*< desc="Divide (legacy)",         abbrev="Divide (l)"            >*/
  GIMP_LAYER_MODE_DODGE_LEGACY,          /*< desc="Dodge (legacy)",          abbrev="Dodge (l)"             >*/
  GIMP_LAYER_MODE_BURN_LEGACY,           /*< desc="Burn (legacy)",           abbrev="Burn (l)"              >*/
  GIMP_LAYER_MODE_HARDLIGHT_LEGACY,      /*< desc="Hard light (legacy)",     abbrev="Hard light (l)"        >*/

  /*  Since 2.8 (XCF version 2)  */
  GIMP_LAYER_MODE_SOFTLIGHT_LEGACY,      /*< desc="Soft light (legacy)",     abbrev="Soft light (l)"        >*/
  GIMP_LAYER_MODE_GRAIN_EXTRACT_LEGACY,  /*< desc="Grain extract (legacy)",  abbrev="Grain extract (l)"     >*/
  GIMP_LAYER_MODE_GRAIN_MERGE_LEGACY,    /*< desc="Grain merge (legacy)",    abbrev="Grain merge (l)"       >*/
  GIMP_LAYER_MODE_COLOR_ERASE_LEGACY,    /*< desc="Color erase (legacy)",    abbrev="Color erase (l)"       >*/

  /*  Since 2.10 (XCF version 9) */
  GIMP_LAYER_MODE_OVERLAY,               /*< desc="Overlay"                                                 >*/
  GIMP_LAYER_MODE_LCH_HUE,               /*< desc="LCh Hue"                                                 >*/
  GIMP_LAYER_MODE_LCH_CHROMA,            /*< desc="LCh Chroma"                                              >*/
  GIMP_LAYER_MODE_LCH_COLOR,             /*< desc="LCh Color"                                               >*/
  GIMP_LAYER_MODE_LCH_LIGHTNESS,         /*< desc="LCh Lightness"                                           >*/

  /*  Since 2.10 (XCF version 10)  */
  GIMP_LAYER_MODE_NORMAL,                /*< desc="Normal"                                                  >*/
  GIMP_LAYER_MODE_BEHIND,                /*< desc="Behind"                                                  >*/
  GIMP_LAYER_MODE_MULTIPLY,              /*< desc="Multiply"                                                >*/
  GIMP_LAYER_MODE_SCREEN,                /*< desc="Screen"                                                  >*/
  GIMP_LAYER_MODE_DIFFERENCE,            /*< desc="Difference"                                              >*/
  GIMP_LAYER_MODE_ADDITION,              /*< desc="Addition"                                                >*/
  GIMP_LAYER_MODE_SUBTRACT,              /*< desc="Subtract"                                                >*/
  GIMP_LAYER_MODE_DARKEN_ONLY,           /*< desc="Darken only"                                             >*/
  GIMP_LAYER_MODE_LIGHTEN_ONLY,          /*< desc="Lighten only"                                            >*/
  GIMP_LAYER_MODE_HSV_HUE,               /*< desc="HSV Hue"                                                 >*/
  GIMP_LAYER_MODE_HSV_SATURATION,        /*< desc="HSV Saturation"                                          >*/
  GIMP_LAYER_MODE_HSL_COLOR,             /*< desc="HSL Color"                                               >*/
  GIMP_LAYER_MODE_HSV_VALUE,             /*< desc="HSV Value"                                               >*/
  GIMP_LAYER_MODE_DIVIDE,                /*< desc="Divide"                                                  >*/
  GIMP_LAYER_MODE_DODGE,                 /*< desc="Dodge"                                                   >*/
  GIMP_LAYER_MODE_BURN,                  /*< desc="Burn"                                                    >*/
  GIMP_LAYER_MODE_HARDLIGHT,             /*< desc="Hard light"                                              >*/
  GIMP_LAYER_MODE_SOFTLIGHT,             /*< desc="Soft light"                                              >*/
  GIMP_LAYER_MODE_GRAIN_EXTRACT,         /*< desc="Grain extract"                                           >*/
  GIMP_LAYER_MODE_GRAIN_MERGE,           /*< desc="Grain merge"                                             >*/
  GIMP_LAYER_MODE_VIVID_LIGHT,           /*< desc="Vivid light"                                             >*/
  GIMP_LAYER_MODE_PIN_LIGHT,             /*< desc="Pin light"                                               >*/
  GIMP_LAYER_MODE_LINEAR_LIGHT,          /*< desc="Linear light"                                            >*/
  GIMP_LAYER_MODE_HARD_MIX,              /*< desc="Hard mix"                                                >*/
  GIMP_LAYER_MODE_EXCLUSION,             /*< desc="Exclusion"                                               >*/
  GIMP_LAYER_MODE_LINEAR_BURN,           /*< desc="Linear burn"                                             >*/
  GIMP_LAYER_MODE_LUMA_DARKEN_ONLY,      /*< desc="Luma/Luminance darken only",  abbrev="Luma darken only"  >*/
  GIMP_LAYER_MODE_LUMA_LIGHTEN_ONLY,     /*< desc="Luma/Luminance lighten only", abbrev="Luma lighten only" >*/
  GIMP_LAYER_MODE_LUMINANCE,             /*< desc="Luminance"                                               >*/
  GIMP_LAYER_MODE_COLOR_ERASE,           /*< desc="Color erase"                                             >*/
  GIMP_LAYER_MODE_ERASE,                 /*< desc="Erase"                                                   >*/
  GIMP_LAYER_MODE_MERGE,                 /*< desc="Merge"                                                   >*/
  GIMP_LAYER_MODE_SPLIT,                 /*< desc="Split"                                                   >*/
  GIMP_LAYER_MODE_PASS_THROUGH,          /*< desc="Pass through"                                            >*/

  /*  Mode only used for drawable filters. */
  GIMP_LAYER_MODE_REPLACE,               /*< desc="Replace"                                                 >*/

  /*  Since 3.0 (paint mode only) */
  GIMP_LAYER_MODE_OVERWRITE,             /*< desc="Overwrite"                                               >*/

  /*  Internal modes, not available to the PDB, must be kept at the end  */
  GIMP_LAYER_MODE_ANTI_ERASE,            /*< pdb-skip, desc="Anti erase"                                    >*/

  /*  Layer mode menu separator  */
  GIMP_LAYER_MODE_SEPARATOR = -1         /*< pdb-skip, skip                                                 >*/
} GimpLayerMode;


#define GIMP_TYPE_LAYER_MODE_GROUP (gimp_layer_mode_group_get_type ())

GType gimp_layer_mode_group_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  GIMP_LAYER_MODE_GROUP_DEFAULT,     /*< desc="Default"      >*/
  GIMP_LAYER_MODE_GROUP_LEGACY,      /*< desc="Legacy"       >*/
} GimpLayerModeGroup;


#define GIMP_TYPE_LAYER_MODE_CONTEXT (gimp_layer_mode_context_get_type ())

GType gimp_layer_mode_context_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  GIMP_LAYER_MODE_CONTEXT_LAYER  = 1 << 0,
  GIMP_LAYER_MODE_CONTEXT_GROUP  = 1 << 1,
  GIMP_LAYER_MODE_CONTEXT_PAINT  = 1 << 2,
  GIMP_LAYER_MODE_CONTEXT_FILTER = 1 << 3,

  GIMP_LAYER_MODE_CONTEXT_ALL = (GIMP_LAYER_MODE_CONTEXT_LAYER |
                                 GIMP_LAYER_MODE_CONTEXT_GROUP |
                                 GIMP_LAYER_MODE_CONTEXT_PAINT |
                                 GIMP_LAYER_MODE_CONTEXT_FILTER)
} GimpLayerModeContext;


/*
 * non-registered enums; register them if needed
 */

typedef enum  /*< pdb-skip, skip >*/
{
  GIMP_LAYER_COMPOSITE_REGION_INTERSECTION = 0,
  GIMP_LAYER_COMPOSITE_REGION_DESTINATION  = 1 << 0,
  GIMP_LAYER_COMPOSITE_REGION_SOURCE       = 1 << 1,
  GIMP_LAYER_COMPOSITE_REGION_UNION        = (GIMP_LAYER_COMPOSITE_REGION_DESTINATION |
                                              GIMP_LAYER_COMPOSITE_REGION_SOURCE),
} GimpLayerCompositeRegion;

typedef enum  /*< pdb-skip, skip >*/
{
  GIMP_LAYER_MODE_FLAG_LEGACY                    =  1 << 0,
  GIMP_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     =  1 << 1,
  GIMP_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE =  1 << 2,
  GIMP_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE  =  1 << 3,
  GIMP_LAYER_MODE_FLAG_SUBTRACTIVE               =  1 << 4,
  GIMP_LAYER_MODE_FLAG_ALPHA_ONLY                =  1 << 5,
  GIMP_LAYER_MODE_FLAG_TRIVIAL                   =  1 << 6
} GimpLayerModeFlags;
