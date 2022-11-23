/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmacontainericonview.h
 * Copyright (C) 2010 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_CONTAINER_ICON_VIEW_H__
#define __LIGMA_CONTAINER_ICON_VIEW_H__


#include "ligmacontainerbox.h"


#define LIGMA_TYPE_CONTAINER_ICON_VIEW            (ligma_container_icon_view_get_type ())
#define LIGMA_CONTAINER_ICON_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_CONTAINER_ICON_VIEW, LigmaContainerIconView))
#define LIGMA_CONTAINER_ICON_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_CONTAINER_ICON_VIEW, LigmaContainerIconViewClass))
#define LIGMA_IS_CONTAINER_ICON_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_CONTAINER_ICON_VIEW))
#define LIGMA_IS_CONTAINER_ICON_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_CONTAINER_ICON_VIEW))
#define LIGMA_CONTAINER_ICON_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_CONTAINER_ICON_VIEW, LigmaContainerIconViewClass))


typedef struct _LigmaContainerIconViewClass   LigmaContainerIconViewClass;
typedef struct _LigmaContainerIconViewPrivate LigmaContainerIconViewPrivate;

struct _LigmaContainerIconView
{
  LigmaContainerBox              parent_instance;

  GtkTreeModel                 *model;
  gint                          n_model_columns;
  GType                         model_columns[16];

  GtkIconView                  *view;

  GtkCellRenderer              *renderer_cell;

  Ligma                         *dnd_ligma; /* eek */

  LigmaContainerIconViewPrivate *priv;
};

struct _LigmaContainerIconViewClass
{
  LigmaContainerBoxClass  parent_class;
};


GType       ligma_container_icon_view_get_type (void) G_GNUC_CONST;

GtkWidget * ligma_container_icon_view_new      (LigmaContainer *container,
                                               LigmaContext   *context,
                                               gint           view_size,
                                               gint           view_border_width);


#endif  /*  __LIGMA_CONTAINER_ICON_VIEW_H__  */
