/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcontainergridview.h
 * Copyright (C) 2001 Michael Natterer <mitch@gimp.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __GIMP_CONTAINER_GRID_VIEW_H__
#define __GIMP_CONTAINER_GRID_VIEW_H__


#include "gimpcontainerview.h"


#define GIMP_TYPE_CONTAINER_GRID_VIEW            (gimp_container_grid_view_get_type ())
#define GIMP_CONTAINER_GRID_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_CONTAINER_GRID_VIEW, GimpContainerGridView))
#define GIMP_CONTAINER_GRID_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_CONTAINER_GRID_VIEW, GimpContainerGridViewClass))
#define GIMP_IS_CONTAINER_GRID_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_CONTAINER_GRID_VIEW))
#define GIMP_IS_CONTAINER_GRID_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_CONTAINER_GRID_VIEW))
#define GIMP_CONTAINER_GRID_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_CONTAINER_GRID_VIEW, GimpContainerGridViewClass))


typedef struct _GimpContainerGridViewClass  GimpContainerGridViewClass;

struct _GimpContainerGridView
{
  GimpContainerView  parent_instance;

  GtkWidget         *name_label;
  GtkWidget         *wrap_box;

  gint               rows;
  gint               columns;
};

struct _GimpContainerGridViewClass
{
  GimpContainerViewClass  parent_class;
};


GType       gimp_container_grid_view_get_type (void) G_GNUC_CONST;

GtkWidget * gimp_container_grid_view_new      (GimpContainer *container,
					       GimpContext   *context,
					       gint           preview_size,
                                               gboolean       reorderable,
					       gint           min_items_x,
					       gint           min_items_y);


#endif  /*  __GIMP_CONTAINER_GRID_VIEW_H__  */
