/* LIGMA - The GNU Image Manipulation Program
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __LIGMA_DEVICES_H__
#define __LIGMA_DEVICES_H__


void                ligma_devices_init           (Ligma       *ligma);
void                ligma_devices_exit           (Ligma       *ligma);

void                ligma_devices_restore        (Ligma       *ligma);
void                ligma_devices_save           (Ligma       *ligma,
                                                 gboolean    always_save);

gboolean            ligma_devices_clear          (Ligma       *ligma,
                                                 GError    **error);

LigmaDeviceManager * ligma_devices_get_manager    (Ligma       *ligma);

GdkDevice         * ligma_devices_get_from_event (Ligma            *ligma,
                                                 const GdkEvent  *event,
                                                 GdkDevice      **grab_device);

void                ligma_devices_add_widget     (Ligma       *ligma,
                                                 GtkWidget  *widget);

gboolean            ligma_devices_check_callback (GtkWidget  *widget,
                                                 GdkEvent   *event,
                                                 Ligma       *ligma);
gboolean            ligma_devices_check_change   (Ligma       *ligma,
                                                 GdkDevice  *device);


#endif /* __LIGMA_DEVICES_H__ */
