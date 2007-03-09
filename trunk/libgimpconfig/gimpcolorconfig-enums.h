/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpColorConfig enums
 * Copyright (C) 2004  Stefan Döhla <stefan@doehla.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __GIMP_COLOR_CONFIG_ENUMS_H__
#define __GIMP_COLOR_CONFIG_ENUMS_H__


#define GIMP_TYPE_COLOR_MANAGEMENT_MODE (gimp_color_management_mode_get_type ())

GType gimp_color_management_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_COLOR_MANAGEMENT_OFF,       /*< desc="No color management"   >*/
  GIMP_COLOR_MANAGEMENT_DISPLAY,   /*< desc="Color managed display" >*/
  GIMP_COLOR_MANAGEMENT_SOFTPROOF  /*< desc="Print simulation"      >*/
} GimpColorManagementMode;


#define GIMP_TYPE_COLOR_RENDERING_INTENT \
  (gimp_color_rendering_intent_get_type ())

GType gimp_color_rendering_intent_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_COLOR_RENDERING_INTENT_PERCEPTUAL,            /*< desc="Perceptual"            >*/
  GIMP_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC, /*< desc="Relative colorimetric" >*/
  GIMP_COLOR_RENDERING_INTENT_SATURATION,            /*< desc="intent|Saturation"            >*/
  GIMP_COLOR_RENDERING_INTENT_ABSOLUTE_COLORIMETRIC  /*< desc="Absolute colorimetric" >*/
} GimpColorRenderingIntent;



#endif /* GIMP_COLOR_CONFIG_ENUMS_H__ */
