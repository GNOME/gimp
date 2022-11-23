/* LIGMA - The GNU Image Manipulation Program
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
   and ligma-mkenums. All enums that are not marked with
   /*< pdb-skip >*/ are exported to libligma and the PDB. Enums that are
   not marked with /*< skip >*/ are registered with the GType system.
   If you want the enum to be skipped by both scripts, you have to use
   /*< pdb-skip, skip >*/.

   The same syntax applies to enum values.
#endif


/*
 * enums that are registered with the type system
 */

#define LIGMA_TYPE_BRUSH_APPLICATION_MODE (ligma_brush_application_mode_get_type ())

GType ligma_brush_application_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_BRUSH_HARD,
  LIGMA_BRUSH_SOFT,
  LIGMA_BRUSH_PRESSURE  /*< pdb-skip, skip >*/
} LigmaBrushApplicationMode;


#define LIGMA_TYPE_PERSPECTIVE_CLONE_MODE (ligma_perspective_clone_mode_get_type ())

GType ligma_perspective_clone_mode_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  LIGMA_PERSPECTIVE_CLONE_MODE_ADJUST,  /*< desc="Modify Perspective" >*/
  LIGMA_PERSPECTIVE_CLONE_MODE_PAINT    /*< desc="Perspective Clone"  >*/
} LigmaPerspectiveCloneMode;


#define LIGMA_TYPE_SOURCE_ALIGN_MODE (ligma_source_align_mode_get_type ())

GType ligma_source_align_mode_get_type (void) G_GNUC_CONST;

typedef enum /*< pdb-skip >*/
{
  LIGMA_SOURCE_ALIGN_NO,          /*< desc="None"        >*/
  LIGMA_SOURCE_ALIGN_YES,         /*< desc="Aligned"     >*/
  LIGMA_SOURCE_ALIGN_REGISTERED,  /*< desc="Registered"  >*/
  LIGMA_SOURCE_ALIGN_FIXED        /*< desc="Fixed"       >*/
} LigmaSourceAlignMode;


/*
 * non-registered enums; register them if needed
 */

typedef enum  /*< skip, pdb-skip >*/
{
  LIGMA_PAINT_STATE_INIT,    /*  Setup PaintFunc internals                    */
  LIGMA_PAINT_STATE_MOTION,  /*  PaintFunc performs motion-related rendering  */
  LIGMA_PAINT_STATE_FINISH   /*  Cleanup and/or reset PaintFunc operation     */
} LigmaPaintState;


#endif /* __PAINT_ENUMS_H__ */
