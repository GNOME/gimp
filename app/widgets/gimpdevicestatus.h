/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmadevicestatus.h
 * Copyright (C) 2003 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_DEVICE_STATUS_H__
#define __LIGMA_DEVICE_STATUS_H__


#include "ligmaeditor.h"


#define LIGMA_TYPE_DEVICE_STATUS            (ligma_device_status_get_type ())
#define LIGMA_DEVICE_STATUS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_DEVICE_STATUS, LigmaDeviceStatus))
#define LIGMA_DEVICE_STATUS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_DEVICE_STATUS, LigmaDeviceStatusClass))
#define LIGMA_IS_DEVICE_STATUS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_DEVICE_STATUS))
#define LIGMA_IS_DEVICE_STATUS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_DEVICE_STATUS))
#define LIGMA_DEVICE_STATUS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_DEVICE_STATUS, LigmaDeviceStatusClass))


typedef struct _LigmaDeviceStatusEntry LigmaDeviceStatusEntry;
typedef struct _LigmaDeviceStatusClass LigmaDeviceStatusClass;

struct _LigmaDeviceStatus
{
  LigmaEditor      parent_instance;

  Ligma           *ligma;
  LigmaDeviceInfo *current_device;

  GList          *devices;

  GtkWidget      *vbox;

  GtkWidget      *save_button;
  GtkWidget      *edit_button;
};

struct _LigmaDeviceStatusClass
{
  LigmaEditorClass  parent_class;
};


GType       ligma_device_status_get_type (void) G_GNUC_CONST;

GtkWidget * ligma_device_status_new      (Ligma *ligma);


#endif  /*  __LIGMA_DEVICE_STATUS_H__  */
