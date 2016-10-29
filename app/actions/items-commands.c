/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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
#include <gtk/gtk.h>

#include "actions-types.h"

#include "core/gimpimage.h"
#include "core/gimpimage-undo.h"
#include "core/gimpitem.h"
#include "core/gimpitemundo.h"

#include "items-commands.h"

#include "gimp-intl.h"


/*  public functions  */

void
items_visible_cmd_callback (GtkAction *action,
                            GimpImage *image,
                            GimpItem  *item)
{
  gboolean visible;

  visible = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

  if (visible != gimp_item_get_visible (item))
    {
      GimpUndo *undo;
      gboolean  push_undo = TRUE;

      undo = gimp_image_undo_can_compress (image, GIMP_TYPE_ITEM_UNDO,
                                           GIMP_UNDO_ITEM_VISIBILITY);

      if (undo && GIMP_ITEM_UNDO (undo)->item == item)
        push_undo = FALSE;

      gimp_item_set_visible (item, visible, push_undo);
      gimp_image_flush (image);
    }
}

void
items_linked_cmd_callback (GtkAction *action,
                           GimpImage *image,
                           GimpItem  *item)
{
  gboolean linked;

  linked = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

  if (linked != gimp_item_get_linked (item))
    {
      GimpUndo *undo;
      gboolean  push_undo = TRUE;

      undo = gimp_image_undo_can_compress (image, GIMP_TYPE_ITEM_UNDO,
                                           GIMP_UNDO_ITEM_LINKED);

      if (undo && GIMP_ITEM_UNDO (undo)->item == item)
        push_undo = FALSE;

      gimp_item_set_linked (item, linked, push_undo);
      gimp_image_flush (image);
    }
}

void
items_lock_content_cmd_callback (GtkAction *action,
                                 GimpImage *image,
                                 GimpItem  *item)
{
  gboolean locked;

  locked = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

  if (locked != gimp_item_get_lock_content (item))
    {
      GimpUndo *undo;
      gboolean  push_undo = TRUE;

      undo = gimp_image_undo_can_compress (image, GIMP_TYPE_ITEM_UNDO,
                                           GIMP_UNDO_ITEM_LINKED);

      if (undo && GIMP_ITEM_UNDO (undo)->item == item)
        push_undo = FALSE;

      gimp_item_set_lock_content (item, locked, push_undo);
      gimp_image_flush (image);
    }
}

void
items_lock_position_cmd_callback (GtkAction *action,
                                  GimpImage *image,
                                  GimpItem  *item)
{
  gboolean locked;

  locked = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

  if (locked != gimp_item_get_lock_position (item))
    {
      GimpUndo *undo;
      gboolean  push_undo = TRUE;

      undo = gimp_image_undo_can_compress (image, GIMP_TYPE_ITEM_UNDO,
                                           GIMP_UNDO_ITEM_LOCK_POSITION);

      if (undo && GIMP_ITEM_UNDO (undo)->item == item)
        push_undo = FALSE;


      gimp_item_set_lock_position (item, locked, push_undo);
      gimp_image_flush (image);
    }
}

void
items_color_tag_cmd_callback (GtkAction    *action,
                              GimpImage    *image,
                              GimpItem     *item,
                              GimpColorTag  color_tag)
{
  if (color_tag != gimp_item_get_color_tag (item))
    {
      GimpUndo *undo;
      gboolean  push_undo = TRUE;

      undo = gimp_image_undo_can_compress (image, GIMP_TYPE_ITEM_UNDO,
                                           GIMP_UNDO_ITEM_COLOR_TAG);

      if (undo && GIMP_ITEM_UNDO (undo)->item == item)
        push_undo = FALSE;

      gimp_item_set_color_tag (item, color_tag, push_undo);
      gimp_image_flush (image);
    }
}
