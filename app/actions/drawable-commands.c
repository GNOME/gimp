/* LIGMA - The GNU Image Manipulation Program
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmawidgets/ligmawidgets.h"

#include "actions-types.h"

#include "core/ligma.h"
#include "core/ligmadrawable-equalize.h"
#include "core/ligmadrawable-levels.h"
#include "core/ligmadrawable-operation.h"
#include "core/ligmaimage.h"
#include "core/ligmaimage-undo.h"
#include "core/ligmaitemundo.h"
#include "core/ligmalayermask.h"
#include "core/ligmaprogress.h"

#include "dialogs/dialogs.h"

#include "actions.h"
#include "drawable-commands.h"

#include "ligma-intl.h"


/*  public functions  */

void
drawable_equalize_cmd_callback (LigmaAction *action,
                                GVariant   *value,
                                gpointer    data)
{
  LigmaImage *image;
  GList     *drawables;
  GList     *iter;

  return_if_no_drawables (image, drawables, data);

  if (g_list_length (drawables) > 1)
    ligma_image_undo_group_start (image,
                                 LIGMA_UNDO_GROUP_DRAWABLE_MOD,
                                 _("Equalize"));

  for (iter = drawables; iter; iter = iter->next)
    ligma_drawable_equalize (iter->data, TRUE);

  if (g_list_length (drawables) > 1)
    ligma_image_undo_group_end (image);

  ligma_image_flush (image);
  g_list_free (drawables);
}

void
drawable_levels_stretch_cmd_callback (LigmaAction *action,
                                      GVariant   *value,
                                      gpointer    data)
{
  LigmaImage    *image;
  GList        *drawables;
  GList        *iter;
  LigmaDisplay  *display;
  GtkWidget    *widget;

  return_if_no_drawables (image, drawables, data);
  return_if_no_display (display, data);
  return_if_no_widget (widget, data);

  for (iter = drawables; iter; iter = iter->next)
    {
      if (! ligma_drawable_is_rgb (iter->data))
        {
          ligma_message_literal (image->ligma,
                                G_OBJECT (widget), LIGMA_MESSAGE_WARNING,
                                _("White Balance operates only on RGB color "
                                  "layers."));
          return;
        }
    }

  if (g_list_length (drawables) > 1)
    ligma_image_undo_group_start (image,
                                 LIGMA_UNDO_GROUP_DRAWABLE_MOD,
                                 _("Levels"));

  for (iter = drawables; iter; iter = iter->next)
    ligma_drawable_levels_stretch (iter->data, LIGMA_PROGRESS (display));

  if (g_list_length (drawables) > 1)
    ligma_image_undo_group_end (image);

  ligma_image_flush (image);
  g_list_free (drawables);
}

void
drawable_visible_cmd_callback (LigmaAction *action,
                               GVariant   *value,
                               gpointer    data)
{
  LigmaImage    *image;
  GList        *drawables;
  GList        *iter;
  LigmaUndo     *undo;
  gboolean      push_undo = TRUE;
  gboolean      visible;

  return_if_no_drawables (image, drawables, data);

  visible = g_variant_get_boolean (value);

  if (LIGMA_IS_LAYER_MASK (drawables->data))
    {
      LigmaLayerMask *mask = LIGMA_LAYER_MASK (drawables->data);

      g_list_free (drawables);
      drawables = g_list_prepend (NULL, ligma_layer_mask_get_layer (mask));
    }

  for (iter = drawables; iter; iter = iter->next)
    {
      if (visible && ligma_item_get_visible (iter->data))
        {
          /* If any of the drawables are already visible, we don't
           * toggle the selection visibility. This prevents the
           * SET_ACTIVE() in drawables-actions.c to toggle visibility
           * unexpectedly.
           */
          g_list_free (drawables);
          return;
        }
    }

  for (iter = drawables; iter; iter = iter->next)
    if (visible != ligma_item_get_visible (iter->data))
      break;

  if (! iter)
    {
      g_list_free (drawables);
      return;
    }

  if (g_list_length (drawables) == 1)
    {
      undo = ligma_image_undo_can_compress (image, LIGMA_TYPE_ITEM_UNDO,
                                           LIGMA_UNDO_ITEM_VISIBILITY);

      if (undo && LIGMA_ITEM_UNDO (undo)->item == LIGMA_ITEM (drawables->data))
        push_undo = FALSE;
    }
  else
    {
      /* TODO: undo groups cannot be compressed so far. */
      ligma_image_undo_group_start (image,
                                   LIGMA_UNDO_GROUP_ITEM_VISIBILITY,
                                   "Item visibility");
    }

  for (; iter; iter = iter->next)
    ligma_item_set_visible (iter->data, visible, push_undo);

  if (g_list_length (drawables) != 1)
    ligma_image_undo_group_end (image);

  ligma_image_flush (image);
  g_list_free (drawables);
}

void
drawable_lock_content_cmd_callback (LigmaAction *action,
                                    GVariant   *value,
                                    gpointer    data)
{
  LigmaImage *image;
  GList     *drawables;
  GList     *iter;
  gboolean   locked;
  gboolean   push_undo = TRUE;

  return_if_no_drawables (image, drawables, data);

  locked = g_variant_get_boolean (value);

  if (LIGMA_IS_LAYER_MASK (drawables->data))
    {
      LigmaLayerMask *mask = LIGMA_LAYER_MASK (drawables->data);

      g_list_free (drawables);
      drawables = g_list_prepend (NULL, ligma_layer_mask_get_layer (mask));
    }

  for (iter = drawables; iter; iter = iter->next)
    {
      if (! locked && ! ligma_item_get_lock_content (iter->data))
        {
          /* If any of the drawables are already unlocked, we don't toggle the
           * lock. This prevents the SET_ACTIVE() in drawables-actions.c to
           * toggle locks unexpectedly.
           */
          g_list_free (drawables);
          return;
        }
    }

  for (iter = drawables; iter; iter = iter->next)
    if (locked != ligma_item_get_lock_content (iter->data))
      break;

  if (g_list_length (drawables) == 1)
    {
      LigmaUndo *undo;

      undo = ligma_image_undo_can_compress (image, LIGMA_TYPE_ITEM_UNDO,
                                           LIGMA_UNDO_ITEM_LOCK_CONTENT);

      if (undo && LIGMA_ITEM_UNDO (undo)->item == LIGMA_ITEM (drawables->data))
        push_undo = FALSE;
    }
  else
    {
      /* TODO: undo groups cannot be compressed so far. */
      ligma_image_undo_group_start (image,
                                   LIGMA_UNDO_GROUP_ITEM_LOCK_CONTENTS,
                                   _("Lock/Unlock content"));
    }

  for (; iter; iter = iter->next)
    ligma_item_set_lock_content (iter->data, locked, push_undo);

  if (g_list_length (drawables) != 1)
    ligma_image_undo_group_end (image);

  ligma_image_flush (image);
  g_list_free (drawables);
}

void
drawable_lock_position_cmd_callback (LigmaAction *action,
                                     GVariant   *value,
                                     gpointer    data)
{
  LigmaImage *image;
  GList     *drawables;
  GList     *iter;
  gboolean   locked;
  gboolean   push_undo = TRUE;

  return_if_no_drawables (image, drawables, data);

  locked = g_variant_get_boolean (value);

  if (LIGMA_IS_LAYER_MASK (drawables->data))
    {
      LigmaLayerMask *mask = LIGMA_LAYER_MASK (drawables->data);

      g_list_free (drawables);
      drawables = g_list_prepend (NULL, ligma_layer_mask_get_layer (mask));
    }

  for (iter = drawables; iter; iter = iter->next)
    {
      if (! locked && ! ligma_item_get_lock_position (iter->data))
        {
          /* If any of the drawables are already unlocked, we don't toggle the
           * lock. This prevents the SET_ACTIVE() in drawables-actions.c to
           * toggle locks unexpectedly.
           */
          g_list_free (drawables);
          return;
        }
    }

  for (iter = drawables; iter; iter = iter->next)
    if (locked != ligma_item_get_lock_position (iter->data))
      break;

  if (g_list_length (drawables) == 1)
    {
      LigmaUndo *undo;

      undo = ligma_image_undo_can_compress (image, LIGMA_TYPE_ITEM_UNDO,
                                           LIGMA_UNDO_ITEM_LOCK_POSITION);

      if (undo && LIGMA_ITEM_UNDO (undo)->item == LIGMA_ITEM (drawables->data))
        push_undo = FALSE;
    }
  else
    {
      /* TODO: undo groups cannot be compressed so far. */
      ligma_image_undo_group_start (image,
                                   LIGMA_UNDO_GROUP_ITEM_LOCK_POSITION,
                                   _("Lock/Unlock position"));
    }

  for (; iter; iter = iter->next)
    ligma_item_set_lock_position (iter->data, locked, push_undo);

  if (g_list_length (drawables) != 1)
    ligma_image_undo_group_end (image);

  ligma_image_flush (image);
  g_list_free (drawables);
}

void
drawable_flip_cmd_callback (LigmaAction *action,
                            GVariant   *value,
                            gpointer    data)
{
  LigmaImage           *image;
  GList               *drawables;
  GList               *iter;
  LigmaContext         *context;
  gint                 off_x, off_y;
  gdouble              axis = 0.0;
  LigmaOrientationType  orientation;

  return_if_no_drawables (image, drawables, data);
  return_if_no_context (context, data);

  orientation = (LigmaOrientationType) g_variant_get_int32 (value);

  if (g_list_length (drawables) > 1)
    ligma_image_undo_group_start (image,
                                 LIGMA_UNDO_GROUP_DRAWABLE_MOD,
                                 _("Flip Drawables"));

  for (iter = drawables; iter; iter = iter->next)
    {
      LigmaItem *item;

      item = LIGMA_ITEM (iter->data);

      ligma_item_get_offset (item, &off_x, &off_y);

      switch (orientation)
        {
        case LIGMA_ORIENTATION_HORIZONTAL:
          axis = ((gdouble) off_x + (gdouble) ligma_item_get_width (item) / 2.0);
          break;

        case LIGMA_ORIENTATION_VERTICAL:
          axis = ((gdouble) off_y + (gdouble) ligma_item_get_height (item) / 2.0);
          break;

        default:
          break;
        }

      ligma_item_flip (item, context, orientation, axis,
                      ligma_item_get_clip (item, FALSE));
    }

  if (g_list_length (drawables) > 1)
    ligma_image_undo_group_end (image);

  ligma_image_flush (image);
  g_list_free (drawables);
}

void
drawable_rotate_cmd_callback (LigmaAction *action,
                              GVariant   *value,
                              gpointer    data)
{
  LigmaImage        *image;
  GList            *drawables;
  GList            *iter;
  LigmaContext      *context;
  LigmaRotationType  rotation_type;

  return_if_no_drawables (image, drawables, data);
  return_if_no_context (context, data);

  rotation_type = (LigmaRotationType) g_variant_get_int32 (value);

  if (g_list_length (drawables) > 1)
    ligma_image_undo_group_start (image,
                                 LIGMA_UNDO_GROUP_DRAWABLE_MOD,
                                 _("Rotate Drawables"));

  for (iter = drawables; iter; iter = iter->next)
    {
      LigmaItem *item;
      gint      off_x, off_y;
      gdouble   center_x, center_y;

      item = LIGMA_ITEM (iter->data);

      ligma_item_get_offset (item, &off_x, &off_y);

      center_x = ((gdouble) off_x + (gdouble) ligma_item_get_width  (item) / 2.0);
      center_y = ((gdouble) off_y + (gdouble) ligma_item_get_height (item) / 2.0);

      ligma_item_rotate (item, context,
                        rotation_type, center_x, center_y,
                        ligma_item_get_clip (item, FALSE));
    }

  if (g_list_length (drawables) > 1)
    ligma_image_undo_group_end (image);

  ligma_image_flush (image);
  g_list_free (drawables);
}
