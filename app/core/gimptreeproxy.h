/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmatreeproxy.h
 * Copyright (C) 2020 Ell
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

#ifndef __LIGMA_TREE_PROXY_H__
#define __LIGMA_TREE_PROXY_H__


#include "ligmalist.h"


#define LIGMA_TYPE_TREE_PROXY            (ligma_tree_proxy_get_type ())
#define LIGMA_TREE_PROXY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_TREE_PROXY, LigmaTreeProxy))
#define LIGMA_TREE_PROXY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_TREE_PROXY, LigmaTreeProxyClass))
#define LIGMA_IS_TREE_PROXY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_TREE_PROXY))
#define LIGMA_IS_TREE_PROXY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_TREE_PROXY))
#define LIGMA_TREE_PROXY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_TREE_PROXY, LigmaTreeProxyClass))


typedef struct _LigmaTreeProxyPrivate LigmaTreeProxyPrivate;
typedef struct _LigmaTreeProxyClass   LigmaTreeProxyClass;

struct _LigmaTreeProxy
{
  LigmaList              parent_instance;

  LigmaTreeProxyPrivate *priv;
};

struct _LigmaTreeProxyClass
{
  LigmaListClass  parent_class;
};


GType           ligma_tree_proxy_get_type (void) G_GNUC_CONST;

LigmaContainer * ligma_tree_proxy_new               (GType          children_type);
LigmaContainer * ligma_tree_proxy_new_for_container (LigmaContainer *container);

void            ligma_tree_proxy_set_container     (LigmaTreeProxy *tree_proxy,
                                                   LigmaContainer *container);
LigmaContainer * ligma_tree_proxy_get_container     (LigmaTreeProxy *tree_proxy);

void            ligma_tree_proxy_set_flat          (LigmaTreeProxy *tree_proxy,
                                                   gboolean       flat);
gboolean        ligma_tree_proxy_get_flat          (LigmaTreeProxy *tree_proxy);


#endif  /*  __LIGMA_TREE_PROXY_H__  */
