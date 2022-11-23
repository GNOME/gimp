/* LIGMA - The GNU Image Manipulation Program
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

#ifndef __OPERATIONS_ENUMS_H__
#define __OPERATIONS_ENUMS_H__


#define LIGMA_TYPE_LAYER_COLOR_SPACE (ligma_layer_color_space_get_type ())

GType ligma_layer_color_space_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_LAYER_COLOR_SPACE_AUTO,           /*< desc="Auto"             >*/
  LIGMA_LAYER_COLOR_SPACE_RGB_LINEAR,     /*< desc="RGB (linear)"     >*/
  LIGMA_LAYER_COLOR_SPACE_RGB_PERCEPTUAL, /*< desc="RGB (perceptual)" >*/
  LIGMA_LAYER_COLOR_SPACE_LAB,            /*< desc="LAB", pdb-skip    >*/
} LigmaLayerColorSpace;


#define LIGMA_TYPE_LAYER_COMPOSITE_MODE (ligma_layer_composite_mode_get_type ())

GType ligma_layer_composite_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_LAYER_COMPOSITE_AUTO,             /*< desc="Auto"             >*/
  LIGMA_LAYER_COMPOSITE_UNION,            /*< desc="Union"            >*/
  LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP, /*< desc="Clip to backdrop" >*/
  LIGMA_LAYER_COMPOSITE_CLIP_TO_LAYER,    /*< desc="Clip to layer"    >*/
  LIGMA_LAYER_COMPOSITE_INTERSECTION      /*< desc="Intersection"     >*/
} LigmaLayerCompositeMode;


#define LIGMA_TYPE_LAYER_MODE (ligma_layer_mode_get_type ())

GType ligma_layer_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
  /*  Modes that exist since ancient times  */
  LIGMA_LAYER_MODE_NORMAL_LEGACY,         /*< desc="Normal (legacy)",         abbrev="Normal (l)"            >*/
  LIGMA_LAYER_MODE_DISSOLVE,              /*< desc="Dissolve"                                                >*/
  LIGMA_LAYER_MODE_BEHIND_LEGACY,         /*< desc="Behind (legacy)",         abbrev="Behind (l)"            >*/
  LIGMA_LAYER_MODE_MULTIPLY_LEGACY,       /*< desc="Multiply (legacy)",       abbrev="Multiply (l)"          >*/
  LIGMA_LAYER_MODE_SCREEN_LEGACY,         /*< desc="Screen (legacy)",         abbrev="Screen (l)"            >*/
  LIGMA_LAYER_MODE_OVERLAY_LEGACY,        /*< desc="Old broken Overlay",      abbrev="Old Overlay"           >*/
  LIGMA_LAYER_MODE_DIFFERENCE_LEGACY,     /*< desc="Difference (legacy)",     abbrev="Difference (l)"        >*/
  LIGMA_LAYER_MODE_ADDITION_LEGACY,       /*< desc="Addition (legacy)",       abbrev="Addition (l)"          >*/
  LIGMA_LAYER_MODE_SUBTRACT_LEGACY,       /*< desc="Subtract (legacy)",       abbrev="Subtract (l)"          >*/
  LIGMA_LAYER_MODE_DARKEN_ONLY_LEGACY,    /*< desc="Darken only (legacy)",    abbrev="Darken only (l)"       >*/
  LIGMA_LAYER_MODE_LIGHTEN_ONLY_LEGACY,   /*< desc="Lighten only (legacy)",   abbrev="Lighten only (l)"      >*/
  LIGMA_LAYER_MODE_HSV_HUE_LEGACY,        /*< desc="HSV Hue (legacy)",        abbrev="HSV Hue (l)"           >*/
  LIGMA_LAYER_MODE_HSV_SATURATION_LEGACY, /*< desc="HSV Saturation (legacy)", abbrev="HSV Saturation (l)"    >*/
  LIGMA_LAYER_MODE_HSL_COLOR_LEGACY,      /*< desc="HSL Color (legacy)",      abbrev="HSL Color (l)"         >*/
  LIGMA_LAYER_MODE_HSV_VALUE_LEGACY,      /*< desc="HSV Value (legacy)",      abbrev="HSV Value (l)"         >*/
  LIGMA_LAYER_MODE_DIVIDE_LEGACY,         /*< desc="Divide (legacy)",         abbrev="Divide (l)"            >*/
  LIGMA_LAYER_MODE_DODGE_LEGACY,          /*< desc="Dodge (legacy)",          abbrev="Dodge (l)"             >*/
  LIGMA_LAYER_MODE_BURN_LEGACY,           /*< desc="Burn (legacy)",           abbrev="Burn (l)"              >*/
  LIGMA_LAYER_MODE_HARDLIGHT_LEGACY,      /*< desc="Hard light (legacy)",     abbrev="Hard light (l)"        >*/

  /*  Since 2.8 (XCF version 2)  */
  LIGMA_LAYER_MODE_SOFTLIGHT_LEGACY,      /*< desc="Soft light (legacy)",     abbrev="Soft light (l)"        >*/
  LIGMA_LAYER_MODE_GRAIN_EXTRACT_LEGACY,  /*< desc="Grain extract (legacy)",  abbrev="Grain extract (l)"     >*/
  LIGMA_LAYER_MODE_GRAIN_MERGE_LEGACY,    /*< desc="Grain merge (legacy)",    abbrev="Grain merge (l)"       >*/
  LIGMA_LAYER_MODE_COLOR_ERASE_LEGACY,    /*< desc="Color erase (legacy)",    abbrev="Color erase (l)"       >*/

  /*  Since 2.10 (XCF version 9) */
  LIGMA_LAYER_MODE_OVERLAY,               /*< desc="Overlay"                                                 >*/
  LIGMA_LAYER_MODE_LCH_HUE,               /*< desc="LCh Hue"                                                 >*/
  LIGMA_LAYER_MODE_LCH_CHROMA,            /*< desc="LCh Chroma"                                              >*/
  LIGMA_LAYER_MODE_LCH_COLOR,             /*< desc="LCh Color"                                               >*/
  LIGMA_LAYER_MODE_LCH_LIGHTNESS,         /*< desc="LCh Lightness"                                           >*/

  /*  Since 2.10 (XCF version 10)  */
  LIGMA_LAYER_MODE_NORMAL,                /*< desc="Normal"                                                  >*/
  LIGMA_LAYER_MODE_BEHIND,                /*< desc="Behind"                                                  >*/
  LIGMA_LAYER_MODE_MULTIPLY,              /*< desc="Multiply"                                                >*/
  LIGMA_LAYER_MODE_SCREEN,                /*< desc="Screen"                                                  >*/
  LIGMA_LAYER_MODE_DIFFERENCE,            /*< desc="Difference"                                              >*/
  LIGMA_LAYER_MODE_ADDITION,              /*< desc="Addition"                                                >*/
  LIGMA_LAYER_MODE_SUBTRACT,              /*< desc="Subtract"                                                >*/
  LIGMA_LAYER_MODE_DARKEN_ONLY,           /*< desc="Darken only"                                             >*/
  LIGMA_LAYER_MODE_LIGHTEN_ONLY,          /*< desc="Lighten only"                                            >*/
  LIGMA_LAYER_MODE_HSV_HUE,               /*< desc="HSV Hue"                                                 >*/
  LIGMA_LAYER_MODE_HSV_SATURATION,        /*< desc="HSV Saturation"                                          >*/
  LIGMA_LAYER_MODE_HSL_COLOR,             /*< desc="HSL Color"                                               >*/
  LIGMA_LAYER_MODE_HSV_VALUE,             /*< desc="HSV Value"                                               >*/
  LIGMA_LAYER_MODE_DIVIDE,                /*< desc="Divide"                                                  >*/
  LIGMA_LAYER_MODE_DODGE,                 /*< desc="Dodge"                                                   >*/
  LIGMA_LAYER_MODE_BURN,                  /*< desc="Burn"                                                    >*/
  LIGMA_LAYER_MODE_HARDLIGHT,             /*< desc="Hard light"                                              >*/
  LIGMA_LAYER_MODE_SOFTLIGHT,             /*< desc="Soft light"                                              >*/
  LIGMA_LAYER_MODE_GRAIN_EXTRACT,         /*< desc="Grain extract"                                           >*/
  LIGMA_LAYER_MODE_GRAIN_MERGE,           /*< desc="Grain merge"                                             >*/
  LIGMA_LAYER_MODE_VIVID_LIGHT,           /*< desc="Vivid light"                                             >*/
  LIGMA_LAYER_MODE_PIN_LIGHT,             /*< desc="Pin light"                                               >*/
  LIGMA_LAYER_MODE_LINEAR_LIGHT,          /*< desc="Linear light"                                            >*/
  LIGMA_LAYER_MODE_HARD_MIX,              /*< desc="Hard mix"                                                >*/
  LIGMA_LAYER_MODE_EXCLUSION,             /*< desc="Exclusion"                                               >*/
  LIGMA_LAYER_MODE_LINEAR_BURN,           /*< desc="Linear burn"                                             >*/
  LIGMA_LAYER_MODE_LUMA_DARKEN_ONLY,      /*< desc="Luma/Luminance darken only",  abbrev="Luma darken only"  >*/
  LIGMA_LAYER_MODE_LUMA_LIGHTEN_ONLY,     /*< desc="Luma/Luminance lighten only", abbrev="Luma lighten only" >*/
  LIGMA_LAYER_MODE_LUMINANCE,             /*< desc="Luminance"                                               >*/
  LIGMA_LAYER_MODE_COLOR_ERASE,           /*< desc="Color erase"                                             >*/
  LIGMA_LAYER_MODE_ERASE,                 /*< desc="Erase"                                                   >*/
  LIGMA_LAYER_MODE_MERGE,                 /*< desc="Merge"                                                   >*/
  LIGMA_LAYER_MODE_SPLIT,                 /*< desc="Split"                                                   >*/
  LIGMA_LAYER_MODE_PASS_THROUGH,          /*< desc="Pass through"                                            >*/

  /*  Internal modes, not available to the PDB, must be kept at the end  */
  LIGMA_LAYER_MODE_REPLACE,               /*< pdb-skip, desc="Replace"                                       >*/
  LIGMA_LAYER_MODE_ANTI_ERASE,            /*< pdb-skip, desc="Anti erase"                                    >*/

  /*  Layer mode menu separator  */
  LIGMA_LAYER_MODE_SEPARATOR = -1         /*< pdb-skip, skip                                                 >*/
} LigmaLayerMode;


#define LIGMA_TYPE_LAYER_MODE_GROUP (ligma_layer_mode_group_get_type ())

GType ligma_layer_mode_group_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  LIGMA_LAYER_MODE_GROUP_DEFAULT,     /*< desc="Default"      >*/
  LIGMA_LAYER_MODE_GROUP_LEGACY,      /*< desc="Legacy"       >*/
} LigmaLayerModeGroup;


#define LIGMA_TYPE_LAYER_MODE_CONTEXT (ligma_layer_mode_context_get_type ())

GType ligma_layer_mode_context_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  LIGMA_LAYER_MODE_CONTEXT_LAYER  = 1 << 0,
  LIGMA_LAYER_MODE_CONTEXT_GROUP  = 1 << 1,
  LIGMA_LAYER_MODE_CONTEXT_PAINT  = 1 << 2,
  LIGMA_LAYER_MODE_CONTEXT_FILTER = 1 << 3,

  LIGMA_LAYER_MODE_CONTEXT_ALL = (LIGMA_LAYER_MODE_CONTEXT_LAYER |
                                 LIGMA_LAYER_MODE_CONTEXT_GROUP |
                                 LIGMA_LAYER_MODE_CONTEXT_PAINT |
                                 LIGMA_LAYER_MODE_CONTEXT_FILTER)
} LigmaLayerModeContext;


/*
 * non-registered enums; register them if needed
 */

typedef enum  /*< pdb-skip, skip >*/
{
  LIGMA_LAYER_COMPOSITE_REGION_INTERSECTION = 0,
  LIGMA_LAYER_COMPOSITE_REGION_DESTINATION  = 1 << 0,
  LIGMA_LAYER_COMPOSITE_REGION_SOURCE       = 1 << 1,
  LIGMA_LAYER_COMPOSITE_REGION_UNION        = (LIGMA_LAYER_COMPOSITE_REGION_DESTINATION |
                                              LIGMA_LAYER_COMPOSITE_REGION_SOURCE),
} LigmaLayerCompositeRegion;

typedef enum  /*< pdb-skip, skip >*/
{
  LIGMA_LAYER_MODE_FLAG_LEGACY                    =  1 << 0,
  LIGMA_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     =  1 << 1,
  LIGMA_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE =  1 << 2,
  LIGMA_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE  =  1 << 3,
  LIGMA_LAYER_MODE_FLAG_SUBTRACTIVE               =  1 << 4,
  LIGMA_LAYER_MODE_FLAG_ALPHA_ONLY                =  1 << 5,
  LIGMA_LAYER_MODE_FLAG_TRIVIAL                   =  1 << 6
} LigmaLayerModeFlags;


#endif /* __OPERATIONS_ENUMS_H__ */
