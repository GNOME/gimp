/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmatreeproxy.c
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

#include "libligmabase/ligmabase.h"

#include "core-types.h"

#include "ligmaviewable.h"
#include "ligmatreeproxy.h"


enum
{
  PROP_0,
  PROP_CONTAINER,
  PROP_FLAT
};


struct _LigmaTreeProxyPrivate
{
  LigmaContainer *container;
  gboolean       flat;
};


/*  local function prototypes  */

static void   ligma_tree_proxy_dispose             (GObject      *object);
static void   ligma_tree_proxy_set_property        (GObject      *object,
                                                   guint         property_id,
                                                   const GValue *value,
                                                   GParamSpec   *pspec);
static void   ligma_tree_proxy_get_property        (GObject      *object,
                                                   guint         property_id,
                                                   GValue       *value,
                                                   GParamSpec   *pspec);

static void   ligma_tree_proxy_container_add       (LigmaContainer *container,
                                                   LigmaObject    *object,
                                                   LigmaTreeProxy *tree_proxy);
static void   ligma_tree_proxy_container_remove    (LigmaContainer *container,
                                                   LigmaObject    *object,
                                                   LigmaTreeProxy *tree_proxy);
static void   ligma_tree_proxy_container_reorder   (LigmaContainer *container,
                                                   LigmaObject    *object,
                                                   gint           new_index,
                                                   LigmaTreeProxy *tree_proxy);
static void   ligma_tree_proxy_container_freeze    (LigmaContainer *container,
                                                   LigmaTreeProxy *tree_proxy);
static void   ligma_tree_proxy_container_thaw      (LigmaContainer *container,
                                                   LigmaTreeProxy *tree_proxy);

static gint   ligma_tree_proxy_add_container       (LigmaTreeProxy *tree_proxy,
                                                   LigmaContainer *container,
                                                   gint           index);
static void   ligma_tree_proxy_remove_container    (LigmaTreeProxy *tree_proxy,
                                                   LigmaContainer *container);

static gint   ligma_tree_proxy_add_object          (LigmaTreeProxy *tree_proxy,
                                                   LigmaObject    *object,
                                                   gint           index);
static void   ligma_tree_proxy_remove_object       (LigmaTreeProxy *tree_proxy,
                                                   LigmaObject    *object);

static gint   ligma_tree_proxy_find_container      (LigmaTreeProxy *tree_proxy,
                                                   LigmaContainer *container);
static gint   ligma_tree_proxy_find_object         (LigmaContainer *container,
                                                   LigmaObject    *object);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaTreeProxy, ligma_tree_proxy, LIGMA_TYPE_LIST)

#define parent_class ligma_tree_proxy_parent_class


/*  private functions  */

static void
ligma_tree_proxy_class_init (LigmaTreeProxyClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose      = ligma_tree_proxy_dispose;
  object_class->set_property = ligma_tree_proxy_set_property;
  object_class->get_property = ligma_tree_proxy_get_property;

  g_object_class_install_property (object_class, PROP_CONTAINER,
                                   g_param_spec_object ("container", NULL, NULL,
                                                        LIGMA_TYPE_CONTAINER,
                                                        LIGMA_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_FLAT,
                                   g_param_spec_boolean ("flat", NULL, NULL,
                                                         FALSE,
                                                         LIGMA_PARAM_READWRITE));
}

static void
ligma_tree_proxy_init (LigmaTreeProxy *tree_proxy)
{
  tree_proxy->priv = ligma_tree_proxy_get_instance_private (tree_proxy);
}

static void
ligma_tree_proxy_dispose (GObject *object)
{
  LigmaTreeProxy *tree_proxy = LIGMA_TREE_PROXY (object);

  ligma_tree_proxy_set_container (tree_proxy, NULL);

  G_OBJECT_CLASS (parent_class)->dispose (object);;
}

static void
ligma_tree_proxy_set_property (GObject      *object,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  LigmaTreeProxy *tree_proxy = LIGMA_TREE_PROXY (object);

  switch (property_id)
    {
    case PROP_CONTAINER:
      ligma_tree_proxy_set_container (tree_proxy, g_value_get_object (value));
      break;

    case PROP_FLAT:
      ligma_tree_proxy_set_flat (tree_proxy, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_tree_proxy_get_property (GObject    *object,
                             guint       property_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  LigmaTreeProxy *tree_proxy = LIGMA_TREE_PROXY (object);

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
ligma_tree_proxy_container_add (LigmaContainer *container,
                               LigmaObject    *object,
                               LigmaTreeProxy *tree_proxy)
{
  gint index;

  if (tree_proxy->priv->flat)
    {
      index = ligma_tree_proxy_find_container (tree_proxy, container) +
              ligma_tree_proxy_find_object    (container,  object);
    }
  else
    {
      index = ligma_container_get_child_index (container, object);
    }

  ligma_tree_proxy_add_object (tree_proxy, object, index);
}

static void
ligma_tree_proxy_container_remove (LigmaContainer *container,
                                  LigmaObject    *object,
                                  LigmaTreeProxy *tree_proxy)
{
  ligma_tree_proxy_remove_object (tree_proxy, object);
}

static void
ligma_tree_proxy_container_reorder (LigmaContainer *container,
                                   LigmaObject    *object,
                                   gint           new_index,
                                   LigmaTreeProxy *tree_proxy)
{
  gint index;

  if (tree_proxy->priv->flat)
    {
      index = ligma_tree_proxy_find_container (tree_proxy, container) +
              ligma_tree_proxy_find_object    (container,  object);

      if (ligma_viewable_get_children (LIGMA_VIEWABLE (object)))
        {
          ligma_container_freeze (LIGMA_CONTAINER (tree_proxy));

          ligma_tree_proxy_remove_object (tree_proxy, object);
          ligma_tree_proxy_add_object    (tree_proxy, object, index);

          ligma_container_thaw (LIGMA_CONTAINER (tree_proxy));

          return;
        }
    }
  else
    {
      index = new_index;
    }

  ligma_container_reorder (LIGMA_CONTAINER (tree_proxy), object, index);
}

static void
ligma_tree_proxy_container_freeze (LigmaContainer *container,
                                  LigmaTreeProxy *tree_proxy)
{
  ligma_container_freeze (LIGMA_CONTAINER (tree_proxy));
}

static void
ligma_tree_proxy_container_thaw (LigmaContainer *container,
                                LigmaTreeProxy *tree_proxy)
{
  ligma_container_thaw (LIGMA_CONTAINER (tree_proxy));
}

typedef struct
{
  LigmaTreeProxy *tree_proxy;
  gint           index;
} AddContainerData;

static void
ligma_tree_proxy_add_container_func (LigmaObject       *object,
                                    AddContainerData *data)
{
  data->index = ligma_tree_proxy_add_object (data->tree_proxy,
                                            object, data->index);
}

static gint
ligma_tree_proxy_add_container (LigmaTreeProxy *tree_proxy,
                               LigmaContainer *container,
                               gint           index)
{
  AddContainerData data;

  g_signal_connect (container, "add",
                    G_CALLBACK (ligma_tree_proxy_container_add),
                    tree_proxy);
  g_signal_connect (container, "remove",
                    G_CALLBACK (ligma_tree_proxy_container_remove),
                    tree_proxy);
  g_signal_connect (container, "reorder",
                    G_CALLBACK (ligma_tree_proxy_container_reorder),
                    tree_proxy);
  g_signal_connect (container, "freeze",
                    G_CALLBACK (ligma_tree_proxy_container_freeze),
                    tree_proxy);
  g_signal_connect (container, "thaw",
                    G_CALLBACK (ligma_tree_proxy_container_thaw),
                    tree_proxy);

  data.tree_proxy = tree_proxy;
  data.index      = index;

  ligma_container_freeze (LIGMA_CONTAINER (tree_proxy));

  ligma_container_foreach (container,
                          (GFunc) ligma_tree_proxy_add_container_func,
                          &data);

  ligma_container_thaw (LIGMA_CONTAINER (tree_proxy));

  return data.index;
}

static void
ligma_tree_proxy_remove_container_func (LigmaObject    *object,
                                       LigmaTreeProxy *tree_proxy)
{
  ligma_tree_proxy_remove_object (tree_proxy, object);
}

static void
ligma_tree_proxy_remove_container (LigmaTreeProxy *tree_proxy,
                                  LigmaContainer *container)
{
  ligma_container_freeze (LIGMA_CONTAINER (tree_proxy));

  ligma_container_foreach (container,
                          (GFunc) ligma_tree_proxy_remove_container_func,
                          tree_proxy);

  ligma_container_thaw (LIGMA_CONTAINER (tree_proxy));

  g_signal_handlers_disconnect_by_func (
    container,
    ligma_tree_proxy_container_add,
    tree_proxy);
  g_signal_handlers_disconnect_by_func (
    container,
    ligma_tree_proxy_container_remove,
    tree_proxy);
  g_signal_handlers_disconnect_by_func (
    container,
    ligma_tree_proxy_container_reorder,
    tree_proxy);
  g_signal_handlers_disconnect_by_func (
    container,
    ligma_tree_proxy_container_freeze,
    tree_proxy);
  g_signal_handlers_disconnect_by_func (
    container,
    ligma_tree_proxy_container_thaw,
    tree_proxy);
}

static gint
ligma_tree_proxy_add_object (LigmaTreeProxy *tree_proxy,
                            LigmaObject    *object,
                            gint           index)
{
  if (index == ligma_container_get_n_children (LIGMA_CONTAINER (tree_proxy)))
    index = -1;

  if (tree_proxy->priv->flat)
    {
      LigmaContainer *children;

      children = ligma_viewable_get_children (LIGMA_VIEWABLE (object));

      if (children)
        return ligma_tree_proxy_add_container (tree_proxy, children, index);
    }

  if (index >= 0)
    {
      ligma_container_insert (LIGMA_CONTAINER (tree_proxy), object, index);

      return index + 1;
    }
  else
    {
      ligma_container_add (LIGMA_CONTAINER (tree_proxy), object);

      return index;
    }
}

static void
ligma_tree_proxy_remove_object (LigmaTreeProxy *tree_proxy,
                               LigmaObject    *object)
{
  if (tree_proxy->priv->flat)
    {
      LigmaContainer *children;

      children = ligma_viewable_get_children (LIGMA_VIEWABLE (object));

      if (children)
        return ligma_tree_proxy_remove_container (tree_proxy, children);
    }

  ligma_container_remove (LIGMA_CONTAINER (tree_proxy), object);
}

typedef struct
{
  LigmaContainer *container;
  gint           index;
} FindContainerData;

static gboolean
ligma_tree_proxy_find_container_search_func (LigmaObject        *object,
                                            FindContainerData *data)
{
  LigmaContainer *children;

  children = ligma_viewable_get_children (LIGMA_VIEWABLE (object));

  if (children)
    {
      if (children == data->container)
        return TRUE;

      return ligma_container_search (
        children,
        (LigmaContainerSearchFunc) ligma_tree_proxy_find_container_search_func,
        data) != NULL;
    }

  data->index++;

  return FALSE;
}

static gint
ligma_tree_proxy_find_container (LigmaTreeProxy *tree_proxy,
                                LigmaContainer *container)
{
  FindContainerData data;

  if (container == tree_proxy->priv->container)
    return 0;

  data.container = container;
  data.index     = 0;

  if (ligma_container_search (
        tree_proxy->priv->container,
        (LigmaContainerSearchFunc) ligma_tree_proxy_find_container_search_func,
        &data))
    {
      return data.index;
    }

  g_return_val_if_reached (0);
}

typedef struct
{
  LigmaObject *object;
  gint        index;
} FindObjectData;

static gboolean
ligma_tree_proxy_find_object_search_func (LigmaObject     *object,
                                         FindObjectData *data)
{
  LigmaContainer *children;

  if (object == data->object)
    return TRUE;

  children = ligma_viewable_get_children (LIGMA_VIEWABLE (object));

  if (children)
    {
      return ligma_container_search (
        children,
        (LigmaContainerSearchFunc) ligma_tree_proxy_find_object_search_func,
        data) != NULL;
    }

  data->index++;

  return FALSE;
}

static gint
ligma_tree_proxy_find_object (LigmaContainer *container,
                             LigmaObject    *object)
{
  FindObjectData data;

  data.object = object;
  data.index  = 0;

  if (ligma_container_search (
        container,
        (LigmaContainerSearchFunc) ligma_tree_proxy_find_object_search_func,
        &data))
    {
      return data.index;
    }

  g_return_val_if_reached (0);
}


/*  public functions  */

LigmaContainer *
ligma_tree_proxy_new (GType children_type)
{
  GTypeClass *children_class;

  children_class = g_type_class_ref (children_type);

  g_return_val_if_fail (G_TYPE_CHECK_CLASS_TYPE (children_class,
                                                 LIGMA_TYPE_VIEWABLE),
                        NULL);

  g_type_class_unref (children_class);

  return g_object_new (LIGMA_TYPE_TREE_PROXY,
                       "children-type", children_type,
                       "policy",        LIGMA_CONTAINER_POLICY_WEAK,
                       "append",        TRUE,
                       NULL);
}

LigmaContainer *
ligma_tree_proxy_new_for_container (LigmaContainer *container)
{
  LigmaTreeProxy *tree_proxy;

  g_return_val_if_fail (LIGMA_IS_CONTAINER (container), NULL);

  tree_proxy = LIGMA_TREE_PROXY (
    ligma_tree_proxy_new (ligma_container_get_children_type (container)));

  ligma_tree_proxy_set_container (tree_proxy, container);

  return LIGMA_CONTAINER (tree_proxy);
}

void
ligma_tree_proxy_set_container (LigmaTreeProxy *tree_proxy,
                               LigmaContainer *container)
{
  g_return_if_fail (LIGMA_IS_TREE_PROXY (tree_proxy));
  g_return_if_fail (container == NULL || LIGMA_IS_CONTAINER (container));

  if (container)
    {
      GTypeClass *children_class;

      children_class = g_type_class_ref (
        ligma_container_get_children_type (container));

      g_return_if_fail (
        G_TYPE_CHECK_CLASS_TYPE (
          children_class,
          ligma_container_get_children_type (LIGMA_CONTAINER (tree_proxy))));

      g_type_class_unref (children_class);
    }

  if (container != tree_proxy->priv->container)
    {
      ligma_container_freeze (LIGMA_CONTAINER (tree_proxy));

      if (tree_proxy->priv->container)
        {
          ligma_tree_proxy_remove_container (tree_proxy,
                                            tree_proxy->priv->container);
        }

      g_set_object (&tree_proxy->priv->container, container);

      if (tree_proxy->priv->container)
        {
          ligma_tree_proxy_add_container (tree_proxy,
                                         tree_proxy->priv->container,
                                         -1);
        }

      ligma_container_thaw (LIGMA_CONTAINER (tree_proxy));

      g_object_notify (G_OBJECT (tree_proxy), "container");
    }
}

LigmaContainer *
ligma_tree_proxy_get_container (LigmaTreeProxy *tree_proxy)
{
  g_return_val_if_fail (LIGMA_IS_TREE_PROXY (tree_proxy), NULL);

  return tree_proxy->priv->container;
}

void
ligma_tree_proxy_set_flat (LigmaTreeProxy *tree_proxy,
                          gboolean       flat)
{
  g_return_if_fail (LIGMA_IS_TREE_PROXY (tree_proxy));

  if (flat != tree_proxy->priv->flat)
    {
      ligma_container_freeze (LIGMA_CONTAINER (tree_proxy));

      if (tree_proxy->priv->container)
        {
          ligma_tree_proxy_remove_container (tree_proxy,
                                            tree_proxy->priv->container);
        }

      tree_proxy->priv->flat = flat;

      if (tree_proxy->priv->container)
        {
          ligma_tree_proxy_add_container (tree_proxy,
                                         tree_proxy->priv->container,
                                         -1);
        }

      ligma_container_thaw (LIGMA_CONTAINER (tree_proxy));

      g_object_notify (G_OBJECT (tree_proxy), "flat");
    }
}

gboolean
ligma_tree_proxy_get_flat (LigmaTreeProxy *tree_proxy)
{
  g_return_val_if_fail (LIGMA_IS_TREE_PROXY (tree_proxy), FALSE);

  return tree_proxy->priv->flat;
}
