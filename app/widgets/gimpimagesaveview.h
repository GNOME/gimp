/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpimagesaveview.h
 * Copyright (C) 2026 Michael Natterer <mitch@gimp.org>
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

#include "gimpcontainerlistview.h"


#define GIMP_TYPE_IMAGE_SAVE_VIEW (gimp_image_save_view_get_type ())
G_DECLARE_DERIVABLE_TYPE (GimpImageSaveView,
                          gimp_image_save_view,
                          GIMP, IMAGE_SAVE_VIEW,
                          GimpContainerListView)


struct _GimpImageSaveViewClass
{
  GimpContainerListViewClass  parent_class;
};


GtkWidget * gimp_image_save_view_new (GimpContainer *container,
                                      GimpContext   *context,
                                      gint           view_size,
                                      gint           view_border_width);
