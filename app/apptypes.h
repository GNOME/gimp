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
#ifndef __APPTYPES_H__
#define __APPTYPES_H__

/* To avoid problems with headers including each others like spaghetti
 * (even recursively), and various types not being defined when they
 * are needed depending on the order you happen to include headers,
 * this file defines those enumeration and opaque struct types that
 * don't depend on any other header. These problems began creeping up
 * when we started to actually use these enums in function prototypes.
*/

/* Should we instead use the enums in libgimp/gimpenums.h? */

/* Base image types */
typedef enum
{
  RGB,
  GRAY,
  INDEXED
} GimpImageBaseType;

/* Image types */
typedef enum
{
  RGB_GIMAGE,		/*< nick=RGB_IMAGE >*/
  RGBA_GIMAGE,		/*< nick=RGBA_IMAGE >*/
  GRAY_GIMAGE,		/*< nick=GRAY_IMAGE >*/
  GRAYA_GIMAGE,		/*< nick=GRAYA_IMAGE >*/
  INDEXED_GIMAGE,	/*< nick=INDEXED_IMAGE >*/
  INDEXEDA_GIMAGE	/*< nick=INDEXEDA_IMAGE >*/
} GimpImageType;

/* Fill types */
typedef enum
{
  FOREGROUND_FILL,	/*< nick=FG_IMAGE_FILL >*/
  BACKGROUND_FILL,	/*< nick=BG_IMAGE_FILL >*/
  WHITE_FILL,		/*< nick=WHITE_IMAGE_FILL >*/
  TRANSPARENT_FILL,	/*< nick=TRANS_IMAGE_FILL >*/
  NO_FILL		/*< nick=NO_IMAGE_FILL >*/
} GimpFillType;

/* Layer modes  */
typedef enum 
{
  NORMAL_MODE,
  DISSOLVE_MODE,
  BEHIND_MODE,
  MULTIPLY_MODE,
  SCREEN_MODE,
  OVERLAY_MODE,
  DIFFERENCE_MODE,
  ADDITION_MODE,
  SUBTRACT_MODE,
  DARKEN_ONLY_MODE,
  LIGHTEN_ONLY_MODE,
  HUE_MODE,
  SATURATION_MODE,
  COLOR_MODE,
  VALUE_MODE,
  DIVIDE_MODE,
  ERASE_MODE,         /*< skip >*/
  REPLACE_MODE,       /*< skip >*/
  ANTI_ERASE_MODE,    /*< skip >*/
} LayerModeEffects;

/* Types of convolutions  */
typedef enum 
{
  NORMAL_CONVOL,		/*  Negative numbers truncated  */
  ABSOLUTE_CONVOL,		/*  Absolute value              */
  NEGATIVE_CONVOL		/*  add 127 to values           */
} ConvolutionType;

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

typedef enum
{
  APPLY,
  DISCARD
} MaskApplyMode;

typedef enum  /*< chop=ADD_ >*/
{
  ADD_WHITE_MASK,
  ADD_BLACK_MASK,
  ADD_ALPHA_MASK
} AddMaskType;

typedef struct _GimpChannel      GimpChannel;
typedef struct _GimpChannelClass GimpChannelClass;

typedef GimpChannel Channel;		/* convenience */

typedef struct _GimpLayer      GimpLayer;
typedef struct _GimpLayerClass GimpLayerClass;
typedef struct _GimpLayerMask      GimpLayerMask;
typedef struct _GimpLayerMaskClass GimpLayerMaskClass;

typedef GimpLayer Layer;		/* convenience */
typedef GimpLayerMask LayerMask;	/* convenience */

typedef struct _layer_undo LayerUndo;

typedef struct _layer_mask_undo LayerMaskUndo;

typedef struct _fs_to_layer_undo FStoLayerUndo;

typedef struct _PlugIn             PlugIn;
typedef struct _PlugInDef          PlugInDef;
typedef struct _PlugInProcDef      PlugInProcDef;

#endif /* __APPTYPES_H__ */
