/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcontainergridview.h
 * Copyright (C) 2001-2004 Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_CONTAINER_GRID_VIEW_H__
#define __GIMP_CONTAINER_GRID_VIEW_H__


#include "gimpcontainerbox.h"


#define GIMP_TYPE_CONTAINER_GRID_VIEW            (gimp_container_grid_view_get_type ())
#define GIMP_CONTAINER_GRID_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_CONTAINER_GRID_VIEW, GimpContainerGridView))
#define GIMP_CONTAINER_GRID_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_CONTAINER_GRID_VIEW, GimpContainerGridViewClass))
#define GIMP_IS_CONTAINER_GRID_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_CONTAINER_GRID_VIEW))
#define GIMP_IS_CONTAINER_GRID_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_CONTAINER_GRID_VIEW))
#define GIMP_CONTAINER_GRID_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_CONTAINER_GRID_VIEW, GimpContainerGridViewClass))


typedef struct _GimpContainerGridViewClass GimpContainerGridViewClass;

struct _GimpContainerGridView
{
  GimpContainerBox  parent_instance;

  GtkWidget        *wrap_box;

  gint              rows;
  gint              columns;
  gint              visible_rows;

  GimpView         *selected_item;
};

struct _GimpContainerGridViewClass
{
  GimpContainerBoxClass  parent_class;

  gboolean (* move_cursor) (GimpContainerGridView *grid_view,
                            GtkMovementStep        step,
                            gint                   count);
};


GType       gimp_container_grid_view_get_type (void) G_GNUC_CONST;

GtkWidget * gimp_container_grid_view_new      (GimpContainer *container,
                                               GimpContext   *context,
                                               gint           view_size,
                                               gint           view_border_width);


#endif  /*  __GIMP_CONTAINER_GRID_VIEW_H__  */
