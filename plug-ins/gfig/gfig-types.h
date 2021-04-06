/*
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This is a plug-in for GIMP.
 *
 * Generates images containing vector type drawings.
 *
 * Copyright (C) 1997 Andy Thomas  alt@picnic.demon.co.uk
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
 *
 */

#ifndef __GFIG_TYPES_H__
#define __GFIG_TYPES_H__

typedef enum
{
  RECT_GRID = 0,
  POLAR_GRID,
  ISO_GRID
} GridType;

typedef enum
{
  ARC_SEGMENT = 0,
  ARC_SECTOR
} ArcType;

typedef enum
{
  FILL_NONE = 0,
  FILL_COLOR,
  FILL_PATTERN,
  FILL_GRADIENT,
  FILL_VERTICAL,
  FILL_HORIZONTAL
} FillType;

typedef enum
{
  ORIGINAL_LAYER = 0,
  SINGLE_LAYER,
  MULTI_LAYER
} DrawonLayers;

typedef enum
{
  LAYER_TRANS_BG = 0,
  LAYER_BG_BG,
  LAYER_FG_BG,
  LAYER_WHITE_BG,
  LAYER_COPY_BG
} LayersBGType;

typedef enum
{
  PAINT_NONE = 0,
  PAINT_BRUSH_TYPE = 1
} PaintType;

typedef enum
{
  BRUSH_BRUSH_TYPE = 0,
  BRUSH_PENCIL_TYPE,
  BRUSH_AIRBRUSH_TYPE,
  BRUSH_PATTERN_TYPE
} BrushType;

typedef enum
{
  OBJ_TYPE_NONE = 0,
  LINE,
  RECTANGLE,
  CIRCLE,
  ELLIPSE,
  ARC,
  POLY,
  STAR,
  SPIRAL,
  BEZIER,
  NUM_OBJ_TYPES,
  MOVE_OBJ,
  MOVE_POINT,
  COPY_OBJ,
  MOVE_COPY_OBJ,
  DEL_OBJ,
  SELECT_OBJ,
  NULL_OPER
} DobjType;

typedef struct _GFigObj    GFigObj;
typedef struct _GfigObject GfigObject;
typedef struct _Style      Style;

#endif /* __GFIG_TYPES_H__ */
