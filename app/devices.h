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
void        devices_init          (Gimp         *gimp);

/*  Restores device settings from rc file  */
void        devices_restore       (Gimp         *gimp);

GdkDevice * devices_get_current   (Gimp         *gimp);

/*  Returns TRUE, and makes necessary global changes
 *  if event is not for current_device
 */
gboolean    devices_check_change  (Gimp         *gimp,
                                   GdkEvent     *event);

/*  Loads stored device settings (tool, cursor, ...)  */
void        devices_select_device (Gimp         *gimp,
                                   GdkDevice    *device);

/*  The device info dialog  */
GtkWidget * input_dialog_create   (Gimp         *gimp);
void        input_dialog_free     (Gimp         *gimp);

/*  The device status dialog  */
GtkWidget * device_status_create  (Gimp         *gimp);
void        device_status_free    (Gimp         *gimp);

/* Add information about one tool from rc file */
void        devices_rc_update     (Gimp         *gimp,
                                   gchar        *name,
                                   DeviceValues  values,
                                   GdkInputMode  mode,
                                   gint          num_axes,
                                   GdkAxisUse   *axes,
                                   gint          num_keys,
                                   GdkDeviceKey *keys,
                                   const gchar  *tool_name,
                                   GimpRGB      *foreground,
                                   GimpRGB      *background,
                                   const gchar  *brush_name,
                                   const gchar  *pattern_name,
                                   const gchar  *gradient_name);


#endif /* __DEVICES_H__ */
