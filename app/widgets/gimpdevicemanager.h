/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmadevicemanager.h
 * Copyright (C) 2011 Michael Natterer
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

#ifndef __LIGMA_DEVICE_MANAGER_H__
#define __LIGMA_DEVICE_MANAGER_H__


#include "core/ligmalist.h"


G_BEGIN_DECLS


#define LIGMA_TYPE_DEVICE_MANAGER            (ligma_device_manager_get_type ())
#define LIGMA_DEVICE_MANAGER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_DEVICE_MANAGER, LigmaDeviceManager))
#define LIGMA_DEVICE_MANAGER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_DEVICE_MANAGER, LigmaDeviceManagerClass))
#define LIGMA_IS_DEVICE_MANAGER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_DEVICE_MANAGER))
#define LIGMA_IS_DEVICE_MANAGER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_DEVICE_MANAGER))
#define LIGMA_DEVICE_MANAGER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_DEVICE_MANAGER, LigmaDeviceManagerClass))


typedef struct _LigmaDeviceManagerPrivate LigmaDeviceManagerPrivate;
typedef struct _LigmaDeviceManagerClass   LigmaDeviceManagerClass;

struct _LigmaDeviceManager
{
  LigmaList                  parent_instance;

  LigmaDeviceManagerPrivate *priv;
};

struct _LigmaDeviceManagerClass
{
  LigmaListClass  parent_class;
};


GType               ligma_device_manager_get_type           (void) G_GNUC_CONST;

LigmaDeviceManager * ligma_device_manager_new                (Ligma *ligma);

LigmaDeviceInfo    * ligma_device_manager_get_current_device (LigmaDeviceManager *manager);
void                ligma_device_manager_set_current_device (LigmaDeviceManager *manager,
                                                            LigmaDeviceInfo    *info);

void                ligma_device_manager_reset              (LigmaDeviceManager *manager);

G_END_DECLS

#endif /* __LIGMA_DEVICE_MANAGER_H__ */
