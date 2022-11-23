/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaitem-exclusive.c
 * Copyright (C) 2011 Michael Natterer <mitch@ligma.org>
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

#include "core-types.h"

#include "ligmacontext.h"
#include "ligmadrawable.h"
#include "ligmaimage.h"
#include "ligmaimage-undo.h"
#include "ligmaimage-undo-push.h"
#include "ligmaitem.h"
#include "ligmaitem-exclusive.h"
#include "ligmaitemstack.h"
#include "ligmaitemtree.h"
#include "ligmaundostack.h"

#include "ligma-intl.h"


static GList * ligma_item_exclusive_get_ancestry (LigmaItem              *item);
static void    ligma_item_exclusive_get_lists    (LigmaItem              *item,
                                                 LigmaItemIsEnabledFunc  is_enabled,
                                                 LigmaItemCanSetFunc     can_set,
                                                 LigmaItemIsPropLocked   is_prop_locked,
                                                 gboolean               only_selected,
                                                 GList                **on,
                                                 GList                **off);


/*  public functions  */

void
ligma_item_toggle_exclusive_visible (LigmaItem    *item,
                                    gboolean     only_selected,
                                    LigmaContext *context)
{
  ligma_item_toggle_exclusive (item,
                              (LigmaItemIsEnabledFunc) ligma_item_is_visible,
                              (LigmaItemSetFunc)       ligma_item_set_visible,
                              NULL,
                              (LigmaItemIsPropLocked)  ligma_item_get_lock_visibility,
                              (LigmaItemUndoPush)      ligma_image_undo_push_item_visibility,
                              _("Set Item Exclusive Visibility"),
                              LIGMA_UNDO_GROUP_ITEM_VISIBILITY,
                              only_selected, context);
}

void
ligma_item_toggle_exclusive (LigmaItem               *item,
                            LigmaItemIsEnabledFunc   is_enabled,
                            LigmaItemSetFunc         set_prop,
                            LigmaItemCanSetFunc      can_set,
                            LigmaItemIsPropLocked    is_prop_locked,
                            LigmaItemUndoPush        undo_push,
                            const gchar            *undo_desc,
                            LigmaUndoType            group_undo_type,
                            gboolean                only_selected,
                            LigmaContext            *context)
{
  GList *ancestry;
  GList *on;
  GList *off;
  GList *list;

  g_return_if_fail (LIGMA_IS_ITEM (item));
  g_return_if_fail (ligma_item_is_attached (item));
  g_return_if_fail (undo_desc != NULL);
  g_return_if_fail (context == NULL || LIGMA_IS_CONTEXT (context));

  ancestry = ligma_item_exclusive_get_ancestry (item);
  ligma_item_exclusive_get_lists (item, is_enabled, can_set, is_prop_locked,
                                 only_selected, &on, &off);

  if (on || off || (! is_enabled (item) && (can_set == NULL || can_set (item))))
    {
      LigmaImage *image = ligma_item_get_image (item);
      LigmaUndo  *undo;
      gboolean   push_undo = TRUE;

      undo = ligma_image_undo_can_compress (image, LIGMA_TYPE_UNDO_STACK,
                                           group_undo_type);

      /* Use the undo description as the undo object key, as we should
       * assume that a same undo description means the same exclusive
       * action.
       */
      if (undo && (g_object_get_data (G_OBJECT (undo), undo_desc) ==
                   (gpointer) item))
        push_undo = FALSE;

      if (push_undo)
        {
          if (ligma_image_undo_group_start (image,
                                           group_undo_type,
                                           undo_desc))
            {
              undo = ligma_image_undo_can_compress (image, LIGMA_TYPE_UNDO_STACK,
                                                   group_undo_type);

              if (undo)
                g_object_set_data (G_OBJECT (undo), undo_desc, (gpointer) item);
            }

          for (list = ancestry; list; list = g_list_next (list))
            undo_push (image, NULL, list->data);

          for (list = on; list; list = g_list_next (list))
            undo_push (image, NULL, list->data);

          for (list = off; list; list = g_list_next (list))
            undo_push (image, NULL, list->data);

          ligma_image_undo_group_end (image);
        }
      else if (context)
        {
          ligma_undo_refresh_preview (undo, context);
        }

      for (list = ancestry; list; list = g_list_next (list))
        set_prop (list->data, TRUE, FALSE);

      if (on)
        {
          for (list = on; list; list = g_list_next (list))
            set_prop (list->data, FALSE, FALSE);
        }
      else if (off)
        {
          for (list = off; list; list = g_list_next (list))
            set_prop (list->data, TRUE, FALSE);
        }

      g_list_free (on);
      g_list_free (off);
    }

  g_list_free (ancestry);
}


/*  private functions  */

static GList *
ligma_item_exclusive_get_ancestry (LigmaItem *item)
{
  LigmaViewable *parent;
  GList        *ancestry = NULL;

  for (parent = LIGMA_VIEWABLE (item);
       parent;
       parent = ligma_viewable_get_parent (parent))
    {
      ancestry = g_list_prepend (ancestry, parent);
    }

  return ancestry;
}

static void
ligma_item_exclusive_get_lists (LigmaItem              *item,
                               LigmaItemIsEnabledFunc  is_enabled,
                               LigmaItemCanSetFunc     can_set,
                               LigmaItemIsPropLocked   is_prop_locked,
                               gboolean               only_selected,
                               GList                 **on,
                               GList                 **off)
{
  LigmaImage    *image = NULL;
  LigmaItemTree *tree;
  GList        *items;
  GList        *list;

  *on  = NULL;
  *off = NULL;

  tree = ligma_item_get_tree (item);

  items = ligma_item_stack_get_item_list (LIGMA_ITEM_STACK (tree->container));

  if (only_selected)
    image = ligma_item_get_image (item);

  for (list = items; list; list = g_list_next (list))
    {
      LigmaItem *other = list->data;

      if (other != item                                                                &&
          /* Don't include item with visibility locks. */
          (is_prop_locked == NULL || ! is_prop_locked (other))                         &&
          /* Don't include item which can be changed. */
          (can_set == NULL || can_set (other))                                         &&
          /* Do we care only about selected drawables? */
          (! only_selected  || ligma_image_is_selected_drawable (image,
                                                                LIGMA_DRAWABLE (other))) &&
          /* We are only interested in same level items unless
           * @only_selected is TRUE. */
          (only_selected ||
           ligma_viewable_get_parent (LIGMA_VIEWABLE (other)) ==
           ligma_viewable_get_parent (LIGMA_VIEWABLE (item))))
        {
          if (is_enabled (other))
            *on = g_list_prepend (*on, other);
          else
            *off = g_list_prepend (*off, other);
        }
    }

  g_list_free (items);
}
