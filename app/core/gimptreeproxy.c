/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimptreeproxy.c
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

#include "config.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "gimpviewable.h"
#include "gimptreeproxy.h"


enum
{
  PROP_0,
  PROP_CONTAINER,
  PROP_FLAT
};


struct _GimpTreeProxyPrivate
{
  GimpContainer *container;
  gboolean       flat;
};


/*  local function prototypes  */

static void   gimp_tree_proxy_dispose             (GObject      *object);
static void   gimp_tree_proxy_set_property        (GObject      *object,
                                                   guint         property_id,
                                                   const GValue *value,
                                                   GParamSpec   *pspec);
static void   gimp_tree_proxy_get_property        (GObject      *object,
                                                   guint         property_id,
                                                   GValue       *value,
                                                   GParamSpec   *pspec);

static void   gimp_tree_proxy_container_add       (GimpContainer *container,
                                                   GimpObject    *object,
                                                   GimpTreeProxy *tree_proxy);
static void   gimp_tree_proxy_container_remove    (GimpContainer *container,
                                                   GimpObject    *object,
                                                   GimpTreeProxy *tree_proxy);
static void   gimp_tree_proxy_container_reorder   (GimpContainer *container,
                                                   GimpObject    *object,
                                                   gint           old_index,
                                                   gint           new_index,
                                                   GimpTreeProxy *tree_proxy);
static void   gimp_tree_proxy_container_freeze    (GimpContainer *container,
                                                   GimpTreeProxy *tree_proxy);
static void   gimp_tree_proxy_container_thaw      (GimpContainer *container,
                                                   GimpTreeProxy *tree_proxy);

static gint   gimp_tree_proxy_add_container       (GimpTreeProxy *tree_proxy,
                                                   GimpContainer *container,
                                                   gint           index);
static void   gimp_tree_proxy_remove_container    (GimpTreeProxy *tree_proxy,
                                                   GimpContainer *container);

static gint   gimp_tree_proxy_add_object          (GimpTreeProxy *tree_proxy,
                                                   GimpObject    *object,
                                                   gint           index);
static void   gimp_tree_proxy_remove_object       (GimpTreeProxy *tree_proxy,
                                                   GimpObject    *object);

static gint   gimp_tree_proxy_find_container      (GimpTreeProxy *tree_proxy,
                                                   GimpContainer *container);
static gint   gimp_tree_proxy_find_object         (GimpContainer *container,
                                                   GimpObject    *object);


G_DEFINE_TYPE_WITH_PRIVATE (GimpTreeProxy, gimp_tree_proxy, GIMP_TYPE_LIST)

#define parent_class gimp_tree_proxy_parent_class


/*  private functions  */

static void
gimp_tree_proxy_class_init (GimpTreeProxyClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose      = gimp_tree_proxy_dispose;
  object_class->set_property = gimp_tree_proxy_set_property;
  object_class->get_property = gimp_tree_proxy_get_property;

  g_object_class_install_property (object_class, PROP_CONTAINER,
                                   g_param_spec_object ("container", NULL, NULL,
                                                        GIMP_TYPE_CONTAINER,
                                                        GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_FLAT,
                                   g_param_spec_boolean ("flat", NULL, NULL,
                                                         FALSE,
                                                         GIMP_PARAM_READWRITE));
}

static void
gimp_tree_proxy_init (GimpTreeProxy *tree_proxy)
{
  tree_proxy->priv = gimp_tree_proxy_get_instance_private (tree_proxy);
}

static void
gimp_tree_proxy_dispose (GObject *object)
{
  GimpTreeProxy *tree_proxy = GIMP_TREE_PROXY (object);

  gimp_tree_proxy_set_container (tree_proxy, NULL);

  G_OBJECT_CLASS (parent_class)->dispose (object);;
}

static void
gimp_tree_proxy_set_property (GObject      *object,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  GimpTreeProxy *tree_proxy = GIMP_TREE_PROXY (object);

  switch (property_id)
    {
    case PROP_CONTAINER:
      gimp_tree_proxy_set_container (tree_proxy, g_value_get_object (value));
      break;

    case PROP_FLAT:
      gimp_tree_proxy_set_flat (tree_proxy, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_tree_proxy_get_property (GObject    *object,
                             guint       property_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  GimpTreeProxy *tree_proxy = GIMP_TREE_PROXY (object);

  switch (property_id)
    {
    case PROP_CONTAINER:
      g_value_set_object (value, tree_proxy->priv->container);
      break;

    case PROP_FLAT:
      g_value_set_boolean (value, tree_proxy->priv->flat);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_tree_proxy_container_add (GimpContainer *container,
                               GimpObject    *object,
                               GimpTreeProxy *tree_proxy)
{
  gint index;

  if (tree_proxy->priv->flat)
    {
      index = gimp_tree_proxy_find_container (tree_proxy, container) +
              gimp_tree_proxy_find_object    (container,  object);
    }
  else
    {
      index = gimp_container_get_child_index (container, object);
    }

  gimp_tree_proxy_add_object (tree_proxy, object, index);
}

static void
gimp_tree_proxy_container_remove (GimpContainer *container,
                                  GimpObject    *object,
                                  GimpTreeProxy *tree_proxy)
{
  gimp_tree_proxy_remove_object (tree_proxy, object);
}

static void
gimp_tree_proxy_container_reorder (GimpContainer *container,
                                   GimpObject    *object,
                                   gint           old_index,
                                   gint           new_index,
                                   GimpTreeProxy *tree_proxy)
{
  gint index;

  if (tree_proxy->priv->flat)
    {
      index = gimp_tree_proxy_find_container (tree_proxy, container) +
              gimp_tree_proxy_find_object    (container,  object);

      if (gimp_viewable_get_children (GIMP_VIEWABLE (object)))
        {
          gimp_container_freeze (GIMP_CONTAINER (tree_proxy));

          gimp_tree_proxy_remove_object (tree_proxy, object);
          gimp_tree_proxy_add_object    (tree_proxy, object, index);

          gimp_container_thaw (GIMP_CONTAINER (tree_proxy));

          return;
        }
    }
  else
    {
      index = new_index;
    }

  gimp_container_reorder (GIMP_CONTAINER (tree_proxy), object, index);
}

static void
gimp_tree_proxy_container_freeze (GimpContainer *container,
                                  GimpTreeProxy *tree_proxy)
{
  gimp_container_freeze (GIMP_CONTAINER (tree_proxy));
}

static void
gimp_tree_proxy_container_thaw (GimpContainer *container,
                                GimpTreeProxy *tree_proxy)
{
  gimp_container_thaw (GIMP_CONTAINER (tree_proxy));
}

typedef struct
{
  GimpTreeProxy *tree_proxy;
  gint           index;
} AddContainerData;

static void
gimp_tree_proxy_add_container_func (GimpObject       *object,
                                    AddContainerData *data)
{
  data->index = gimp_tree_proxy_add_object (data->tree_proxy,
                                            object, data->index);
}

static gint
gimp_tree_proxy_add_container (GimpTreeProxy *tree_proxy,
                               GimpContainer *container,
                               gint           index)
{
  AddContainerData data;

  g_signal_connect (container, "add",
                    G_CALLBACK (gimp_tree_proxy_container_add),
                    tree_proxy);
  g_signal_connect (container, "remove",
                    G_CALLBACK (gimp_tree_proxy_container_remove),
                    tree_proxy);
  g_signal_connect (container, "reorder",
                    G_CALLBACK (gimp_tree_proxy_container_reorder),
                    tree_proxy);
  g_signal_connect (container, "freeze",
                    G_CALLBACK (gimp_tree_proxy_container_freeze),
                    tree_proxy);
  g_signal_connect (container, "thaw",
                    G_CALLBACK (gimp_tree_proxy_container_thaw),
                    tree_proxy);

  data.tree_proxy = tree_proxy;
  data.index      = index;

  gimp_container_freeze (GIMP_CONTAINER (tree_proxy));

  gimp_container_foreach (container,
                          (GFunc) gimp_tree_proxy_add_container_func,
                          &data);

  gimp_container_thaw (GIMP_CONTAINER (tree_proxy));

  return data.index;
}

static void
gimp_tree_proxy_remove_container_func (GimpObject    *object,
                                       GimpTreeProxy *tree_proxy)
{
  gimp_tree_proxy_remove_object (tree_proxy, object);
}

static void
gimp_tree_proxy_remove_container (GimpTreeProxy *tree_proxy,
                                  GimpContainer *container)
{
  gimp_container_freeze (GIMP_CONTAINER (tree_proxy));

  gimp_container_foreach (container,
                          (GFunc) gimp_tree_proxy_remove_container_func,
                          tree_proxy);

  gimp_container_thaw (GIMP_CONTAINER (tree_proxy));

  g_signal_handlers_disconnect_by_func (
    container,
    gimp_tree_proxy_container_add,
    tree_proxy);
  g_signal_handlers_disconnect_by_func (
    container,
    gimp_tree_proxy_container_remove,
    tree_proxy);
  g_signal_handlers_disconnect_by_func (
    container,
    gimp_tree_proxy_container_reorder,
    tree_proxy);
  g_signal_handlers_disconnect_by_func (
    container,
    gimp_tree_proxy_container_freeze,
    tree_proxy);
  g_signal_handlers_disconnect_by_func (
    container,
    gimp_tree_proxy_container_thaw,
    tree_proxy);
}

static gint
gimp_tree_proxy_add_object (GimpTreeProxy *tree_proxy,
                            GimpObject    *object,
                            gint           index)
{
  if (index == gimp_container_get_n_children (GIMP_CONTAINER (tree_proxy)))
    index = -1;

  if (tree_proxy->priv->flat)
    {
      GimpContainer *children;

      children = gimp_viewable_get_children (GIMP_VIEWABLE (object));

      if (children)
        return gimp_tree_proxy_add_container (tree_proxy, children, index);
    }

  if (index >= 0)
    {
      gimp_container_insert (GIMP_CONTAINER (tree_proxy), object, index);

      return index + 1;
    }
  else
    {
      gimp_container_add (GIMP_CONTAINER (tree_proxy), object);

      return index;
    }
}

static void
gimp_tree_proxy_remove_object (GimpTreeProxy *tree_proxy,
                               GimpObject    *object)
{
  if (tree_proxy->priv->flat)
    {
      GimpContainer *children;

      children = gimp_viewable_get_children (GIMP_VIEWABLE (object));

      if (children)
        return gimp_tree_proxy_remove_container (tree_proxy, children);
    }

  gimp_container_remove (GIMP_CONTAINER (tree_proxy), object);
}

typedef struct
{
  GimpContainer *container;
  gint           index;
} FindContainerData;

static gboolean
gimp_tree_proxy_find_container_search_func (GimpObject        *object,
                                            FindContainerData *data)
{
  GimpContainer *children;

  children = gimp_viewable_get_children (GIMP_VIEWABLE (object));

  if (children)
    {
      if (children == data->container)
        return TRUE;

      return gimp_container_search (
        children,
        (GimpContainerSearchFunc) gimp_tree_proxy_find_container_search_func,
        data) != NULL;
    }

  data->index++;

  return FALSE;
}

static gint
gimp_tree_proxy_find_container (GimpTreeProxy *tree_proxy,
                                GimpContainer *container)
{
  FindContainerData data;

  if (container == tree_proxy->priv->container)
    return 0;

  data.container = container;
  data.index     = 0;

  if (gimp_container_search (
        tree_proxy->priv->container,
        (GimpContainerSearchFunc) gimp_tree_proxy_find_container_search_func,
        &data))
    {
      return data.index;
    }

  g_return_val_if_reached (0);
}

typedef struct
{
  GimpObject *object;
  gint        index;
} FindObjectData;

static gboolean
gimp_tree_proxy_find_object_search_func (GimpObject     *object,
                                         FindObjectData *data)
{
  GimpContainer *children;

  if (object == data->object)
    return TRUE;

  children = gimp_viewable_get_children (GIMP_VIEWABLE (object));

  if (children)
    {
      return gimp_container_search (
        children,
        (GimpContainerSearchFunc) gimp_tree_proxy_find_object_search_func,
        data) != NULL;
    }

  data->index++;

  return FALSE;
}

static gint
gimp_tree_proxy_find_object (GimpContainer *container,
                             GimpObject    *object)
{
  FindObjectData data;

  data.object = object;
  data.index  = 0;

  if (gimp_container_search (
        container,
        (GimpContainerSearchFunc) gimp_tree_proxy_find_object_search_func,
        &data))
    {
      return data.index;
    }

  g_return_val_if_reached (0);
}


/*  public functions  */

GimpContainer *
gimp_tree_proxy_new (GType child_type)
{
  GTypeClass *children_class;

  children_class = g_type_class_ref (child_type);

  g_return_val_if_fail (G_TYPE_CHECK_CLASS_TYPE (children_class,
                                                 GIMP_TYPE_VIEWABLE),
                        NULL);

  g_type_class_unref (children_class);

  return g_object_new (GIMP_TYPE_TREE_PROXY,
                       "child-type", child_type,
                       "policy",     GIMP_CONTAINER_POLICY_WEAK,
                       "append",     TRUE,
                       NULL);
}

GimpContainer *
gimp_tree_proxy_new_for_container (GimpContainer *container)
{
  GimpTreeProxy *tree_proxy;

  g_return_val_if_fail (GIMP_IS_CONTAINER (container), NULL);

  tree_proxy = GIMP_TREE_PROXY (
    gimp_tree_proxy_new (gimp_container_get_child_type (container)));

  gimp_tree_proxy_set_container (tree_proxy, container);

  return GIMP_CONTAINER (tree_proxy);
}

void
gimp_tree_proxy_set_container (GimpTreeProxy *tree_proxy,
                               GimpContainer *container)
{
  g_return_if_fail (GIMP_IS_TREE_PROXY (tree_proxy));
  g_return_if_fail (container == NULL || GIMP_IS_CONTAINER (container));

  if (container)
    {
      GTypeClass *children_class;

      children_class = g_type_class_ref (
        gimp_container_get_child_type (container));

      g_return_if_fail (
        G_TYPE_CHECK_CLASS_TYPE (
          children_class,
          gimp_container_get_child_type (GIMP_CONTAINER (tree_proxy))));

      g_type_class_unref (children_class);
    }

  if (container != tree_proxy->priv->container)
    {
      gimp_container_freeze (GIMP_CONTAINER (tree_proxy));

      if (tree_proxy->priv->container)
        {
          gimp_tree_proxy_remove_container (tree_proxy,
                                            tree_proxy->priv->container);
        }

      g_set_object (&tree_proxy->priv->container, container);

      if (tree_proxy->priv->container)
        {
          gimp_tree_proxy_add_container (tree_proxy,
                                         tree_proxy->priv->container,
                                         -1);
        }

      gimp_container_thaw (GIMP_CONTAINER (tree_proxy));

      g_object_notify (G_OBJECT (tree_proxy), "container");
    }
}

GimpContainer *
gimp_tree_proxy_get_container (GimpTreeProxy *tree_proxy)
{
  g_return_val_if_fail (GIMP_IS_TREE_PROXY (tree_proxy), NULL);

  return tree_proxy->priv->container;
}

void
gimp_tree_proxy_set_flat (GimpTreeProxy *tree_proxy,
                          gboolean       flat)
{
  g_return_if_fail (GIMP_IS_TREE_PROXY (tree_proxy));

  if (flat != tree_proxy->priv->flat)
    {
      gimp_container_freeze (GIMP_CONTAINER (tree_proxy));

      if (tree_proxy->priv->container)
        {
          gimp_tree_proxy_remove_container (tree_proxy,
                                            tree_proxy->priv->container);
        }

      tree_proxy->priv->flat = flat;

      if (tree_proxy->priv->container)
        {
          gimp_tree_proxy_add_container (tree_proxy,
                                         tree_proxy->priv->container,
                                         -1);
        }

      gimp_container_thaw (GIMP_CONTAINER (tree_proxy));

      g_object_notify (G_OBJECT (tree_proxy), "flat");
    }
}

gboolean
gimp_tree_proxy_get_flat (GimpTreeProxy *tree_proxy)
{
  g_return_val_if_fail (GIMP_IS_TREE_PROXY (tree_proxy), FALSE);

  return tree_proxy->priv->flat;
}
