/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * operations-enums.h
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __OPERATIONS_ENUMS_H__
#define __OPERATIONS_ENUMS_H__


#define GIMP_TYPE_LAYER_COLOR_SPACE (gimp_layer_color_space_get_type ())

GType gimp_layer_color_space_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_LAYER_COLOR_SPACE_RGB_LINEAR,
  GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL,
  GIMP_LAYER_COLOR_SPACE_LAB,
} GimpLayerColorSpace;


#define GIMP_TYPE_LAYER_COMPOSITE_MODE (gimp_layer_composite_mode_get_type ())

GType gimp_layer_composite_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_LAYER_COMPOSITE_SRC_ATOP,
  GIMP_LAYER_COMPOSITE_SRC_OVER,
  GIMP_LAYER_COMPOSITE_SRC_IN,
  GIMP_LAYER_COMPOSITE_DST_ATOP
} GimpLayerCompositeMode;


#endif /* __OPERATIONS_ENUMS_H__ */
