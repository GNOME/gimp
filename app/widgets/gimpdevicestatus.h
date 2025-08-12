/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdevicestatus.h
 * Copyright (C) 2003 Michael Natterer <mitch@gimp.org>
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

#pragma once

#include "gimpeditor.h"


#define GIMP_TYPE_DEVICE_STATUS            (gimp_device_status_get_type ())
#define GIMP_DEVICE_STATUS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_DEVICE_STATUS, GimpDeviceStatus))
#define GIMP_DEVICE_STATUS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_DEVICE_STATUS, GimpDeviceStatusClass))
#define GIMP_IS_DEVICE_STATUS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_DEVICE_STATUS))
#define GIMP_IS_DEVICE_STATUS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_DEVICE_STATUS))
#define GIMP_DEVICE_STATUS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_DEVICE_STATUS, GimpDeviceStatusClass))


typedef struct _GimpDeviceStatusEntry GimpDeviceStatusEntry;
typedef struct _GimpDeviceStatusClass GimpDeviceStatusClass;

struct _GimpDeviceStatus
{
  GimpEditor      parent_instance;

  Gimp           *gimp;
  GimpDeviceInfo *current_device;

  GList          *devices;

  GtkWidget      *vbox;

  GtkWidget      *save_button;
  GtkWidget      *edit_button;
};

struct _GimpDeviceStatusClass
{
  GimpEditorClass  parent_class;
};


GType       gimp_device_status_get_type (void) G_GNUC_CONST;

GtkWidget * gimp_device_status_new      (Gimp *gimp);
