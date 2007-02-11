/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpinputdevicestore.h
 * Copyright (C) 2007  Sven Neumann <sven@gimp.org>
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __GIMP_INPUT_DEVICE_STORE_H__
#define __GIMP_INPUT_DEVICE_STORE_H__


#define GIMP_TYPE_INPUT_DEVICE_STORE    (gimp_input_device_store_get_type ())
#define GIMP_INPUT_DEVICE_STORE(obj)    (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_INPUT_DEVICE_STORE, GimpInputDeviceStore))
#define GIMP_IS_INPUT_DEVICE_STORE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_INPUT_DEVICE_STORE))

typedef struct _GimpInputDeviceStore GimpInputDeviceStore;


GType          gimp_input_device_store_get_type        (void) G_GNUC_CONST;

GtkListStore * gimp_input_device_store_new             (void);
gchar        * gimp_input_device_store_get_device_file (GimpInputDeviceStore *store,
                                                        const gchar          *udi);


#endif  /* __GIMP_INPUT_DEVICE_STORE_H__ */
