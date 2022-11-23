/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * ligmaitemstack.c
 * Copyright (C) 2008 Michael Natterer <mitch@ligma.org>
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

#include <string.h>

#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libligmacolor/ligmacolor.h"

#include "core-types.h"

#include "ligmaitem.h"
#include "ligmaitemstack.h"


/*  local function prototypes  */

static void   ligma_item_stack_constructed (GObject       *object);

static void   ligma_item_stack_add         (LigmaContainer *container,
                                           LigmaObject    *object);
static void   ligma_item_stack_remove      (LigmaContainer *container,
                                           LigmaObject    *object);


G_DEFINE_TYPE (LigmaItemStack, ligma_item_stack, LIGMA_TYPE_FILTER_STACK)

#define parent_class ligma_item_stack_parent_class


static void
ligma_item_stack_class_init (LigmaItemStackClass *klass)
{
  GObjectClass       *object_class    = G_OBJECT_CLASS (klass);
  LigmaContainerClass *container_class = LIGMA_CONTAINER_CLASS (klass);

  object_class->constructed = ligma_item_stack_constructed;

  container_class->add      = ligma_item_stack_add;
  container_class->remove   = ligma_item_stack_remove;
}

static void
ligma_item_stack_init (LigmaItemStack *stack)
{
}

static void
ligma_item_stack_constructed (GObject *object)
{
  LigmaContainer *container = LIGMA_CONTAINER (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  ligma_assert (g_type_is_a (ligma_container_get_children_type (container),
                         LIGMA_TYPE_ITEM));
}

static void
ligma_item_stack_add (LigmaContainer *container,
                     LigmaObject    *object)
{
  g_object_ref_sink (object);

  LIGMA_CONTAINER_CLASS (parent_class)->add (container, object);

  g_object_unref (object);
}

static void
ligma_item_stack_remove (LigmaContainer *container,
                        LigmaObject    *object)
{
  LIGMA_CONTAINER_CLASS (parent_class)->remove (container, object);
}


/*  public functions  */

LigmaContainer *
ligma_item_stack_new (GType item_type)
{
  g_return_val_if_fail (g_type_is_a (item_type, LIGMA_TYPE_ITEM), NULL);

  return g_object_new (LIGMA_TYPE_ITEM_STACK,
                       "name",          g_type_name (item_type),
                       "children-type", item_type,
                       "policy",        LIGMA_CONTAINER_POLICY_STRONG,
                       NULL);
}

gint
ligma_item_stack_get_n_items (LigmaItemStack *stack)
{
  GList *list;
  gint   n_items = 0;

  g_return_val_if_fail (LIGMA_IS_ITEM_STACK (stack), 0);

  for (list = LIGMA_LIST (stack)->queue->head; list; list = g_list_next (list))
    {
      LigmaItem      *item = list->data;
      LigmaContainer *children;

      n_items++;

      children = ligma_viewable_get_children (LIGMA_VIEWABLE (item));

      if (children)
        n_items += ligma_item_stack_get_n_items (LIGMA_ITEM_STACK (children));
    }

  return n_items;
}

gboolean
ligma_item_stack_is_flat (LigmaItemStack *stack)
{
  GList *list;

  g_return_val_if_fail (LIGMA_IS_ITEM_STACK (stack), TRUE);

  for (list = LIGMA_LIST (stack)->queue->head; list; list = g_list_next (list))
    {
      LigmaViewable *viewable = list->data;

      if (ligma_viewable_get_children (viewable))
        return FALSE;
    }

  return TRUE;
}

GList *
ligma_item_stack_get_item_iter (LigmaItemStack *stack)
{
  g_return_val_if_fail (LIGMA_IS_ITEM_STACK (stack), NULL);

  return LIGMA_LIST (stack)->queue->head;
}

GList *
ligma_item_stack_get_item_list (LigmaItemStack *stack)
{
  GList *list;
  GList *result = NULL;

  g_return_val_if_fail (LIGMA_IS_ITEM_STACK (stack), NULL);

  for (list = LIGMA_LIST (stack)->queue->head;
       list;
       list = g_list_next (list))
    {
      LigmaViewable  *viewable = list->data;
      LigmaContainer *children;

      result = g_list_prepend (result, viewable);

      children = ligma_viewable_get_children (viewable);

      if (children)
        {
          GList *child_list;

          child_list = ligma_item_stack_get_item_list (LIGMA_ITEM_STACK (children));

          while (child_list)
            {
              result = g_list_prepend (result, child_list->data);

              child_list = g_list_remove (child_list, child_list->data);
            }
        }
    }

  return g_list_reverse (result);
}

LigmaItem *
ligma_item_stack_get_item_by_tattoo (LigmaItemStack *stack,
                                    LigmaTattoo     tattoo)
{
  GList *list;

  g_return_val_if_fail (LIGMA_IS_ITEM_STACK (stack), NULL);

  for (list = LIGMA_LIST (stack)->queue->head; list; list = g_list_next (list))
    {
      LigmaItem      *item = list->data;
      LigmaContainer *children;

      if (ligma_item_get_tattoo (item) == tattoo)
        return item;

      children = ligma_viewable_get_children (LIGMA_VIEWABLE (item));

      if (children)
        {
          item = ligma_item_stack_get_item_by_tattoo (LIGMA_ITEM_STACK (children),
                                                     tattoo);

          if (item)
            return item;
        }
    }

  return NULL;
}

LigmaItem *
ligma_item_stack_get_item_by_path (LigmaItemStack *stack,
                                  GList         *path)
{
  LigmaContainer *container;
  LigmaItem      *item = NULL;

  g_return_val_if_fail (LIGMA_IS_ITEM_STACK (stack), NULL);
  g_return_val_if_fail (path != NULL, NULL);

  container = LIGMA_CONTAINER (stack);

  while (path)
    {
      guint32 i = GPOINTER_TO_UINT (path->data);

      item = LIGMA_ITEM (ligma_container_get_child_by_index (container, i));

      g_return_val_if_fail (LIGMA_IS_ITEM (item), item);

      if (path->next)
        {
          container = ligma_viewable_get_children (LIGMA_VIEWABLE (item));

          g_return_val_if_fail (LIGMA_IS_ITEM_STACK (container), item);
        }

      path = path->next;
    }

  return item;
}

LigmaItem *
ligma_item_stack_get_parent_by_path (LigmaItemStack *stack,
                                    GList         *path,
                                    gint          *index)
{
  LigmaItem *parent = NULL;
  guint32   i;

  g_return_val_if_fail (LIGMA_IS_ITEM_STACK (stack), NULL);
  g_return_val_if_fail (path != NULL, NULL);

  i = GPOINTER_TO_UINT (path->data);

  if (index)
    *index = i;

  while (path->next)
    {
      LigmaObject    *child;
      LigmaContainer *children;

      child = ligma_container_get_child_by_index (LIGMA_CONTAINER (stack), i);

      g_return_val_if_fail (LIGMA_IS_ITEM (child), parent);

      children = ligma_viewable_get_children (LIGMA_VIEWABLE (child));

      g_return_val_if_fail (LIGMA_IS_ITEM_STACK (children), parent);

      parent = LIGMA_ITEM (child);
      stack  = LIGMA_ITEM_STACK (children);

      path = path->next;

      i = GPOINTER_TO_UINT (path->data);

      if (index)
        *index = i;
    }

  return parent;
}

static void
ligma_item_stack_viewable_invalidate_previews (LigmaViewable *viewable)
{
  LigmaContainer *children = ligma_viewable_get_children (viewable);

  if (children)
    ligma_item_stack_invalidate_previews (LIGMA_ITEM_STACK (children));

  ligma_viewable_invalidate_preview (viewable);
}

void
ligma_item_stack_invalidate_previews (LigmaItemStack *stack)
{
  g_return_if_fail (LIGMA_IS_ITEM_STACK (stack));

  ligma_container_foreach (LIGMA_CONTAINER (stack),
                          (GFunc) ligma_item_stack_viewable_invalidate_previews,
                          NULL);
}

static void
ligma_item_stack_viewable_profile_changed (LigmaViewable *viewable)
{
  LigmaContainer *children = ligma_viewable_get_children (viewable);

  if (children)
    ligma_item_stack_profile_changed (LIGMA_ITEM_STACK (children));

  if (LIGMA_IS_COLOR_MANAGED (viewable))
    ligma_color_managed_profile_changed (LIGMA_COLOR_MANAGED (viewable));
}

void
ligma_item_stack_profile_changed (LigmaItemStack *stack)
{
  g_return_if_fail (LIGMA_IS_ITEM_STACK (stack));

  ligma_container_foreach (LIGMA_CONTAINER (stack),
                          (GFunc) ligma_item_stack_viewable_profile_changed,
                          NULL);
}
