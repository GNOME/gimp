/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpcompat.h
 * Compatibility defines to ease migration from the GIMP-1.2 API
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __GIMP_COMPAT_H__
#define __GIMP_COMPAT_H__

G_BEGIN_DECLS

/* This file contains aliases that are kept for historical
 * reasons, because a wide code base depends on them.
 *
 * These defines will be removed in the next development cycle. 
 */

#define GimpRunModeType           GimpRunMode

#define gimp_use_xshm             TRUE
#define gimp_color_cube           ((guchar *) { 6, 6, 4, 24 })

#define gimp_crop                 gimp_image_crop

#define gimp_gradients_get_active gimp_gradients_get_gradient
#define gimp_gradients_set_active gimp_gradients_set_gradient

#define gimp_help_init()          ((void) 0)
#define gimp_help_free()          ((void) 0)


enum
{
  GIMP_WHITE_MASK         = GIMP_ADD_WHITE_MASK,
  GIMP_BLACK_MASK         = GIMP_ADD_BLACK_MASK,
  GIMP_ALPHA_MASK         = GIMP_ADD_ALPHA_MASK,
  GIMP_SELECTION_MASK     = GIMP_ADD_SELECTION_MASK,
  GIMP_INV_SELECTION_MASK = GIMP_ADD_INVERSE_SELECTION_MASK,
  GIMP_COPY_MASK          = GIMP_ADD_COPY_MASK,
  GIMP_INV_COPY_MASK      = GIMP_ADD_INVERSE_COPY_MASK
};

enum
{
  GIMP_ADD       = GIMP_CHANNEL_OP_ADD,
  GIMP_SUB       = GIMP_CHANNEL_OP_SUBTRACT,
  GIMP_REPLACE   = GIMP_CHANNEL_OP_REPLACE,
  GIMP_INTERSECT = GIMP_CHANNEL_OP_INTERSECT
};

enum
{
  GIMP_FG_BG_RGB = GIMP_FG_BG_RGB_MODE,
  GIMP_FG_BG_HSV = GIMP_FG_BG_HSV_MODE,
  GIMP_FG_TRANS  = GIMP_FG_TRANSPARENT_MODE,
  GIMP_CUSTOM    = GIMP_CUSTOM_MODE
};

enum
{
  GIMP_FG_IMAGE_FILL    = GIMP_FOREGROUND_FILL,
  GIMP_BG_IMAGE_FILL    = GIMP_BACKGROUND_FILL,
  GIMP_WHITE_IMAGE_FILL = GIMP_WHITE_FILL,
  GIMP_TRANS_IMAGE_FILL = GIMP_TRANSPARENT_FILL,
  GIMP_NO_IMAGE_FILL    = GIMP_NO_FILL
};

enum
{
  GIMP_APPLY   = GIMP_MASK_APPLY,
  GIMP_DISCARD = GIMP_MASK_DISCARD
};

enum
{
  GIMP_ONCE_FORWARD   = GIMP_GRADIENT_ONCE_FORWARD,
  GIMP_ONCE_BACKWARDS = GIMP_GRADIENT_ONCE_BACKWARD,
  GIMP_LOOP_SAWTOOTH  = GIMP_GRADIENT_LOOP_SAWTOOTH,
  GIMP_LOOP_TRIANGLE  = GIMP_GRADIENT_LOOP_TRIANGLE,
};

enum
{
  GIMP_HARD = GIMP_BRUSH_HARD,
  GIMP_SOFT = GIMP_BRUSH_SOFT,
};

enum
{
  GIMP_CONTINUOUS  = GIMP_PAINT_CONSTANT,
  GIMP_INCREMENTAL = GIMP_PAINT_INCREMENTAL
};

enum
{
  GIMP_HORIZONTAL = GIMP_ORIENTATION_HORIZONTAL,
  GIMP_VERTICAL   = GIMP_ORIENTATION_VERTICAL,
  GIMP_UNKNOWN    = GIMP_ORIENTATION_UNKNOWN
} GimpOrientationType;

G_END_DECLS

#endif  /* __GIMP_COMPAT_H__ */
