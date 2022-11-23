/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaconfigenums.h
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

#ifndef __LIGMA_CONFIG_ENUMS_H__
#define __LIGMA_CONFIG_ENUMS_H__


#define LIGMA_TYPE_COLOR_MANAGEMENT_MODE (ligma_color_management_mode_get_type ())

GType ligma_color_management_mode_get_type (void) G_GNUC_CONST;

/**
 * LigmaColorManagementMode:
 * @LIGMA_COLOR_MANAGEMENT_OFF:       Color management is off
 * @LIGMA_COLOR_MANAGEMENT_DISPLAY:   Color managed display
 * @LIGMA_COLOR_MANAGEMENT_SOFTPROOF: Soft-proofing
 *
 * Modes of color management.
 **/
typedef enum
{
  LIGMA_COLOR_MANAGEMENT_OFF,       /*< desc="No color management"   >*/
  LIGMA_COLOR_MANAGEMENT_DISPLAY,   /*< desc="Color-managed display" >*/
  LIGMA_COLOR_MANAGEMENT_SOFTPROOF  /*< desc="Soft-proofing"      >*/
} LigmaColorManagementMode;


#define LIGMA_TYPE_COLOR_RENDERING_INTENT (ligma_color_rendering_intent_get_type ())

GType ligma_color_rendering_intent_get_type (void) G_GNUC_CONST;

/**
 * LigmaColorRenderingIntent:
 * @LIGMA_COLOR_RENDERING_INTENT_PERCEPTUAL:            Perceptual
 * @LIGMA_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC: Relative colorimetric
 * @LIGMA_COLOR_RENDERING_INTENT_SATURATION:            Saturation
 * @LIGMA_COLOR_RENDERING_INTENT_ABSOLUTE_COLORIMETRIC: Absolute colorimetric
 *
 * Intents for color management.
 **/
typedef enum
{
  LIGMA_COLOR_RENDERING_INTENT_PERCEPTUAL,            /*< desc="Perceptual"            >*/
  LIGMA_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC, /*< desc="Relative colorimetric" >*/
  LIGMA_COLOR_RENDERING_INTENT_SATURATION,            /*< desc="Saturation"            >*/
  LIGMA_COLOR_RENDERING_INTENT_ABSOLUTE_COLORIMETRIC  /*< desc="Absolute colorimetric" >*/
} LigmaColorRenderingIntent;


#endif /* __LIGMA_CONFIG_ENUMS_H__ */
