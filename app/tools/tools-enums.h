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

#ifndef __TOOLS_ENUMS_H__
#define __TOOLS_ENUMS_H__

#if 0
   This file is parsed by two scripts, enumgen.pl in tools/pdbgen
   and gimp-mkenums. All enums that are not marked with /*< pdb-skip >*/
   are exported to libgimp and the PDB. Enums that are not marked with
   /*< skip >*/ are registered with the GType system. If you want the
   enum to be skipped by both scripts, you have to use /*< pdb-skip >*/
   _before_ /*< skip >*/. 

   All enum values that are marked with /*< skip >*/ are skipped for
   both targets.
#endif


/* 
 * these enums that are registered with the type system
 */

#define GIMP_TYPE_CROP_TYPE (gimp_crop_type_get_type ())

GType gimp_crop_type_get_type (void) G_GNUC_CONST;

typedef enum /*< pdb-skip >*/
{
  GIMP_CROP,   /*< desc="Crop"   >*/
  GIMP_RESIZE  /*< desc="Resize" >*/
} GimpCropType;


/*
 * non-registered enums; register them if needed
 */

typedef enum /*< pdb-skip >*/ /*< skip >*/
{
  SELECTION_ADD       = GIMP_CHANNEL_OP_ADD,
  SELECTION_SUBTRACT  = GIMP_CHANNEL_OP_SUBTRACT,
  SELECTION_REPLACE   = GIMP_CHANNEL_OP_REPLACE,
  SELECTION_INTERSECT = GIMP_CHANNEL_OP_INTERSECT,
  SELECTION_MOVE_MASK,
  SELECTION_MOVE,
  SELECTION_MOVE_COPY,
  SELECTION_ANCHOR
} SelectOps;

/*  Tool control actions  */
typedef enum /*< pdb-skip >*/ /*< skip >*/
{
  PAUSE,
  RESUME,
  HALT
} GimpToolAction;

/*  Motion event report modes  */
typedef enum /*< pdb-skip >*/ /*< skip >*/
{
  GIMP_MOTION_MODE_EXACT,
  GIMP_MOTION_MODE_HINT,
  GIMP_MOTION_MODE_COMPRESS
} GimpMotionMode;


#define GIMP_TYPE_RECT_SELECT_MODE (gimp_rect_select_mode_get_type ())

GType gimp_rect_select_mode_get_type (void) G_GNUC_CONST;

typedef enum /*< pdb-skip >*/
{
  GIMP_RECT_SELECT_MODE_FREE,        /*< desc="Free Select"        >*/
  GIMP_RECT_SELECT_MODE_FIXED_SIZE,  /*< desc="Fixed Size"         >*/
  GIMP_RECT_SELECT_MODE_FIXED_RATIO  /*< desc="Fixed Aspect Ratio" >*/
} GimpRectSelectMode;


#define GIMP_TYPE_TRANSFORM_TYPE (gimp_transform_type_get_type ())

GType gimp_transform_type_get_type (void) G_GNUC_CONST;

typedef enum /*< pdb-skip >*/
{
  GIMP_TRANSFORM_TYPE_LAYER,     /*< desc="Transform Active Layer" >*/
  GIMP_TRANSFORM_TYPE_SELECTION, /*< desc="Transform Selection"    >*/
  GIMP_TRANSFORM_TYPE_PATH       /*< desc="Transform Active Path"  >*/
} GimpTransformType;


#define GIMP_TYPE_TRANSFORM_GRID_TYPE (gimp_transform_grid_type_get_type ())

GType gimp_transform_grid_type_get_type (void) G_GNUC_CONST;

typedef enum /*< pdb-skip >*/
{
  GIMP_TRANSFORM_GRID_TYPE_NONE,     /*< desc="Don't Show Grid"      >*/
  GIMP_TRANSFORM_GRID_TYPE_N_LINES,  /*< desc="Number of Grid Lines" >*/
  GIMP_TRANSFORM_GRID_TYPE_SPACING   /*< desc="Grid Line Spacing"    >*/
} GimpTransformGridType;


/*  Possible transform functions  */
typedef enum /*< pdb-skip >*/ /*< skip >*/
{
  TRANSFORM_CREATING,
  TRANSFORM_HANDLE_1,
  TRANSFORM_HANDLE_2,
  TRANSFORM_HANDLE_3,
  TRANSFORM_HANDLE_4,
  TRANSFORM_HANDLE_CENTER
} TransformAction;


#endif /* __TOOLS_ENUMS_H__ */
