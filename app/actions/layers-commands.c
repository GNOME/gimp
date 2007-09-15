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

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "config/gimpcoreconfig.h"

#include "core/gimp.h"
#include "core/gimpchannel-select.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimpimage-merge.h"
#include "core/gimpimage-undo.h"
#include "core/gimpimage-undo-push.h"
#include "core/gimpitemundo.h"
#include "core/gimplayer.h"
#include "core/gimplayer-floating-sel.h"
#include "core/gimplayermask.h"
#include "core/gimptoolinfo.h"
#include "core/gimpundostack.h"
#include "core/gimpprogress.h"

#include "text/gimptext.h"
#include "text/gimptext-vectors.h"
#include "text/gimptextlayer.h"

#include "vectors/gimpvectors-warp.h"

#include "widgets/gimpaction.h"
#include "widgets/gimpdock.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpprogressdialog.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"

#include "tools/gimptexttool.h"
#include "tools/tool_manager.h"

#include "dialogs/layer-add-mask-dialog.h"
#include "dialogs/layer-options-dialog.h"
#include "dialogs/resize-dialog.h"
#include "dialogs/scale-dialog.h"

#include "actions.h"
#include "layers-commands.h"

#include "gimp-intl.h"


static const GimpLayerModeEffects layer_modes[] =
{
  GIMP_NORMAL_MODE,
  GIMP_DISSOLVE_MODE,
  GIMP_MULTIPLY_MODE,
  GIMP_DIVIDE_MODE,
  GIMP_SCREEN_MODE,
  GIMP_OVERLAY_MODE,
  GIMP_DODGE_MODE,
  GIMP_BURN_MODE,
  GIMP_HARDLIGHT_MODE,
  GIMP_SOFTLIGHT_MODE,
  GIMP_GRAIN_EXTRACT_MODE,
  GIMP_GRAIN_MERGE_MODE,
  GIMP_DIFFERENCE_MODE,
  GIMP_ADDITION_MODE,
  GIMP_SUBTRACT_MODE,
  GIMP_DARKEN_ONLY_MODE,
  GIMP_LIGHTEN_ONLY_MODE,
  GIMP_HUE_MODE,
  GIMP_SATURATION_MODE,
  GIMP_COLOR_MODE,
  GIMP_VALUE_MODE
};


/*  local function prototypes  */

static void   layers_new_layer_response    (GtkWidget             *widget,
                                            gint                   response_id,
                                            LayerOptionsDialog    *dialog);
static void   layers_edit_layer_response   (GtkWidget             *widget,
                                            gint                   response_id,
                                            LayerOptionsDialog    *dialog);
static void   layers_add_mask_response     (GtkWidget             *widget,
                                            gint                   response_id,
                                            LayerAddMaskDialog    *dialog);

static void   layers_scale_layer_callback  (GtkWidget             *dialog,
                                            GimpViewable          *viewable,
                                            gint                   width,
                                            gint                   height,
                                            GimpUnit               unit,
                                            GimpInterpolationType  interpolation,
                                            gdouble                xresolution,
                                            gdouble                yresolution,
                                            GimpUnit               resolution_unit,
                                            gpointer               data);
static void   layers_resize_layer_callback (GtkWidget             *dialog,
                                            GimpViewable          *viewable,
                                            gint                   width,
                                            gint                   height,
                                            GimpUnit               unit,
                                            gint                   offset_x,
                                            gint                   offset_y,
                                            GimpItemSet            unused,
                                            gpointer               data);

static gint   layers_mode_index            (GimpLayerModeEffects   layer_mode);


/*  private variables  */

static GimpFillType           layer_fill_type     = GIMP_TRANSPARENT_FILL;
static gchar                 *layer_name          = NULL;
static GimpUnit               layer_resize_unit   = GIMP_UNIT_PIXEL;
static GimpUnit               layer_scale_unit    = GIMP_UNIT_PIXEL;
static GimpInterpolationType  layer_scale_interp  = -1;
static GimpAddMaskType        layer_add_mask_type = GIMP_ADD_WHITE_MASK;
static gboolean               layer_mask_invert   = FALSE;


/*  public functions  */

void
layers_text_tool_cmd_callback (GtkAction *action,
                               gpointer   data)
{
  GimpImage *image;
  GimpLayer *layer;
  GtkWidget *widget;
  GimpTool  *active_tool;
  return_if_no_layer (image, layer, data);
  return_if_no_widget (widget, data);

  if (! gimp_drawable_is_text_layer (GIMP_DRAWABLE (layer)))
    {
      layers_edit_attributes_cmd_callback (action, data);
      return;
    }

  active_tool = tool_manager_get_active (image->gimp);

  if (! GIMP_IS_TEXT_TOOL (active_tool))
    {
      GimpToolInfo *tool_info = gimp_get_tool_info (image->gimp,
                                                    "gimp-text-tool");

      if (GIMP_IS_TOOL_INFO (tool_info))
        {
          gimp_context_set_tool (action_data_get_context (data), tool_info);
          active_tool = tool_manager_get_active (image->gimp);
        }
    }

  if (GIMP_IS_TEXT_TOOL (active_tool))
    gimp_text_tool_set_layer (GIMP_TEXT_TOOL (active_tool), layer);
}

void
layers_edit_attributes_cmd_callback (GtkAction *action,
                                     gpointer   data)
{
  LayerOptionsDialog *dialog;
  GimpImage          *image;
  GimpLayer          *layer;
  GtkWidget          *widget;
  return_if_no_layer (image, layer, data);
  return_if_no_widget (widget, data);

  dialog = layer_options_dialog_new (gimp_item_get_image (GIMP_ITEM (layer)),
                                     layer,
                                     action_data_get_context (data),
                                     widget,
                                     gimp_object_get_name (GIMP_OBJECT (layer)),
                                     layer_fill_type,
                                     _("Layer Attributes"),
                                     "gimp-layer-edit",
                                     GTK_STOCK_EDIT,
                                     _("Edit Layer Attributes"),
                                     GIMP_HELP_LAYER_EDIT);

  g_signal_connect (dialog->dialog, "response",
                    G_CALLBACK (layers_edit_layer_response),
                    dialog);

  gtk_widget_show (dialog->dialog);
}

void
layers_new_cmd_callback (GtkAction *action,
                         gpointer   data)
{
  LayerOptionsDialog *dialog;
  GimpImage          *image;
  GtkWidget          *widget;
  GimpLayer          *floating_sel;
  return_if_no_image (image, data);
  return_if_no_widget (widget, data);

  /*  If there is a floating selection, the new command transforms
   *  the current fs into a new layer
   */
  if ((floating_sel = gimp_image_floating_sel (image)))
    {
      floating_sel_to_layer (floating_sel);
      gimp_image_flush (image);
      return;
    }

  dialog = layer_options_dialog_new (image, NULL,
                                     action_data_get_context (data),
                                     widget,
                                     layer_name ? layer_name : _("New Layer"),
                                     layer_fill_type,
                                     _("New Layer"),
                                     "gimp-layer-new",
                                     GIMP_STOCK_LAYER,
                                     _("Create a New Layer"),
                                     GIMP_HELP_LAYER_NEW);

  g_signal_connect (dialog->dialog, "response",
                    G_CALLBACK (layers_new_layer_response),
                    dialog);

  gtk_widget_show (dialog->dialog);
}

void
layers_new_last_vals_cmd_callback (GtkAction *action,
                                   gpointer   data)
{
  GimpImage            *image;
  GimpLayer            *floating_sel;
  GimpLayer            *new_layer;
  gint                  width, height;
  gint                  off_x, off_y;
  gdouble               opacity;
  GimpLayerModeEffects  mode;
  return_if_no_image (image, data);

  /*  If there is a floating selection, the new command transforms
   *  the current fs into a new layer
   */
  if ((floating_sel = gimp_image_floating_sel (image)))
    {
      floating_sel_to_layer (floating_sel);
      gimp_image_flush (image);
      return;
    }

  if (GIMP_IS_LAYER (GIMP_ACTION (action)->viewable))
    {
      GimpLayer *template = GIMP_LAYER (GIMP_ACTION (action)->viewable);

      gimp_item_offsets (GIMP_ITEM (template), &off_x, &off_y);
      width   = gimp_item_width  (GIMP_ITEM (template));
      height  = gimp_item_height (GIMP_ITEM (template));
      opacity = template->opacity;
      mode    = template->mode;
    }
  else
    {
      width   = gimp_image_get_width (image);
      height  = gimp_image_get_height (image);
      off_x   = 0;
      off_y   = 0;
      opacity = 1.0;
      mode    = GIMP_NORMAL_MODE;
    }

  gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_EDIT_PASTE,
                               _("New Layer"));

  new_layer = gimp_layer_new (image, width, height,
                              gimp_image_base_type_with_alpha (image),
                              layer_name ? layer_name : _("New Layer"),
                              opacity, mode);

  gimp_drawable_fill_by_type (GIMP_DRAWABLE (new_layer),
                              action_data_get_context (data),
                              layer_fill_type);
  gimp_item_translate (GIMP_ITEM (new_layer), off_x, off_y, FALSE);

  gimp_image_add_layer (image, new_layer, -1);

  gimp_image_undo_group_end (image);

  gimp_image_flush (image);
}

void
layers_select_cmd_callback (GtkAction *action,
                            gint       value,
                            gpointer   data)
{
  GimpImage *image;
  GimpLayer *layer;
  GimpLayer *new_layer;
  return_if_no_image (image, data);

  layer = gimp_image_get_active_layer (image);

  new_layer = (GimpLayer *) action_select_object ((GimpActionSelectType) value,
                                                  image->layers,
                                                  (GimpObject *) layer);

  if (new_layer && new_layer != layer)
    {
      gimp_image_set_active_layer (image, new_layer);
      gimp_image_flush (image);
    }
}

void
layers_raise_cmd_callback (GtkAction *action,
                           gpointer   data)
{
  GimpImage *image;
  GimpLayer *layer;
  return_if_no_layer (image, layer, data);

  gimp_image_raise_layer (image, layer);
  gimp_image_flush (image);
}

void
layers_raise_to_top_cmd_callback (GtkAction *action,
                                  gpointer   data)
{
  GimpImage *image;
  GimpLayer *layer;
  return_if_no_layer (image, layer, data);

  gimp_image_raise_layer_to_top (image, layer);
  gimp_image_flush (image);
}

void
layers_lower_cmd_callback (GtkAction *action,
                           gpointer   data)
{
  GimpImage *image;
  GimpLayer *layer;
  return_if_no_layer (image, layer, data);

  gimp_image_lower_layer (image, layer);
  gimp_image_flush (image);
}

void
layers_lower_to_bottom_cmd_callback (GtkAction *action,
                                     gpointer   data)
{
  GimpImage *image;
  GimpLayer *layer;
  return_if_no_layer (image, layer, data);

  gimp_image_lower_layer_to_bottom (image, layer);
  gimp_image_flush (image);
}

void
layers_duplicate_cmd_callback (GtkAction *action,
                               gpointer   data)
{
  GimpImage *image;
  GimpLayer *layer;
  GimpLayer *new_layer;
  return_if_no_layer (image, layer, data);

  new_layer =
    GIMP_LAYER (gimp_item_duplicate (GIMP_ITEM (layer),
                                     G_TYPE_FROM_INSTANCE (layer),
                                     TRUE));
  gimp_image_add_layer (image, new_layer, -1);

  gimp_image_flush (image);
}

void
layers_anchor_cmd_callback (GtkAction *action,
                            gpointer   data)
{
  GimpImage *image;
  GimpLayer *layer;
  return_if_no_layer (image, layer, data);

  if (gimp_layer_is_floating_sel (layer))
    {
      floating_sel_anchor (layer);
      gimp_image_flush (image);
    }
}

void
layers_merge_down_cmd_callback (GtkAction *action,
                                gpointer   data)
{
  GimpImage *image;
  GimpLayer *layer;
  return_if_no_layer (image, layer, data);

  gimp_image_merge_down (image, layer, action_data_get_context (data),
                         GIMP_EXPAND_AS_NECESSARY);
  gimp_image_flush (image);
}

void
layers_delete_cmd_callback (GtkAction *action,
                            gpointer   data)
{
  GimpImage *image;
  GimpLayer *layer;
  return_if_no_layer (image, layer, data);

  if (gimp_layer_is_floating_sel (layer))
    floating_sel_remove (layer);
  else
    gimp_image_remove_layer (image, layer);

  gimp_image_flush (image);
}

void
layers_text_discard_cmd_callback (GtkAction *action,
                                  gpointer   data)
{
  GimpImage *image;
  GimpLayer *layer;
  return_if_no_layer (image, layer, data);

  if (GIMP_IS_TEXT_LAYER (layer))
    gimp_text_layer_discard (GIMP_TEXT_LAYER (layer));
}

void
layers_text_to_vectors_cmd_callback (GtkAction *action,
                                     gpointer   data)
{
  GimpImage *image;
  GimpLayer *layer;
  return_if_no_layer (image, layer, data);

  if (GIMP_IS_TEXT_LAYER (layer))
    {
      GimpVectors *vectors;
      gint         x, y;

      vectors = gimp_text_vectors_new (image, GIMP_TEXT_LAYER (layer)->text);

      gimp_item_offsets (GIMP_ITEM (layer), &x, &y);
      gimp_item_translate (GIMP_ITEM (vectors), x, y, FALSE);

      gimp_image_add_vectors (image, vectors, -1);
      gimp_image_set_active_vectors (image, vectors);

      gimp_image_flush (image);
    }
}

void
layers_text_along_vectors_cmd_callback (GtkAction *action,
                                        gpointer   data)
{
  GimpImage   *image;
  GimpLayer   *layer;
  GimpVectors *vectors;
  return_if_no_layer (image, layer, data);
  return_if_no_vectors (image, vectors, data);

  if (GIMP_IS_TEXT_LAYER (layer))
    {
      GimpVectors *new_vectors;

      new_vectors = gimp_text_vectors_new (image, GIMP_TEXT_LAYER (layer)->text);

      gimp_vectors_warp_vectors (vectors, new_vectors,
                                 0.5 * gimp_item_height (GIMP_ITEM (layer)));

      gimp_item_set_visible (GIMP_ITEM (new_vectors), TRUE, FALSE);

      gimp_image_add_vectors (image, new_vectors, -1);
      gimp_image_set_active_vectors (image, new_vectors);

      gimp_image_flush (image);
    }
}

void
layers_resize_cmd_callback (GtkAction *action,
                            gpointer   data)
{
  GimpImage *image;
  GimpLayer *layer;
  GtkWidget *widget;
  GtkWidget *dialog;
  return_if_no_layer (image, layer, data);
  return_if_no_widget (widget, data);

  if (layer_resize_unit != GIMP_UNIT_PERCENT && GIMP_IS_DISPLAY (data))
    layer_resize_unit = GIMP_DISPLAY_SHELL (GIMP_DISPLAY (data)->shell)->unit;

  dialog = resize_dialog_new (GIMP_VIEWABLE (layer),
                              action_data_get_context (data),
                              _("Set Layer Boundary Size"), "gimp-layer-resize",
                              widget,
                              gimp_standard_help_func, GIMP_HELP_LAYER_RESIZE,
                              layer_resize_unit,
                              layers_resize_layer_callback,
                              action_data_get_context (data));

  gtk_widget_show (dialog);
}

void
layers_resize_to_image_cmd_callback (GtkAction *action,
                                     gpointer   data)
{
  GimpImage *image;
  GimpLayer *layer;
  return_if_no_layer (image, layer, data);

  gimp_layer_resize_to_image (layer, action_data_get_context (data));
  gimp_image_flush (image);
}

void
layers_scale_cmd_callback (GtkAction *action,
                           gpointer   data)
{
  GimpImage *image;
  GimpLayer *layer;
  GtkWidget *widget;
  GtkWidget *dialog;
  return_if_no_layer (image, layer, data);
  return_if_no_widget (widget, data);

  if (layer_scale_unit != GIMP_UNIT_PERCENT && GIMP_IS_DISPLAY (data))
    layer_scale_unit = GIMP_DISPLAY_SHELL (GIMP_DISPLAY (data)->shell)->unit;

  if (layer_scale_interp == -1)
    layer_scale_interp = image->gimp->config->interpolation_type;

  dialog = scale_dialog_new (GIMP_VIEWABLE (layer),
                             action_data_get_context (data),
                             _("Scale Layer"), "gimp-layer-scale",
                             widget,
                             gimp_standard_help_func, GIMP_HELP_LAYER_SCALE,
                             layer_scale_unit,
                             layer_scale_interp,
                             layers_scale_layer_callback,
                             GIMP_IS_DISPLAY (data) ? data : NULL);

  gtk_widget_show (dialog);
}

void
layers_crop_cmd_callback (GtkAction *action,
                          gpointer   data)
{
  GimpImage *image;
  GimpLayer *layer;
  GtkWidget *widget;
  gint       x1, y1, x2, y2;
  gint       off_x, off_y;
  return_if_no_layer (image, layer, data);
  return_if_no_widget (widget, data);

  if (! gimp_channel_bounds (gimp_image_get_mask (image),
                             &x1, &y1, &x2, &y2))
    {
      gimp_message (image->gimp, G_OBJECT (widget), GIMP_MESSAGE_WARNING,
                    _("Cannot crop because the current selection is empty."));
      return;
    }

  gimp_item_offsets (GIMP_ITEM (layer), &off_x, &off_y);

  off_x -= x1;
  off_y -= y1;

  gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_ITEM_RESIZE,
                               _("Crop Layer"));

  gimp_item_resize (GIMP_ITEM (layer), action_data_get_context (data),
                    x2 - x1, y2 - y1, off_x, off_y);

  gimp_image_undo_group_end (image);

  gimp_image_flush (image);
}

void
layers_mask_add_cmd_callback (GtkAction *action,
                              gpointer   data)
{
  LayerAddMaskDialog *dialog;
  GimpImage          *image;
  GimpLayer          *layer;
  GtkWidget          *widget;
  return_if_no_layer (image, layer, data);
  return_if_no_widget (widget, data);

  dialog = layer_add_mask_dialog_new (layer, action_data_get_context (data),
                                      widget,
                                      layer_add_mask_type, layer_mask_invert);

  g_signal_connect (dialog->dialog, "response",
                    G_CALLBACK (layers_add_mask_response),
                    dialog);

  gtk_widget_show (dialog->dialog);
}

void
layers_mask_apply_cmd_callback (GtkAction *action,
                                gint       value,
                                gpointer   data)
{
  GimpImage         *image;
  GimpLayer         *layer;
  GimpMaskApplyMode  mode;
  return_if_no_layer (image, layer, data);

  mode = (GimpMaskApplyMode) value;

  if (gimp_layer_get_mask (layer))
    {
      gimp_layer_apply_mask (layer, mode, TRUE);
      gimp_image_flush (image);
    }
}

void
layers_mask_edit_cmd_callback (GtkAction *action,
                               gpointer   data)
{
  GimpImage     *image;
  GimpLayer     *layer;
  GimpLayerMask *mask;
  return_if_no_layer (image, layer, data);

  mask = gimp_layer_get_mask (layer);

  if (mask)
    {
      gboolean active;

      active = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

      gimp_layer_mask_set_edit (mask, active);
      gimp_image_flush (image);
    }
}

void
layers_mask_show_cmd_callback (GtkAction *action,
                               gpointer   data)
{
  GimpImage     *image;
  GimpLayer     *layer;
  GimpLayerMask *mask;
  return_if_no_layer (image, layer, data);

  mask = gimp_layer_get_mask (layer);

  if (mask)
    {
      gboolean active;

      active = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

      gimp_layer_mask_set_show (mask, active, TRUE);
      gimp_image_flush (image);
    }
}

void
layers_mask_disable_cmd_callback (GtkAction *action,
                                  gpointer   data)
{
  GimpImage     *image;
  GimpLayer     *layer;
  GimpLayerMask *mask;
  return_if_no_layer (image, layer, data);

  mask = gimp_layer_get_mask (layer);

  if (mask)
    {
      gboolean active;

      active = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

      gimp_layer_mask_set_apply (mask, ! active, TRUE);
      gimp_image_flush (image);
    }
}

void
layers_mask_to_selection_cmd_callback (GtkAction *action,
                                       gint       value,
                                       gpointer   data)
{
  GimpChannelOps  op;
  GimpImage      *image;
  GimpLayer      *layer;
  GimpLayerMask  *mask;
  return_if_no_layer (image, layer, data);

  op = (GimpChannelOps) value;

  mask = gimp_layer_get_mask (layer);

  if (mask)
    {
      gint off_x, off_y;

      gimp_item_offsets (GIMP_ITEM (mask), &off_x, &off_y);

      gimp_channel_select_channel (gimp_image_get_mask (image),
                                   _("Layer Mask to Selection"),
                                   GIMP_CHANNEL (mask),
                                   off_x, off_y,
                                   op, FALSE, 0.0, 0.0);
      gimp_image_flush (image);
    }
}

void
layers_alpha_add_cmd_callback (GtkAction *action,
                               gpointer   data)
{
  GimpImage *image;
  GimpLayer *layer;
  return_if_no_layer (image, layer, data);

  if (! gimp_drawable_has_alpha (GIMP_DRAWABLE (layer)))
    {
      gimp_layer_add_alpha (layer);
      gimp_image_flush (image);
    }
}

void
layers_alpha_remove_cmd_callback (GtkAction *action,
                                  gpointer   data)
{
  GimpImage *image;
  GimpLayer *layer;
  return_if_no_layer (image, layer, data);

  if (gimp_drawable_has_alpha (GIMP_DRAWABLE (layer)))
    {
      gimp_layer_flatten (layer, action_data_get_context (data));
      gimp_image_flush (image);
    }
}

void
layers_alpha_to_selection_cmd_callback (GtkAction *action,
                                        gint       value,
                                        gpointer   data)
{
  GimpChannelOps  op;
  GimpImage      *image;
  GimpLayer      *layer;
  return_if_no_layer (image, layer, data);

  op = (GimpChannelOps) value;

  gimp_channel_select_alpha (gimp_image_get_mask (image),
                             GIMP_DRAWABLE (layer),
                             op, FALSE, 0.0, 0.0);
  gimp_image_flush (image);
}

void
layers_opacity_cmd_callback (GtkAction *action,
                             gint       value,
                             gpointer   data)
{
  GimpImage      *image;
  GimpLayer      *layer;
  gdouble         opacity;
  GimpUndo       *undo;
  gboolean        push_undo = TRUE;
  return_if_no_layer (image, layer, data);

  undo = gimp_image_undo_can_compress (image, GIMP_TYPE_ITEM_UNDO,
                                       GIMP_UNDO_LAYER_OPACITY);

  if (undo && GIMP_ITEM_UNDO (undo)->item == GIMP_ITEM (layer))
    push_undo = FALSE;

  opacity = action_select_value ((GimpActionSelectType) value,
                                 gimp_layer_get_opacity (layer),
                                 0.0, 1.0,
                                 1.0 / 255.0, 0.01, 0.1, 0.0, FALSE);
  gimp_layer_set_opacity (layer, opacity, push_undo);
  gimp_image_flush (image);
}

void
layers_mode_cmd_callback (GtkAction *action,
                          gint       value,
                          gpointer   data)
{
  GimpImage            *image;
  GimpLayer            *layer;
  GimpLayerModeEffects  layer_mode;
  gint                  index;
  GimpUndo             *undo;
  gboolean              push_undo = TRUE;
  return_if_no_layer (image, layer, data);

  undo = gimp_image_undo_can_compress (image, GIMP_TYPE_ITEM_UNDO,
                                       GIMP_UNDO_LAYER_MODE);

  if (undo && GIMP_ITEM_UNDO (undo)->item == GIMP_ITEM (layer))
    push_undo = FALSE;

  layer_mode = gimp_layer_get_mode (layer);

  index = action_select_value ((GimpActionSelectType) value,
                               layers_mode_index (layer_mode),
                               0, G_N_ELEMENTS (layer_modes) - 1,
                               0.0, 1.0, 1.0, 0.0, FALSE);
  gimp_layer_set_mode (layer, layer_modes[index], push_undo);
  gimp_image_flush (image);
}

void
layers_lock_alpha_cmd_callback (GtkAction *action,
                                gpointer   data)
{
  GimpImage *image;
  GimpLayer *layer;
  gboolean   lock_alpha;
  return_if_no_layer (image, layer, data);

  lock_alpha = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

  if (lock_alpha != gimp_layer_get_lock_alpha (layer))
    {
      GimpUndo *undo;
      gboolean  push_undo = TRUE;

      undo = gimp_image_undo_can_compress (image, GIMP_TYPE_ITEM_UNDO,
                                           GIMP_UNDO_LAYER_LOCK_ALPHA);

      if (undo && GIMP_ITEM_UNDO (undo)->item == GIMP_ITEM (layer))
        push_undo = FALSE;

      gimp_layer_set_lock_alpha (layer, lock_alpha, push_undo);
      gimp_image_flush (image);
    }
}


/*  private functions  */

static void
layers_new_layer_response (GtkWidget          *widget,
                           gint                response_id,
                           LayerOptionsDialog *dialog)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      GimpLayer *layer;

      if (layer_name)
        g_free (layer_name);
      layer_name =
        g_strdup (gtk_entry_get_text (GTK_ENTRY (dialog->name_entry)));

      layer_fill_type = dialog->fill_type;

      dialog->xsize =
        RINT (gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (dialog->size_se),
                                          0));
      dialog->ysize =
        RINT (gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (dialog->size_se),
                                          1));

      layer = gimp_layer_new (dialog->image,
                              dialog->xsize,
                              dialog->ysize,
                              gimp_image_base_type_with_alpha (dialog->image),
                              layer_name,
                              GIMP_OPACITY_OPAQUE, GIMP_NORMAL_MODE);

      if (layer)
        {
          gimp_drawable_fill_by_type (GIMP_DRAWABLE (layer),
                                      dialog->context,
                                      layer_fill_type);
          gimp_image_add_layer (dialog->image, layer, -1);

          gimp_image_flush (dialog->image);
        }
      else
        {
          g_warning ("%s: could not allocate new layer", G_STRFUNC);
        }
    }

  gtk_widget_destroy (dialog->dialog);
}

static void
layers_edit_layer_response (GtkWidget          *widget,
                            gint                response_id,
                            LayerOptionsDialog *dialog)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      GimpLayer   *layer = dialog->layer;
      const gchar *new_name;

      new_name = gtk_entry_get_text (GTK_ENTRY (dialog->name_entry));

      if (strcmp (new_name, gimp_object_get_name (GIMP_OBJECT (layer))))
        {
          gimp_item_rename (GIMP_ITEM (layer), new_name);
          gimp_image_flush (dialog->image);
        }

      if (dialog->rename_toggle &&
          gimp_drawable_is_text_layer (GIMP_DRAWABLE (layer)))
        {
          g_object_set (layer,
                        "auto-rename",
                        GTK_TOGGLE_BUTTON (dialog->rename_toggle)->active,
                        NULL);
        }
    }

  gtk_widget_destroy (dialog->dialog);
}

static void
layers_add_mask_response (GtkWidget          *widget,
                          gint                response_id,
                          LayerAddMaskDialog *dialog)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      GimpLayer     *layer = dialog->layer;
      GimpImage     *image = gimp_item_get_image (GIMP_ITEM (layer));
      GimpLayerMask *mask;

      if (dialog->add_mask_type == GIMP_ADD_CHANNEL_MASK &&
          ! dialog->channel)
        {
          gimp_message (image->gimp, G_OBJECT (widget), GIMP_MESSAGE_WARNING,
                        _("Please select a channel first"));
          return;
        }

      layer_add_mask_type = dialog->add_mask_type;
      layer_mask_invert   = dialog->invert;

      gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_LAYER_ADD_MASK,
                                   _("Add Layer Mask"));

      mask = gimp_layer_create_mask (layer, layer_add_mask_type,
                                     dialog->channel);

      if (layer_mask_invert)
        gimp_channel_invert (GIMP_CHANNEL (mask), FALSE);

      gimp_layer_add_mask (layer, mask, TRUE);

      gimp_image_undo_group_end (image);

      gimp_image_flush (image);
    }

  gtk_widget_destroy (dialog->dialog);
}

static void
layers_scale_layer_callback (GtkWidget             *dialog,
                             GimpViewable          *viewable,
                             gint                   width,
                             gint                   height,
                             GimpUnit               unit,
                             GimpInterpolationType  interpolation,
                             gdouble                xresolution,    /* unused */
                             gdouble                yresolution,    /* unused */
                             GimpUnit               resolution_unit,/* unused */
                             gpointer               data)
{
  GimpDisplay *display = GIMP_DISPLAY (data);

  layer_scale_unit   = unit;
  layer_scale_interp = interpolation;

  if (width > 0 && height > 0)
    {
      GimpItem     *item = GIMP_ITEM (viewable);
      GimpProgress *progress;
      GtkWidget    *progress_dialog = NULL;

      gtk_widget_destroy (dialog);

      if (width  == gimp_item_width (item) &&
          height == gimp_item_height (item))
        return;

      if (display)
        {
          progress = GIMP_PROGRESS (display);
        }
      else
        {
          progress_dialog = gimp_progress_dialog_new ();
          progress = GIMP_PROGRESS (progress_dialog);
        }

      progress = gimp_progress_start (progress, _("Scaling"), FALSE);

      gimp_item_scale_by_origin (item,
                                 width, height, interpolation,
                                 progress, TRUE);

      if (progress)
        gimp_progress_end (progress);

      if (progress_dialog)
        gtk_widget_destroy (progress_dialog);

      gimp_image_flush (gimp_item_get_image (item));
    }
  else
    {
      g_warning ("Scale Error: "
                 "Both width and height must be greater than zero.");
    }
}

static void
layers_resize_layer_callback (GtkWidget    *dialog,
                              GimpViewable *viewable,
                              gint          width,
                              gint          height,
                              GimpUnit      unit,
                              gint          offset_x,
                              gint          offset_y,
                              GimpItemSet   unused,
                              gpointer      data)
{
  GimpContext *context = GIMP_CONTEXT (data);

  layer_resize_unit = unit;

  if (width > 0 && height > 0)
    {
      GimpItem *item = GIMP_ITEM (viewable);

      gtk_widget_destroy (dialog);

      if (width  == gimp_item_width (item) &&
          height == gimp_item_height (item))
        return;

      gimp_item_resize (item, context,
                        width, height, offset_x, offset_y);

      gimp_image_flush (gimp_item_get_image (item));
    }
  else
    {
      g_warning ("Resize Error: "
                 "Both width and height must be greater than zero.");
    }
}

static gint
layers_mode_index (GimpLayerModeEffects layer_mode)
{
  gint i = 0;

  while (i < (G_N_ELEMENTS (layer_modes) - 1) && layer_modes[i] != layer_mode)
    i++;

  return i;
}
