/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdeviceinfoeditor.h
 * Copyright (C) 2010 Michael Natterer <mitch@gimp.org>
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


#define GIMP_TYPE_DEVICE_INFO_EDITOR            (gimp_device_info_editor_get_type ())
#define GIMP_DEVICE_INFO_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_DEVICE_INFO_EDITOR, GimpDeviceInfoEditor))
#define GIMP_DEVICE_INFO_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_DEVICE_INFO_EDITOR, GimpDeviceInfoEditorClass))
#define GIMP_IS_DEVICE_INFO_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_DEVICE_INFO_EDITOR))
#define GIMP_IS_DEVICE_INFO_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_DEVICE_INFO_EDITOR))
#define GIMP_DEVICE_INFO_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_DEVICE_INFO_EDITOR, GimpDeviceInfoEditorClass))


typedef struct _GimpDeviceInfoEditorClass GimpDeviceInfoEditorClass;

struct _GimpDeviceInfoEditor
{
  GtkBox  parent_instance;
};

struct _GimpDeviceInfoEditorClass
{
  GtkBoxClass  parent_class;
};


GType       gimp_device_info_editor_get_type (void) G_GNUC_CONST;

GtkWidget * gimp_device_info_editor_new      (GimpDeviceInfo *info);
