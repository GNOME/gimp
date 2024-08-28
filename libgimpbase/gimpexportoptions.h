/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpexportoptions.h
 * Copyright (C) 2024 Alx Sa.
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

#if !defined (__GIMP_BASE_H_INSIDE__) && !defined (GIMP_BASE_COMPILATION)
#error "Only <libgimpbase/gimpbase.h> can be included directly."
#endif


#ifndef __GIMP_EXPORT_OPTIONS_H__
#define __GIMP_EXPORT_OPTIONS_H__

G_BEGIN_DECLS


/* For information look into the C source or the html documentation */


#define GIMP_TYPE_EXPORT_OPTIONS               (gimp_export_options_get_type ())
#define GIMP_VALUE_HOLDS_EXPORT_OPTIONS(value) (G_TYPE_CHECK_VALUE_TYPE ((value), GIMP_TYPE_EXPORT_OPTIONS))
G_DECLARE_FINAL_TYPE (GimpExportOptions, gimp_export_options, GIMP, EXPORT_OPTIONS, GObject)


/**
 * GimpExportCapabilities:
 * @GIMP_EXPORT_CAN_HANDLE_RGB:                 Handles RGB images
 * @GIMP_EXPORT_CAN_HANDLE_GRAY:                Handles grayscale images
 * @GIMP_EXPORT_CAN_HANDLE_INDEXED:             Handles indexed images
 * @GIMP_EXPORT_CAN_HANDLE_BITMAP:              Handles two-color indexed images
 * @GIMP_EXPORT_CAN_HANDLE_ALPHA:               Handles alpha channels
 * @GIMP_EXPORT_CAN_HANDLE_LAYERS:              Handles layers
 * @GIMP_EXPORT_CAN_HANDLE_LAYERS_AS_ANIMATION: Handles animation of layers
 * @GIMP_EXPORT_CAN_HANDLE_LAYER_EFFECTS:       Handles layer effects
 * @GIMP_EXPORT_CAN_HANDLE_LAYER_MASKS:         Handles layer masks
 * @GIMP_EXPORT_NEEDS_ALPHA:                    Needs alpha channels
 * @GIMP_EXPORT_NEEDS_CROP:                     Needs to crop content to image bounds
 *
 * The types of images and layers an export procedure can handle
 **/
typedef enum
{
  GIMP_EXPORT_CAN_HANDLE_RGB                 = 1 << 0,
  GIMP_EXPORT_CAN_HANDLE_GRAY                = 1 << 1,
  GIMP_EXPORT_CAN_HANDLE_INDEXED             = 1 << 2,
  GIMP_EXPORT_CAN_HANDLE_BITMAP              = 1 << 3,
  GIMP_EXPORT_CAN_HANDLE_ALPHA               = 1 << 4,
  GIMP_EXPORT_CAN_HANDLE_LAYERS              = 1 << 5,
  GIMP_EXPORT_CAN_HANDLE_LAYERS_AS_ANIMATION = 1 << 6,
  GIMP_EXPORT_CAN_HANDLE_LAYER_MASKS         = 1 << 7,
  GIMP_EXPORT_CAN_HANDLE_LAYER_EFFECTS       = 1 << 8,
  GIMP_EXPORT_NEEDS_ALPHA                    = 1 << 9,
  GIMP_EXPORT_NEEDS_CROP                     = 1 << 10
} GimpExportCapabilities;


G_END_DECLS

#endif /* __GIMP_EXPORT_OPTIONS_H__ */
