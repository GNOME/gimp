/* LIBGIMP - The GIMP Library                                                   
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball                
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.             
 *                                                                              
 * This library is distributed in the hope that it will be useful,              
 * but WITHOUT ANY WARRANTY; without even the implied warranty of               
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU            
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */                                                                             
#ifndef __GIMP_ENUMS_H__
#define __GIMP_ENUMS_H__


typedef enum
{
  RGB     = 0,
  GRAY    = 1,
  INDEXED = 2
} GImageType;

typedef enum
{
  RGB_IMAGE      = 0,
  RGBA_IMAGE     = 1,
  GRAY_IMAGE     = 2,
  GRAYA_IMAGE    = 3,
  INDEXED_IMAGE  = 4,
  INDEXEDA_IMAGE = 5
} GDrawableType;

typedef enum
{
  NORMAL_MODE       = 0,
  DISSOLVE_MODE     = 1,
  MULTIPLY_MODE     = 3,
  SCREEN_MODE       = 4,
  OVERLAY_MODE      = 5,
  DIFFERENCE_MODE   = 6,
  ADDITION_MODE     = 7,
  SUBTRACT_MODE     = 8,
  DARKEN_ONLY_MODE  = 9,
  LIGHTEN_ONLY_MODE = 10,
  HUE_MODE          = 11,
  SATURATION_MODE   = 12,
  COLOR_MODE        = 13,
  VALUE_MODE        = 14
} GLayerMode;

typedef enum
{
  BG_IMAGE_FILL,
  WHITE_IMAGE_FILL,
  TRANS_IMAGE_FILL
} GFillType;

typedef enum
{
  PARAM_INT32,
  PARAM_INT16,
  PARAM_INT8,
  PARAM_FLOAT,
  PARAM_STRING,
  PARAM_INT32ARRAY,
  PARAM_INT16ARRAY,
  PARAM_INT8ARRAY,
  PARAM_FLOATARRAY,
  PARAM_STRINGARRAY,
  PARAM_COLOR,
  PARAM_REGION,
  PARAM_DISPLAY,
  PARAM_IMAGE,
  PARAM_LAYER,
  PARAM_CHANNEL,
  PARAM_DRAWABLE,
  PARAM_SELECTION,
  PARAM_BOUNDARY,
  PARAM_PATH,
  PARAM_STATUS,
  PARAM_END
} GParamType;

typedef enum
{
  PROC_PLUG_IN = 1,
  PROC_EXTENSION = 2,
  PROC_TEMPORARY = 3
} GProcedureType;

/* This enum is mirrored in "app/plug_in.c", make sure
 *  they are identical or bad things will happen.
 */
typedef enum
{
  RUN_INTERACTIVE    = 0x0,
  RUN_NONINTERACTIVE = 0x1,
  RUN_WITH_LAST_VALS = 0x2
} GRunModeType;

typedef enum
{
  STATUS_EXECUTION_ERROR,
  STATUS_CALLING_ERROR,
  STATUS_PASS_THROUGH,
  STATUS_SUCCESS
} GStatusType;


#endif /* __GIMP_ENUMS_H__ */
