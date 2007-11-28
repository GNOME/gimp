/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "core/gimp.h"
#include "core/gimpdrawable-desaturate.h"
#include "core/gimpdrawable-equalize.h"
#include "core/gimpdrawable-invert.h"
#include "core/gimpdrawable-levels.h"
#include "core/gimpimage.h"
#include "core/gimpimage-undo.h"
#include "core/gimpitem-linked.h"
#include "core/gimpitemundo.h"
#include "core/gimplayermask.h"

#include "dialogs/desaturate-dialog.h"
#include "dialogs/offset-dialog.h"

#include "actions.h"
#include "drawable-commands.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   desaturate_response (GtkWidget        *widget,
                                   gint              response_id,
                                   DesaturateDialog *dialog);


/*  private variables  */

static GimpDesaturateMode  desaturate_mode = GIMP_DESATURATE_LIGHTNESS;


/*  public functions  */

void
drawable_desaturate_cmd_callback (GtkAction *action,
                                  gpointer   data)
{
  DesaturateDialog *dialog;
  GimpImage        *image;
  GimpDrawable     *drawable;
  GtkWidget        *widget;
  return_if_no_drawable (image, drawable, data);
  return_if_no_widget (widget, data);

  if (! gimp_drawable_is_rgb (drawable))
    {
      gimp_message (image->gimp, G_OBJECT (widget), GIMP_MESSAGE_WARNING,
                    _("Desaturate operates only on RGB color layers."));
      return;
    }

  dialog = desaturate_dialog_new (drawable,
                                  action_data_get_context (data),
                                  widget, desaturate_mode);

  g_signal_connect (dialog->dialog, "response",
                    G_CALLBACK (desaturate_response),
                    dialog);

  gtk_widget_show (dialog->dialog);
}

void
drawable_equalize_cmd_callback (GtkAction *action,
                                gpointer   data)
{
  GimpImage    *image;
  GimpDrawable *drawable;
  GtkWidget    *widget;
  return_if_no_drawable (image, drawable, data);
  return_if_no_widget (widget, data);

  if (gimp_drawable_is_indexed (drawable))
    {
      gimp_message (image->gimp, G_OBJECT (widget), GIMP_MESSAGE_WARNING,
                    _("Equalize does not operate on indexed layers."));
      return;
    }

  gimp_drawable_equalize (drawable, TRUE);
  gimp_image_flush (image);
}

void
drawable_invert_cmd_callback (GtkAction *action,
                              gpointer   data)
{
  GimpImage    *image;
  GimpDrawable *drawable;
  GtkWidget    *widget;
  return_if_no_drawable (image, drawable, data);
  return_if_no_widget (widget, data);

  if (gimp_drawable_is_indexed (drawable))
    {
      gimp_message (image->gimp, G_OBJECT (widget), GIMP_MESSAGE_WARNING,
                    _("Invert does not operate on indexed layers."));
      return;
    }

  gimp_drawable_invert (drawable);
  gimp_image_flush (image);
}

void
drawable_levels_stretch_cmd_callback (GtkAction *action,
                                      gpointer   data)
{
  GimpImage    *image;
  GimpDrawable *drawable;
  GimpContext  *context;
  GtkWidget    *widget;
  return_if_no_drawable (image, drawable, data);
  return_if_no_context (context, data);
  return_if_no_widget (widget, data);

  if (! gimp_drawable_is_rgb (drawable))
    {
      gimp_message (image->gimp, G_OBJECT (widget), GIMP_MESSAGE_WARNING,
                    _("White Balance operates only on RGB color layers."));
      return;
    }

  gimp_drawable_levels_stretch (drawable, context);
  gimp_image_flush (image);
}

void
drawable_offset_cmd_callback (GtkAction *action,
                              gpointer   data)
{
  GimpImage    *image;
  GimpDrawable *drawable;
  GtkWidget    *widget;
  GtkWidget    *dialog;
  return_if_no_drawable (image, drawable, data);
  return_if_no_widget (widget, data);

  dialog = offset_dialog_new (drawable, action_data_get_context (data),
                              widget);
  gtk_widget_show (dialog);
}


void
drawable_linked_cmd_callback (GtkAction *action,
                              gpointer   data)
{
  GimpImage    *image;
  GimpDrawable *drawable;
  gboolean      linked;
  return_if_no_drawable (image, drawable, data);

  linked = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

  if (GIMP_IS_LAYER_MASK (drawable))
    drawable =
      GIMP_DRAWABLE (gimp_layer_mask_get_layer (GIMP_LAYER_MASK (drawable)));

  if (linked != gimp_item_get_linked (GIMP_ITEM (drawable)))
    {
      GimpUndo *undo;
      gboolean  push_undo = TRUE;

      undo = gimp_image_undo_can_compress (image, GIMP_TYPE_ITEM_UNDO,
                                           GIMP_UNDO_ITEM_LINKED);

      if (undo && GIMP_ITEM_UNDO (undo)->item == GIMP_ITEM (drawable))
        push_undo = FALSE;

      gimp_item_set_linked (GIMP_ITEM (drawable), linked, push_undo);
      gimp_image_flush (image);
    }
}

void
drawable_visible_cmd_callback (GtkAction *action,
                               gpointer   data)
{
  GimpImage    *image;
  GimpDrawable *drawable;
  gboolean      visible;
  return_if_no_drawable (image, drawable, data);

  visible = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

  if (GIMP_IS_LAYER_MASK (drawable))
    drawable =
      GIMP_DRAWABLE (gimp_layer_mask_get_layer (GIMP_LAYER_MASK (drawable)));

  if (visible != gimp_item_get_visible (GIMP_ITEM (drawable)))
    {
      GimpUndo *undo;
      gboolean  push_undo = TRUE;

      undo = gimp_image_undo_can_compress (image, GIMP_TYPE_ITEM_UNDO,
                                           GIMP_UNDO_ITEM_VISIBILITY);

      if (undo && GIMP_ITEM_UNDO (undo)->item == GIMP_ITEM (drawable))
        push_undo = FALSE;

      gimp_item_set_visible (GIMP_ITEM (drawable), visible, push_undo);
      gimp_image_flush (image);
    }
}


void
drawable_flip_cmd_callback (GtkAction *action,
                            gint       value,
                            gpointer   data)
{
  GimpImage    *image;
  GimpDrawable *drawable;
  GimpItem     *item;
  GimpContext  *context;
  gint          off_x, off_y;
  gdouble       axis = 0.0;
  return_if_no_drawable (image, drawable, data);
  return_if_no_context (context, data);

  item = GIMP_ITEM (drawable);

  gimp_item_offsets (item, &off_x, &off_y);

  switch ((GimpOrientationType) value)
    {
    case GIMP_ORIENTATION_HORIZONTAL:
      axis = ((gdouble) off_x + (gdouble) gimp_item_width (item) / 2.0);
      break;

    case GIMP_ORIENTATION_VERTICAL:
      axis = ((gdouble) off_y + (gdouble) gimp_item_height (item) / 2.0);
      break;

    default:
      break;
    }

  if (gimp_item_get_linked (item))
    gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_TRANSFORM,
                                 GIMP_ITEM_GET_CLASS (item)->flip_desc);

  gimp_item_flip (item, context,
                  (GimpOrientationType) value, axis, FALSE);

  if (gimp_item_get_linked (item))
    {
      gimp_item_linked_flip (item, context,
                             (GimpOrientationType) value, axis, FALSE);
      gimp_image_undo_group_end (image);
    }

  gimp_image_flush (image);
}

void
drawable_rotate_cmd_callback (GtkAction *action,
                              gint       value,
                              gpointer   data)
{
  GimpImage    *image;
  GimpDrawable *drawable;
  GimpContext  *context;
  GimpItem     *item;
  gint          off_x, off_y;
  gdouble       center_x, center_y;
  gboolean      clip_result = FALSE;
  return_if_no_drawable (image, drawable, data);
  return_if_no_context (context, data);

  item = GIMP_ITEM (drawable);

  gimp_item_offsets (item, &off_x, &off_y);

  center_x = ((gdouble) off_x + (gdouble) gimp_item_width  (item) / 2.0);
  center_y = ((gdouble) off_y + (gdouble) gimp_item_height (item) / 2.0);

  if (gimp_item_get_linked (item))
    gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_TRANSFORM,
                                 GIMP_ITEM_GET_CLASS (item)->rotate_desc);

  if (GIMP_IS_CHANNEL (item))
    clip_result = TRUE;

  gimp_item_rotate (item, context, (GimpRotationType) value,
                    center_x, center_y, clip_result);

  if (gimp_item_get_linked (item))
    {
      gimp_item_linked_rotate (item, context, (GimpRotationType) value,
                               center_x, center_y, FALSE);
      gimp_image_undo_group_end (image);
    }

  gimp_image_flush (image);
}

/*  private functions  */

static void
desaturate_response (GtkWidget        *widget,
                     gint              response_id,
                     DesaturateDialog *dialog)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      GimpDrawable *drawable = dialog->drawable;
      GimpImage    *image   = gimp_item_get_image (GIMP_ITEM (drawable));

      /*  remember for next invocation of the dialog  */
      desaturate_mode = dialog->mode;

      gimp_drawable_desaturate (drawable, desaturate_mode);

      gimp_image_flush (image);
    }

  gtk_widget_destroy (dialog->dialog);
}
