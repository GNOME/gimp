/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpitem-exclusive.c
 * Copyright (C) 2011 Michael Natterer <mitch@gimp.org>
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

#include "gimpcontext.h"
#include "gimpimage-undo.h"
#include "gimpimage-undo-push.h"
#include "gimpitem.h"
#include "gimpitem-exclusive.h"
#include "gimpitemstack.h"
#include "gimpitemtree.h"
#include "gimpundostack.h"

#include "gimp-intl.h"


static GList * gimp_item_exclusive_get_ancestry (GimpItem     *item);
static void    gimp_item_exclusive_get_lists    (GimpItem     *item,
                                                 const gchar  *property,
                                                 GList       **on,
                                                 GList       **off);


/*  public functions  */

void
gimp_item_toggle_exclusive_visible (GimpItem    *item,
                                    GimpContext *context)
{
  GList *ancestry;
  GList *on;
  GList *off;
  GList *list;

  g_return_if_fail (GIMP_IS_ITEM (item));
  g_return_if_fail (gimp_item_is_attached (item));
  g_return_if_fail (GIMP_IS_CONTEXT (context));

  ancestry = gimp_item_exclusive_get_ancestry (item);
  gimp_item_exclusive_get_lists (item, "visible", &on, &off);

  if (on || off || ! gimp_item_is_visible (item))
    {
      GimpImage *image = gimp_item_get_image (item);
      GimpUndo  *undo;
      gboolean   push_undo = TRUE;

      undo = gimp_image_undo_can_compress (image, GIMP_TYPE_UNDO_STACK,
                                           GIMP_UNDO_GROUP_ITEM_VISIBILITY);

      if (undo && (g_object_get_data (G_OBJECT (undo), "exclusive-visible-item") ==
                   (gpointer) item))
        push_undo = FALSE;

      if (push_undo)
        {
          if (gimp_image_undo_group_start (image,
                                           GIMP_UNDO_GROUP_ITEM_VISIBILITY,
                                           _("Set Item Exclusive Visible")))
            {
              undo = gimp_image_undo_can_compress (image, GIMP_TYPE_UNDO_STACK,
                                                   GIMP_UNDO_GROUP_ITEM_VISIBILITY);

              if (undo)
                g_object_set_data (G_OBJECT (undo), "exclusive-visible-item",
                                   (gpointer) item);
            }

          for (list = ancestry; list; list = g_list_next (list))
            gimp_image_undo_push_item_visibility (image, NULL, list->data);

          for (list = on; list; list = g_list_next (list))
            gimp_image_undo_push_item_visibility (image, NULL, list->data);

          for (list = off; list; list = g_list_next (list))
            gimp_image_undo_push_item_visibility (image, NULL, list->data);

          gimp_image_undo_group_end (image);
        }
      else
        {
          gimp_undo_refresh_preview (undo, context);
        }

      for (list = ancestry; list; list = g_list_next (list))
        gimp_item_set_visible (list->data, TRUE, FALSE);

      if (on)
        {
          for (list = on; list; list = g_list_next (list))
            gimp_item_set_visible (list->data, FALSE, FALSE);
        }
      else if (off)
        {
          for (list = off; list; list = g_list_next (list))
            gimp_item_set_visible (list->data, TRUE, FALSE);
        }

      g_list_free (on);
      g_list_free (off);
    }

  g_list_free (ancestry);
}

void
gimp_item_toggle_exclusive_linked (GimpItem    *item,
                                   GimpContext *context)
{
  GList *on  = NULL;
  GList *off = NULL;
  GList *list;

  g_return_if_fail (GIMP_IS_ITEM (item));
  g_return_if_fail (gimp_item_is_attached (item));
  g_return_if_fail (GIMP_IS_CONTEXT (context));

  for (list = gimp_item_get_container_iter (item);
       list;
       list = g_list_next (list))
    {
      GimpItem *other = list->data;

      if (other != item)
        {
          if (gimp_item_get_linked (other))
            on = g_list_prepend (on, other);
          else
            off = g_list_prepend (off, other);
        }
    }

  if (on || off || ! gimp_item_get_linked (item))
    {
      GimpImage *image = gimp_item_get_image (item);
      GimpUndo  *undo;
      gboolean   push_undo = TRUE;

      undo = gimp_image_undo_can_compress (image, GIMP_TYPE_UNDO_STACK,
                                           GIMP_UNDO_GROUP_ITEM_LINKED);

      if (undo && (g_object_get_data (G_OBJECT (undo), "exclusive-linked-item") ==
                   (gpointer) item))
        push_undo = FALSE;

      if (push_undo)
        {
          if (gimp_image_undo_group_start (image,
                                           GIMP_UNDO_GROUP_ITEM_LINKED,
                                           _("Set Item Exclusive Linked")))
            {
              undo = gimp_image_undo_can_compress (image, GIMP_TYPE_UNDO_STACK,
                                                   GIMP_UNDO_GROUP_ITEM_LINKED);

              if (undo)
                g_object_set_data (G_OBJECT (undo), "exclusive-linked-item",
                                   (gpointer) item);
            }

          gimp_image_undo_push_item_linked (image, NULL, item);

          for (list = on; list; list = g_list_next (list))
            gimp_image_undo_push_item_linked (image, NULL, list->data);

          for (list = off; list; list = g_list_next (list))
            gimp_image_undo_push_item_linked (image, NULL, list->data);

          gimp_image_undo_group_end (image);
        }
      else
        {
          gimp_undo_refresh_preview (undo, context);
        }

      if (off || ! gimp_item_get_linked (item))
        {
          gimp_item_set_linked (item, TRUE, FALSE);

          for (list = off; list; list = g_list_next (list))
            gimp_item_set_linked (list->data, TRUE, FALSE);
        }
      else
        {
          for (list = on; list; list = g_list_next (list))
            gimp_item_set_linked (list->data, FALSE, FALSE);
        }

      g_list_free (on);
      g_list_free (off);
    }
}


/*  private functions  */

static GList *
gimp_item_exclusive_get_ancestry (GimpItem *item)
{
  GimpViewable *parent;
  GList        *ancestry = NULL;

  for (parent = GIMP_VIEWABLE (item);
       parent;
       parent = gimp_viewable_get_parent (parent))
    {
      ancestry = g_list_prepend (ancestry, parent);
    }

  return ancestry;
}

static void
gimp_item_exclusive_get_lists (GimpItem     *item,
                               const gchar  *property,
                               GList       **on,
                               GList       **off)
{
  GimpItemTree *tree;
  GList        *items;
  GList        *list;

  *on  = NULL;
  *off = NULL;

  tree = gimp_item_get_tree (item);

  items = gimp_item_stack_get_item_list (GIMP_ITEM_STACK (tree->container));

  for (list = items; list; list = g_list_next (list))
    {
      GimpItem *other = list->data;

      if (other != item)
        {
          /* we are only interested in same level items.
           */
          if (gimp_viewable_get_parent (GIMP_VIEWABLE (other)) ==
              gimp_viewable_get_parent (GIMP_VIEWABLE (item)))
            {
              gboolean value;

              g_object_get (other, property, &value, NULL);

              if (value)
                *on = g_list_prepend (*on, other);
              else
                *off = g_list_prepend (*off, other);
            }
        }
    }

  g_list_free (items);
}
