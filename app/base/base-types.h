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

#include "base/base-enums.h"


/*  magic constants  */
/* FIXME: Remove magic constants! */
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

/* Types of convolutions  */
typedef enum 
{
  GIMP_NORMAL_CONVOL,      /*  Negative numbers truncated  */
  GIMP_ABSOLUTE_CONVOL,    /*  Absolute value              */
  GIMP_NEGATIVE_CONVOL     /*  add 127 to values           */
} GimpConvolutionType;

typedef enum
{
  GIMP_VALUE_LUT,
  GIMP_RED_LUT,
  GIMP_GREEN_LUT,
  GIMP_BLUE_LUT,
  GIMP_ALPHA_LUT,
  GIMP_GRAY_LUT = 0  /*< skip >*/
} GimpChannelLutType;

/*  Transparency representation  */
typedef enum /*< skip >*/
{
  GIMP_LIGHT_CHECKS = 0,
  GIMP_GRAY_CHECKS  = 1,
  GIMP_DARK_CHECKS  = 2,
  GIMP_WHITE_ONLY   = 3,
  GIMP_GRAY_ONLY    = 4,
  GIMP_BLACK_ONLY   = 5
} GimpCheckType;

typedef enum /*< skip >*/
{
  GIMP_SMALL_CHECKS  = 0,
  GIMP_MEDIUM_CHECKS = 1,
  GIMP_LARGE_CHECKS  = 2
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

typedef struct _PixelDataHandle     PixelDataHandle;

/*  functions  */

typedef void   (* TileValidateProc)  (TileManager *tm,
				      Tile        *tile);


#endif /* __BASE_TYPES_H__ */
