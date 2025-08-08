/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcontainerlistview.h
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

#pragma once

#include "gimpcontainerbox.h"


#define GIMP_TYPE_CONTAINER_LIST_VIEW (gimp_container_list_view_get_type ())
G_DECLARE_DERIVABLE_TYPE (GimpContainerListView,
                          gimp_container_list_view,
                          GIMP, CONTAINER_LIST_VIEW,
                          GimpContainerBox)


struct _GimpContainerListViewClass
{
  GimpContainerBoxClass  parent_class;
};


GType       gimp_container_list_view_get_type (void) G_GNUC_CONST;

GtkWidget * gimp_container_list_view_new      (GimpContainer *container,
                                               GimpContext   *context,
                                               gint           view_size,
                                               gint           view_border_width);

void        gimp_container_list_view_set_search_entry
                                              (GimpContainerListView *list_view,
                                               GtkSearchEntry        *entry);
