/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimprowdeviceinfo.c
 * Copyright (C) 2025 Michael Natterer <mitch@gimp.org>
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

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "widgets-types.h"

#include "core/gimpcontext.h"
#include "core/gimpviewable.h"

#include "gimpdeviceinfo.h"
#include "gimprowdeviceinfo.h"


struct _GimpRowDeviceInfo
{
  GimpRow  parent_instance;
};


static void   gimp_row_device_info_name_changed (GimpRow *row);


G_DEFINE_TYPE (GimpRowDeviceInfo,
               gimp_row_device_info,
               GIMP_TYPE_ROW)

#define parent_class gimp_row_device_info_parent_class


static void
gimp_row_device_info_class_init (GimpRowDeviceInfoClass *klass)
{
  GimpRowClass *row_class = GIMP_ROW_CLASS (klass);

  row_class->name_changed = gimp_row_device_info_name_changed;
}

static void
gimp_row_device_info_init (GimpRowDeviceInfo *row)
{
}

static void
gimp_row_device_info_name_changed (GimpRow *row)
{
  GimpViewable *viewable;

  GIMP_ROW_CLASS (parent_class)->name_changed (row);

  viewable = gimp_row_get_viewable (row);

  if (viewable)
    {
      GimpDeviceInfo *info = GIMP_DEVICE_INFO (viewable);

      gtk_widget_set_sensitive (_gimp_row_get_label (row),
                                gimp_device_info_get_device (info, NULL) != NULL);
    }
}
