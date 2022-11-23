/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmavectorstreeview.h
 * Copyright (C) 2001-2003 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_VECTORS_TREE_VIEW_H__
#define __LIGMA_VECTORS_TREE_VIEW_H__


#include "ligmaitemtreeview.h"


#define LIGMA_TYPE_VECTORS_TREE_VIEW            (ligma_vectors_tree_view_get_type ())
#define LIGMA_VECTORS_TREE_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_VECTORS_TREE_VIEW, LigmaVectorsTreeView))
#define LIGMA_VECTORS_TREE_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_VECTORS_TREE_VIEW, LigmaVectorsTreeViewClass))
#define LIGMA_IS_VECTORS_TREE_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_VECTORS_TREE_VIEW))
#define LIGMA_IS_VECTORS_TREE_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_VECTORS_TREE_VIEW))
#define LIGMA_VECTORS_TREE_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_VECTORS_TREE_VIEW, LigmaVectorsTreeViewClass))


typedef struct _LigmaVectorsTreeViewClass  LigmaVectorsTreeViewClass;

struct _LigmaVectorsTreeView
{
  LigmaItemTreeView  parent_instance;

  GtkWidget        *toselection_button;
  GtkWidget        *tovectors_button;
  GtkWidget        *stroke_button;
};

struct _LigmaVectorsTreeViewClass
{
  LigmaItemTreeViewClass  parent_class;
};


GType   ligma_vectors_tree_view_get_type (void) G_GNUC_CONST;


#endif  /*  __LIGMA_VECTORS_TREE_VIEW_H__  */
