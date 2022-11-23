/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * ligmaitemtree.c
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

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libligmabase/ligmabase.h"

#include "core-types.h"

#include "ligmaimage.h"
#include "ligmaimage-undo-push.h"
#include "ligmaitem.h"
#include "ligmaitemstack.h"
#include "ligmaitemtree.h"


enum
{
  PROP_0,
  PROP_IMAGE,
  PROP_CONTAINER_TYPE,
  PROP_ITEM_TYPE,
  PROP_ACTIVE_ITEM,
  PROP_SELECTED_ITEMS
};


typedef struct _LigmaItemTreePrivate LigmaItemTreePrivate;

struct _LigmaItemTreePrivate
{
  LigmaImage  *image;

  GType       container_type;
  GType       item_type;

  GList      *selected_items;

  GHashTable *name_hash;
};

#define LIGMA_ITEM_TREE_GET_PRIVATE(object) \
        ((LigmaItemTreePrivate *) ligma_item_tree_get_instance_private ((LigmaItemTree *) (object)))


/*  local function prototypes  */

static void     ligma_item_tree_constructed   (GObject      *object);
static void     ligma_item_tree_dispose       (GObject      *object);
static void     ligma_item_tree_finalize      (GObject      *object);
static void     ligma_item_tree_set_property  (GObject      *object,
                                              guint         property_id,
                                              const GValue *value,
                                              GParamSpec   *pspec);
static void     ligma_item_tree_get_property  (GObject      *object,
                                              guint         property_id,
                                              GValue       *value,
                                              GParamSpec   *pspec);

static gint64   ligma_item_tree_get_memsize   (LigmaObject   *object,
                                              gint64       *gui_size);

static void     ligma_item_tree_uniquefy_name (LigmaItemTree *tree,
                                              LigmaItem     *item,
                                              const gchar  *new_name);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaItemTree, ligma_item_tree, LIGMA_TYPE_OBJECT)

#define parent_class ligma_item_tree_parent_class


static void
ligma_item_tree_class_init (LigmaItemTreeClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  LigmaObjectClass *ligma_object_class = LIGMA_OBJECT_CLASS (klass);

  object_class->constructed      = ligma_item_tree_constructed;
  object_class->dispose          = ligma_item_tree_dispose;
  object_class->finalize         = ligma_item_tree_finalize;
  object_class->set_property     = ligma_item_tree_set_property;
  object_class->get_property     = ligma_item_tree_get_property;

  ligma_object_class->get_memsize = ligma_item_tree_get_memsize;

  g_object_class_install_property (object_class, PROP_IMAGE,
                                   g_param_spec_object ("image",
                                                        NULL, NULL,
                                                        LIGMA_TYPE_IMAGE,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_CONTAINER_TYPE,
                                   g_param_spec_gtype ("container-type",
                                                       NULL, NULL,
                                                       LIGMA_TYPE_ITEM_STACK,
                                                       LIGMA_PARAM_READWRITE |
                                                       G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_ITEM_TYPE,
                                   g_param_spec_gtype ("item-type",
                                                       NULL, NULL,
                                                       LIGMA_TYPE_ITEM,
                                                       LIGMA_PARAM_READWRITE |
                                                       G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_ACTIVE_ITEM,
                                   g_param_spec_object ("active-item",
                                                        NULL, NULL,
                                                        LIGMA_TYPE_ITEM,
                                                        LIGMA_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_SELECTED_ITEMS,
                                   g_param_spec_pointer ("selected-items",
                                                         NULL, NULL,
                                                         LIGMA_PARAM_READWRITE));
}

static void
ligma_item_tree_init (LigmaItemTree *tree)
{
  LigmaItemTreePrivate *private = LIGMA_ITEM_TREE_GET_PRIVATE (tree);

  private->name_hash      = g_hash_table_new (g_str_hash, g_str_equal);
  private->selected_items = NULL;
}

static void
ligma_item_tree_constructed (GObject *object)
{
  LigmaItemTree        *tree    = LIGMA_ITEM_TREE (object);
  LigmaItemTreePrivate *private = LIGMA_ITEM_TREE_GET_PRIVATE (tree);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  ligma_assert (LIGMA_IS_IMAGE (private->image));
  ligma_assert (g_type_is_a (private->container_type, LIGMA_TYPE_ITEM_STACK));
  ligma_assert (g_type_is_a (private->item_type,      LIGMA_TYPE_ITEM));
  ligma_assert (private->item_type != LIGMA_TYPE_ITEM);

  tree->container = g_object_new (private->container_type,
                                  "name",          g_type_name (private->item_type),
                                  "children-type", private->item_type,
                                  "policy",        LIGMA_CONTAINER_POLICY_STRONG,
                                  NULL);
}

static void
ligma_item_tree_dispose (GObject *object)
{
  LigmaItemTree        *tree    = LIGMA_ITEM_TREE (object);
  LigmaItemTreePrivate *private = LIGMA_ITEM_TREE_GET_PRIVATE (tree);

  ligma_item_tree_set_selected_items (tree, NULL);

  ligma_container_foreach (tree->container,
                          (GFunc) ligma_item_removed, NULL);

  ligma_container_clear (tree->container);
  g_hash_table_remove_all (private->name_hash);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
ligma_item_tree_finalize (GObject *object)
{
  LigmaItemTree        *tree    = LIGMA_ITEM_TREE (object);
  LigmaItemTreePrivate *private = LIGMA_ITEM_TREE_GET_PRIVATE (tree);

  g_clear_pointer (&private->name_hash, g_hash_table_unref);
  g_clear_object (&tree->container);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_item_tree_set_property (GObject      *object,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  LigmaItemTreePrivate *private = LIGMA_ITEM_TREE_GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_IMAGE:
      private->image = g_value_get_object (value); /* don't ref */
      break;
    case PROP_CONTAINER_TYPE:
      private->container_type = g_value_get_gtype (value);
      break;
    case PROP_ITEM_TYPE:
      private->item_type = g_value_get_gtype (value);
      break;
    case PROP_ACTIVE_ITEM:
      /* Don't ref the item. */
      private->selected_items = g_list_prepend (NULL, g_value_get_object (value));
      break;
    case PROP_SELECTED_ITEMS:
      /* Don't ref the item. */
      private->selected_items = g_value_get_pointer (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_item_tree_get_property (GObject    *object,
                             guint       property_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  LigmaItemTreePrivate *private = LIGMA_ITEM_TREE_GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_IMAGE:
      g_value_set_object (value, private->image);
      break;
    case PROP_CONTAINER_TYPE:
      g_value_set_gtype (value, private->container_type);
      break;
    case PROP_ITEM_TYPE:
      g_value_set_gtype (value, private->item_type);
      break;
    case PROP_ACTIVE_ITEM:
      g_value_set_object (value, ligma_item_tree_get_active_item (LIGMA_ITEM_TREE (object)));
      break;
    case PROP_SELECTED_ITEMS:
      g_value_set_pointer (value, private->selected_items);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gint64
ligma_item_tree_get_memsize (LigmaObject *object,
                            gint64     *gui_size)
{
  LigmaItemTree *tree    = LIGMA_ITEM_TREE (object);
  gint64        memsize = 0;

  memsize += ligma_object_get_memsize (LIGMA_OBJECT (tree->container), gui_size);

  return memsize + LIGMA_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}


/*  public functions  */

LigmaItemTree *
ligma_item_tree_new (LigmaImage *image,
                    GType      container_type,
                    GType      item_type)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (g_type_is_a (container_type, LIGMA_TYPE_ITEM_STACK), NULL);
  g_return_val_if_fail (g_type_is_a (item_type, LIGMA_TYPE_ITEM), NULL);

  return g_object_new (LIGMA_TYPE_ITEM_TREE,
                       "image",          image,
                       "container-type", container_type,
                       "item-type",      item_type,
                       NULL);
}

LigmaItem *
ligma_item_tree_get_active_item (LigmaItemTree *tree)
{
  GList *items;

  g_return_val_if_fail (LIGMA_IS_ITEM_TREE (tree), NULL);

  items = LIGMA_ITEM_TREE_GET_PRIVATE (tree)->selected_items;
  if (g_list_length (items) == 1)
    return items->data;
  else
    return NULL;
}

void
ligma_item_tree_set_active_item (LigmaItemTree *tree,
                                LigmaItem     *item)
{
  ligma_item_tree_set_selected_items (tree,
                                     g_list_prepend (NULL, item));
}

GList *
ligma_item_tree_get_selected_items (LigmaItemTree *tree)
{
  g_return_val_if_fail (LIGMA_IS_ITEM_TREE (tree), NULL);

  return LIGMA_ITEM_TREE_GET_PRIVATE (tree)->selected_items;
}

/**
 * ligma_item_tree_set_selected_items:
 * @tree:
 * @items: (transfer container):
 *
 * Sets the list of selected items. @tree takes ownership of @items
 * container (not the #LigmaItem themselves).
 */
void
ligma_item_tree_set_selected_items (LigmaItemTree *tree,
                                   GList        *items)
{
  LigmaItemTreePrivate *private;
  GList               *iter;
  gboolean             selection_changed = TRUE;
  gint                 prev_selected_count;
  gint                 selected_count;

  g_return_if_fail (LIGMA_IS_ITEM_TREE (tree));

  private = LIGMA_ITEM_TREE_GET_PRIVATE (tree);

  for (iter = items; iter; iter = iter->next)
    {
      g_return_if_fail (G_TYPE_CHECK_INSTANCE_TYPE (iter->data, private->item_type));
      g_return_if_fail (ligma_item_get_tree (iter->data) == tree);
    }

  prev_selected_count = g_list_length (private->selected_items);
  selected_count      = g_list_length (items);

  if (selected_count == prev_selected_count)
    {
      selection_changed = FALSE;
      for (iter = items; iter; iter = iter->next)
        {
          if (g_list_find (private->selected_items, iter->data) == NULL)
            {
              selection_changed = TRUE;
              break;
            }
        }
    }

  if (selection_changed)
    {
      if (private->selected_items)
        g_list_free (private->selected_items);

      private->selected_items = items;
      g_object_notify (G_OBJECT (tree), "selected-items");

      /* XXX: if we add back a proper concept of active item (not just
       * meaning selection of 1), we may also notify of active item
       * update.
       */
      /*g_object_notify (G_OBJECT (tree), "active-item");*/
    }
  else if (items != private->selected_items)
    {
      g_list_free (items);
    }
}

LigmaItem *
ligma_item_tree_get_item_by_name (LigmaItemTree *tree,
                                 const gchar  *name)
{
  g_return_val_if_fail (LIGMA_IS_ITEM_TREE (tree), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  return g_hash_table_lookup (LIGMA_ITEM_TREE_GET_PRIVATE (tree)->name_hash,
                              name);
}

gboolean
ligma_item_tree_get_insert_pos (LigmaItemTree  *tree,
                               LigmaItem      *item,
                               LigmaItem     **parent,
                               gint          *position)
{
  LigmaItemTreePrivate *private;
  LigmaContainer       *container;

  g_return_val_if_fail (LIGMA_IS_ITEM_TREE (tree), FALSE);
  g_return_val_if_fail (parent != NULL, FALSE);

  private = LIGMA_ITEM_TREE_GET_PRIVATE (tree);

  g_return_val_if_fail (G_TYPE_CHECK_INSTANCE_TYPE (item, private->item_type),
                        FALSE);
  g_return_val_if_fail (! ligma_item_is_attached (item), FALSE);
  g_return_val_if_fail (ligma_item_get_image (item) == private->image, FALSE);
  g_return_val_if_fail (*parent == NULL ||
                        *parent == LIGMA_IMAGE_ACTIVE_PARENT ||
                        G_TYPE_CHECK_INSTANCE_TYPE (*parent, private->item_type),
                        FALSE);
  g_return_val_if_fail (*parent == NULL ||
                        *parent == LIGMA_IMAGE_ACTIVE_PARENT ||
                        ligma_item_get_tree (*parent) == tree, FALSE);
  g_return_val_if_fail (*parent == NULL ||
                        *parent == LIGMA_IMAGE_ACTIVE_PARENT ||
                        ligma_viewable_get_children (LIGMA_VIEWABLE (*parent)),
                        FALSE);
  g_return_val_if_fail (position != NULL, FALSE);

  /*  if we want to insert in the active item's parent container  */
  if (*parent == LIGMA_IMAGE_ACTIVE_PARENT)
    {
      GList    *iter;
      LigmaItem *selected_parent;

      *parent = NULL;
      for (iter = private->selected_items; iter; iter = iter->next)
        {
          /*  if the selected item is a branch, add to the top of that
           *  branch; add to the selected item's parent container
           *  otherwise
           */
          if (ligma_viewable_get_children (LIGMA_VIEWABLE (iter->data)))
            {
              selected_parent = iter->data;
              *position       = 0;
            }
          else
            {
              selected_parent = ligma_item_get_parent (iter->data);
            }

          /* Only allow if all selected items have the same parent.
           * If hierarchy is different, use the toplevel container
           * (same if there are no selected items).
           */
          if (iter != private->selected_items && *parent != selected_parent)
            {
              *parent = NULL;
              break;
            }
          else
            {
              *parent = selected_parent;
            }
        }
    }

  if (*parent)
    container = ligma_viewable_get_children (LIGMA_VIEWABLE (*parent));
  else
    container = tree->container;

  /* We want to add on top of the selected items. */
  if (*position == -1)
    {
      GList    *iter;
      gint      selected_position;

      for (iter = private->selected_items; iter; iter = iter->next)
        {
          selected_position =
            ligma_container_get_child_index (container, iter->data);

          /* If one the selected items is not in the specified parent
           * container, fall back to index 0.
           */
          if (selected_position == -1)
            {
              *position = 0;
              break;
            }

          if (*position == -1)
            *position = selected_position;
          else
            /* Higher selected items has the lower index. */
            *position = MIN (*position, selected_position);
        }
    }

  /*  don't add at a non-existing index  */
  *position = CLAMP (*position, 0, ligma_container_get_n_children (container));

  return TRUE;
}

void
ligma_item_tree_add_item (LigmaItemTree *tree,
                         LigmaItem     *item,
                         LigmaItem     *parent,
                         gint          position)
{
  LigmaItemTreePrivate *private;
  LigmaContainer       *container;
  LigmaContainer       *children;

  g_return_if_fail (LIGMA_IS_ITEM_TREE (tree));

  private = LIGMA_ITEM_TREE_GET_PRIVATE (tree);

  g_return_if_fail (G_TYPE_CHECK_INSTANCE_TYPE (item, private->item_type));
  g_return_if_fail (! ligma_item_is_attached (item));
  g_return_if_fail (ligma_item_get_image (item) == private->image);
  g_return_if_fail (parent == NULL ||
                    G_TYPE_CHECK_INSTANCE_TYPE (parent, private->item_type));
  g_return_if_fail (parent == NULL || ligma_item_get_tree (parent) == tree);
  g_return_if_fail (parent == NULL ||
                    ligma_viewable_get_children (LIGMA_VIEWABLE (parent)));

  ligma_item_tree_uniquefy_name (tree, item, NULL);

  children = ligma_viewable_get_children (LIGMA_VIEWABLE (item));

  if (children)
    {
      GList *list = ligma_item_stack_get_item_list (LIGMA_ITEM_STACK (children));

      while (list)
        {
          ligma_item_tree_uniquefy_name (tree, list->data, NULL);

          list = g_list_remove (list, list->data);
        }
    }

  if (parent)
    container = ligma_viewable_get_children (LIGMA_VIEWABLE (parent));
  else
    container = tree->container;

  if (parent)
    ligma_viewable_set_parent (LIGMA_VIEWABLE (item),
                              LIGMA_VIEWABLE (parent));

  ligma_container_insert (container, LIGMA_OBJECT (item), position);

  /*  if the item came from the undo stack, reset its "removed" state  */
  if (ligma_item_is_removed (item))
    ligma_item_unset_removed (item);
}

GList *
ligma_item_tree_remove_item (LigmaItemTree *tree,
                            LigmaItem     *item,
                            GList        *new_selected)
{
  LigmaItemTreePrivate *private;
  LigmaItem            *parent;
  LigmaContainer       *container;
  LigmaContainer       *children;
  gint                 index;

  g_return_val_if_fail (LIGMA_IS_ITEM_TREE (tree), NULL);

  private = LIGMA_ITEM_TREE_GET_PRIVATE (tree);

  g_return_val_if_fail (G_TYPE_CHECK_INSTANCE_TYPE (item, private->item_type),
                        NULL);
  g_return_val_if_fail (ligma_item_get_tree (item) == tree, NULL);

  parent    = ligma_item_get_parent (item);
  container = ligma_item_get_container (item);
  index     = ligma_item_get_index (item);

  g_object_ref (item);

  g_hash_table_remove (private->name_hash,
                       ligma_object_get_name (item));

  children = ligma_viewable_get_children (LIGMA_VIEWABLE (item));

  if (children)
    {
      GList *list = ligma_item_stack_get_item_list (LIGMA_ITEM_STACK (children));

      while (list)
        {
          g_hash_table_remove (private->name_hash,
                               ligma_object_get_name (list->data));

          list = g_list_remove (list, list->data);
        }
    }

  ligma_container_remove (container, LIGMA_OBJECT (item));

  if (parent)
    ligma_viewable_set_parent (LIGMA_VIEWABLE (item), NULL);

  ligma_item_removed (item);

  if (! new_selected)
    {
      LigmaItem *selected   = NULL;
      gint      n_children = ligma_container_get_n_children (container);

      if (n_children > 0)
        {
          index = CLAMP (index, 0, n_children - 1);

          selected =
            LIGMA_ITEM (ligma_container_get_child_by_index (container, index));
        }
      else if (parent)
        {
          selected = parent;
        }

      if (selected)
        new_selected = g_list_prepend (NULL, selected);
    }
  else
    {
      new_selected = g_list_copy (new_selected);
    }

  g_object_unref (item);

  return new_selected;
}

gboolean
ligma_item_tree_reorder_item (LigmaItemTree *tree,
                             LigmaItem     *item,
                             LigmaItem     *new_parent,
                             gint          new_index,
                             gboolean      push_undo,
                             const gchar  *undo_desc)
{
  LigmaItemTreePrivate *private;
  LigmaContainer       *container;
  LigmaContainer       *new_container;
  gint                 n_items;

  g_return_val_if_fail (LIGMA_IS_ITEM_TREE (tree), FALSE);

  private = LIGMA_ITEM_TREE_GET_PRIVATE (tree);

  g_return_val_if_fail (G_TYPE_CHECK_INSTANCE_TYPE (item, private->item_type),
                        FALSE);
  g_return_val_if_fail (ligma_item_get_tree (item) == tree, FALSE);
  g_return_val_if_fail (new_parent == NULL ||
                        G_TYPE_CHECK_INSTANCE_TYPE (new_parent,
                                                    private->item_type),
                        FALSE);
  g_return_val_if_fail (new_parent == NULL ||
                        ligma_item_get_tree (new_parent) == tree, FALSE);
  g_return_val_if_fail (new_parent == NULL ||
                        ligma_viewable_get_children (LIGMA_VIEWABLE (new_parent)),
                        FALSE);
  g_return_val_if_fail (item != new_parent, FALSE);
  g_return_val_if_fail (new_parent == NULL ||
                        ! ligma_viewable_is_ancestor (LIGMA_VIEWABLE (item),
                                                     LIGMA_VIEWABLE (new_parent)),
                        FALSE);

  container = ligma_item_get_container (item);

  if (new_parent)
    new_container = ligma_viewable_get_children (LIGMA_VIEWABLE (new_parent));
  else
    new_container = tree->container;

  n_items = ligma_container_get_n_children (new_container);

  if (new_container == container)
    n_items--;

  new_index = CLAMP (new_index, 0, n_items);

  if (new_container != container ||
      new_index     != ligma_item_get_index (item))
    {
      GList *selected_items = g_list_copy (private->selected_items);
      if (push_undo)
        ligma_image_undo_push_item_reorder (private->image, undo_desc, item);

      if (new_container != container)
        {
          g_object_ref (item);

          ligma_container_remove (container, LIGMA_OBJECT (item));

          ligma_viewable_set_parent (LIGMA_VIEWABLE (item),
                                    LIGMA_VIEWABLE (new_parent));

          ligma_container_insert (new_container, LIGMA_OBJECT (item), new_index);

          g_object_unref (item);
        }
      else
        {
          ligma_container_reorder (container, LIGMA_OBJECT (item), new_index);
        }
      /* After reorder, selection is likely lost. We must recreate it as
       * it was.
       */
      ligma_item_tree_set_selected_items (tree, selected_items);
    }

  return TRUE;
}

void
ligma_item_tree_rename_item (LigmaItemTree *tree,
                            LigmaItem     *item,
                            const gchar  *new_name,
                            gboolean      push_undo,
                            const gchar  *undo_desc)
{
  LigmaItemTreePrivate *private;

  g_return_if_fail (LIGMA_IS_ITEM_TREE (tree));

  private = LIGMA_ITEM_TREE_GET_PRIVATE (tree);

  g_return_if_fail (G_TYPE_CHECK_INSTANCE_TYPE (item, private->item_type));
  g_return_if_fail (ligma_item_get_tree (item) == tree);
  g_return_if_fail (new_name != NULL);

  if (strcmp (new_name, ligma_object_get_name (item)))
    {
      if (push_undo)
        ligma_image_undo_push_item_rename (ligma_item_get_image (item),
                                          undo_desc, item);

      ligma_item_tree_uniquefy_name (tree, item, new_name);
    }
}


/*  private functions  */

static void
ligma_item_tree_uniquefy_name (LigmaItemTree *tree,
                              LigmaItem     *item,
                              const gchar  *new_name)
{
  LigmaItemTreePrivate *private = LIGMA_ITEM_TREE_GET_PRIVATE (tree);

  if (new_name)
    {
      g_hash_table_remove (private->name_hash,
                           ligma_object_get_name (item));

      ligma_object_set_name (LIGMA_OBJECT (item), new_name);
    }

  /* Remove any trailing whitespace. */
  if (ligma_object_get_name (item))
    {
      gchar *name = g_strchomp (g_strdup (ligma_object_get_name (item)));

      ligma_object_take_name (LIGMA_OBJECT (item), name);
    }

  if (g_hash_table_lookup (private->name_hash,
                           ligma_object_get_name (item)))
    {
      gchar      *name        = g_strdup (ligma_object_get_name (item));
      gchar      *new_name    = NULL;
      gint        number      = 0;
      gint        precision   = 1;
      GRegex     *end_numbers = g_regex_new (" ?#([0-9]+)\\s*$", 0, 0, NULL);
      GMatchInfo *match_info  = NULL;

      if (g_regex_match (end_numbers, name, 0, &match_info))
        {
          gchar *match;
          gint   start_pos;

          match  = g_match_info_fetch (match_info, 1);
          if (match && match[0] == '0')
            {
              precision = strlen (match);
            }
          number = atoi (match);
          g_free (match);

          g_match_info_fetch_pos (match_info, 0,
                                  &start_pos, NULL);
          name[start_pos] = '\0';
        }
      g_match_info_free (match_info);
      g_regex_unref (end_numbers);

      do
        {
          number++;

          g_free (new_name);

          new_name = g_strdup_printf ("%s #%.*d",
                                      name,
                                      precision,
                                      number);
        }
      while (g_hash_table_lookup (private->name_hash, new_name));

      g_free (name);

      ligma_object_take_name (LIGMA_OBJECT (item), new_name);
    }

  g_hash_table_insert (private->name_hash,
                       (gpointer) ligma_object_get_name (item),
                       item);
}
