/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1999 Peter Mattis and Spencer Kimball
 *
 * gimpexport.h
 * Copyright (C) 1999-2000 Sven Neumann <sven@gimp.org>
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

#ifndef __GIMP_EXPORT_H__
#define __GIMP_EXPORT_H__

#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum {
  CAN_HANDLE_RGB                 = 1 << 0,
  CAN_HANDLE_GRAY                = 1 << 1,
  CAN_HANDLE_INDEXED             = 1 << 2,
  CAN_HANDLE_ALPHA               = 1 << 3,
  CAN_HANDLE_LAYERS              = 1 << 4,
  CAN_HANDLE_LAYERS_AS_ANIMATION = 1 << 5,
  NEEDS_ALPHA                    = 1 << 6
} GimpExportCapabilities;
  
typedef enum
{
  EXPORT_CANCEL,
  EXPORT_IGNORE,
  EXPORT_EXPORT
} GimpExportReturnType;

GimpExportReturnType gimp_export_image (gint32                 *image_ID,
					gint32                 *drawable_ID,
					gchar                  *format_name,
					GimpExportCapabilities  capabilities);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
