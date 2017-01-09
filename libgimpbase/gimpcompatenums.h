/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_COMPAT_ENUMS_H__
#define __GIMP_COMPAT_ENUMS_H__


G_BEGIN_DECLS

/*  These enums exist only for compatibility, their nicks are needed
 *  for config file parsing; they are registered in gimp_base_init().
 */


#define GIMP_TYPE_ADD_MASK_TYPE_COMPAT (gimp_add_mask_type_compat_get_type ())

GType gimp_add_mask_type_compat_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_ADD_WHITE_MASK          = GIMP_ADD_MASK_WHITE,
  GIMP_ADD_BLACK_MASK          = GIMP_ADD_MASK_BLACK,
  GIMP_ADD_ALPHA_MASK          = GIMP_ADD_MASK_ALPHA,
  GIMP_ADD_ALPHA_TRANSFER_MASK = GIMP_ADD_MASK_ALPHA_TRANSFER,
  GIMP_ADD_SELECTION_MASK      = GIMP_ADD_MASK_SELECTION,
  GIMP_ADD_COPY_MASK           = GIMP_ADD_MASK_COPY,
  GIMP_ADD_CHANNEL_MASK        = GIMP_ADD_MASK_CHANNEL
} GimpAddMaskTypeCompat;


#define GIMP_TYPE_BLEND_MODE_COMPAT (gimp_blend_mode_compat_get_type ())

GType gimp_blend_mode_compat_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_FG_BG_RGB_MODE      = GIMP_BLEND_FG_BG_RGB,
  GIMP_FG_BG_HSV_MODE      = GIMP_BLEND_FG_BG_HSV,
  GIMP_FG_TRANSPARENT_MODE = GIMP_BLEND_FG_TRANSPARENT,
  GIMP_CUSTOM_MODE         = GIMP_BLEND_CUSTOM
} GimpBlendModeCompat;


#define GIMP_TYPE_BUCKET_FILL_MODE_COMPAT (gimp_bucket_fill_mode_compat_get_type ())

GType gimp_bucket_fill_mode_compat_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_FG_BUCKET_FILL      = GIMP_BUCKET_FILL_FG,
  GIMP_BG_BUCKET_FILL      = GIMP_BUCKET_FILL_BG,
  GIMP_PATTERN_BUCKET_FILL = GIMP_BUCKET_FILL_PATTERN
} GimpBucketFillModeCompat;


#define GIMP_TYPE_CLONE_TYPE_COMPAT (gimp_clone_type_compat_get_type ())

GType gimp_clone_type_compat_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_IMAGE_CLONE   = GIMP_CLONE_IMAGE,
  GIMP_PATTERN_CLONE = GIMP_CLONE_PATTERN
} GimpCloneTypeCompat;


#define GIMP_TYPE_CONVOLVE_TYPE_COMPAT (gimp_convolve_type_compat_get_type ())

GType gimp_convolve_type_compat_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_BLUR_CONVOLVE    = GIMP_CONVOLVE_BLUR,
  GIMP_SHARPEN_CONVOLVE = GIMP_CONVOLVE_SHARPEN
} GimpConvolveTypeCompat;


#define GIMP_TYPE_FILL_TYPE_COMPAT (gimp_fill_type_compat_get_type ())

GType gimp_fill_type_compat_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_FOREGROUND_FILL  = GIMP_FILL_FOREGROUND,
  GIMP_BACKGROUND_FILL  = GIMP_FILL_BACKGROUND,
  GIMP_WHITE_FILL       = GIMP_FILL_WHITE,
  GIMP_TRANSPARENT_FILL = GIMP_FILL_TRANSPARENT,
  GIMP_PATTERN_FILL     = GIMP_FILL_PATTERN
} GimpFillTypeCompat;


#define GIMP_TYPE_HUE_RANGE_COMPAT (gimp_hue_range_compat_get_type ())

GType gimp_hue_range_compat_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_ALL_HUES,
  GIMP_RED_HUES,
  GIMP_YELLOW_HUES,
  GIMP_GREEN_HUES,
  GIMP_CYAN_HUES,
  GIMP_BLUE_HUES,
  GIMP_MAGENTA_HUES
} GimpHueRangeCompat;


G_END_DECLS

#endif  /* __GIMP_COMPAT_ENUMS_H__ */
