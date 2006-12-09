/* GIMP - The GNU Image Manipulation Program
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

#ifndef __GIMP_DEVICES_H__
#define __GIMP_DEVICES_H__


typedef void (* GimpDeviceChangeNotify) (Gimp *gimp);


void            gimp_devices_init          (Gimp                   *gimp,
                                            GimpDeviceChangeNotify  callback);
void            gimp_devices_exit          (Gimp                   *gimp);

void            gimp_devices_restore       (Gimp                   *gimp);
void            gimp_devices_save          (Gimp                   *gimp,
                                            gboolean                always_save);

gboolean        gimp_devices_clear         (Gimp                   *gimp,
                                            GError                **error);

GimpContainer * gimp_devices_get_list      (Gimp                   *gimp);
GdkDevice     * gimp_devices_get_current   (Gimp                   *gimp);

gboolean        gimp_devices_check_change  (Gimp                   *gimp,
                                            GdkEvent               *event);
void            gimp_devices_select_device (Gimp                   *gimp,
                                            GdkDevice              *device);


#endif /* __GIMP_DEVICES_H__ */
