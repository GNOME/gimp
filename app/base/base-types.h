/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#ifndef __BASE_TYPES_H__
#define __BASE_TYPES_H__


#include "libgimpbase/gimpbasetypes.h"
#include "libgimpmath/gimpmathtypes.h"
#include "libgimpcolor/gimpcolortypes.h"

#include "paint-funcs/paint-funcs-types.h"

#include "base/base-enums.h"

#include "config/config-types.h"


/* convenient defines */
#define MAX_CHANNELS  4

#define RED           0
#define GREEN         1
#define BLUE          2
#define ALPHA         3

#define GRAY          0
#define ALPHA_G       1

#define INDEXED       0
#define ALPHA_I       1


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

typedef void (* TileValidateProc)   (TileManager *tm,
                                     Tile        *tile,
                                     gpointer     user_data);
typedef void (* PixelProcessorFunc) (void);


#endif /* __BASE_TYPES_H__ */
