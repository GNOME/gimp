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


#define GIMP_TYPE_DEVICE_INFO_EDITOR (gimp_device_info_editor_get_type ())
G_DECLARE_DERIVABLE_TYPE (GimpDeviceInfoEditor,
                          gimp_device_info_editor,
                          GIMP, DEVICE_INFO_EDITOR,
                          GtkBox)


struct _GimpDeviceInfoEditorClass
{
  GtkBoxClass  parent_class;
};


GtkWidget * gimp_device_info_editor_new (GimpDeviceInfo *info);
