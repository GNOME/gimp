/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmalayertreeview.h
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

#ifndef __LIGMA_LAYER_TREE_VIEW_H__
#define __LIGMA_LAYER_TREE_VIEW_H__


#include "ligmadrawabletreeview.h"


#define LIGMA_TYPE_LAYER_TREE_VIEW            (ligma_layer_tree_view_get_type ())
#define LIGMA_LAYER_TREE_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_LAYER_TREE_VIEW, LigmaLayerTreeView))
#define LIGMA_LAYER_TREE_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_LAYER_TREE_VIEW, LigmaLayerTreeViewClass))
#define LIGMA_IS_LAYER_TREE_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_LAYER_TREE_VIEW))
#define LIGMA_IS_LAYER_TREE_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_LAYER_TREE_VIEW))
#define LIGMA_LAYER_TREE_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_LAYER_TREE_VIEW, LigmaLayerTreeViewClass))


typedef struct _LigmaLayerTreeViewClass   LigmaLayerTreeViewClass;
typedef struct _LigmaLayerTreeViewPrivate LigmaLayerTreeViewPrivate;

struct _LigmaLayerTreeView
{
  LigmaDrawableTreeView      parent_instance;

  LigmaLayerTreeViewPrivate *priv;
};

struct _LigmaLayerTreeViewClass
{
  LigmaDrawableTreeViewClass  parent_class;
};


GType   ligma_layer_tree_view_get_type (void) G_GNUC_CONST;


#endif  /*  __LIGMA_LAYER_TREE_VIEW_H__  */
