/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpconfigenums.h
 * Copyright (C) 2004  Stefan Döhla <stefan@doehla.de>
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
 * <https://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_CONFIG_ENUMS_H__
#define __GIMP_CONFIG_ENUMS_H__


#define GIMP_TYPE_COLOR_MANAGEMENT_MODE (gimp_color_management_mode_get_type ())

GType gimp_color_management_mode_get_type (void) G_GNUC_CONST;

/**
 * GimpColorManagementMode:
 * @GIMP_COLOR_MANAGEMENT_OFF:       Color management is off
 * @GIMP_COLOR_MANAGEMENT_DISPLAY:   Color managed display
 * @GIMP_COLOR_MANAGEMENT_SOFTPROOF: Soft-proofing
 *
 * Modes of color management.
 **/
typedef enum
{
  GIMP_COLOR_MANAGEMENT_OFF,       /*< desc="No color management"   >*/
  GIMP_COLOR_MANAGEMENT_DISPLAY,   /*< desc="Color-managed display" >*/
  GIMP_COLOR_MANAGEMENT_SOFTPROOF  /*< desc="Soft-proofing"      >*/
} GimpColorManagementMode;


#define GIMP_TYPE_COLOR_RENDERING_INTENT (gimp_color_rendering_intent_get_type ())

GType gimp_color_rendering_intent_get_type (void) G_GNUC_CONST;

/**
 * GimpColorRenderingIntent:
 * @GIMP_COLOR_RENDERING_INTENT_PERCEPTUAL:            Perceptual
 * @GIMP_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC: Relative colorimetric
 * @GIMP_COLOR_RENDERING_INTENT_SATURATION:            Saturation
 * @GIMP_COLOR_RENDERING_INTENT_ABSOLUTE_COLORIMETRIC: Absolute colorimetric
 *
 * Intents for color management.
 **/
typedef enum
{
  GIMP_COLOR_RENDERING_INTENT_PERCEPTUAL,            /*< desc="Perceptual"            >*/
  GIMP_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC, /*< desc="Relative colorimetric" >*/
  GIMP_COLOR_RENDERING_INTENT_SATURATION,            /*< desc="Saturation"            >*/
  GIMP_COLOR_RENDERING_INTENT_ABSOLUTE_COLORIMETRIC  /*< desc="Absolute colorimetric" >*/
} GimpColorRenderingIntent;


#endif /* __GIMP_CONFIG_ENUMS_H__ */
