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

#ifndef __PAINT_ENUMS_H__
#define __PAINT_ENUMS_H__

#if 0
   This file is parsed by two scripts, enumgen.pl in tools/pdbgen,
   and gimp-mkenums. All enums that are not marked with
   /*< pdb-skip >*/ are exported to libgimp and the PDB. Enums that are
   not marked with /*< skip >*/ are registered with the GType system.
   If you want the enum to be skipped by both scripts, you have to use
   /*< pdb-skip, skip >*/.

   The same syntax applies to enum values.
#endif


/*
 * these enums that are registered with the type system
 */

#define GIMP_TYPE_BRUSH_APPLICATION_MODE (gimp_brush_application_mode_get_type ())

GType gimp_brush_application_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_BRUSH_HARD,
  GIMP_BRUSH_SOFT,
  GIMP_BRUSH_PRESSURE  /*< pdb-skip, skip >*/
} GimpBrushApplicationMode;


#define GIMP_TYPE_CLONE_ALIGN_MODE (gimp_clone_align_mode_get_type ())

GType gimp_clone_align_mode_get_type (void) G_GNUC_CONST;

typedef enum /*< pdb-skip >*/
{
  GIMP_CLONE_ALIGN_NO,          /*< desc="None"        >*/
  GIMP_CLONE_ALIGN_YES,         /*< desc="Aligned"     >*/
  GIMP_CLONE_ALIGN_REGISTERED,  /*< desc="Registered"  >*/
  GIMP_CLONE_ALIGN_FIXED        /*< desc="Fixed"       >*/
} GimpCloneAlignMode;


#define GIMP_TYPE_CONVOLVE_TYPE (gimp_convolve_type_get_type ())

GType gimp_convolve_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_BLUR_CONVOLVE,     /*< desc="Blur"    >*/
  GIMP_SHARPEN_CONVOLVE,  /*< desc="Sharpen" >*/
  GIMP_CUSTOM_CONVOLVE    /*< pdb-skip, skip >*/
} GimpConvolveType;


#define GIMP_TYPE_INK_BLOB_TYPE (gimp_ink_blob_type_get_type ())

GType gimp_ink_blob_type_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  GIMP_INK_BLOB_TYPE_ELLIPSE,
  GIMP_INK_BLOB_TYPE_SQUARE,
  GIMP_INK_BLOB_TYPE_DIAMOND
} GimpInkBlobType;


#define GIMP_TYPE_HEAL_ALIGN_MODE (gimp_heal_align_mode_get_type ())

GType gimp_heal_align_mode_get_type (void) G_GNUC_CONST;

typedef enum /*< pdb-skip >*/
{
  GIMP_HEAL_ALIGN_NO,          /*< desc="None"        >*/
  GIMP_HEAL_ALIGN_YES,         /*< desc="Aligned"     >*/
  GIMP_HEAL_ALIGN_FIXED        /*< desc="Fixed"       >*/
} GimpHealAlignMode;


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
