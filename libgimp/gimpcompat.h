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

/* This file contains aliases that are kept for historical reasons,
 * because a wide code base depends on them. We suggest that you
 * only use this header temporarily while porting a plug-in to the
 * new API.
 *
 * These defines will be removed in the next development cycle.
 */

#ifndef GIMP_DISABLE_DEPRECATED


#define GimpRunModeType                         GimpRunMode
#define GimpExportReturnType                    GimpExportReturn

#define gimp_use_xshm                           TRUE
#define gimp_color_cube                         ((guchar *) { 6, 6, 4, 24 })

#define gimp_crop                               gimp_image_crop

#define gimp_gradients_get_active               gimp_gradients_get_gradient
#define gimp_gradients_set_active               gimp_gradients_set_gradient

#define gimp_help_init()                        ((void) 0)
#define gimp_help_free()                        ((void) 0)

#define gimp_interactive_selection_brush        gimp_brush_select_new
#define gimp_brush_select_widget                gimp_brush_select_widget_new
#define gimp_brush_select_widget_set_popup      gimp_brush_select_widget_set
#define gimp_brush_select_widget_close_popup    gimp_brush_select_widget_close

#define gimp_interactive_selection_font         gimp_font_select_new
#define gimp_gradient_select_widget             gimp_gradient_select_widget_new
#define gimp_gradient_select_widget_set_popup   gimp_gradient_select_widget_set
#define gimp_gradient_select_widget_close_popup gimp_gradient_select_widget_close

#define gimp_interactive_selection_gradient     gimp_gradient_select_new
#define gimp_font_select_widget                 gimp_font_select_widget_new
#define gimp_font_select_widget_set_popup       gimp_font_select_widget_set
#define gimp_font_select_widget_close_popup     gimp_font_select_widget_close

#define gimp_interactive_selection_pattern      gimp_pattern_select_new
#define gimp_pattern_select_widget              gimp_pattern_select_widget_new
#define gimp_pattern_select_widget_set_popup    gimp_pattern_select_widget_set
#define gimp_pattern_select_widget_close_popup  gimp_pattern_select_widget_close
#define INTENSITY(r,g,b)                        GIMP_RGB_INTENSITY(r,g,b)
#define INTENSITY_RED                           GIMP_RGB_INTENSITY_RED
#define INTENSITY_GREEN                         GIMP_RGB_INTENSITY_GREEN
#define INTENSITY_BLUE                          GIMP_RGB_INTENSITY_BLUE


enum
{
  GIMP_WHITE_MASK         = GIMP_ADD_WHITE_MASK,
  GIMP_BLACK_MASK         = GIMP_ADD_BLACK_MASK,
  GIMP_ALPHA_MASK         = GIMP_ADD_ALPHA_MASK,
  GIMP_SELECTION_MASK     = GIMP_ADD_SELECTION_MASK,
  GIMP_COPY_MASK          = GIMP_ADD_COPY_MASK,
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
};

enum
{
  GIMP_LINEAR               = GIMP_GRADIENT_LINEAR,
  GIMP_BILNEAR              = GIMP_GRADIENT_BILINEAR,
  GIMP_RADIAL               = GIMP_GRADIENT_RADIAL,
  GIMP_SQUARE               = GIMP_GRADIENT_SQUARE,
  GIMP_CONICAL_SYMMETRIC    = GIMP_GRADIENT_CONICAL_SYMMETRIC,
  GIMP_CONICAL_ASYMMETRIC   = GIMP_GRADIENT_CONICAL_ASYMMETRIC,
  GIMP_SHAPEBURST_ANGULAR   = GIMP_GRADIENT_SHAPEBURST_ANGULAR,
  GIMP_SHAPEBURST_SPHERICAL = GIMP_GRADIENT_SHAPEBURST_SPHERICAL,
  GIMP_SHAPEBURST_DIMPLED   = GIMP_GRADIENT_SHAPEBURST_DIMPLED,
  GIMP_SPIRAL_CLOCKWISE     = GIMP_GRADIENT_SPIRAL_CLOCKWISE,
  GIMP_SPIRAL_ANTICLOCKWISE = GIMP_GRADIENT_SPIRAL_ANTICLOCKWISE
};


#endif  /* GIMP_DISABLE_DEPRECATED */

G_END_DECLS

#endif  /* __GIMP_COMPAT_H__ */
