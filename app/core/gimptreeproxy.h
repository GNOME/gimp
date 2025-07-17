/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimptreeproxy.h
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

#pragma once

#include "gimplist.h"


#define GIMP_TYPE_TREE_PROXY            (gimp_tree_proxy_get_type ())
#define GIMP_TREE_PROXY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_TREE_PROXY, GimpTreeProxy))
#define GIMP_TREE_PROXY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_TREE_PROXY, GimpTreeProxyClass))
#define GIMP_IS_TREE_PROXY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_TREE_PROXY))
#define GIMP_IS_TREE_PROXY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_TREE_PROXY))
#define GIMP_TREE_PROXY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_TREE_PROXY, GimpTreeProxyClass))


typedef struct _GimpTreeProxyPrivate GimpTreeProxyPrivate;
typedef struct _GimpTreeProxyClass   GimpTreeProxyClass;

struct _GimpTreeProxy
{
  GimpList              parent_instance;

  GimpTreeProxyPrivate *priv;
};

struct _GimpTreeProxyClass
{
  GimpListClass  parent_class;
};


GType           gimp_tree_proxy_get_type (void) G_GNUC_CONST;

GimpContainer * gimp_tree_proxy_new               (GType          child_type);
GimpContainer * gimp_tree_proxy_new_for_container (GimpContainer *container);

void            gimp_tree_proxy_set_container     (GimpTreeProxy *tree_proxy,
                                                   GimpContainer *container);
GimpContainer * gimp_tree_proxy_get_container     (GimpTreeProxy *tree_proxy);

void            gimp_tree_proxy_set_flat          (GimpTreeProxy *tree_proxy,
                                                   gboolean       flat);
gboolean        gimp_tree_proxy_get_flat          (GimpTreeProxy *tree_proxy);
