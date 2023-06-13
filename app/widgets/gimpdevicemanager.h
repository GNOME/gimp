/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdevicemanager.h
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

#ifndef __GIMP_DEVICE_MANAGER_H__
#define __GIMP_DEVICE_MANAGER_H__


#include "core/gimplist.h"


G_BEGIN_DECLS


#define GIMP_TYPE_DEVICE_MANAGER            (gimp_device_manager_get_type ())
#define GIMP_DEVICE_MANAGER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_DEVICE_MANAGER, GimpDeviceManager))
#define GIMP_DEVICE_MANAGER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_DEVICE_MANAGER, GimpDeviceManagerClass))
#define GIMP_IS_DEVICE_MANAGER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_DEVICE_MANAGER))
#define GIMP_IS_DEVICE_MANAGER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_DEVICE_MANAGER))
#define GIMP_DEVICE_MANAGER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_DEVICE_MANAGER, GimpDeviceManagerClass))


typedef struct _GimpDeviceManagerPrivate GimpDeviceManagerPrivate;
typedef struct _GimpDeviceManagerClass   GimpDeviceManagerClass;

struct _GimpDeviceManager
{
  GimpList                  parent_instance;

  GimpDeviceManagerPrivate *priv;
};

struct _GimpDeviceManagerClass
{
  GimpListClass  parent_class;
};


GType               gimp_device_manager_get_type           (void) G_GNUC_CONST;

GimpDeviceManager * gimp_device_manager_new                (Gimp *gimp);

GimpDeviceInfo    * gimp_device_manager_get_current_device (GimpDeviceManager *manager);
void                gimp_device_manager_set_current_device (GimpDeviceManager *manager,
                                                            GimpDeviceInfo    *info);

void                gimp_device_manager_reset              (GimpDeviceManager *manager);

void                gimp_device_manager_reconfigure_pads   (GimpDeviceManager *manager);


G_END_DECLS

#endif /* __GIMP_DEVICE_MANAGER_H__ */
