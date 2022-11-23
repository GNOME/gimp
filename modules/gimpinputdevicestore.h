/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmainputdevicestore.h
 * Copyright (C) 2007  Sven Neumann <sven@ligma.org>
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

#ifndef __LIGMA_INPUT_DEVICE_STORE_H__
#define __LIGMA_INPUT_DEVICE_STORE_H__


#define LIGMA_TYPE_INPUT_DEVICE_STORE    (ligma_input_device_store_get_type ())
#define LIGMA_INPUT_DEVICE_STORE(obj)    (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_INPUT_DEVICE_STORE, LigmaInputDeviceStore))
#define LIGMA_IS_INPUT_DEVICE_STORE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_INPUT_DEVICE_STORE))

typedef struct _LigmaInputDeviceStore LigmaInputDeviceStore;


void                   ligma_input_device_store_register_types  (GTypeModule           *module);

GType                  ligma_input_device_store_get_type        (void);

LigmaInputDeviceStore * ligma_input_device_store_new             (void);
gchar                * ligma_input_device_store_get_device_file (LigmaInputDeviceStore  *store,
                                                                const gchar           *udi);
GError               * ligma_input_device_store_get_error       (LigmaInputDeviceStore  *store);


#endif  /* __LIGMA_INPUT_DEVICE_STORE_H__ */
