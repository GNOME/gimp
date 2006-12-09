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

#ifndef __TEXT_ENUMS_H__
#define __TEXT_ENUMS_H__


#define GIMP_TYPE_TEXT_BOX_MODE (gimp_text_box_mode_get_type ())

GType gimp_text_box_mode_get_type (void) G_GNUC_CONST;

typedef enum /*< pdb-skip >*/
{
  GIMP_TEXT_BOX_DYNAMIC,
  GIMP_TEXT_BOX_FIXED
} GimpTextBoxMode;


#define GIMP_TYPE_TEXT_DIRECTION (gimp_text_direction_get_type ())

GType gimp_text_direction_get_type (void) G_GNUC_CONST;

typedef enum /*< pdb-skip >*/
{
  GIMP_TEXT_DIRECTION_LTR,   /*< desc="From left to right" >*/
  GIMP_TEXT_DIRECTION_RTL    /*< desc="From right to left" >*/
} GimpTextDirection;


#define GIMP_TYPE_TEXT_JUSTIFICATION (gimp_text_justification_get_type ())

GType gimp_text_justification_get_type (void) G_GNUC_CONST;

typedef enum /*< pdb-skip >*/
{
  GIMP_TEXT_JUSTIFY_LEFT,    /*< desc="Left justified"  >*/
  GIMP_TEXT_JUSTIFY_RIGHT,   /*< desc="Right justified" >*/
  GIMP_TEXT_JUSTIFY_CENTER,  /*< desc="Centered"        >*/
  GIMP_TEXT_JUSTIFY_FILL     /*< desc="Filled"          >*/
} GimpTextJustification;


#define GIMP_TYPE_TEXT_OUTLINE (gimp_text_outline_get_type ())

GType gimp_text_outline_get_type (void) G_GNUC_CONST;

typedef enum /*< pdb-skip >*/
{
  GIMP_TEXT_OUTLINE_NONE,
  GIMP_TEXT_OUTLINE_STROKE_ONLY,
  GIMP_TEXT_OUTLINE_STROKE_FILL
} GimpTextOutline;


#endif /* __TEXT_ENUMS_H__ */
