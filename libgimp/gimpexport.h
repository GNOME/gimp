/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1999 Peter Mattis and Spencer Kimball
 *
 * gimpexport.h
 * Copyright (C) 1999-2000 Sven Neumann <sven@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#if !defined (__GIMP_UI_H_INSIDE__) && !defined (GIMP_COMPILATION)
#error "Only <libgimp/gimpui.h> can be included directly."
#endif

#ifndef __GIMP_EXPORT_H__
#define __GIMP_EXPORT_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


/**
 * GimpExportCapabilities:
 * @GIMP_EXPORT_CAN_HANDLE_RGB:                 Handles RGB images
 * @GIMP_EXPORT_CAN_HANDLE_GRAY:                Handles grayscale images
 * @GIMP_EXPORT_CAN_HANDLE_INDEXED:             Handles indexed images
 * @GIMP_EXPORT_CAN_HANDLE_BITMAP:              Handles two-color indexed images
 * @GIMP_EXPORT_CAN_HANDLE_ALPHA:               Handles alpha channels
 * @GIMP_EXPORT_CAN_HANDLE_LAYERS:              Hanldes layers
 * @GIMP_EXPORT_CAN_HANDLE_LAYERS_AS_ANIMATION: Handles aminations of layers
 * @GIMP_EXPORT_CAN_HANDLE_LAYER_MASKS:         Handles layer masks
 * @GIMP_EXPORT_NEEDS_ALPHA:                    Needs alpha channels
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
  GIMP_EXPORT_NEEDS_ALPHA                    = 1 << 8
} GimpExportCapabilities;


/**
 * GimpExportReturn:
 * @GIMP_EXPORT_CANCEL: The export was cancelled
 * @GIMP_EXPORT_IGNORE: The image is unmodified but export shall continue anyway
 * @GIMP_EXPORT_EXPORT: The chosen transforms were applied to the image
 *
 * Possible return values of gimp_export_image().
 **/
typedef enum
{
  GIMP_EXPORT_CANCEL,
  GIMP_EXPORT_IGNORE,
  GIMP_EXPORT_EXPORT
} GimpExportReturn;


GimpExportReturn   gimp_export_image                   (gint32                 *image_ID,
                                                        gint32                 *drawable_ID,
                                                        const gchar            *format_name,
                                                        GimpExportCapabilities  capabilities);

GtkWidget        * gimp_export_dialog_new              (const gchar            *format_name,
                                                        const gchar            *role,
                                                        const gchar            *help_id);
GtkWidget        * gimp_export_dialog_get_content_area (GtkWidget              *dialog);


G_END_DECLS

#endif /* __GIMP_EXPORT_H__ */
