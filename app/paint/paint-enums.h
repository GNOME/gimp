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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __PAINT_ENUMS_H__
#define __PAINT_ENUMS_H__

#if 0
   This file is parsed by two scripts, enumgen.pl in pdb,
   and gimp-mkenums. All enums that are not marked with
   /*< pdb-skip >*/ are exported to libgimp and the PDB. Enums that are
   not marked with /*< skip >*/ are registered with the GType system.
   If you want the enum to be skipped by both scripts, you have to use
   /*< pdb-skip, skip >*/.

   The same syntax applies to enum values.
#endif


/*
 * enums that are registered with the type system
 */

#define GIMP_TYPE_BRUSH_APPLICATION_MODE (gimp_brush_application_mode_get_type ())

GType gimp_brush_application_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_BRUSH_HARD,
  GIMP_BRUSH_SOFT,
  GIMP_BRUSH_PRESSURE  /*< pdb-skip, skip >*/
} GimpBrushApplicationMode;


#define GIMP_TYPE_PERSPECTIVE_CLONE_MODE (gimp_perspective_clone_mode_get_type ())

GType gimp_perspective_clone_mode_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  GIMP_PERSPECTIVE_CLONE_MODE_ADJUST,  /*< desc="Modify Perspective" >*/
  GIMP_PERSPECTIVE_CLONE_MODE_PAINT    /*< desc="Perspective Clone"  >*/
} GimpPerspectiveCloneMode;


#define GIMP_TYPE_SOURCE_ALIGN_MODE (gimp_source_align_mode_get_type ())

GType gimp_source_align_mode_get_type (void) G_GNUC_CONST;

typedef enum /*< pdb-skip >*/
{
  GIMP_SOURCE_ALIGN_NO,          /*< desc="None"        >*/
  GIMP_SOURCE_ALIGN_YES,         /*< desc="Aligned"     >*/
  GIMP_SOURCE_ALIGN_REGISTERED,  /*< desc="Registered"  >*/
  GIMP_SOURCE_ALIGN_FIXED        /*< desc="Fixed"       >*/
} GimpSourceAlignMode;


/*
 * non-registered enums; register them if needed
 */

typedef enum  /*< skip, pdb-skip >*/
{
  GIMP_PAINT_STATE_INIT,    /*  Setup PaintFunc internals                    */
  GIMP_PAINT_STATE_MOTION,  /*  PaintFunc performs motion-related rendering  */
  GIMP_PAINT_STATE_FINISH   /*  Cleanup and/or reset PaintFunc operation     */
} GimpPaintState;


#endif /* __PAINT_ENUMS_H__ */
