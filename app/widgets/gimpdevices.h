/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#ifndef __GIMP_DEVICES_H__
#define __GIMP_DEVICES_H__


void                gimp_devices_init           (Gimp       *gimp);
void                gimp_devices_exit           (Gimp       *gimp);

void                gimp_devices_restore        (Gimp       *gimp);
void                gimp_devices_save           (Gimp       *gimp,
                                                 gboolean    always_save);

gboolean            gimp_devices_clear          (Gimp       *gimp,
                                                 GError    **error);

GimpDeviceManager * gimp_devices_get_manager    (Gimp       *gimp);

GdkDevice         * gimp_devices_get_from_event (Gimp            *gimp,
                                                 const GdkEvent  *event,
                                                 GdkDevice      **grab_device);

void                gimp_devices_add_widget     (Gimp       *gimp,
                                                 GtkWidget  *widget);

gboolean            gimp_devices_check_callback (GtkWidget  *widget,
                                                 GdkEvent   *event,
                                                 Gimp       *gimp);
gboolean            gimp_devices_check_change   (Gimp       *gimp,
                                                 GdkDevice  *device);


#endif /* __GIMP_DEVICES_H__ */
