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


#define GIMP_TYPE_CONTAINER_LIST_VIEW            (gimp_container_list_view_get_type ())
#define GIMP_CONTAINER_LIST_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_CONTAINER_LIST_VIEW, GimpContainerListView))
#define GIMP_CONTAINER_LIST_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_CONTAINER_LIST_VIEW, GimpContainerListViewClass))
#define GIMP_IS_CONTAINER_LIST_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_CONTAINER_LIST_VIEW))
#define GIMP_IS_CONTAINER_LIST_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_CONTAINER_LIST_VIEW))
#define GIMP_CONTAINER_LIST_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_CONTAINER_LIST_VIEW, GimpContainerListViewClass))


typedef struct _GimpContainerListViewClass GimpContainerListViewClass;

struct _GimpContainerListView
{
  GimpContainerBox   parent_instance;
};

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
