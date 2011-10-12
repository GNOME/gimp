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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

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


static void   gimp_item_exclusive_get_lists (GimpItem     *item,
                                             const gchar  *property,
                                             GList       **on,
                                             GList       **off);


/*  public functions  */

void
gimp_item_toggle_exclusive_visible (GimpItem    *item,
                                    GimpContext *context)
{
  GList *on;
  GList *off;
  GList *list;

  g_return_if_fail (GIMP_IS_ITEM (item));
  g_return_if_fail (gimp_item_is_attached (item));
  g_return_if_fail (GIMP_IS_CONTEXT (context));

  gimp_item_exclusive_get_lists (item, "visible", &on, &off);

  if (on || off || ! gimp_item_get_visible (item))
    {
      GimpImage *image = gimp_item_get_image (item);
      GimpUndo  *undo;

      gboolean  push_undo = TRUE;

      undo = gimp_image_undo_can_compress (image, GIMP_TYPE_UNDO_STACK,
                                           GIMP_UNDO_GROUP_ITEM_VISIBILITY);

      if (undo && (g_object_get_data (G_OBJECT (undo), "exclusive-item") ==
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
                g_object_set_data (G_OBJECT (undo), "exclusive-item",
                                   (gpointer) item);
            }

          gimp_image_undo_push_item_visibility (image, NULL, item);

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

      gimp_item_set_visible (item, TRUE, FALSE);

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
}


/*  private functions  */

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
          gboolean value;

          g_object_get (other, property, &value, NULL);

          if (value)
            *on = g_list_prepend (*on, other);
          else
            *off = g_list_prepend (*off, other);
        }
    }

  g_list_free (items);
}
