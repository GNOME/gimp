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

#ifndef __BASE_TYPES_H__
#define __BASE_TYPES_H__


#include "paint-funcs/paint-funcs-types.h"


/*  magic constants  */

#define MAX_CHANNELS     4

#define GRAY_PIX         0
#define ALPHA_G_PIX      1
#define RED_PIX          0
#define GREEN_PIX        1
#define BLUE_PIX         2
#define ALPHA_PIX        3
#define INDEXED_PIX      0
#define ALPHA_I_PIX      1


/*  enums  */

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
  DODGE_MODE,
  BURN_MODE,
  HARDLIGHT_MODE,
  COLOR_ERASE_MODE,
  ERASE_MODE,         /*< skip >*/
  REPLACE_MODE,       /*< skip >*/
  ANTI_ERASE_MODE     /*< skip >*/
} LayerModeEffects;

/* Types of convolutions  */
typedef enum 
{
  NORMAL_CONVOL,		/*  Negative numbers truncated  */
  ABSOLUTE_CONVOL,		/*  Absolute value              */
  NEGATIVE_CONVOL		/*  add 127 to values           */
} ConvolutionType;

typedef enum
{
  LINEAR_INTERPOLATION,
  CUBIC_INTERPOLATION,
  NEAREST_NEIGHBOR_INTERPOLATION
} InterpolationType;

typedef enum
{
  VALUE_LUT,
  RED_LUT,
  GREEN_LUT,
  BLUE_LUT,
  ALPHA_LUT,
  GRAY_LUT = 0  /*< skip >*/
} ChannelLutType;

/*  Transparency representation  */
typedef enum /*< skip >*/
{
  LIGHT_CHECKS = 0,
  GRAY_CHECKS  = 1,
  DARK_CHECKS  = 2,
  WHITE_ONLY   = 3,
  GRAY_ONLY    = 4,
  BLACK_ONLY   = 5
} GimpCheckType;

typedef enum /*< skip >*/
{
  SMALL_CHECKS  = 0,
  MEDIUM_CHECKS = 1,
  LARGE_CHECKS  = 2
} GimpCheckSize;

typedef enum /*< skip >*/
{
  GIMP_HISTOGRAM_VALUE = 0,
  GIMP_HISTOGRAM_RED   = 1,
  GIMP_HISTOGRAM_GREEN = 2,
  GIMP_HISTOGRAM_BLUE  = 3,
  GIMP_HISTOGRAM_ALPHA = 4
} GimpHistogramChannel;


/*  types  */

typedef struct _BoundSeg            BoundSeg;

typedef struct _GimpHistogram       GimpHistogram;
typedef struct _GimpLut             GimpLut;

typedef struct _PixelRegionIterator PixelRegionIterator;
typedef struct _PixelRegion         PixelRegion;
typedef struct _PixelRegionHolder   PixelRegionHolder;

typedef struct _TempBuf             TempBuf;
typedef struct _TempBuf             MaskBuf;

typedef struct _Tile                Tile;
typedef struct _TileManager         TileManager;


/*  functions  */

typedef void   (* TileValidateProc)  (TileManager *tm,
				      Tile        *tile);


#endif /* __BASE_TYPES_H__ */
