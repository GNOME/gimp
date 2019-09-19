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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "config/gimpdialogconfig.h"

#include "operations/layer-modes/gimp-layer-modes.h"

#include "core/gimp.h"
#include "core/gimpchannel.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpdrawable-fill.h"
#include "core/gimpgrouplayer.h"
#include "core/gimpimage.h"
#include "core/gimpimage-merge.h"
#include "core/gimpimage-undo.h"
#include "core/gimpimage-undo-push.h"
#include "core/gimpitemundo.h"
#include "core/gimplayerpropundo.h"
#include "core/gimplayer-floating-selection.h"
#include "core/gimplayer-new.h"
#include "core/gimppickable.h"
#include "core/gimppickable-auto-shrink.h"
#include "core/gimptoolinfo.h"
#include "core/gimpundostack.h"
#include "core/gimpprogress.h"

#include "text/gimptext.h"
#include "text/gimptext-vectors.h"
#include "text/gimptextlayer.h"

#include "vectors/gimpstroke.h"
#include "vectors/gimpvectors.h"
#include "vectors/gimpvectors-warp.h"

#include "widgets/gimpaction.h"
#include "widgets/gimpdock.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpprogressdialog.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpimagewindow.h"

#include "tools/gimptexttool.h"
#include "tools/tool_manager.h"

#include "dialogs/dialogs.h"
#include "dialogs/layer-add-mask-dialog.h"
#include "dialogs/layer-options-dialog.h"
#include "dialogs/resize-dialog.h"
#include "dialogs/scale-dialog.h"

#include "actions.h"
#include "items-commands.h"
#include "layers-commands.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   layers_new_callback             (GtkWidget             *dialog,
                                               GimpImage             *image,
                                               GimpLayer             *layer,
                                               GimpContext           *context,
                                               const gchar           *layer_name,
                                               GimpLayerMode          layer_mode,
                                               GimpLayerColorSpace    layer_blend_space,
                                               GimpLayerColorSpace    layer_composite_space,
                                               GimpLayerCompositeMode layer_composite_mode,
                                               gdouble                layer_opacity,
                                               GimpFillType           layer_fill_type,
                                               gint                   layer_width,
                                               gint                   layer_height,
                                               gint                   layer_offset_x,
                                               gint                   layer_offset_y,
                                               gboolean               layer_visible,
                                               gboolean               layer_linked,
                                               GimpColorTag           layer_color_tag,
                                               gboolean               layer_lock_pixels,
                                               gboolean               layer_lock_position,
                                               gboolean               layer_lock_alpha,
                                               gboolean               rename_text_layer,
                                               gpointer               user_data);
static void   layers_edit_attributes_callback (GtkWidget             *dialog,
                                               GimpImage             *image,
                                               GimpLayer             *layer,
                                               GimpContext           *context,
                                               const gchar           *layer_name,
                                               GimpLayerMode          layer_mode,
                                               GimpLayerColorSpace    layer_blend_space,
                                               GimpLayerColorSpace    layer_composite_space,
                                               GimpLayerCompositeMode layer_composite_mode,
                                               gdouble                layer_opacity,
                                               GimpFillType           layer_fill_type,
                                               gint                   layer_width,
                                               gint                   layer_height,
                                               gint                   layer_offset_x,
                                               gint                   layer_offset_y,
                                               gboolean               layer_visible,
                                               gboolean               layer_linked,
                                               GimpColorTag           layer_color_tag,
                                               gboolean               layer_lock_pixels,
                                               gboolean               layer_lock_position,
                                               gboolean               layer_lock_alpha,
                                               gboolean               rename_text_layer,
                                               gpointer               user_data);
static void   layers_add_mask_callback        (GtkWidget             *dialog,
                                               GimpLayer             *layer,
                                               GimpAddMaskType        add_mask_type,
                                               GimpChannel           *channel,
                                               gboolean               invert,
                                               gpointer               user_data);
static void   layers_scale_callback           (GtkWidget             *dialog,
                                               GimpViewable          *viewable,
                                               gint                   width,
                                               gint                   height,
                                               GimpUnit               unit,
                                               GimpInterpolationType  interpolation,
                                               gdouble                xresolution,
                                               gdouble                yresolution,
                                               GimpUnit               resolution_unit,
                                               gpointer               user_data);
static void   layers_resize_callback          (GtkWidget             *dialog,
                                               GimpViewable          *viewable,
                                               GimpContext           *context,
                                               gint                   width,
                                               gint                   height,
                                               GimpUnit               unit,
                                               gint                   offset_x,
                                               gint                   offset_y,
                                               GimpFillType           fill_type,
                                               GimpItemSet            unused,
                                               gboolean               unused2,
                                               gpointer               data);

static gint   layers_mode_index               (GimpLayerMode          layer_mode,
                                               const GimpLayerMode   *modes,
                                               gint                   n_modes);


/*  private variables  */

static GimpUnit               layer_resize_unit   = GIMP_UNIT_PIXEL;
static GimpUnit               layer_scale_unit    = GIMP_UNIT_PIXEL;
static GimpInterpolationType  layer_scale_interp  = -1;


/*  public functions  */

void
layers_edit_cmd_callback (GimpAction *action,
                          GVariant   *value,
                          gpointer    data)
{
  GimpImage *image;
  GimpLayer *layer;
  GtkWidget *widget;
  return_if_no_layer (image, layer, data);
  return_if_no_widget (widget, data);

  if (gimp_item_is_text_layer (GIMP_ITEM (layer)))
    {
      layers_edit_text_cmd_callback (action, value, data);
    }
  else
    {
      layers_edit_attributes_cmd_callback (action, value, data);
    }
}

void
layers_edit_text_cmd_callback (GimpAction *action,
                               GVariant   *value,
                               gpointer    data)
{
  GimpImage *image;
  GimpLayer *layer;
  GtkWidget *widget;
  GimpTool  *active_tool;
  return_if_no_layer (image, layer, data);
  return_if_no_widget (widget, data);

  g_return_if_fail (gimp_item_is_text_layer (GIMP_ITEM (layer)));

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
    {
      if (gimp_text_tool_set_layer (GIMP_TEXT_TOOL (active_tool), layer))
        {
          GimpDisplayShell *shell;

          shell = gimp_display_get_shell (active_tool->display);
          gtk_widget_grab_focus (shell->canvas);
        }
    }
}

void
layers_edit_attributes_cmd_callback (GimpAction *action,
                                     GVariant   *value,
                                     gpointer    data)
{
  GimpImage *image;
  GimpLayer *layer;
  GtkWidget *widget;
  GtkWidget *dialog;
  return_if_no_layer (image, layer, data);
  return_if_no_widget (widget, data);

#define EDIT_DIALOG_KEY "gimp-layer-edit-attributes-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (layer), EDIT_DIALOG_KEY);

  if (! dialog)
    {
      GimpItem *item = GIMP_ITEM (layer);

      dialog = layer_options_dialog_new (gimp_item_get_image (GIMP_ITEM (layer)),
                                         layer,
                                         action_data_get_context (data),
                                         widget,
                                         _("Layer Attributes"),
                                         "gimp-layer-edit",
                                         GIMP_ICON_EDIT,
                                         _("Edit Layer Attributes"),
                                         GIMP_HELP_LAYER_EDIT,
                                         gimp_object_get_name (layer),
                                         gimp_layer_get_mode (layer),
                                         gimp_layer_get_blend_space (layer),
                                         gimp_layer_get_composite_space (layer),
                                         gimp_layer_get_composite_mode (layer),
                                         gimp_layer_get_opacity (layer),
                                         0 /* unused */,
                                         gimp_item_get_visible (item),
                                         gimp_item_get_linked (item),
                                         gimp_item_get_color_tag (item),
                                         gimp_item_get_lock_content (item),
                                         gimp_item_get_lock_position (item),
                                         gimp_layer_get_lock_alpha (layer),
                                         layers_edit_attributes_callback,
                                         NULL);

      dialogs_attach_dialog (G_OBJECT (layer), EDIT_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

void
layers_new_cmd_callback (GimpAction *action,
                         GVariant   *value,
                         gpointer    data)
{
  GimpImage *image;
  GtkWidget *widget;
  GimpLayer *floating_sel;
  GtkWidget *dialog;
  return_if_no_image (image, data);
  return_if_no_widget (widget, data);

  /*  If there is a floating selection, the new command transforms
   *  the current fs into a new layer
   */
  if ((floating_sel = gimp_image_get_floating_selection (image)))
    {
      GError *error = NULL;

      if (! floating_sel_to_layer (floating_sel, &error))
        {
          gimp_message_literal (image->gimp,
                                G_OBJECT (widget), GIMP_MESSAGE_WARNING,
                                error->message);
          g_clear_error (&error);
          return;
        }

      gimp_image_flush (image);
      return;
    }

#define NEW_DIALOG_KEY "gimp-layer-new-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (image), NEW_DIALOG_KEY);

  if (! dialog)
    {
      GimpDialogConfig *config     = GIMP_DIALOG_CONFIG (image->gimp->config);
      GimpLayerMode     layer_mode = config->layer_new_mode;

      if (layer_mode == GIMP_LAYER_MODE_NORMAL ||
          layer_mode == GIMP_LAYER_MODE_NORMAL_LEGACY)
        {
          layer_mode = gimp_image_get_default_new_layer_mode (image);
        }

      dialog = layer_options_dialog_new (image, NULL,
                                         action_data_get_context (data),
                                         widget,
                                         _("New Layer"),
                                         "gimp-layer-new",
                                         GIMP_ICON_LAYER,
                                         _("Create a New Layer"),
                                         GIMP_HELP_LAYER_NEW,
                                         config->layer_new_name,
                                         layer_mode,
                                         config->layer_new_blend_space,
                                         config->layer_new_composite_space,
                                         config->layer_new_composite_mode,
                                         config->layer_new_opacity,
                                         config->layer_new_fill_type,
                                         TRUE,
                                         FALSE,
                                         GIMP_COLOR_TAG_NONE,
                                         FALSE,
                                         FALSE,
                                         FALSE,
                                         layers_new_callback,
                                         NULL);

      dialogs_attach_dialog (G_OBJECT (image), NEW_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

void
layers_new_last_vals_cmd_callback (GimpAction *action,
                                   GVariant   *value,
                                   gpointer    data)
{
  GimpImage        *image;
  GtkWidget        *widget;
  GimpLayer        *layer;
  GimpDialogConfig *config;
  GimpLayerMode     layer_mode;

  return_if_no_image (image, data);
  return_if_no_widget (widget, data);

  config = GIMP_DIALOG_CONFIG (image->gimp->config);

  /*  If there is a floating selection, the new command transforms
   *  the current fs into a new layer
   */
  if (gimp_image_get_floating_selection (image))
    {
      layers_new_cmd_callback (action, value, data);
      return;
    }

  layer_mode = config->layer_new_mode;

  if (layer_mode == GIMP_LAYER_MODE_NORMAL ||
      layer_mode == GIMP_LAYER_MODE_NORMAL_LEGACY)
    {
      layer_mode = gimp_image_get_default_new_layer_mode (image);
    }

  layer = gimp_layer_new (image,
                          gimp_image_get_width  (image),
                          gimp_image_get_height (image),
                          gimp_image_get_layer_format (image, TRUE),
                          config->layer_new_name,
                          config->layer_new_opacity,
                          layer_mode);

  gimp_drawable_fill (GIMP_DRAWABLE (layer),
                      action_data_get_context (data),
                      config->layer_new_fill_type);
  gimp_layer_set_blend_space (layer,
                              config->layer_new_blend_space, FALSE);
  gimp_layer_set_composite_space (layer,
                                  config->layer_new_composite_space, FALSE);
  gimp_layer_set_composite_mode (layer,
                                 config->layer_new_composite_mode, FALSE);

  gimp_image_add_layer (image, layer, GIMP_IMAGE_ACTIVE_PARENT, -1, TRUE);
  gimp_image_flush (image);
}

void
layers_new_from_visible_cmd_callback (GimpAction *action,
                                      GVariant   *value,
                                      gpointer    data)
{
  GimpImage        *image;
  GimpDisplayShell *shell;
  GimpLayer        *layer;
  GimpPickable     *pickable;
  GimpColorProfile *profile;
  return_if_no_image (image, data);
  return_if_no_shell (shell, data);

  pickable = gimp_display_shell_get_canvas_pickable (shell);

  gimp_pickable_flush (pickable);

  profile = gimp_color_managed_get_color_profile (GIMP_COLOR_MANAGED (image));

  layer = gimp_layer_new_from_gegl_buffer (gimp_pickable_get_buffer (pickable),
                                           image,
                                           gimp_image_get_layer_format (image,
                                                                        TRUE),
                                           _("Visible"),
                                           GIMP_OPACITY_OPAQUE,
                                           gimp_image_get_default_new_layer_mode (image),
                                           profile);

  gimp_image_add_layer (image, layer, GIMP_IMAGE_ACTIVE_PARENT, -1, TRUE);
  gimp_image_flush (image);
}

void
layers_new_group_cmd_callback (GimpAction *action,
                               GVariant   *value,
                               gpointer    data)
{
  GimpImage *image;
  GimpLayer *layer;
  return_if_no_image (image, data);

  layer = gimp_group_layer_new (image);

  gimp_image_add_layer (image, layer, GIMP_IMAGE_ACTIVE_PARENT, -1, TRUE);
  gimp_image_flush (image);
}

void
layers_select_cmd_callback (GimpAction *action,
                            GVariant   *value,
                            gpointer    data)
{
  GimpImage            *image;
  GimpLayer            *layer;
  GimpContainer        *container;
  GimpLayer            *new_layer;
  GimpActionSelectType  select_type;
  return_if_no_image (image, data);

  select_type = (GimpActionSelectType) g_variant_get_int32 (value);

  layer = gimp_image_get_active_layer (image);

  if (layer)
    container = gimp_item_get_container (GIMP_ITEM (layer));
  else
    container = gimp_image_get_layers (image);

  new_layer = (GimpLayer *) action_select_object (select_type,
                                                  container,
                                                  (GimpObject *) layer);

  if (new_layer && new_layer != layer)
    {
      gimp_image_set_active_layer (image, new_layer);
      gimp_image_flush (image);
    }
}

void
layers_raise_cmd_callback (GimpAction *action,
                           GVariant   *value,
                           gpointer    data)
{
  GimpImage *image;
  GimpLayer *layer;
  return_if_no_layer (image, layer, data);

  gimp_image_raise_item (image, GIMP_ITEM (layer), NULL);
  gimp_image_flush (image);
}

void
layers_raise_to_top_cmd_callback (GimpAction *action,
                                  GVariant   *value,
                                  gpointer    data)
{
  GimpImage *image;
  GimpLayer *layer;
  return_if_no_layer (image, layer, data);

  gimp_image_raise_item_to_top (image, GIMP_ITEM (layer));
  gimp_image_flush (image);
}

void
layers_lower_cmd_callback (GimpAction *action,
                           GVariant   *value,
                           gpointer    data)
{
  GimpImage *image;
  GimpLayer *layer;
  return_if_no_layer (image, layer, data);

  gimp_image_lower_item (image, GIMP_ITEM (layer), NULL);
  gimp_image_flush (image);
}

void
layers_lower_to_bottom_cmd_callback (GimpAction *action,
                                     GVariant   *value,
                                     gpointer    data)
{
  GimpImage *image;
  GimpLayer *layer;
  return_if_no_layer (image, layer, data);

  gimp_image_lower_item_to_bottom (image, GIMP_ITEM (layer));
  gimp_image_flush (image);
}

void
layers_duplicate_cmd_callback (GimpAction *action,
                               GVariant   *value,
                               gpointer    data)
{
  GimpImage *image;
  GimpLayer *layer;
  GimpLayer *new_layer;
  return_if_no_layer (image, layer, data);

  new_layer = GIMP_LAYER (gimp_item_duplicate (GIMP_ITEM (layer),
                                               G_TYPE_FROM_INSTANCE (layer)));

  /*  use the actual parent here, not GIMP_IMAGE_ACTIVE_PARENT because
   *  the latter would add a duplicated group inside itself instead of
   *  above it
   */
  gimp_image_add_layer (image, new_layer,
                        gimp_layer_get_parent (layer), -1,
                        TRUE);
  gimp_image_flush (image);
}

void
layers_anchor_cmd_callback (GimpAction *action,
                            GVariant   *value,
                            gpointer    data)
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
layers_merge_down_cmd_callback (GimpAction *action,
                                GVariant   *value,
                                gpointer    data)
{
  GimpImage   *image;
  GimpLayer   *layer;
  GimpDisplay *display;
  return_if_no_layer (image, layer, data);
  return_if_no_display (display, data);

  gimp_image_merge_down (image, layer, action_data_get_context (data),
                         GIMP_EXPAND_AS_NECESSARY,
                         GIMP_PROGRESS (display), NULL);
  gimp_image_flush (image);
}

void
layers_merge_group_cmd_callback (GimpAction *action,
                                 GVariant   *value,
                                 gpointer    data)
{
  GimpImage *image;
  GimpLayer *layer;
  return_if_no_layer (image, layer, data);

  gimp_image_merge_group_layer (image, GIMP_GROUP_LAYER (layer));
  gimp_image_flush (image);
}

void
layers_delete_cmd_callback (GimpAction *action,
                            GVariant   *value,
                            gpointer    data)
{
  GimpImage *image;
  GimpLayer *layer;
  return_if_no_layer (image, layer, data);

  gimp_image_remove_layer (image, layer, TRUE, NULL);
  gimp_image_flush (image);
}

void
layers_text_discard_cmd_callback (GimpAction *action,
                                  GVariant   *value,
                                  gpointer    data)
{
  GimpImage *image;
  GimpLayer *layer;
  return_if_no_layer (image, layer, data);

  if (GIMP_IS_TEXT_LAYER (layer))
    gimp_text_layer_discard (GIMP_TEXT_LAYER (layer));
}

void
layers_text_to_vectors_cmd_callback (GimpAction *action,
                                     GVariant   *value,
                                     gpointer    data)
{
  GimpImage *image;
  GimpLayer *layer;
  return_if_no_layer (image, layer, data);

  if (GIMP_IS_TEXT_LAYER (layer))
    {
      GimpVectors *vectors;
      gint         x, y;

      vectors = gimp_text_vectors_new (image, GIMP_TEXT_LAYER (layer)->text);

      gimp_item_get_offset (GIMP_ITEM (layer), &x, &y);
      gimp_item_translate (GIMP_ITEM (vectors), x, y, FALSE);

      gimp_image_add_vectors (image, vectors,
                              GIMP_IMAGE_ACTIVE_PARENT, -1, TRUE);
      gimp_image_flush (image);
    }
}

void
layers_text_along_vectors_cmd_callback (GimpAction *action,
                                        GVariant   *value,
                                        gpointer    data)
{
  GimpImage   *image;
  GimpLayer   *layer;
  GimpVectors *vectors;
  return_if_no_layer (image, layer, data);
  return_if_no_vectors (image, vectors, data);

  if (GIMP_IS_TEXT_LAYER (layer))
    {
      gdouble      box_width;
      gdouble      box_height;
      GimpVectors *new_vectors;
      gdouble      offset;

      box_width  = gimp_item_get_width  (GIMP_ITEM (layer));
      box_height = gimp_item_get_height (GIMP_ITEM (layer));

      new_vectors = gimp_text_vectors_new (image, GIMP_TEXT_LAYER (layer)->text);

      offset = 0;
      switch (GIMP_TEXT_LAYER (layer)->text->base_dir)
        {
        case GIMP_TEXT_DIRECTION_LTR:
        case GIMP_TEXT_DIRECTION_RTL:
          offset = 0.5 * box_height;
          break;
        case GIMP_TEXT_DIRECTION_TTB_RTL:
        case GIMP_TEXT_DIRECTION_TTB_RTL_UPRIGHT:
        case GIMP_TEXT_DIRECTION_TTB_LTR:
        case GIMP_TEXT_DIRECTION_TTB_LTR_UPRIGHT:
          {
            GimpStroke *stroke = NULL;

            while ((stroke = gimp_vectors_stroke_get_next (new_vectors, stroke)))
              {
                gimp_stroke_rotate (stroke, 0, 0, 270);
                gimp_stroke_translate (stroke, 0, box_width);
              }
          }
          offset = 0.5 * box_width;
          break;
        }


      gimp_vectors_warp_vectors (vectors, new_vectors, offset);

      gimp_item_set_visible (GIMP_ITEM (new_vectors), TRUE, FALSE);

      gimp_image_add_vectors (image, new_vectors,
                              GIMP_IMAGE_ACTIVE_PARENT, -1, TRUE);
      gimp_image_flush (image);
    }
}

void
layers_resize_cmd_callback (GimpAction *action,
                            GVariant   *value,
                            gpointer    data)
{
  GimpImage *image;
  GimpLayer *layer;
  GtkWidget *widget;
  GtkWidget *dialog;
  return_if_no_layer (image, layer, data);
  return_if_no_widget (widget, data);

#define RESIZE_DIALOG_KEY "gimp-resize-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (layer), RESIZE_DIALOG_KEY);

  if (! dialog)
    {
      GimpDialogConfig *config  = GIMP_DIALOG_CONFIG (image->gimp->config);
      GimpDisplay      *display = NULL;

      if (GIMP_IS_IMAGE_WINDOW (data))
        display = action_data_get_display (data);

      if (layer_resize_unit != GIMP_UNIT_PERCENT && display)
        layer_resize_unit = gimp_display_get_shell (display)->unit;

      dialog = resize_dialog_new (GIMP_VIEWABLE (layer),
                                  action_data_get_context (data),
                                  _("Set Layer Boundary Size"),
                                  "gimp-layer-resize",
                                  widget,
                                  gimp_standard_help_func,
                                  GIMP_HELP_LAYER_RESIZE,
                                  layer_resize_unit,
                                  config->layer_resize_fill_type,
                                  GIMP_ITEM_SET_NONE,
                                  FALSE,
                                  layers_resize_callback,
                                  NULL);

      dialogs_attach_dialog (G_OBJECT (layer), RESIZE_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

void
layers_resize_to_image_cmd_callback (GimpAction *action,
                                     GVariant   *value,
                                     gpointer    data)
{
  GimpImage *image;
  GimpLayer *layer;
  return_if_no_layer (image, layer, data);

  gimp_layer_resize_to_image (layer,
                              action_data_get_context (data),
                              GIMP_FILL_TRANSPARENT);
  gimp_image_flush (image);
}

void
layers_scale_cmd_callback (GimpAction *action,
                           GVariant   *value,
                           gpointer    data)
{
  GimpImage *image;
  GimpLayer *layer;
  GtkWidget *widget;
  GtkWidget *dialog;
  return_if_no_layer (image, layer, data);
  return_if_no_widget (widget, data);

#define SCALE_DIALOG_KEY "gimp-scale-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (layer), SCALE_DIALOG_KEY);

  if (! dialog)
    {
      GimpDisplay *display = NULL;

      if (GIMP_IS_IMAGE_WINDOW (data))
        display = action_data_get_display (data);

      if (layer_scale_unit != GIMP_UNIT_PERCENT && display)
        layer_scale_unit = gimp_display_get_shell (display)->unit;

      if (layer_scale_interp == -1)
        layer_scale_interp = image->gimp->config->interpolation_type;

      dialog = scale_dialog_new (GIMP_VIEWABLE (layer),
                                 action_data_get_context (data),
                                 _("Scale Layer"), "gimp-layer-scale",
                                 widget,
                                 gimp_standard_help_func, GIMP_HELP_LAYER_SCALE,
                                 layer_scale_unit,
                                 layer_scale_interp,
                                 layers_scale_callback,
                                 display);

      dialogs_attach_dialog (G_OBJECT (layer), SCALE_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

void
layers_crop_to_selection_cmd_callback (GimpAction *action,
                                       GVariant   *value,
                                       gpointer    data)
{
  GimpImage *image;
  GimpLayer *layer;
  GtkWidget *widget;
  gint       x, y;
  gint       width, height;
  gint       off_x, off_y;
  return_if_no_layer (image, layer, data);
  return_if_no_widget (widget, data);

  if (! gimp_item_bounds (GIMP_ITEM (gimp_image_get_mask (image)),
                          &x, &y, &width, &height))
    {
      gimp_message_literal (image->gimp,
                            G_OBJECT (widget), GIMP_MESSAGE_WARNING,
                            _("Cannot crop because the current selection "
                              "is empty."));
      return;
    }

  gimp_item_get_offset (GIMP_ITEM (layer), &off_x, &off_y);
  off_x -= x;
  off_y -= y;

  gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_ITEM_RESIZE,
                               _("Crop Layer to Selection"));

  gimp_item_resize (GIMP_ITEM (layer),
                    action_data_get_context (data), GIMP_FILL_TRANSPARENT,
                    width, height, off_x, off_y);

  gimp_image_undo_group_end (image);
  gimp_image_flush (image);
}

void
layers_crop_to_content_cmd_callback (GimpAction *action,
                                     GVariant   *value,
                                     gpointer    data)
{
  GimpImage *image;
  GimpLayer *layer;
  GtkWidget *widget;
  gint       x, y;
  gint       width, height;
  return_if_no_layer (image, layer, data);
  return_if_no_widget (widget, data);

  switch (gimp_pickable_auto_shrink (GIMP_PICKABLE (layer),
                                     0, 0,
                                     gimp_item_get_width  (GIMP_ITEM (layer)),
                                     gimp_item_get_height (GIMP_ITEM (layer)),
                                     &x, &y, &width, &height))
    {
    case GIMP_AUTO_SHRINK_SHRINK:
      gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_ITEM_RESIZE,
                                   _("Crop Layer to Content"));

      gimp_item_resize (GIMP_ITEM (layer),
                        action_data_get_context (data), GIMP_FILL_TRANSPARENT,
                        width, height, -x, -y);

      gimp_image_undo_group_end (image);
      gimp_image_flush (image);
      break;

    case GIMP_AUTO_SHRINK_EMPTY:
      gimp_message_literal (image->gimp,
                            G_OBJECT (widget), GIMP_MESSAGE_INFO,
                            _("Cannot crop because the active layer "
                              "has no content."));
      break;

    case GIMP_AUTO_SHRINK_UNSHRINKABLE:
      gimp_message_literal (image->gimp,
                            G_OBJECT (widget), GIMP_MESSAGE_INFO,
                            _("Cannot crop because the active layer "
                              "is already cropped to its content."));
      break;
    }
}

void
layers_mask_add_cmd_callback (GimpAction *action,
                              GVariant   *value,
                              gpointer    data)
{
  GimpImage *image;
  GimpLayer *layer;
  GtkWidget *widget;
  GtkWidget *dialog;
  return_if_no_layer (image, layer, data);
  return_if_no_widget (widget, data);

  if (gimp_layer_get_mask (layer))
    return;

#define ADD_MASK_DIALOG_KEY "gimp-add-mask-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (layer), ADD_MASK_DIALOG_KEY);

  if (! dialog)
    {
      GimpDialogConfig *config = GIMP_DIALOG_CONFIG (image->gimp->config);

      dialog = layer_add_mask_dialog_new (layer, action_data_get_context (data),
                                          widget,
                                          config->layer_add_mask_type,
                                          config->layer_add_mask_invert,
                                          layers_add_mask_callback,
                                          NULL);

      dialogs_attach_dialog (G_OBJECT (layer), ADD_MASK_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

void
layers_mask_add_last_vals_cmd_callback (GimpAction *action,
                                        GVariant   *value,
                                        gpointer    data)
{
  GimpImage        *image;
  GimpLayer        *layer;
  GtkWidget        *widget;
  GimpDialogConfig *config;
  GimpChannel      *channel = NULL;
  GimpLayerMask    *mask;
  return_if_no_layer (image, layer, data);
  return_if_no_widget (widget, data);

  if (gimp_layer_get_mask (layer))
    return;

  config = GIMP_DIALOG_CONFIG (image->gimp->config);

  if (config->layer_add_mask_type == GIMP_ADD_MASK_CHANNEL)
    {
      channel = gimp_image_get_active_channel (image);

      if (! channel)
        {
          GimpContainer *channels = gimp_image_get_channels (image);

          channel = GIMP_CHANNEL (gimp_container_get_first_child (channels));
        }

      if (! channel)
        {
          layers_mask_add_cmd_callback (action, value, data);
          return;
        }
    }

  mask = gimp_layer_create_mask (layer,
                                 config->layer_add_mask_type,
                                 channel);

  if (config->layer_add_mask_invert)
    gimp_channel_invert (GIMP_CHANNEL (mask), FALSE);

  gimp_layer_add_mask (layer, mask, TRUE, NULL);
  gimp_image_flush (image);
}

void
layers_mask_apply_cmd_callback (GimpAction *action,
                                GVariant   *value,
                                gpointer    data)
{
  GimpImage *image;
  GimpLayer *layer;
  return_if_no_layer (image, layer, data);

  if (gimp_layer_get_mask (layer))
    {
      GimpMaskApplyMode mode = (GimpMaskApplyMode) g_variant_get_int32 (value);

      gimp_layer_apply_mask (layer, mode, TRUE);
      gimp_image_flush (image);
    }
}

void
layers_mask_edit_cmd_callback (GimpAction *action,
                               GVariant   *value,
                               gpointer    data)
{
  GimpImage *image;
  GimpLayer *layer;
  return_if_no_layer (image, layer, data);

  if (gimp_layer_get_mask (layer))
    {
      gboolean active = g_variant_get_boolean (value);

      gimp_layer_set_edit_mask (layer, active);
      gimp_image_flush (image);
    }
}

void
layers_mask_show_cmd_callback (GimpAction *action,
                               GVariant   *value,
                               gpointer    data)
{
  GimpImage *image;
  GimpLayer *layer;
  return_if_no_layer (image, layer, data);

  if (gimp_layer_get_mask (layer))
    {
      gboolean active = g_variant_get_boolean (value);

      gimp_layer_set_show_mask (layer, active, TRUE);
      gimp_image_flush (image);
    }
}

void
layers_mask_disable_cmd_callback (GimpAction *action,
                                  GVariant   *value,
                                  gpointer    data)
{
  GimpImage *image;
  GimpLayer *layer;
  return_if_no_layer (image, layer, data);

  if (gimp_layer_get_mask (layer))
    {
      gboolean active = g_variant_get_boolean (value);

      gimp_layer_set_apply_mask (layer, ! active, TRUE);
      gimp_image_flush (image);
    }
}

void
layers_mask_to_selection_cmd_callback (GimpAction *action,
                                       GVariant   *value,
                                       gpointer    data)
{
  GimpImage     *image;
  GimpLayer     *layer;
  GimpLayerMask *mask;
  return_if_no_layer (image, layer, data);

  mask = gimp_layer_get_mask (layer);

  if (mask)
    {
      GimpChannelOps operation = (GimpChannelOps) g_variant_get_int32 (value);

      gimp_item_to_selection (GIMP_ITEM (mask), operation,
                              TRUE, FALSE, 0.0, 0.0);
      gimp_image_flush (image);
    }
}

void
layers_alpha_add_cmd_callback (GimpAction *action,
                               GVariant   *value,
                               gpointer    data)
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
layers_alpha_remove_cmd_callback (GimpAction *action,
                                  GVariant   *value,
                                  gpointer    data)
{
  GimpImage *image;
  GimpLayer *layer;
  return_if_no_layer (image, layer, data);

  if (gimp_drawable_has_alpha (GIMP_DRAWABLE (layer)))
    {
      gimp_layer_remove_alpha (layer, action_data_get_context (data));
      gimp_image_flush (image);
    }
}

void
layers_alpha_to_selection_cmd_callback (GimpAction *action,
                                        GVariant   *value,
                                        gpointer    data)
{
  GimpImage      *image;
  GimpLayer      *layer;
  GimpChannelOps  operation;
  return_if_no_layer (image, layer, data);

  operation = (GimpChannelOps) g_variant_get_int32 (value);

  gimp_item_to_selection (GIMP_ITEM (layer), operation,
                          TRUE, FALSE, 0.0, 0.0);
  gimp_image_flush (image);
}

void
layers_opacity_cmd_callback (GimpAction *action,
                             GVariant   *value,
                             gpointer    data)
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
                                 0.0, 1.0, 1.0,
                                 1.0 / 255.0, 0.01, 0.1, 0.0, FALSE);
  gimp_layer_set_opacity (layer, opacity, push_undo);
  gimp_image_flush (image);
}

void
layers_mode_cmd_callback (GimpAction *action,
                          GVariant   *value,
                          gpointer    data)
{
  GimpImage     *image;
  GimpLayer     *layer;
  GimpLayerMode *modes;
  gint           n_modes;
  GimpLayerMode  layer_mode;
  gint           index;
  GimpUndo      *undo;
  gboolean       push_undo = TRUE;
  return_if_no_layer (image, layer, data);

  undo = gimp_image_undo_can_compress (image, GIMP_TYPE_ITEM_UNDO,
                                       GIMP_UNDO_LAYER_MODE);

  if (undo && GIMP_ITEM_UNDO (undo)->item == GIMP_ITEM (layer))
    push_undo = FALSE;

  layer_mode = gimp_layer_get_mode (layer);

  modes = gimp_layer_mode_get_context_array (layer_mode,
                                             GIMP_LAYER_MODE_CONTEXT_LAYER,
                                             &n_modes);
  index = layers_mode_index (layer_mode, modes, n_modes);
  index = action_select_value ((GimpActionSelectType) value,
                               index, 0, n_modes - 1, 0,
                               0.0, 1.0, 1.0, 0.0, FALSE);
  layer_mode = modes[index];
  g_free (modes);

  gimp_layer_set_mode (layer, layer_mode, push_undo);
  gimp_image_flush (image);
}

void
layers_blend_space_cmd_callback (GimpAction *action,
                                 GVariant   *value,
                                 gpointer    data)
{
  GimpImage           *image;
  GimpLayer           *layer;
  GimpLayerColorSpace  blend_space;
  return_if_no_layer (image, layer, data);

  blend_space = (GimpLayerColorSpace) g_variant_get_int32 (value);

  if (blend_space != gimp_layer_get_blend_space (layer))
    {
      GimpUndo *undo;
      gboolean  push_undo = TRUE;

      undo = gimp_image_undo_can_compress (image, GIMP_TYPE_LAYER_PROP_UNDO,
                                           GIMP_UNDO_LAYER_MODE);

      if (undo && GIMP_ITEM_UNDO (undo)->item == GIMP_ITEM (layer))
        push_undo = FALSE;

      gimp_layer_set_blend_space (layer, blend_space, push_undo);
      gimp_image_flush (image);
    }
}

void
layers_composite_space_cmd_callback (GimpAction *action,
                                     GVariant   *value,
                                     gpointer    data)
{
  GimpImage           *image;
  GimpLayer           *layer;
  GimpLayerColorSpace  composite_space;
  return_if_no_layer (image, layer, data);

  composite_space = (GimpLayerColorSpace) g_variant_get_int32 (value);

  if (composite_space != gimp_layer_get_composite_space (layer))
    {
      GimpUndo *undo;
      gboolean  push_undo = TRUE;

      undo = gimp_image_undo_can_compress (image, GIMP_TYPE_LAYER_PROP_UNDO,
                                           GIMP_UNDO_LAYER_MODE);

      if (undo && GIMP_ITEM_UNDO (undo)->item == GIMP_ITEM (layer))
        push_undo = FALSE;

      gimp_layer_set_composite_space (layer, composite_space, push_undo);
      gimp_image_flush (image);
    }
}

void
layers_composite_mode_cmd_callback (GimpAction *action,
                                    GVariant   *value,
                                    gpointer    data)
{
  GimpImage              *image;
  GimpLayer              *layer;
  GimpLayerCompositeMode  composite_mode;
  return_if_no_layer (image, layer, data);

  composite_mode = (GimpLayerCompositeMode) g_variant_get_int32 (value);

  if (composite_mode != gimp_layer_get_composite_mode (layer))
    {
      GimpUndo *undo;
      gboolean  push_undo = TRUE;

      undo = gimp_image_undo_can_compress (image, GIMP_TYPE_LAYER_PROP_UNDO,
                                           GIMP_UNDO_LAYER_MODE);

      if (undo && GIMP_ITEM_UNDO (undo)->item == GIMP_ITEM (layer))
        push_undo = FALSE;

      gimp_layer_set_composite_mode (layer, composite_mode, push_undo);
      gimp_image_flush (image);
    }
}

void
layers_visible_cmd_callback (GimpAction *action,
                             GVariant   *value,
                             gpointer   data)
{
  GimpImage *image;
  GimpLayer *layer;
  return_if_no_layer (image, layer, data);

  items_visible_cmd_callback (action, value, image, GIMP_ITEM (layer));
}

void
layers_linked_cmd_callback (GimpAction *action,
                            GVariant   *value,
                            gpointer    data)
{
  GimpImage *image;
  GimpLayer *layer;
  return_if_no_layer (image, layer, data);

  items_linked_cmd_callback (action, value, image, GIMP_ITEM (layer));
}

void
layers_lock_content_cmd_callback (GimpAction *action,
                                  GVariant   *value,
                                  gpointer    data)
{
  GimpImage *image;
  GimpLayer *layer;
  return_if_no_layer (image, layer, data);

  items_lock_content_cmd_callback (action, value, image, GIMP_ITEM (layer));
}

void
layers_lock_position_cmd_callback (GimpAction *action,
                                   GVariant   *value,
                                   gpointer    data)
{
  GimpImage *image;
  GimpLayer *layer;
  return_if_no_layer (image, layer, data);

  items_lock_position_cmd_callback (action, value, image, GIMP_ITEM (layer));
}

void
layers_lock_alpha_cmd_callback (GimpAction *action,
                                GVariant   *value,
                                gpointer    data)
{
  GimpImage *image;
  GimpLayer *layer;
  gboolean   lock_alpha;
  return_if_no_layer (image, layer, data);

  lock_alpha = g_variant_get_boolean (value);

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

void
layers_color_tag_cmd_callback (GimpAction *action,
                               GVariant   *value,
                               gpointer    data)
{
  GimpImage    *image;
  GimpLayer    *layer;
  GimpColorTag  color_tag;
  return_if_no_layer (image, layer, data);

  color_tag = (GimpColorTag) g_variant_get_int32 (value);

  items_color_tag_cmd_callback (action, image, GIMP_ITEM (layer),
                                color_tag);
}


/*  private functions  */

static void
layers_new_callback (GtkWidget              *dialog,
                     GimpImage              *image,
                     GimpLayer              *layer,
                     GimpContext            *context,
                     const gchar            *layer_name,
                     GimpLayerMode           layer_mode,
                     GimpLayerColorSpace     layer_blend_space,
                     GimpLayerColorSpace     layer_composite_space,
                     GimpLayerCompositeMode  layer_composite_mode,
                     gdouble                 layer_opacity,
                     GimpFillType            layer_fill_type,
                     gint                    layer_width,
                     gint                    layer_height,
                     gint                    layer_offset_x,
                     gint                    layer_offset_y,
                     gboolean                layer_visible,
                     gboolean                layer_linked,
                     GimpColorTag            layer_color_tag,
                     gboolean                layer_lock_pixels,
                     gboolean                layer_lock_position,
                     gboolean                layer_lock_alpha,
                     gboolean                rename_text_layer, /* unused */
                     gpointer                user_data)
{
  GimpDialogConfig *config = GIMP_DIALOG_CONFIG (image->gimp->config);

  g_object_set (config,
                "layer-new-name",            layer_name,
                "layer-new-mode",            layer_mode,
                "layer-new-blend-space",     layer_blend_space,
                "layer-new-composite-space", layer_composite_space,
                "layer-new-composite-mode",  layer_composite_mode,
                "layer-new-opacity",         layer_opacity,
                "layer-new-fill-type",       layer_fill_type,
                NULL);

  layer = gimp_layer_new (image, layer_width, layer_height,
                          gimp_image_get_layer_format (image, TRUE),
                          config->layer_new_name,
                          config->layer_new_opacity,
                          config->layer_new_mode);

  if (layer)
    {
      gimp_item_set_offset (GIMP_ITEM (layer), layer_offset_x, layer_offset_y);
      gimp_drawable_fill (GIMP_DRAWABLE (layer), context,
                          config->layer_new_fill_type);
      gimp_item_set_visible (GIMP_ITEM (layer), layer_visible, FALSE);
      gimp_item_set_linked (GIMP_ITEM (layer), layer_linked, FALSE);
      gimp_item_set_color_tag (GIMP_ITEM (layer), layer_color_tag, FALSE);
      gimp_item_set_lock_content (GIMP_ITEM (layer), layer_lock_pixels,
                                  FALSE);
      gimp_item_set_lock_position (GIMP_ITEM (layer), layer_lock_position,
                                   FALSE);
      gimp_layer_set_lock_alpha (layer, layer_lock_alpha, FALSE);
      gimp_layer_set_blend_space (layer, layer_blend_space, FALSE);
      gimp_layer_set_composite_space (layer, layer_composite_space, FALSE);
      gimp_layer_set_composite_mode (layer, layer_composite_mode, FALSE);

      gimp_image_add_layer (image, layer,
                            GIMP_IMAGE_ACTIVE_PARENT, -1, TRUE);
      gimp_image_flush (image);
    }
  else
    {
      g_warning ("%s: could not allocate new layer", G_STRFUNC);
    }

  gtk_widget_destroy (dialog);
}

static void
layers_edit_attributes_callback (GtkWidget              *dialog,
                                 GimpImage              *image,
                                 GimpLayer              *layer,
                                 GimpContext            *context,
                                 const gchar            *layer_name,
                                 GimpLayerMode           layer_mode,
                                 GimpLayerColorSpace     layer_blend_space,
                                 GimpLayerColorSpace     layer_composite_space,
                                 GimpLayerCompositeMode  layer_composite_mode,
                                 gdouble                 layer_opacity,
                                 GimpFillType            unused1,
                                 gint                    unused2,
                                 gint                    unused3,
                                 gint                    layer_offset_x,
                                 gint                    layer_offset_y,
                                 gboolean                layer_visible,
                                 gboolean                layer_linked,
                                 GimpColorTag            layer_color_tag,
                                 gboolean                layer_lock_pixels,
                                 gboolean                layer_lock_position,
                                 gboolean                layer_lock_alpha,
                                 gboolean                rename_text_layer,
                                 gpointer                user_data)
{
  GimpItem *item = GIMP_ITEM (layer);

  if (strcmp (layer_name, gimp_object_get_name (layer))               ||
      layer_mode            != gimp_layer_get_mode (layer)            ||
      layer_blend_space     != gimp_layer_get_blend_space (layer)     ||
      layer_composite_space != gimp_layer_get_composite_space (layer) ||
      layer_composite_mode  != gimp_layer_get_composite_mode (layer)  ||
      layer_opacity         != gimp_layer_get_opacity (layer)         ||
      layer_offset_x        != gimp_item_get_offset_x (item)          ||
      layer_offset_y        != gimp_item_get_offset_y (item)          ||
      layer_visible         != gimp_item_get_visible (item)           ||
      layer_linked          != gimp_item_get_linked (item)            ||
      layer_color_tag       != gimp_item_get_color_tag (item)         ||
      layer_lock_pixels     != gimp_item_get_lock_content (item)      ||
      layer_lock_position   != gimp_item_get_lock_position (item)     ||
      layer_lock_alpha      != gimp_layer_get_lock_alpha (layer))
    {
      gimp_image_undo_group_start (image,
                                   GIMP_UNDO_GROUP_ITEM_PROPERTIES,
                                   _("Layer Attributes"));

      if (strcmp (layer_name, gimp_object_get_name (layer)))
        {
          GError *error = NULL;

          if (! gimp_item_rename (GIMP_ITEM (layer), layer_name, &error))
            {
              gimp_message_literal (image->gimp,
                                    G_OBJECT (dialog), GIMP_MESSAGE_WARNING,
                                    error->message);
              g_clear_error (&error);
            }
        }

      if (layer_mode != gimp_layer_get_mode (layer))
        gimp_layer_set_mode (layer, layer_mode, TRUE);

      if (layer_blend_space != gimp_layer_get_blend_space (layer))
        gimp_layer_set_blend_space (layer, layer_blend_space, TRUE);

      if (layer_composite_space != gimp_layer_get_composite_space (layer))
        gimp_layer_set_composite_space (layer, layer_composite_space, TRUE);

      if (layer_composite_mode != gimp_layer_get_composite_mode (layer))
        gimp_layer_set_composite_mode (layer, layer_composite_mode, TRUE);

      if (layer_opacity != gimp_layer_get_opacity (layer))
        gimp_layer_set_opacity (layer, layer_opacity, TRUE);

      if (layer_offset_x != gimp_item_get_offset_x (item) ||
          layer_offset_y != gimp_item_get_offset_y (item))
        {
          gimp_item_translate (item,
                               layer_offset_x - gimp_item_get_offset_x (item),
                               layer_offset_y - gimp_item_get_offset_y (item),
                               TRUE);
        }

      if (layer_visible != gimp_item_get_visible (item))
        gimp_item_set_visible (item, layer_visible, TRUE);

      if (layer_linked != gimp_item_get_linked (item))
        gimp_item_set_linked (item, layer_linked, TRUE);

      if (layer_color_tag != gimp_item_get_color_tag (item))
        gimp_item_set_color_tag (item, layer_color_tag, TRUE);

      if (layer_lock_pixels != gimp_item_get_lock_content (item))
        gimp_item_set_lock_content (item, layer_lock_pixels, TRUE);

      if (layer_lock_position != gimp_item_get_lock_position (item))
        gimp_item_set_lock_position (item, layer_lock_position, TRUE);

      if (layer_lock_alpha != gimp_layer_get_lock_alpha (layer))
        gimp_layer_set_lock_alpha (layer, layer_lock_alpha, TRUE);

      gimp_image_undo_group_end (image);

      gimp_image_flush (image);
    }

  if (gimp_item_is_text_layer (GIMP_ITEM (layer)))
    {
      g_object_set (layer,
                    "auto-rename", rename_text_layer,
                    NULL);
    }

  gtk_widget_destroy (dialog);
}

static void
layers_add_mask_callback (GtkWidget       *dialog,
                          GimpLayer       *layer,
                          GimpAddMaskType  add_mask_type,
                          GimpChannel     *channel,
                          gboolean         invert,
                          gpointer         user_data)
{
  GimpImage        *image  = gimp_item_get_image (GIMP_ITEM (layer));
  GimpDialogConfig *config = GIMP_DIALOG_CONFIG (image->gimp->config);
  GimpLayerMask    *mask;
  GError           *error = NULL;

  g_object_set (config,
                "layer-add-mask-type",   add_mask_type,
                "layer-add-mask-invert", invert,
                NULL);

  mask = gimp_layer_create_mask (layer,
                                 config->layer_add_mask_type,
                                 channel);

  if (config->layer_add_mask_invert)
    gimp_channel_invert (GIMP_CHANNEL (mask), FALSE);

  if (! gimp_layer_add_mask (layer, mask, TRUE, &error))
    {
      gimp_message_literal (image->gimp,
                            G_OBJECT (dialog), GIMP_MESSAGE_WARNING,
                            error->message);
      g_object_unref (mask);
      g_clear_error (&error);
      return;
    }

  gimp_image_flush (image);

  gtk_widget_destroy (dialog);
}

static void
layers_scale_callback (GtkWidget             *dialog,
                       GimpViewable          *viewable,
                       gint                   width,
                       gint                   height,
                       GimpUnit               unit,
                       GimpInterpolationType  interpolation,
                       gdouble                xresolution,    /* unused */
                       gdouble                yresolution,    /* unused */
                       GimpUnit               resolution_unit,/* unused */
                       gpointer               user_data)
{
  GimpDisplay *display = GIMP_DISPLAY (user_data);

  layer_scale_unit   = unit;
  layer_scale_interp = interpolation;

  if (width > 0 && height > 0)
    {
      GimpItem     *item = GIMP_ITEM (viewable);
      GimpProgress *progress;
      GtkWidget    *progress_dialog = NULL;

      gtk_widget_destroy (dialog);

      if (width  == gimp_item_get_width  (item) &&
          height == gimp_item_get_height (item))
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

      progress = gimp_progress_start (progress, FALSE, _("Scaling"));

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
layers_resize_callback (GtkWidget    *dialog,
                        GimpViewable *viewable,
                        GimpContext  *context,
                        gint          width,
                        gint          height,
                        GimpUnit      unit,
                        gint          offset_x,
                        gint          offset_y,
                        GimpFillType  fill_type,
                        GimpItemSet   unused,
                        gboolean      unused2,
                        gpointer      user_data)
{
  layer_resize_unit = unit;

  if (width > 0 && height > 0)
    {
      GimpItem         *item   = GIMP_ITEM (viewable);
      GimpImage        *image  = gimp_item_get_image (item);
      GimpDialogConfig *config = GIMP_DIALOG_CONFIG (image->gimp->config);

      g_object_set (config,
                    "layer-resize-fill-type", fill_type,
                    NULL);

      gtk_widget_destroy (dialog);

      if (width  == gimp_item_get_width  (item) &&
          height == gimp_item_get_height (item))
        return;

      gimp_item_resize (item, context, fill_type,
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
layers_mode_index (GimpLayerMode         layer_mode,
                   const GimpLayerMode  *modes,
                   gint                  n_modes)
{
  gint i = 0;

  while (i < (n_modes - 1) && modes[i] != layer_mode)
    i++;

  return i;
}
