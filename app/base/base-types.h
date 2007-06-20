/* GIMP - The GNU Image Manipulation Program
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


#include "libgimpbase/gimpbasetypes.h"
#include "libgimpmath/gimpmathtypes.h"
#include "libgimpcolor/gimpcolortypes.h"

#include "paint-funcs/paint-funcs-types.h"

#include "base/base-enums.h"

#include "config/config-types.h"


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


/* types */

typedef struct _BoundSeg            BoundSeg;

typedef struct _GimpHistogram       GimpHistogram;
typedef struct _GimpLut             GimpLut;

typedef struct _ColorBalance        ColorBalance;
typedef struct _Colorize            Colorize;
typedef struct _Curves              Curves;
typedef struct _HueSaturation       HueSaturation;
typedef struct _Levels              Levels;
typedef struct _Threshold           Threshold;

typedef struct _PixelRegionIterator PixelRegionIterator;
typedef struct _PixelRegion         PixelRegion;
typedef struct _PixelRegionHolder   PixelRegionHolder;

typedef struct _PixelSurround       PixelSurround;

typedef struct _SioxState           SioxState;

typedef struct _TempBuf             TempBuf;

typedef struct _Tile                Tile;
typedef struct _TileManager         TileManager;
typedef struct _TilePyramid         TilePyramid;


/*  functions  */

typedef void   (* TileValidateProc)  (TileManager *tm,
                                      Tile        *tile);


#endif /* __BASE_TYPES_H__ */
