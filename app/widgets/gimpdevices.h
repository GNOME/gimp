/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifndef __DEVICES_H__
#define __DEVICES_H__

#include "tools.h"

typedef enum
{
  DEVICE_MODE       = 1 << 0,
  DEVICE_AXES       = 1 << 1,
  DEVICE_KEYS       = 1 << 2,
  DEVICE_TOOL       = 1 << 3,
  DEVICE_FOREGROUND = 1 << 4,
  DEVICE_BACKGROUND = 1 << 5,
  DEVICE_BRUSH      = 1 << 6,
  DEVICE_PATTERN    = 1 << 7,
  DEVICE_GRADIENT   = 1 << 8
} DeviceValues;

/*  Initialize the input devices  */
void   devices_init         (void);

/*  Restores device settings from rc file  */
void   devices_restore      (void);

/*  Create device info dialog  */
void   input_dialog_create  (void);

/*  Create the device status dialog  */
void   device_status_create (void);

/*  Returns TRUE, and makes necessary global changes
 *  event is not for current_device
 */
gint   devices_check_change (GdkEvent     *event);

/*  Loads stored device settings (tool, cursor, ...)  */
void   select_device        (guint32       device);

/* Add information about one tool from rc file */
void   devices_rc_update    (gchar        *name,
			     DeviceValues  values,
			     GdkInputMode  mode,
			     gint          num_axes,
			     GdkAxisUse   *axes,
			     gint          num_keys,
			     GdkDeviceKey *keys,
			     ToolType      tool,
			     guchar        foreground[],
			     guchar        background[],
			     gchar        *brush_name,
			     gchar        *pattern_name,
			     gchar        *gradient_name);

/*  Free device status (only for session-managment)  */
void   device_status_free   (void);

/*  Current device id  */
extern gint current_device;

#endif /* __DEVICES_H__ */
