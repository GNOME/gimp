/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1999 Peter Mattis and Spencer Kimball
 *
 * ligmaexport.h
 * Copyright (C) 1999-2000 Sven Neumann <sven@ligma.org>
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

#if !defined (__LIGMA_UI_H_INSIDE__) && !defined (LIGMA_COMPILATION)
#error "Only <libligma/ligmaui.h> can be included directly."
#endif

#ifndef __LIGMA_EXPORT_H__
#define __LIGMA_EXPORT_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


/**
 * LigmaExportCapabilities:
 * @LIGMA_EXPORT_CAN_HANDLE_RGB:                 Handles RGB images
 * @LIGMA_EXPORT_CAN_HANDLE_GRAY:                Handles grayscale images
 * @LIGMA_EXPORT_CAN_HANDLE_INDEXED:             Handles indexed images
 * @LIGMA_EXPORT_CAN_HANDLE_BITMAP:              Handles two-color indexed images
 * @LIGMA_EXPORT_CAN_HANDLE_ALPHA:               Handles alpha channels
 * @LIGMA_EXPORT_CAN_HANDLE_LAYERS:              Handles layers
 * @LIGMA_EXPORT_CAN_HANDLE_LAYERS_AS_ANIMATION: Handles animation of layers
 * @LIGMA_EXPORT_CAN_HANDLE_LAYER_MASKS:         Handles layer masks
 * @LIGMA_EXPORT_NEEDS_ALPHA:                    Needs alpha channels
 * @LIGMA_EXPORT_NEEDS_CROP:                     Needs to crop content to image bounds
 *
 * The types of images and layers an export procedure can handle
 **/
typedef enum
{
  LIGMA_EXPORT_CAN_HANDLE_RGB                 = 1 << 0,
  LIGMA_EXPORT_CAN_HANDLE_GRAY                = 1 << 1,
  LIGMA_EXPORT_CAN_HANDLE_INDEXED             = 1 << 2,
  LIGMA_EXPORT_CAN_HANDLE_BITMAP              = 1 << 3,
  LIGMA_EXPORT_CAN_HANDLE_ALPHA               = 1 << 4,
  LIGMA_EXPORT_CAN_HANDLE_LAYERS              = 1 << 5,
  LIGMA_EXPORT_CAN_HANDLE_LAYERS_AS_ANIMATION = 1 << 6,
  LIGMA_EXPORT_CAN_HANDLE_LAYER_MASKS         = 1 << 7,
  LIGMA_EXPORT_NEEDS_ALPHA                    = 1 << 8,
  LIGMA_EXPORT_NEEDS_CROP                     = 1 << 9
} LigmaExportCapabilities;


/**
 * LigmaExportReturn:
 * @LIGMA_EXPORT_CANCEL: The export was cancelled
 * @LIGMA_EXPORT_IGNORE: The image is unmodified but export shall continue anyway
 * @LIGMA_EXPORT_EXPORT: The chosen transforms were applied to the image
 *
 * Possible return values of ligma_export_image().
 **/
typedef enum
{
  LIGMA_EXPORT_CANCEL,
  LIGMA_EXPORT_IGNORE,
  LIGMA_EXPORT_EXPORT
} LigmaExportReturn;


GtkWidget        * ligma_export_dialog_new              (const gchar            *format_name,
                                                        const gchar            *role,
                                                        const gchar            *help_id);
GtkWidget        * ligma_export_dialog_get_content_area (GtkWidget              *dialog);

LigmaExportReturn   ligma_export_image                   (LigmaImage             **image,
                                                        gint                   *n_drawables,
                                                        LigmaDrawable         ***drawables,
                                                        const gchar            *format_name,
                                                        LigmaExportCapabilities  capabilities);


G_END_DECLS

#endif /* __LIGMA_EXPORT_H__ */
