/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcontainericonview.h
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

#include "gimpcontainerbox.h"


#define GIMP_TYPE_CONTAINER_ICON_VIEW            (gimp_container_icon_view_get_type ())
#define GIMP_CONTAINER_ICON_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_CONTAINER_ICON_VIEW, GimpContainerIconView))
#define GIMP_CONTAINER_ICON_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_CONTAINER_ICON_VIEW, GimpContainerIconViewClass))
#define GIMP_IS_CONTAINER_ICON_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_CONTAINER_ICON_VIEW))
#define GIMP_IS_CONTAINER_ICON_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_CONTAINER_ICON_VIEW))
#define GIMP_CONTAINER_ICON_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_CONTAINER_ICON_VIEW, GimpContainerIconViewClass))


typedef struct _GimpContainerIconViewClass   GimpContainerIconViewClass;
typedef struct _GimpContainerIconViewPrivate GimpContainerIconViewPrivate;

struct _GimpContainerIconView
{
  GimpContainerBox              parent_instance;

  GtkTreeModel                 *model;
  gint                          n_model_columns;
  GType                         model_columns[16];

  GtkIconView                  *view;

  GtkCellRenderer              *renderer_cell;

  GimpContainerIconViewPrivate *priv;
};

struct _GimpContainerIconViewClass
{
  GimpContainerBoxClass  parent_class;
};


GType       gimp_container_icon_view_get_type (void) G_GNUC_CONST;

GtkWidget * gimp_container_icon_view_new      (GimpContainer *container,
                                               GimpContext   *context,
                                               gint           view_size,
                                               gint           view_border_width);
