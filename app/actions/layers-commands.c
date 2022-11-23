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

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmamath/ligmamath.h"
#include "libligmabase/ligmabase.h"
#include "libligmacolor/ligmacolor.h"
#include "libligmawidgets/ligmawidgets.h"

#include "actions-types.h"

#include "config/ligmadialogconfig.h"

#include "operations/layer-modes/ligma-layer-modes.h"

#include "core/ligma.h"
#include "core/ligmachannel.h"
#include "core/ligmachannel-combine.h"
#include "core/ligmacontainer.h"
#include "core/ligmacontext.h"
#include "core/ligmadrawable-fill.h"
#include "core/ligmagrouplayer.h"
#include "core/ligmaimage.h"
#include "core/ligmaimage-merge.h"
#include "core/ligmaimage-undo.h"
#include "core/ligmaimage-undo-push.h"
#include "core/ligmaitemundo.h"
#include "core/ligmalayerpropundo.h"
#include "core/ligmalayer-floating-selection.h"
#include "core/ligmalayer-new.h"
#include "core/ligmapickable.h"
#include "core/ligmapickable-auto-shrink.h"
#include "core/ligmatoolinfo.h"
#include "core/ligmaundostack.h"
#include "core/ligmaprogress.h"

#include "text/ligmatext.h"
#include "text/ligmatext-vectors.h"
#include "text/ligmatextlayer.h"

#include "vectors/ligmastroke.h"
#include "vectors/ligmavectors.h"
#include "vectors/ligmavectors-warp.h"

#include "widgets/ligmaaction.h"
#include "widgets/ligmadock.h"
#include "widgets/ligmahelp-ids.h"
#include "widgets/ligmaprogressdialog.h"

#include "display/ligmadisplay.h"
#include "display/ligmadisplayshell.h"
#include "display/ligmaimagewindow.h"

#include "tools/ligmatexttool.h"
#include "tools/tool_manager.h"

#include "dialogs/dialogs.h"
#include "dialogs/layer-add-mask-dialog.h"
#include "dialogs/layer-options-dialog.h"
#include "dialogs/resize-dialog.h"
#include "dialogs/scale-dialog.h"

#include "actions.h"
#include "items-commands.h"
#include "layers-commands.h"

#include "ligma-intl.h"


/*  local function prototypes  */

static void   layers_new_callback             (GtkWidget             *dialog,
                                               LigmaImage             *image,
                                               LigmaLayer             *layer,
                                               LigmaContext           *context,
                                               const gchar           *layer_name,
                                               LigmaLayerMode          layer_mode,
                                               LigmaLayerColorSpace    layer_blend_space,
                                               LigmaLayerColorSpace    layer_composite_space,
                                               LigmaLayerCompositeMode layer_composite_mode,
                                               gdouble                layer_opacity,
                                               LigmaFillType           layer_fill_type,
                                               gint                   layer_width,
                                               gint                   layer_height,
                                               gint                   layer_offset_x,
                                               gint                   layer_offset_y,
                                               gboolean               layer_visible,
                                               LigmaColorTag           layer_color_tag,
                                               gboolean               layer_lock_pixels,
                                               gboolean               layer_lock_position,
                                               gboolean               layer_lock_alpha,
                                               gboolean               rename_text_layer,
                                               gpointer               user_data);
static void   layers_edit_attributes_callback (GtkWidget             *dialog,
                                               LigmaImage             *image,
                                               LigmaLayer             *layer,
                                               LigmaContext           *context,
                                               const gchar           *layer_name,
                                               LigmaLayerMode          layer_mode,
                                               LigmaLayerColorSpace    layer_blend_space,
                                               LigmaLayerColorSpace    layer_composite_space,
                                               LigmaLayerCompositeMode layer_composite_mode,
                                               gdouble                layer_opacity,
                                               LigmaFillType           layer_fill_type,
                                               gint                   layer_width,
                                               gint                   layer_height,
                                               gint                   layer_offset_x,
                                               gint                   layer_offset_y,
                                               gboolean               layer_visible,
                                               LigmaColorTag           layer_color_tag,
                                               gboolean               layer_lock_pixels,
                                               gboolean               layer_lock_position,
                                               gboolean               layer_lock_alpha,
                                               gboolean               rename_text_layer,
                                               gpointer               user_data);
static void   layers_add_mask_callback        (GtkWidget             *dialog,
                                               GList                 *layers,
                                               LigmaAddMaskType        add_mask_type,
                                               LigmaChannel           *channel,
                                               gboolean               invert,
                                               gpointer               user_data);
static void   layers_scale_callback           (GtkWidget             *dialog,
                                               LigmaViewable          *viewable,
                                               gint                   width,
                                               gint                   height,
                                               LigmaUnit               unit,
                                               LigmaInterpolationType  interpolation,
                                               gdouble                xresolution,
                                               gdouble                yresolution,
                                               LigmaUnit               resolution_unit,
                                               gpointer               user_data);
static void   layers_resize_callback          (GtkWidget             *dialog,
                                               LigmaViewable          *viewable,
                                               LigmaContext           *context,
                                               gint                   width,
                                               gint                   height,
                                               LigmaUnit               unit,
                                               gint                   offset_x,
                                               gint                   offset_y,
                                               gdouble                unused0,
                                               gdouble                unused1,
                                               LigmaUnit               unused2,
                                               LigmaFillType           fill_type,
                                               LigmaItemSet            unused3,
                                               gboolean               unused4,
                                               gpointer               data);

static gint   layers_mode_index               (LigmaLayerMode          layer_mode,
                                               const LigmaLayerMode   *modes,
                                               gint                   n_modes);


/*  private variables  */

static LigmaUnit               layer_resize_unit   = LIGMA_UNIT_PIXEL;
static LigmaUnit               layer_scale_unit    = LIGMA_UNIT_PIXEL;
static LigmaInterpolationType  layer_scale_interp  = -1;


/*  public functions  */

void
layers_edit_cmd_callback (LigmaAction *action,
                          GVariant   *value,
                          gpointer    data)
{
  LigmaImage *image;
  LigmaLayer *layer;
  GtkWidget *widget;
  return_if_no_layer (image, layer, data);
  return_if_no_widget (widget, data);

  if (ligma_item_is_text_layer (LIGMA_ITEM (layer)))
    {
      layers_edit_text_cmd_callback (action, value, data);
    }
  else
    {
      layers_edit_attributes_cmd_callback (action, value, data);
    }
}

void
layers_edit_text_cmd_callback (LigmaAction *action,
                               GVariant   *value,
                               gpointer    data)
{
  LigmaImage *image;
  LigmaLayer *layer;
  GtkWidget *widget;
  LigmaTool  *active_tool;
  return_if_no_layer (image, layer, data);
  return_if_no_widget (widget, data);

  g_return_if_fail (ligma_item_is_text_layer (LIGMA_ITEM (layer)));

  active_tool = tool_manager_get_active (image->ligma);

  if (! LIGMA_IS_TEXT_TOOL (active_tool))
    {
      LigmaToolInfo *tool_info = ligma_get_tool_info (image->ligma,
                                                    "ligma-text-tool");

      if (LIGMA_IS_TOOL_INFO (tool_info))
        {
          ligma_context_set_tool (action_data_get_context (data), tool_info);
          active_tool = tool_manager_get_active (image->ligma);
        }
    }

  if (LIGMA_IS_TEXT_TOOL (active_tool))
    {
      if (ligma_text_tool_set_layer (LIGMA_TEXT_TOOL (active_tool), layer))
        {
          LigmaDisplayShell *shell;

          shell = ligma_display_get_shell (active_tool->display);
          gtk_widget_grab_focus (shell->canvas);
        }
    }
}

void
layers_edit_attributes_cmd_callback (LigmaAction *action,
                                     GVariant   *value,
                                     gpointer    data)
{
  LigmaImage *image;
  LigmaLayer *layer;
  GtkWidget *widget;
  GtkWidget *dialog;
  return_if_no_layer (image, layer, data);
  return_if_no_widget (widget, data);

#define EDIT_DIALOG_KEY "ligma-layer-edit-attributes-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (layer), EDIT_DIALOG_KEY);

  if (! dialog)
    {
      LigmaItem *item = LIGMA_ITEM (layer);

      dialog = layer_options_dialog_new (ligma_item_get_image (LIGMA_ITEM (layer)),
                                         layer,
                                         action_data_get_context (data),
                                         widget,
                                         _("Layer Attributes"),
                                         "ligma-layer-edit",
                                         LIGMA_ICON_EDIT,
                                         _("Edit Layer Attributes"),
                                         LIGMA_HELP_LAYER_EDIT,
                                         ligma_object_get_name (layer),
                                         ligma_layer_get_mode (layer),
                                         ligma_layer_get_blend_space (layer),
                                         ligma_layer_get_composite_space (layer),
                                         ligma_layer_get_composite_mode (layer),
                                         ligma_layer_get_opacity (layer),
                                         0 /* unused */,
                                         ligma_item_get_visible (item),
                                         ligma_item_get_color_tag (item),
                                         ligma_item_get_lock_content (item),
                                         ligma_item_get_lock_position (item),
                                         ligma_layer_get_lock_alpha (layer),
                                         layers_edit_attributes_callback,
                                         NULL);

      dialogs_attach_dialog (G_OBJECT (layer), EDIT_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

void
layers_new_cmd_callback (LigmaAction *action,
                         GVariant   *value,
                         gpointer    data)
{
  LigmaImage *image;
  GtkWidget *widget;
  LigmaLayer *floating_sel;
  GtkWidget *dialog;
  return_if_no_image (image, data);
  return_if_no_widget (widget, data);

  /*  If there is a floating selection, the new command transforms
   *  the current fs into a new layer
   */
  if ((floating_sel = ligma_image_get_floating_selection (image)))
    {
      GError *error = NULL;

      if (! floating_sel_to_layer (floating_sel, &error))
        {
          ligma_message_literal (image->ligma,
                                G_OBJECT (widget), LIGMA_MESSAGE_WARNING,
                                error->message);
          g_clear_error (&error);
          return;
        }

      ligma_image_flush (image);
      return;
    }

#define NEW_DIALOG_KEY "ligma-layer-new-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (image), NEW_DIALOG_KEY);

  if (! dialog)
    {
      LigmaDialogConfig *config     = LIGMA_DIALOG_CONFIG (image->ligma->config);
      const gchar      *title;
      gchar            *desc;
      gint              n_layers;
      LigmaLayerMode     layer_mode = config->layer_new_mode;

      n_layers = g_list_length (ligma_image_get_selected_layers (image));
      title = ngettext ("New Layer", "New Layers", n_layers > 0 ? n_layers : 1);
      desc  = ngettext ("Create a New Layer", "Create %d New Layers", n_layers > 0 ? n_layers : 1);
      desc  = g_strdup_printf (desc, n_layers > 0 ? n_layers : 1);

      if (layer_mode == LIGMA_LAYER_MODE_NORMAL ||
          layer_mode == LIGMA_LAYER_MODE_NORMAL_LEGACY)
        {
          layer_mode = ligma_image_get_default_new_layer_mode (image);
        }

      dialog = layer_options_dialog_new (image, NULL,
                                         action_data_get_context (data),
                                         widget,
                                         title,
                                         "ligma-layer-new",
                                         LIGMA_ICON_LAYER,
                                         desc,
                                         LIGMA_HELP_LAYER_NEW,
                                         config->layer_new_name,
                                         layer_mode,
                                         config->layer_new_blend_space,
                                         config->layer_new_composite_space,
                                         config->layer_new_composite_mode,
                                         config->layer_new_opacity,
                                         config->layer_new_fill_type,
                                         TRUE,
                                         LIGMA_COLOR_TAG_NONE,
                                         FALSE,
                                         FALSE,
                                         FALSE,
                                         layers_new_callback,
                                         NULL);
      g_free (desc);

      dialogs_attach_dialog (G_OBJECT (image), NEW_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

void
layers_new_last_vals_cmd_callback (LigmaAction *action,
                                   GVariant   *value,
                                   gpointer    data)
{
  LigmaImage        *image;
  GtkWidget        *widget;
  LigmaLayer        *layer;
  LigmaDialogConfig *config;
  GList            *layers;
  GList            *new_layers = NULL;
  GList            *iter;
  LigmaLayerMode     layer_mode;
  gint              n_layers;
  gboolean          run_once;

  return_if_no_image (image, data);
  return_if_no_widget (widget, data);

  config = LIGMA_DIALOG_CONFIG (image->ligma->config);

  /*  If there is a floating selection, the new command transforms
   *  the current fs into a new layer
   */
  if (ligma_image_get_floating_selection (image))
    {
      layers_new_cmd_callback (action, value, data);
      return;
    }

  layer_mode = config->layer_new_mode;

  if (layer_mode == LIGMA_LAYER_MODE_NORMAL ||
      layer_mode == LIGMA_LAYER_MODE_NORMAL_LEGACY)
    {
      layer_mode = ligma_image_get_default_new_layer_mode (image);
    }

  layers   = ligma_image_get_selected_layers (image);
  layers   = g_list_copy (layers);
  n_layers = g_list_length (layers);
  run_once = (n_layers == 0);

  ligma_image_undo_group_start (image,
                               LIGMA_UNDO_GROUP_LAYER_ADD,
                               ngettext ("New layer",
                                         "New layers",
                                         n_layers > 0 ? n_layers : 1));
  for (iter = layers; iter || run_once ; iter = iter ? iter->next : NULL)
    {
      LigmaLayer *parent;
      gint       position;

      run_once = FALSE;
      if (iter)
        {
          if (ligma_viewable_get_children (LIGMA_VIEWABLE (iter->data)))
            {
              parent   = iter->data;
              position = 0;
            }
          else
            {
              parent   = LIGMA_LAYER (ligma_item_get_parent (iter->data));
              position = ligma_item_get_index (iter->data);
            }
        }
      else /* run_once */
        {
          parent   = NULL;
          position = -1;
        }
      layer = ligma_layer_new (image,
                              ligma_image_get_width  (image),
                              ligma_image_get_height (image),
                              ligma_image_get_layer_format (image, TRUE),
                              config->layer_new_name,
                              config->layer_new_opacity,
                              layer_mode);

      ligma_drawable_fill (LIGMA_DRAWABLE (layer),
                          action_data_get_context (data),
                          config->layer_new_fill_type);
      ligma_layer_set_blend_space (layer,
                                  config->layer_new_blend_space, FALSE);
      ligma_layer_set_composite_space (layer,
                                      config->layer_new_composite_space, FALSE);
      ligma_layer_set_composite_mode (layer,
                                     config->layer_new_composite_mode, FALSE);

      ligma_image_add_layer (image, layer, parent, position, TRUE);
      new_layers = g_list_prepend (new_layers, layer);
    }
  ligma_image_set_selected_layers (image, new_layers);
  ligma_image_undo_group_end (image);

  g_list_free (layers);
  g_list_free (new_layers);
  ligma_image_flush (image);
}

void
layers_new_from_visible_cmd_callback (LigmaAction *action,
                                      GVariant   *value,
                                      gpointer    data)
{
  LigmaImage        *image;
  LigmaDisplayShell *shell;
  LigmaLayer        *layer;
  LigmaPickable     *pickable;
  LigmaColorProfile *profile;
  return_if_no_image (image, data);
  return_if_no_shell (shell, data);

  pickable = ligma_display_shell_get_canvas_pickable (shell);

  ligma_pickable_flush (pickable);

  profile = ligma_color_managed_get_color_profile (LIGMA_COLOR_MANAGED (image));

  layer = ligma_layer_new_from_gegl_buffer (ligma_pickable_get_buffer (pickable),
                                           image,
                                           ligma_image_get_layer_format (image,
                                                                        TRUE),
                                           _("Visible"),
                                           LIGMA_OPACITY_OPAQUE,
                                           ligma_image_get_default_new_layer_mode (image),
                                           profile);

  ligma_image_add_layer (image, layer, LIGMA_IMAGE_ACTIVE_PARENT, -1, TRUE);
  ligma_image_flush (image);
}

void
layers_new_group_cmd_callback (LigmaAction *action,
                               GVariant   *value,
                               gpointer    data)
{
  LigmaImage *image;
  GList     *new_layers = NULL;
  GList     *layers;
  GList     *iter;
  gint       n_layers;
  gboolean   run_once;

  return_if_no_image (image, data);

  layers     = ligma_image_get_selected_layers (image);
  layers     = g_list_copy (layers);
  n_layers   = g_list_length (layers);
  run_once   = (n_layers == 0);
  ligma_image_undo_group_start (image,
                               LIGMA_UNDO_GROUP_LAYER_ADD,
                               ngettext ("New layer group",
                                         "New layer groups",
                                         n_layers > 0 ? n_layers : 1));

  for (iter = layers; iter || run_once ; iter = iter ? iter->next : NULL)
    {
      LigmaLayer *layer;
      LigmaLayer *parent;
      gint       position;

      run_once = FALSE;
      if (iter)
        {
          if (ligma_viewable_get_children (LIGMA_VIEWABLE (iter->data)))
            {
              parent   = iter->data;
              position = 0;
            }
          else
            {
              parent   = LIGMA_LAYER (ligma_item_get_parent (iter->data));
              position = ligma_item_get_index (iter->data);
            }
        }
      else /* run_once */
        {
          parent   = NULL;
          position = -1;
        }
      layer = ligma_group_layer_new (image);

      ligma_image_add_layer (image, layer, parent, position, TRUE);
      new_layers = g_list_prepend (new_layers, layer);
    }

  ligma_image_set_selected_layers (image, new_layers);
  ligma_image_undo_group_end (image);
  ligma_image_flush (image);

  g_list_free (layers);
  g_list_free (new_layers);
}

void
layers_select_cmd_callback (LigmaAction *action,
                            GVariant   *value,
                            gpointer    data)
{
  LigmaImage            *image;
  GList                *new_layers = NULL;
  GList                *layers;
  GList                *iter;
  LigmaActionSelectType  select_type;
  gboolean              run_once;
  return_if_no_image (image, data);

  select_type = (LigmaActionSelectType) g_variant_get_int32 (value);

  layers   = ligma_image_get_selected_layers (image);
  run_once = (g_list_length (layers) == 0);

  for (iter = layers; iter || run_once; iter = iter ? iter->next : NULL)
    {
      LigmaLayer     *new_layer;
      LigmaContainer *container;

      if (iter)
        {
          container = ligma_item_get_container (LIGMA_ITEM (iter->data));
        }
      else /* run_once */
        {
          container = ligma_image_get_layers (image);
          run_once  = FALSE;
        }
      new_layer = (LigmaLayer *) action_select_object (select_type,
                                                      container,
                                                      iter ? iter->data : NULL);
      if (new_layer)
        new_layers = g_list_prepend (new_layers, new_layer);
    }

  if (new_layers)
    {
      ligma_image_set_selected_layers (image, new_layers);
      ligma_image_flush (image);
    }

  g_list_free (new_layers);
}

void
layers_raise_cmd_callback (LigmaAction *action,
                           GVariant   *value,
                           gpointer    data)
{
  LigmaImage *image;
  GList     *layers;
  GList     *iter;
  GList     *raised_layers = NULL;
  return_if_no_layers (image, layers, data);

  for (iter = layers; iter; iter = iter->next)
    {
      gint index;

      index = ligma_item_get_index (iter->data);
      if (index > 0)
        raised_layers = g_list_prepend (raised_layers, iter->data);
    }

  ligma_image_undo_group_start (image,
                               LIGMA_UNDO_GROUP_ITEM_DISPLACE,
                               ngettext ("Raise Layer",
                                         "Raise Layers",
                                         g_list_length (raised_layers)));
  for (iter = raised_layers; iter; iter = iter->next)
    ligma_image_raise_item (image, iter->data, NULL);

  ligma_image_flush (image);
  ligma_image_undo_group_end (image);

  g_list_free (raised_layers);
}

void
layers_raise_to_top_cmd_callback (LigmaAction *action,
                                  GVariant   *value,
                                  gpointer    data)
{
  LigmaImage *image;
  GList     *layers;
  GList     *iter;
  GList     *raised_layers = NULL;
  return_if_no_layers (image, layers, data);

  for (iter = layers; iter; iter = iter->next)
    {
      gint index;

      index = ligma_item_get_index (iter->data);
      if (index > 0)
        raised_layers = g_list_prepend (raised_layers, iter->data);
    }

  ligma_image_undo_group_start (image,
                               LIGMA_UNDO_GROUP_ITEM_DISPLACE,
                               ngettext ("Raise Layer to Top",
                                         "Raise Layers to Top",
                                         g_list_length (raised_layers)));

  for (iter = raised_layers; iter; iter = iter->next)
    ligma_image_raise_item_to_top (image, iter->data);

  ligma_image_flush (image);
  ligma_image_undo_group_end (image);

  g_list_free (raised_layers);
}

void
layers_lower_cmd_callback (LigmaAction *action,
                           GVariant   *value,
                           gpointer    data)
{
  LigmaImage *image;
  GList     *layers;
  GList     *iter;
  GList     *lowered_layers = NULL;
  return_if_no_layers (image, layers, data);

  for (iter = layers; iter; iter = iter->next)
    {
      GList *layer_list;
      gint   index;

      layer_list = ligma_item_get_container_iter (LIGMA_ITEM (iter->data));
      index = ligma_item_get_index (iter->data);
      if (index < g_list_length (layer_list) - 1)
        lowered_layers = g_list_prepend (lowered_layers, iter->data);
    }

  ligma_image_undo_group_start (image,
                               LIGMA_UNDO_GROUP_ITEM_DISPLACE,
                               ngettext ("Lower Layer",
                                         "Lower Layers",
                                         g_list_length (lowered_layers)));

  for (iter = lowered_layers; iter; iter = iter->next)
    ligma_image_lower_item (image, iter->data, NULL);

  ligma_image_flush (image);
  ligma_image_undo_group_end (image);

  g_list_free (lowered_layers);
}

void
layers_lower_to_bottom_cmd_callback (LigmaAction *action,
                                     GVariant   *value,
                                     gpointer    data)
{
  LigmaImage *image;
  GList     *layers;
  GList     *iter;
  GList     *lowered_layers = NULL;
  return_if_no_layers (image, layers, data);

  for (iter = layers; iter; iter = iter->next)
    {
      GList *layer_list;
      gint   index;

      layer_list = ligma_item_get_container_iter (LIGMA_ITEM (iter->data));
      index = ligma_item_get_index (iter->data);
      if (index < g_list_length (layer_list) - 1)
        lowered_layers = g_list_prepend (lowered_layers, iter->data);
    }

  ligma_image_undo_group_start (image,
                               LIGMA_UNDO_GROUP_ITEM_DISPLACE,
                               ngettext ("Lower Layer to Bottom",
                                         "Lower Layers to Bottom",
                                         g_list_length (lowered_layers)));

  for (iter = lowered_layers; iter; iter = iter->next)
    ligma_image_lower_item_to_bottom (image, iter->data);

  ligma_image_flush (image);
  ligma_image_undo_group_end (image);

  g_list_free (lowered_layers);
}

void
layers_duplicate_cmd_callback (LigmaAction *action,
                               GVariant   *value,
                               gpointer    data)
{
  LigmaImage *image;
  GList     *layers;
  GList     *new_layers = NULL;
  GList     *iter;
  return_if_no_layers (image, layers, data);

  layers = g_list_copy (layers);
  ligma_image_undo_group_start (image,
                               LIGMA_UNDO_GROUP_LAYER_ADD,
                               _("Duplicate layers"));
  for (iter = layers; iter; iter = iter->next)
    {
      LigmaLayer *new_layer;

      new_layer = LIGMA_LAYER (ligma_item_duplicate (LIGMA_ITEM (iter->data),
                                                   G_TYPE_FROM_INSTANCE (iter->data)));

      /*  use the actual parent here, not LIGMA_IMAGE_ACTIVE_PARENT because
       *  the latter would add a duplicated group inside itself instead of
       *  above it
       */
      ligma_image_add_layer (image, new_layer,
                            ligma_layer_get_parent (iter->data),
                            ligma_item_get_index (iter->data),
                            TRUE);
      new_layers = g_list_prepend (new_layers, new_layer);
    }

  ligma_image_set_selected_layers (image, new_layers);
  g_list_free (layers);
  g_list_free (new_layers);

  ligma_image_undo_group_end (image);
  ligma_image_flush (image);
}

void
layers_anchor_cmd_callback (LigmaAction *action,
                            GVariant   *value,
                            gpointer    data)
{
  LigmaImage *image;
  GList     *layers;
  return_if_no_layers (image, layers, data);

  if (g_list_length (layers) == 1 &&
      ligma_layer_is_floating_sel (layers->data))
    {
      floating_sel_anchor (layers->data);
      ligma_image_flush (image);
    }
}

void
layers_merge_down_cmd_callback (LigmaAction *action,
                                GVariant   *value,
                                gpointer    data)
{
  LigmaImage   *image;
  GList       *layers;
  LigmaDisplay *display;
  GError      *error = NULL;

  return_if_no_layers (image, layers, data);
  return_if_no_display (display, data);

  layers = ligma_image_merge_down (image, layers, action_data_get_context (data),
                                  LIGMA_EXPAND_AS_NECESSARY,
                                  LIGMA_PROGRESS (display), &error);

  if (error)
    {
      ligma_message_literal (image->ligma,
                            G_OBJECT (display), LIGMA_MESSAGE_WARNING,
                            error->message);
      g_clear_error (&error);
      return;
    }
  ligma_image_set_selected_layers (image, layers);
  g_list_free (layers);

  ligma_image_flush (image);
}

void
layers_merge_group_cmd_callback (LigmaAction *action,
                                 GVariant   *value,
                                 gpointer    data)
{
  LigmaImage *image;
  GList     *layers;
  GList     *merge_layers = NULL;
  GList     *iter;
  return_if_no_layers (image, layers, data);

  for (iter = layers; iter; iter = iter->next)
    {
      if (ligma_viewable_get_children (LIGMA_VIEWABLE (iter->data)))
        {
          GList *iter2;

          for (iter2 = layers; iter2; iter2 = iter2->next)
            {
              /* Do not merge a layer when we already merge one of its
               * ancestors.
               */
              if (ligma_viewable_is_ancestor (iter2->data, iter->data))
                break;
            }

          if (iter2 == NULL)
            merge_layers = g_list_prepend (merge_layers, iter->data);
        }
    }

  if (g_list_length (merge_layers) > 1)
    {
      gchar *undo_name;

      undo_name = g_strdup_printf (C_("undo-type", "Merge %d Layer Groups"),
                                   g_list_length (merge_layers));
      ligma_image_undo_group_start (image, LIGMA_UNDO_GROUP_IMAGE_LAYERS_MERGE,
                                   undo_name);
      g_free (undo_name);
    }

  for (iter = merge_layers; iter; iter = iter->next)
    ligma_image_merge_group_layer (image, LIGMA_GROUP_LAYER (iter->data));

  if (g_list_length (merge_layers) > 1)
    ligma_image_undo_group_end (image);

  g_list_free (merge_layers);
  ligma_image_flush (image);
}

void
layers_delete_cmd_callback (LigmaAction *action,
                            GVariant   *value,
                            gpointer    data)
{
  LigmaImage *image;
  GList     *layers;
  GList     *removed_layers;
  GList     *iter;
  GList     *iter2;

  return_if_no_image (image, data);

  layers = ligma_image_get_selected_layers (image);

  /*TODO: we should have a failsafe to determine when we are going to
   * delete all layers (i.e. all layers of first level at least) and
   * forbid it. */

  /* Copy of the original selection. */
  removed_layers = g_list_copy (layers);

  /* Removing children layers (they will be removed anyway by removing
   * the parent.
   */
  for (iter = removed_layers; iter; iter = iter->next)
    {
      for (iter2 = removed_layers; iter2; iter2 = iter2->next)
        {
          if (iter->data != iter2->data &&
              ligma_viewable_is_ancestor (iter2->data, iter->data))
            {
              removed_layers = g_list_delete_link (removed_layers, iter);
              iter = removed_layers;
              break;
            }
        }
    }

  if (g_list_length (removed_layers) > 1)
    {
      gchar *undo_name;

      undo_name = g_strdup_printf (C_("undo-type", "Remove %d Layers"),
                                   g_list_length (removed_layers));
      ligma_image_undo_group_start (image, LIGMA_UNDO_GROUP_IMAGE_ITEM_REMOVE,
                                   undo_name);
    }

  for (iter = removed_layers; iter; iter = iter->next)
    ligma_image_remove_layer (image, iter->data, TRUE, NULL);

  if (g_list_length (removed_layers) > 1)
    ligma_image_undo_group_end (image);

  g_list_free (removed_layers);
  ligma_image_flush (image);
}

void
layers_text_discard_cmd_callback (LigmaAction *action,
                                  GVariant   *value,
                                  gpointer    data)
{
  LigmaImage *image;
  LigmaLayer *layer;
  return_if_no_layer (image, layer, data);

  if (LIGMA_IS_TEXT_LAYER (layer))
    ligma_text_layer_discard (LIGMA_TEXT_LAYER (layer));
}

void
layers_text_to_vectors_cmd_callback (LigmaAction *action,
                                     GVariant   *value,
                                     gpointer    data)
{
  LigmaImage *image;
  LigmaLayer *layer;
  return_if_no_layer (image, layer, data);

  if (LIGMA_IS_TEXT_LAYER (layer))
    {
      LigmaVectors *vectors;
      gint         x, y;

      vectors = ligma_text_vectors_new (image, LIGMA_TEXT_LAYER (layer)->text);

      ligma_item_get_offset (LIGMA_ITEM (layer), &x, &y);
      ligma_item_translate (LIGMA_ITEM (vectors), x, y, FALSE);

      ligma_image_add_vectors (image, vectors,
                              LIGMA_IMAGE_ACTIVE_PARENT, -1, TRUE);
      ligma_image_flush (image);
    }
}

void
layers_text_along_vectors_cmd_callback (LigmaAction *action,
                                        GVariant   *value,
                                        gpointer    data)
{
  LigmaImage   *image;
  LigmaLayer   *layer;
  LigmaVectors *vectors;
  return_if_no_layer (image, layer, data);
  return_if_no_vectors (image, vectors, data);

  if (LIGMA_IS_TEXT_LAYER (layer))
    {
      gdouble      box_width;
      gdouble      box_height;
      LigmaVectors *new_vectors;
      gdouble      offset;

      box_width  = ligma_item_get_width  (LIGMA_ITEM (layer));
      box_height = ligma_item_get_height (LIGMA_ITEM (layer));

      new_vectors = ligma_text_vectors_new (image, LIGMA_TEXT_LAYER (layer)->text);

      offset = 0;
      switch (LIGMA_TEXT_LAYER (layer)->text->base_dir)
        {
        case LIGMA_TEXT_DIRECTION_LTR:
        case LIGMA_TEXT_DIRECTION_RTL:
          offset = 0.5 * box_height;
          break;
        case LIGMA_TEXT_DIRECTION_TTB_RTL:
        case LIGMA_TEXT_DIRECTION_TTB_RTL_UPRIGHT:
        case LIGMA_TEXT_DIRECTION_TTB_LTR:
        case LIGMA_TEXT_DIRECTION_TTB_LTR_UPRIGHT:
          {
            LigmaStroke *stroke = NULL;

            while ((stroke = ligma_vectors_stroke_get_next (new_vectors, stroke)))
              {
                ligma_stroke_rotate (stroke, 0, 0, 270);
                ligma_stroke_translate (stroke, 0, box_width);
              }
          }
          offset = 0.5 * box_width;
          break;
        }


      ligma_vectors_warp_vectors (vectors, new_vectors, offset);

      ligma_item_set_visible (LIGMA_ITEM (new_vectors), TRUE, FALSE);

      ligma_image_add_vectors (image, new_vectors,
                              LIGMA_IMAGE_ACTIVE_PARENT, -1, TRUE);
      ligma_image_flush (image);
    }
}

void
layers_resize_cmd_callback (LigmaAction *action,
                            GVariant   *value,
                            gpointer    data)
{
  LigmaImage *image;
  LigmaLayer *layer;
  GList     *layers;
  GtkWidget *widget;
  GtkWidget *dialog;
  return_if_no_layers (image, layers, data);
  return_if_no_widget (widget, data);

#define RESIZE_DIALOG_KEY "ligma-resize-dialog"

  g_return_if_fail (g_list_length (layers) == 1);

  layer = layers->data;

  dialog = dialogs_get_dialog (G_OBJECT (layer), RESIZE_DIALOG_KEY);

  if (! dialog)
    {
      LigmaDialogConfig *config  = LIGMA_DIALOG_CONFIG (image->ligma->config);
      LigmaDisplay      *display = NULL;

      if (LIGMA_IS_IMAGE_WINDOW (data))
        display = action_data_get_display (data);

      if (layer_resize_unit != LIGMA_UNIT_PERCENT && display)
        layer_resize_unit = ligma_display_get_shell (display)->unit;

      dialog = resize_dialog_new (LIGMA_VIEWABLE (layer),
                                  action_data_get_context (data),
                                  _("Set Layer Boundary Size"),
                                  "ligma-layer-resize",
                                  widget,
                                  ligma_standard_help_func,
                                  LIGMA_HELP_LAYER_RESIZE,
                                  layer_resize_unit,
                                  config->layer_resize_fill_type,
                                  LIGMA_ITEM_SET_NONE,
                                  FALSE,
                                  layers_resize_callback,
                                  NULL);

      dialogs_attach_dialog (G_OBJECT (layer), RESIZE_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

void
layers_resize_to_image_cmd_callback (LigmaAction *action,
                                     GVariant   *value,
                                     gpointer    data)
{
  LigmaImage *image;
  GList     *layers;
  GList     *iter;
  return_if_no_layers (image, layers, data);

  if (g_list_length (layers) > 1)
    ligma_image_undo_group_start (image, LIGMA_UNDO_GROUP_ITEM_RESIZE,
                                 _("Layers to Image Size"));
  for (iter = layers; iter; iter = iter->next)
    ligma_layer_resize_to_image (iter->data,
                                action_data_get_context (data),
                                LIGMA_FILL_TRANSPARENT);

  if (g_list_length (layers) > 1)
    ligma_image_undo_group_end (image);

  ligma_image_flush (image);
}

void
layers_scale_cmd_callback (LigmaAction *action,
                           GVariant   *value,
                           gpointer    data)
{
  LigmaImage *image;
  GList     *layers;
  LigmaLayer *layer;
  GtkWidget *widget;
  GtkWidget *dialog;
  return_if_no_layers (image, layers, data);
  return_if_no_widget (widget, data);

#define SCALE_DIALOG_KEY "ligma-scale-dialog"

  g_return_if_fail (g_list_length (layers) == 1);

  layer = layers->data;

  dialog = dialogs_get_dialog (G_OBJECT (layer), SCALE_DIALOG_KEY);

  if (! dialog)
    {
      LigmaDisplay *display = NULL;

      if (LIGMA_IS_IMAGE_WINDOW (data))
        display = action_data_get_display (data);

      if (layer_scale_unit != LIGMA_UNIT_PERCENT && display)
        layer_scale_unit = ligma_display_get_shell (display)->unit;

      if (layer_scale_interp == -1)
        layer_scale_interp = image->ligma->config->interpolation_type;

      dialog = scale_dialog_new (LIGMA_VIEWABLE (layer),
                                 action_data_get_context (data),
                                 _("Scale Layer"), "ligma-layer-scale",
                                 widget,
                                 ligma_standard_help_func, LIGMA_HELP_LAYER_SCALE,
                                 layer_scale_unit,
                                 layer_scale_interp,
                                 layers_scale_callback,
                                 display);

      dialogs_attach_dialog (G_OBJECT (layer), SCALE_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

void
layers_crop_to_selection_cmd_callback (LigmaAction *action,
                                       GVariant   *value,
                                       gpointer    data)
{
  LigmaImage *image;
  GList     *layers;
  GList     *iter;
  GtkWidget *widget;
  gchar     *desc;
  gint       x, y;
  gint       width, height;
  return_if_no_layers (image, layers, data);
  return_if_no_widget (widget, data);

  if (! ligma_item_bounds (LIGMA_ITEM (ligma_image_get_mask (image)),
                          &x, &y, &width, &height))
    {
      ligma_message_literal (image->ligma,
                            G_OBJECT (widget), LIGMA_MESSAGE_WARNING,
                            _("Cannot crop because the current selection "
                              "is empty."));
      return;
    }

  desc = g_strdup_printf (ngettext ("Crop Layer to Selection",
                                    "Crop %d Layers to Selection",
                                    g_list_length (layers)),
                          g_list_length (layers));
  ligma_image_undo_group_start (image, LIGMA_UNDO_GROUP_ITEM_RESIZE, desc);
  g_free (desc);

  for (iter = layers; iter; iter = iter->next)
    {
      gint off_x, off_y;

      ligma_item_get_offset (LIGMA_ITEM (iter->data), &off_x, &off_y);
      off_x -= x;
      off_y -= y;

      ligma_item_resize (LIGMA_ITEM (iter->data),
                        action_data_get_context (data), LIGMA_FILL_TRANSPARENT,
                        width, height, off_x, off_y);
    }

  ligma_image_undo_group_end (image);
  ligma_image_flush (image);
}

void
layers_crop_to_content_cmd_callback (LigmaAction *action,
                                     GVariant   *value,
                                     gpointer    data)
{
  LigmaImage *image;
  GList     *layers;
  GList     *iter;
  GtkWidget *widget;
  gchar     *desc;
  gint       x, y;
  gint       width, height;
  gint       n_croppable = 0;
  return_if_no_layers (image, layers, data);
  return_if_no_widget (widget, data);

  for (iter = layers; iter; iter = iter->next)
    {
      switch (ligma_pickable_auto_shrink (LIGMA_PICKABLE (iter->data),
                                         0, 0,
                                         ligma_item_get_width  (iter->data),
                                         ligma_item_get_height (iter->data),
                                         &x, &y, &width, &height))
        {
        case LIGMA_AUTO_SHRINK_SHRINK:
          n_croppable++;
          break;

        case LIGMA_AUTO_SHRINK_EMPTY:
          /* Cannot crop because the layer has no content. */
        case LIGMA_AUTO_SHRINK_UNSHRINKABLE:
          /* Cannot crop because the active layer is already cropped to
           * its content. */
          break;
        }
    }

  if (n_croppable == 0)
    {
      ligma_message_literal (image->ligma,
                            G_OBJECT (widget), LIGMA_MESSAGE_INFO,
                            _("Cannot crop because none of the selected"
                              " layers have content or they are already"
                              " cropped to their content."));
      return;
    }

  desc = g_strdup_printf (ngettext ("Crop Layer to Content",
                                    "Crop %d Layers to Content",
                                    n_croppable),
                          n_croppable);
  ligma_image_undo_group_start (image, LIGMA_UNDO_GROUP_ITEM_RESIZE, desc);
  g_free (desc);

  for (iter = layers; iter; iter = iter->next)
    {
      switch (ligma_pickable_auto_shrink (LIGMA_PICKABLE (iter->data),
                                         0, 0,
                                         ligma_item_get_width  (iter->data),
                                         ligma_item_get_height (iter->data),
                                         &x, &y, &width, &height))
        {
        case LIGMA_AUTO_SHRINK_SHRINK:
          ligma_item_resize (iter->data,
                            action_data_get_context (data), LIGMA_FILL_TRANSPARENT,
                            width, height, -x, -y);

          break;
        default:
          break;
        }
    }
  ligma_image_flush (image);
  ligma_image_undo_group_end (image);
}

void
layers_mask_add_cmd_callback (LigmaAction *action,
                              GVariant   *value,
                              gpointer    data)
{
  LigmaImage *image;
  GList     *layers;
  GList     *iter;
  GtkWidget *widget;
  GtkWidget *dialog;
  GList     *update_layers = NULL;
  gint       n_channels = 0;
  return_if_no_layers (image, layers, data);
  return_if_no_widget (widget, data);

  for (iter = layers; iter; iter = iter->next)
    {
      g_return_if_fail (LIGMA_IS_LAYER (iter->data));

      if (! ligma_layer_get_mask (iter->data))
        {
          update_layers = g_list_prepend (update_layers, iter->data);
          n_channels++;
        }
    }
  if (n_channels == 0)
    /* No layers or they all have masks already. */
    return;

#define ADD_MASK_DIALOG_KEY "ligma-add-mask-dialog"

  for (iter = update_layers; iter; iter = iter->next)
    {
      dialog = dialogs_get_dialog (G_OBJECT (iter->data), ADD_MASK_DIALOG_KEY);
      if (dialog)
        break;
    }

  if (! dialog)
    {
      LigmaDialogConfig *config = LIGMA_DIALOG_CONFIG (image->ligma->config);

      dialog = layer_add_mask_dialog_new (update_layers, action_data_get_context (data),
                                          widget,
                                          config->layer_add_mask_type,
                                          config->layer_add_mask_invert,
                                          layers_add_mask_callback,
                                          NULL);

      for (iter = update_layers; iter; iter = iter->next)
        dialogs_attach_dialog (G_OBJECT (iter->data), ADD_MASK_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

void
layers_mask_add_last_vals_cmd_callback (LigmaAction *action,
                                        GVariant   *value,
                                        gpointer    data)
{
  LigmaImage        *image;
  GList            *layers;
  GList            *iter;
  GtkWidget        *widget;
  LigmaDialogConfig *config;
  LigmaChannel      *channel = NULL;
  LigmaLayerMask    *mask;
  return_if_no_layers (image, layers, data);
  return_if_no_widget (widget, data);

  config = LIGMA_DIALOG_CONFIG (image->ligma->config);

  if (config->layer_add_mask_type == LIGMA_ADD_MASK_CHANNEL)
    {
      GList *selected_channels;

      selected_channels = ligma_image_get_selected_channels (image);

      if (selected_channels)
        {
          channel = selected_channels->data;
        }
      else
        {
          LigmaContainer *channels = ligma_image_get_channels (image);

          channel = LIGMA_CHANNEL (ligma_container_get_first_child (channels));
        }

      if (! channel)
        {
          layers_mask_add_cmd_callback (action, value, data);
          return;
        }
    }

  for (iter = layers; iter; iter = iter->next)
    {
      if (! ligma_layer_get_mask (iter->data))
        break;
    }
  if (iter == NULL)
    /* No layers or they all have masks already. */
    return;

  ligma_image_undo_group_start (image,
                               LIGMA_UNDO_GROUP_LAYER_ADD,
                               _("Add Layer Masks"));
  for (iter = layers; iter; iter = iter->next)
    {
      if (ligma_layer_get_mask (iter->data))
        continue;

      mask = ligma_layer_create_mask (iter->data,
                                     config->layer_add_mask_type,
                                     channel);

      if (config->layer_add_mask_invert)
        ligma_channel_invert (LIGMA_CHANNEL (mask), FALSE);

      ligma_layer_add_mask (iter->data, mask, TRUE, NULL);
    }

  ligma_image_undo_group_end (image);

  ligma_image_flush (image);
}

void
layers_mask_apply_cmd_callback (LigmaAction *action,
                                GVariant   *value,
                                gpointer    data)
{
  LigmaMaskApplyMode  mode;
  LigmaImage         *image;
  GList             *layers;
  GList             *iter;
  gchar             *undo_text = NULL;
  LigmaUndoType       undo_type = LIGMA_UNDO_GROUP_NONE;

  return_if_no_layers (image, layers, data);

  mode = (LigmaMaskApplyMode) g_variant_get_int32 (value);
  for (iter = layers; iter; iter = iter->next)
    {
      if (ligma_layer_get_mask (iter->data) &&
          (mode != LIGMA_MASK_APPLY ||
           (! ligma_viewable_get_children (LIGMA_VIEWABLE (iter->data)) &&
            ! ligma_item_is_content_locked (LIGMA_ITEM (iter->data), NULL))))
        break;
    }
  if (iter == NULL)
    /* No layers or none have applicable masks. */
    return;

  switch (mode)
    {
    case LIGMA_MASK_APPLY:
      undo_type = LIGMA_UNDO_GROUP_MASK;
      undo_text = _("Apply Layer Masks");
      break;
    case LIGMA_MASK_DISCARD:
      undo_type = LIGMA_UNDO_GROUP_MASK;
      undo_text = _("Delete Layer Masks");
      break;
    default:
      g_warning ("%s: unhandled LigmaMaskApplyMode %d\n",
                 G_STRFUNC, mode);
      break;
    }

  if (undo_type != LIGMA_UNDO_GROUP_NONE)
    ligma_image_undo_group_start (image, undo_type, undo_text);

  for (iter = layers; iter; iter = iter->next)
    {
      if (ligma_layer_get_mask (iter->data))
        {
          if (mode == LIGMA_MASK_APPLY &&
              (ligma_viewable_get_children (LIGMA_VIEWABLE (iter->data)) ||
               ligma_item_is_content_locked (LIGMA_ITEM (iter->data), NULL)))
            /* Layer groups cannot apply masks. Neither can
             * content-locked layers.
             */
            continue;

          ligma_layer_apply_mask (iter->data, mode, TRUE);
        }
    }

  if (undo_type != LIGMA_UNDO_GROUP_NONE)
    ligma_image_undo_group_end (image);

  ligma_image_flush (image);
}

void
layers_mask_edit_cmd_callback (LigmaAction *action,
                               GVariant   *value,
                               gpointer    data)
{
  LigmaImage *image;
  GList     *layers;
  GList     *iter;
  gboolean   active = g_variant_get_boolean (value);
  return_if_no_layers (image, layers, data);

  /* Multiple-layer selection cannot edit masks. */
  active = active && (g_list_length (layers) == 1);

  for (iter = layers; iter; iter = iter->next)
    if (ligma_layer_get_mask (iter->data))
      ligma_layer_set_edit_mask (iter->data, active);

  ligma_image_flush (image);
}

void
layers_mask_show_cmd_callback (LigmaAction *action,
                               GVariant   *value,
                               gpointer    data)
{
  LigmaImage *image;
  GList     *layers;
  GList     *iter;
  gboolean   active     = g_variant_get_boolean (value);
  gboolean   have_masks = FALSE;
  return_if_no_layers (image, layers, data);

  for (iter = layers; iter; iter = iter->next)
    {
      if (ligma_layer_get_mask (iter->data))
        {
          have_masks = TRUE;
          /* A bit of tricky to handle multiple and diverse layers with
           * a toggle action (with only binary state).
           * In non-active state, we will consider sets of both shown
           * and hidden masks as ok and exits. This allows us to switch
           * the action "active" state without actually changing
           * individual masks state without explicit user request.
           */
          if (! active && ! ligma_layer_get_show_mask (iter->data))
            return;
        }
    }
  if (! have_masks)
    return;

  ligma_image_undo_group_start (image,
                               LIGMA_UNDO_GROUP_LAYER_ADD,
                               _("Show Layer Masks"));

  for (iter = layers; iter; iter = iter->next)
    {
      if (ligma_layer_get_mask (iter->data))
        {
          ligma_layer_set_show_mask (iter->data, active, TRUE);
        }
    }

  ligma_image_flush (image);
  ligma_image_undo_group_end (image);
}

void
layers_mask_disable_cmd_callback (LigmaAction *action,
                                  GVariant   *value,
                                  gpointer    data)
{
  LigmaImage *image;
  GList     *layers;
  GList     *iter;
  gboolean   active = g_variant_get_boolean (value);
  gboolean   have_masks = FALSE;
  return_if_no_layers (image, layers, data);

  for (iter = layers; iter; iter = iter->next)
    {
      if (ligma_layer_get_mask (iter->data))
        {
          have_masks = TRUE;
          /* A bit of tricky to handle multiple and diverse layers with
           * a toggle action (with only binary state).
           * In non-active state, we will consider sets of both enabled
           * and disabled masks as ok and exits. This allows us to
           * switch the action "active" state without actually changing
           * individual masks state without explicit user request.
           */
          if (! active && ligma_layer_get_apply_mask (iter->data))
            return;
        }
    }
  if (! have_masks)
    return;

  ligma_image_undo_group_start (image,
                               LIGMA_UNDO_GROUP_LAYER_ADD,
                               _("Disable Layer Masks"));

  for (iter = layers; iter; iter = iter->next)
    {
      if (ligma_layer_get_mask (iter->data))
        {
          ligma_layer_set_apply_mask (iter->data, ! active, TRUE);
        }
    }

  ligma_image_flush (image);
  ligma_image_undo_group_end (image);
}

void
layers_mask_to_selection_cmd_callback (LigmaAction *action,
                                       GVariant   *value,
                                       gpointer    data)
{
  LigmaImage     *image;
  GList         *layers;
  GList         *iter;
  GList         *masks = NULL;
  return_if_no_layers (image, layers, data);

  for (iter = layers; iter; iter = iter->next)
    {
      if (ligma_layer_get_mask (iter->data))
        masks = g_list_prepend (masks, ligma_layer_get_mask (iter->data));
    }

  if (masks)
    {
      LigmaChannelOps operation = (LigmaChannelOps) g_variant_get_int32 (value);

      switch (operation)
        {
        case LIGMA_CHANNEL_OP_REPLACE:
          ligma_channel_push_undo (ligma_image_get_mask (image),
                                  C_("undo-type", "Masks to Selection"));
          break;
        case LIGMA_CHANNEL_OP_ADD:
          ligma_channel_push_undo (ligma_image_get_mask (image),
                                  C_("undo-type", "Add Masks to Selection"));
          break;
        case LIGMA_CHANNEL_OP_SUBTRACT:
          ligma_channel_push_undo (ligma_image_get_mask (image),
                                  C_("undo-type", "Subtract Masks from Selection"));
          break;
        case LIGMA_CHANNEL_OP_INTERSECT:
          ligma_channel_push_undo (ligma_image_get_mask (image),
                                  C_("undo-type", "Intersect Masks with Selection"));
          break;
        }
      ligma_channel_combine_items (ligma_image_get_mask (image),
                                  masks, operation);
      ligma_image_flush (image);
      g_list_free (masks);
    }
}

void
layers_alpha_add_cmd_callback (LigmaAction *action,
                               GVariant   *value,
                               gpointer    data)
{
  LigmaImage *image;
  GList     *layers;
  GList     *iter;
  return_if_no_layers (image, layers, data);

  ligma_image_undo_group_start (image, LIGMA_UNDO_GROUP_LAYER_ADD_ALPHA,
                               _("Add Alpha Channel"));

  for (iter = layers; iter; iter = iter->next)
    if (! ligma_drawable_has_alpha (iter->data))
      ligma_layer_add_alpha (iter->data);

  ligma_image_undo_group_end (image);
  ligma_image_flush (image);
}

void
layers_alpha_remove_cmd_callback (LigmaAction *action,
                                  GVariant   *value,
                                  gpointer    data)
{
  LigmaImage *image;
  GList     *layers;
  GList     *iter;
  return_if_no_layers (image, layers, data);

  ligma_image_undo_group_start (image, LIGMA_UNDO_GROUP_LAYER_ADD_ALPHA,
                               _("Remove Alpha Channel"));

  for (iter = layers; iter; iter = iter->next)
    if (ligma_drawable_has_alpha (iter->data))
      ligma_layer_remove_alpha (iter->data, action_data_get_context (data));

  ligma_image_undo_group_end (image);
  ligma_image_flush (image);
}

void
layers_alpha_to_selection_cmd_callback (LigmaAction *action,
                                        GVariant   *value,
                                        gpointer    data)
{
  LigmaImage      *image;
  LigmaDisplay    *display;
  GList          *layers;
  LigmaChannelOps  operation;
  return_if_no_layers (image, layers, data);
  return_if_no_display (display, data);

  operation = (LigmaChannelOps) g_variant_get_int32 (value);

  switch (operation)
    {
    case LIGMA_CHANNEL_OP_REPLACE:
      ligma_channel_push_undo (ligma_image_get_mask (image),
                              C_("undo-type", "Alpha to Selection"));
      break;
    case LIGMA_CHANNEL_OP_ADD:
      ligma_channel_push_undo (ligma_image_get_mask (image),
                              C_("undo-type", "Add Alpha to Selection"));
      break;
    case LIGMA_CHANNEL_OP_SUBTRACT:
      ligma_channel_push_undo (ligma_image_get_mask (image),
                              C_("undo-type", "Subtract Alpha from Selection"));
      break;
    case LIGMA_CHANNEL_OP_INTERSECT:
      ligma_channel_push_undo (ligma_image_get_mask (image),
                              C_("undo-type", "Intersect Alpha with Selection"));
      break;
    }
  ligma_channel_combine_items (ligma_image_get_mask (image),
                              layers, operation);
  ligma_image_flush (image);

  if (ligma_channel_is_empty (ligma_image_get_mask (image)))
    {
      ligma_message_literal (image->ligma, G_OBJECT (display),
                            LIGMA_MESSAGE_WARNING,
                            _("Empty Selection"));
    }
}

void
layers_opacity_cmd_callback (LigmaAction *action,
                             GVariant   *value,
                             gpointer    data)
{
  LigmaImage            *image;
  GList                *layers;
  GList                *iter;
  gdouble               opacity;
  LigmaUndo             *undo;
  LigmaActionSelectType  select_type;
  gboolean        push_undo = TRUE;
  return_if_no_layers (image, layers, data);

  select_type = (LigmaActionSelectType) g_variant_get_int32 (value);
  if (g_list_length (layers) == 1)
    {
      undo = ligma_image_undo_can_compress (image, LIGMA_TYPE_ITEM_UNDO,
                                           LIGMA_UNDO_LAYER_OPACITY);

      if (undo && LIGMA_ITEM_UNDO (undo)->item == LIGMA_ITEM (layers->data))
        push_undo = FALSE;
    }

  if (g_list_length (layers) > 1)
    ligma_image_undo_group_start (image, LIGMA_UNDO_GROUP_LAYER_OPACITY,
                                 _("Set layers opacity"));

  for (iter = layers; iter; iter = iter->next)
    {
      opacity = action_select_value (select_type,
                                     ligma_layer_get_opacity (iter->data),
                                     0.0, 1.0, 1.0,
                                     1.0 / 255.0, 0.01, 0.1, 0.0, FALSE);
      ligma_layer_set_opacity (iter->data, opacity, push_undo);
    }

  if (g_list_length (layers) > 1)
    ligma_image_undo_group_end (image);

  ligma_image_flush (image);
}

void
layers_mode_cmd_callback (LigmaAction *action,
                          GVariant   *value,
                          gpointer    data)
{
  LigmaImage            *image;
  GList                *layers;
  GList                *iter;
  LigmaActionSelectType  select_type;
  gboolean              push_undo = TRUE;
  return_if_no_layers (image, layers, data);

  select_type = (LigmaActionSelectType) g_variant_get_int32 (value);

  if (g_list_length (layers) == 1)
    {
      LigmaUndo *undo;

      undo = ligma_image_undo_can_compress (image, LIGMA_TYPE_ITEM_UNDO,
                                           LIGMA_UNDO_LAYER_MODE);

      if (undo && LIGMA_ITEM_UNDO (undo)->item == LIGMA_ITEM (layers->data))
        push_undo = FALSE;
    }

  if (g_list_length (layers) > 1)
    ligma_image_undo_group_start (image, LIGMA_UNDO_GROUP_LAYER_OPACITY,
                                 _("Set layers opacity"));

  for (iter = layers; iter; iter = iter->next)
    {
      LigmaLayerMode *modes;
      gint           n_modes;
      LigmaLayerMode  layer_mode;
      gint           index;

      layer_mode = ligma_layer_get_mode (iter->data);

      modes = ligma_layer_mode_get_context_array (layer_mode,
                                                 LIGMA_LAYER_MODE_CONTEXT_LAYER,
                                                 &n_modes);
      index = layers_mode_index (layer_mode, modes, n_modes);
      index = action_select_value (select_type,
                                   index, 0, n_modes - 1, 0,
                                   0.0, 1.0, 1.0, 0.0, FALSE);
      layer_mode = modes[index];
      g_free (modes);

      ligma_layer_set_mode (iter->data, layer_mode, push_undo);
    }

  if (g_list_length (layers) > 1)
    ligma_image_undo_group_end (image);

  ligma_image_flush (image);
}

void
layers_blend_space_cmd_callback (LigmaAction *action,
                                 GVariant   *value,
                                 gpointer    data)
{
  LigmaImage           *image;
  GList               *layers;
  GList               *update_layers = NULL;
  GList               *iter;
  LigmaLayerColorSpace  blend_space;
  gboolean             push_undo = TRUE;
  return_if_no_layers (image, layers, data);

  blend_space = (LigmaLayerColorSpace) g_variant_get_int32 (value);

  for (iter = layers; iter; iter = iter->next)
    {
      LigmaLayerMode mode;

      mode = ligma_layer_get_mode (iter->data);
      if (ligma_layer_mode_is_blend_space_mutable (mode) &&
          blend_space != ligma_layer_get_blend_space (iter->data))
        update_layers = g_list_prepend (update_layers, iter->data);
    }

  if (g_list_length (update_layers) == 1)
    {
      LigmaUndo *undo;

      undo = ligma_image_undo_can_compress (image, LIGMA_TYPE_LAYER_PROP_UNDO,
                                           LIGMA_UNDO_LAYER_MODE);

      if (undo && LIGMA_ITEM_UNDO (undo)->item == LIGMA_ITEM (update_layers->data))
        push_undo = FALSE;
    }

  if (update_layers)
    {
      if (g_list_length (update_layers) > 1)
        ligma_image_undo_group_start (image, LIGMA_UNDO_GROUP_LAYER_MODE,
                                     _("Set layers' blend space"));

      for (iter = update_layers; iter; iter = iter->next)
        ligma_layer_set_blend_space (iter->data, blend_space, push_undo);

      if (g_list_length (update_layers) > 1)
        ligma_image_undo_group_end (image);

      g_list_free (update_layers);
      ligma_image_flush (image);
    }
}

void
layers_composite_space_cmd_callback (LigmaAction *action,
                                     GVariant   *value,
                                     gpointer    data)
{
  LigmaImage           *image;
  GList               *layers;
  GList               *update_layers = NULL;
  GList               *iter;
  LigmaLayerColorSpace  composite_space;
  gboolean             push_undo = TRUE;
  return_if_no_layers (image, layers, data);

  composite_space = (LigmaLayerColorSpace) g_variant_get_int32 (value);

  for (iter = layers; iter; iter = iter->next)
    {
      LigmaLayerMode mode;

      mode = ligma_layer_get_mode (iter->data);
      if (ligma_layer_mode_is_composite_space_mutable (mode) &&
          composite_space != ligma_layer_get_composite_space (iter->data))
        update_layers = g_list_prepend (update_layers, iter->data);
    }

  if (g_list_length (update_layers) == 1)
    {
      LigmaUndo *undo;

      undo = ligma_image_undo_can_compress (image, LIGMA_TYPE_LAYER_PROP_UNDO,
                                           LIGMA_UNDO_LAYER_MODE);

      if (undo && LIGMA_ITEM_UNDO (undo)->item == LIGMA_ITEM (update_layers->data))
        push_undo = FALSE;
    }

  if (update_layers)
    {
      if (g_list_length (update_layers) > 1)
        ligma_image_undo_group_start (image, LIGMA_UNDO_GROUP_LAYER_MODE,
                                     _("Set layers' composite space"));

      for (iter = update_layers; iter; iter = iter->next)
        ligma_layer_set_composite_space (iter->data, composite_space, push_undo);

      if (g_list_length (update_layers) > 1)
        ligma_image_undo_group_end (image);

      g_list_free (update_layers);
      ligma_image_flush (image);
    }
}

void
layers_composite_mode_cmd_callback (LigmaAction *action,
                                    GVariant   *value,
                                    gpointer    data)
{
  LigmaImage              *image;
  GList                  *layers;
  GList                  *update_layers = NULL;
  GList                  *iter;
  LigmaLayerCompositeMode  composite_mode;
  gboolean             push_undo = TRUE;
  return_if_no_layers (image, layers, data);

  composite_mode = (LigmaLayerCompositeMode) g_variant_get_int32 (value);

  for (iter = layers; iter; iter = iter->next)
    {
      LigmaLayerMode mode;

      mode = ligma_layer_get_mode (iter->data);
      if (ligma_layer_mode_is_composite_mode_mutable (mode) &&
          composite_mode != ligma_layer_get_composite_mode (iter->data))
        update_layers = g_list_prepend (update_layers, iter->data);
    }

  if (g_list_length (update_layers) == 1)
    {
      LigmaUndo *undo;

      undo = ligma_image_undo_can_compress (image, LIGMA_TYPE_LAYER_PROP_UNDO,
                                           LIGMA_UNDO_LAYER_MODE);

      if (undo && LIGMA_ITEM_UNDO (undo)->item == LIGMA_ITEM (update_layers->data))
        push_undo = FALSE;
    }

  if (update_layers)
    {
      if (g_list_length (update_layers) > 1)
        ligma_image_undo_group_start (image, LIGMA_UNDO_GROUP_LAYER_MODE,
                                     _("Set layers' composite mode"));

      for (iter = update_layers; iter; iter = iter->next)
        ligma_layer_set_composite_mode (iter->data, composite_mode, push_undo);

      if (g_list_length (update_layers) > 1)
        ligma_image_undo_group_end (image);

      g_list_free (update_layers);
      ligma_image_flush (image);
    }
}

void
layers_visible_cmd_callback (LigmaAction *action,
                             GVariant   *value,
                             gpointer   data)
{
  LigmaImage *image;
  LigmaLayer *layer;
  return_if_no_layer (image, layer, data);

  items_visible_cmd_callback (action, value, image, LIGMA_ITEM (layer));
}

void
layers_lock_content_cmd_callback (LigmaAction *action,
                                  GVariant   *value,
                                  gpointer    data)
{
  LigmaImage *image;
  GList     *layers;
  GList     *iter;
  GList     *locked_layers = NULL;
  gboolean   locked        = g_variant_get_boolean (value);
  gchar     *undo_label;
  return_if_no_layers (image, layers, data);

  for (iter = layers; iter; iter = iter->next)
    if (ligma_item_can_lock_content (iter->data))
      {
        if (! locked && ! ligma_item_get_lock_content (iter->data))
          {
            /* When unlocking, we expect all selected layers to be locked. */
            g_list_free (locked_layers);
            return;
          }
        else if (locked != ligma_item_get_lock_content (iter->data))
          {
            locked_layers = g_list_prepend (locked_layers, iter->data);
          }
      }

  if (! locked_layers)
    return;

  if (locked)
    undo_label = _("Lock content");
  else
    undo_label = _("Unlock content");

  ligma_image_undo_group_start (image,
                               LIGMA_UNDO_GROUP_ITEM_LOCK_CONTENTS,
                               undo_label);

  for (iter = locked_layers; iter; iter = iter->next)
    ligma_item_set_lock_content (iter->data, locked, TRUE);

  ligma_image_flush (image);
  ligma_image_undo_group_end (image);

  g_list_free (locked_layers);
}

void
layers_lock_position_cmd_callback (LigmaAction *action,
                                   GVariant   *value,
                                   gpointer    data)
{
  LigmaImage *image;
  GList     *layers;
  GList     *iter;
  GList     *locked_layers = NULL;
  gboolean   locked        = g_variant_get_boolean (value);
  gchar     *undo_label;
  return_if_no_layers (image, layers, data);

  for (iter = layers; iter; iter = iter->next)
    if (ligma_item_can_lock_position (iter->data))
      {
        if (! locked && ! ligma_item_get_lock_position (iter->data))
          {
           /* When unlocking, we expect all selected layers to be locked. */
            g_list_free (locked_layers);
            return;
          }
        else if (locked != ligma_item_get_lock_position (iter->data))
          {
            locked_layers = g_list_prepend (locked_layers, iter->data);
          }
      }

  if (! locked_layers)
    return;

  if (locked)
    undo_label = _("Lock position");
  else
    undo_label = _("Unlock position");

  ligma_image_undo_group_start (image,
                               LIGMA_UNDO_GROUP_ITEM_LOCK_POSITION,
                               undo_label);

  for (iter = locked_layers; iter; iter = iter->next)
    ligma_item_set_lock_position (iter->data, locked, TRUE);

  ligma_image_flush (image);
  ligma_image_undo_group_end (image);

  g_list_free (locked_layers);
}

void
layers_lock_alpha_cmd_callback (LigmaAction *action,
                                GVariant   *value,
                                gpointer    data)
{
  LigmaImage *image;
  GList     *layers;
  GList     *iter;
  gboolean   lock_alpha;
  gboolean   lock_change = FALSE;
  return_if_no_layers (image, layers, data);

  lock_alpha = g_variant_get_boolean (value);

  for (iter = layers; iter; iter = iter->next)
    {
      if (ligma_layer_can_lock_alpha (iter->data))
        {
          /* Similar trick as in layers_mask_show_cmd_callback().
           * When unlocking, we expect all selected layers to be locked,
           * otherwise SET_ACTIVE() calls in layers-actions.c will
           * trigger lock updates.
           */
          if (! lock_alpha && ! ligma_layer_get_lock_alpha (iter->data))
            return;
          if (lock_alpha != ligma_layer_get_lock_alpha (iter->data))
            lock_change = TRUE;
        }
    }
  if (! lock_change)
    /* No layer locks would be changed. */
    return;

  ligma_image_undo_group_start (image,
                               LIGMA_UNDO_GROUP_LAYER_LOCK_ALPHA,
                               lock_alpha ? _("Lock alpha channels") : _("Unlock alpha channels"));
  for (iter = layers; iter; iter = iter->next)
    {
      if (ligma_layer_can_lock_alpha (iter->data))
        {
          if (lock_alpha != ligma_layer_get_lock_alpha (iter->data))
            ligma_layer_set_lock_alpha (iter->data, lock_alpha, TRUE);
        }
    }
  ligma_image_undo_group_end (image);
  ligma_image_flush (image);
}

void
layers_color_tag_cmd_callback (LigmaAction *action,
                               GVariant   *value,
                               gpointer    data)
{
  LigmaImage    *image;
  GList        *layers;
  GList        *iter;
  LigmaColorTag  color_tag;
  return_if_no_layers (image, layers, data);

  color_tag = (LigmaColorTag) g_variant_get_int32 (value);

  for (iter = layers; iter; iter = iter->next)
    items_color_tag_cmd_callback (action, image, LIGMA_ITEM (iter->data),
                                  color_tag);
}


/*  private functions  */

static void
layers_new_callback (GtkWidget              *dialog,
                     LigmaImage              *image,
                     LigmaLayer              *layer,
                     LigmaContext            *context,
                     const gchar            *layer_name,
                     LigmaLayerMode           layer_mode,
                     LigmaLayerColorSpace     layer_blend_space,
                     LigmaLayerColorSpace     layer_composite_space,
                     LigmaLayerCompositeMode  layer_composite_mode,
                     gdouble                 layer_opacity,
                     LigmaFillType            layer_fill_type,
                     gint                    layer_width,
                     gint                    layer_height,
                     gint                    layer_offset_x,
                     gint                    layer_offset_y,
                     gboolean                layer_visible,
                     LigmaColorTag            layer_color_tag,
                     gboolean                layer_lock_pixels,
                     gboolean                layer_lock_position,
                     gboolean                layer_lock_alpha,
                     gboolean                rename_text_layer, /* unused */
                     gpointer                user_data)
{
  LigmaDialogConfig *config     = LIGMA_DIALOG_CONFIG (image->ligma->config);
  GList            *layers     = ligma_image_get_selected_layers (image);
  GList            *new_layers = NULL;
  GList            *iter;
  gint              n_layers   = g_list_length (layers);
  gboolean          run_once   = (n_layers == 0);

  g_object_set (config,
                "layer-new-name",            layer_name,
                "layer-new-mode",            layer_mode,
                "layer-new-blend-space",     layer_blend_space,
                "layer-new-composite-space", layer_composite_space,
                "layer-new-composite-mode",  layer_composite_mode,
                "layer-new-opacity",         layer_opacity,
                "layer-new-fill-type",       layer_fill_type,
                NULL);

  layers = g_list_copy (layers);
  ligma_image_undo_group_start (image,
                               LIGMA_UNDO_GROUP_LAYER_ADD,
                               ngettext ("New layer",
                                         "New layers",
                                         n_layers > 0 ? n_layers : 1));
  for (iter = layers; iter || run_once; iter = iter ? iter->next : NULL)
    {
      LigmaLayer *parent;
      gint       position;

      run_once = FALSE;
      if (iter)
        {
          if (ligma_viewable_get_children (LIGMA_VIEWABLE (iter->data)))
            {
              parent   = iter->data;
              position = 0;
            }
          else
            {
              parent   = LIGMA_LAYER (ligma_item_get_parent (iter->data));
              position = ligma_item_get_index (iter->data);
            }
        }
      else /* run_once */
        {
          parent = NULL;
          position = 0;
        }

      layer = ligma_layer_new (image, layer_width, layer_height,
                              ligma_image_get_layer_format (image, TRUE),
                              config->layer_new_name,
                              config->layer_new_opacity,
                              config->layer_new_mode);

      if (layer)
        {
          ligma_item_set_offset (LIGMA_ITEM (layer), layer_offset_x, layer_offset_y);
          ligma_drawable_fill (LIGMA_DRAWABLE (layer), context,
                              config->layer_new_fill_type);
          ligma_item_set_visible (LIGMA_ITEM (layer), layer_visible, FALSE);
          ligma_item_set_color_tag (LIGMA_ITEM (layer), layer_color_tag, FALSE);
          ligma_item_set_lock_content (LIGMA_ITEM (layer), layer_lock_pixels,
                                      FALSE);
          ligma_item_set_lock_position (LIGMA_ITEM (layer), layer_lock_position,
                                       FALSE);
          ligma_layer_set_lock_alpha (layer, layer_lock_alpha, FALSE);
          ligma_layer_set_blend_space (layer, layer_blend_space, FALSE);
          ligma_layer_set_composite_space (layer, layer_composite_space, FALSE);
          ligma_layer_set_composite_mode (layer, layer_composite_mode, FALSE);

          ligma_image_add_layer (image, layer, parent, position, TRUE);
          ligma_image_flush (image);

          new_layers = g_list_prepend (new_layers, layer);
        }
      else
        {
          g_warning ("%s: could not allocate new layer", G_STRFUNC);
        }
    }

  ligma_image_undo_group_end (image);
  ligma_image_set_selected_layers (image, new_layers);

  g_list_free (layers);
  g_list_free (new_layers);
  gtk_widget_destroy (dialog);
}

static void
layers_edit_attributes_callback (GtkWidget              *dialog,
                                 LigmaImage              *image,
                                 LigmaLayer              *layer,
                                 LigmaContext            *context,
                                 const gchar            *layer_name,
                                 LigmaLayerMode           layer_mode,
                                 LigmaLayerColorSpace     layer_blend_space,
                                 LigmaLayerColorSpace     layer_composite_space,
                                 LigmaLayerCompositeMode  layer_composite_mode,
                                 gdouble                 layer_opacity,
                                 LigmaFillType            unused1,
                                 gint                    unused2,
                                 gint                    unused3,
                                 gint                    layer_offset_x,
                                 gint                    layer_offset_y,
                                 gboolean                layer_visible,
                                 LigmaColorTag            layer_color_tag,
                                 gboolean                layer_lock_pixels,
                                 gboolean                layer_lock_position,
                                 gboolean                layer_lock_alpha,
                                 gboolean                rename_text_layer,
                                 gpointer                user_data)
{
  LigmaItem *item = LIGMA_ITEM (layer);

  if (strcmp (layer_name, ligma_object_get_name (layer))               ||
      layer_mode            != ligma_layer_get_mode (layer)            ||
      layer_blend_space     != ligma_layer_get_blend_space (layer)     ||
      layer_composite_space != ligma_layer_get_composite_space (layer) ||
      layer_composite_mode  != ligma_layer_get_composite_mode (layer)  ||
      layer_opacity         != ligma_layer_get_opacity (layer)         ||
      layer_offset_x        != ligma_item_get_offset_x (item)          ||
      layer_offset_y        != ligma_item_get_offset_y (item)          ||
      layer_visible         != ligma_item_get_visible (item)           ||
      layer_color_tag       != ligma_item_get_color_tag (item)         ||
      layer_lock_pixels     != ligma_item_get_lock_content (item)      ||
      layer_lock_position   != ligma_item_get_lock_position (item)     ||
      layer_lock_alpha      != ligma_layer_get_lock_alpha (layer))
    {
      ligma_image_undo_group_start (image,
                                   LIGMA_UNDO_GROUP_ITEM_PROPERTIES,
                                   _("Layer Attributes"));

      if (strcmp (layer_name, ligma_object_get_name (layer)))
        {
          GError *error = NULL;

          if (! ligma_item_rename (LIGMA_ITEM (layer), layer_name, &error))
            {
              ligma_message_literal (image->ligma,
                                    G_OBJECT (dialog), LIGMA_MESSAGE_WARNING,
                                    error->message);
              g_clear_error (&error);
            }
        }

      if (layer_mode != ligma_layer_get_mode (layer))
        ligma_layer_set_mode (layer, layer_mode, TRUE);

      if (layer_blend_space != ligma_layer_get_blend_space (layer))
        ligma_layer_set_blend_space (layer, layer_blend_space, TRUE);

      if (layer_composite_space != ligma_layer_get_composite_space (layer))
        ligma_layer_set_composite_space (layer, layer_composite_space, TRUE);

      if (layer_composite_mode != ligma_layer_get_composite_mode (layer))
        ligma_layer_set_composite_mode (layer, layer_composite_mode, TRUE);

      if (layer_opacity != ligma_layer_get_opacity (layer))
        ligma_layer_set_opacity (layer, layer_opacity, TRUE);

      if (layer_offset_x != ligma_item_get_offset_x (item) ||
          layer_offset_y != ligma_item_get_offset_y (item))
        {
          ligma_item_translate (item,
                               layer_offset_x - ligma_item_get_offset_x (item),
                               layer_offset_y - ligma_item_get_offset_y (item),
                               TRUE);
        }

      if (layer_visible != ligma_item_get_visible (item))
        ligma_item_set_visible (item, layer_visible, TRUE);

      if (layer_color_tag != ligma_item_get_color_tag (item))
        ligma_item_set_color_tag (item, layer_color_tag, TRUE);

      if (layer_lock_pixels != ligma_item_get_lock_content (item))
        ligma_item_set_lock_content (item, layer_lock_pixels, TRUE);

      if (layer_lock_position != ligma_item_get_lock_position (item))
        ligma_item_set_lock_position (item, layer_lock_position, TRUE);

      if (layer_lock_alpha != ligma_layer_get_lock_alpha (layer))
        ligma_layer_set_lock_alpha (layer, layer_lock_alpha, TRUE);

      ligma_image_undo_group_end (image);

      ligma_image_flush (image);
    }

  if (ligma_item_is_text_layer (LIGMA_ITEM (layer)))
    {
      g_object_set (layer,
                    "auto-rename", rename_text_layer,
                    NULL);
    }

  gtk_widget_destroy (dialog);
}

static void
layers_add_mask_callback (GtkWidget       *dialog,
                          GList           *layers,
                          LigmaAddMaskType  add_mask_type,
                          LigmaChannel     *channel,
                          gboolean         invert,
                          gpointer         user_data)
{
  LigmaImage        *image  = ligma_item_get_image (LIGMA_ITEM (layers->data));
  LigmaDialogConfig *config = LIGMA_DIALOG_CONFIG (image->ligma->config);
  LigmaLayerMask    *mask;
  GList            *iter;
  GError           *error = NULL;

  g_object_set (config,
                "layer-add-mask-type",   add_mask_type,
                "layer-add-mask-invert", invert,
                NULL);

  ligma_image_undo_group_start (image,
                               LIGMA_UNDO_GROUP_LAYER_ADD,
                               _("Add Layer Masks"));
  for (iter = layers; iter; iter = iter->next)
    {
      mask = ligma_layer_create_mask (iter->data,
                                     config->layer_add_mask_type,
                                     channel);

      if (config->layer_add_mask_invert)
        ligma_channel_invert (LIGMA_CHANNEL (mask), FALSE);

      if (! ligma_layer_add_mask (iter->data, mask, TRUE, &error))
        {
          ligma_message_literal (image->ligma,
                                G_OBJECT (dialog), LIGMA_MESSAGE_WARNING,
                                error->message);
          g_object_unref (mask);
          g_clear_error (&error);
          return;
        }
    }

  ligma_image_undo_group_end (image);
  ligma_image_flush (image);
  gtk_widget_destroy (dialog);
}

static void
layers_scale_callback (GtkWidget             *dialog,
                       LigmaViewable          *viewable,
                       gint                   width,
                       gint                   height,
                       LigmaUnit               unit,
                       LigmaInterpolationType  interpolation,
                       gdouble                xresolution,    /* unused */
                       gdouble                yresolution,    /* unused */
                       LigmaUnit               resolution_unit,/* unused */
                       gpointer               user_data)
{
  LigmaDisplay *display = LIGMA_DISPLAY (user_data);

  layer_scale_unit   = unit;
  layer_scale_interp = interpolation;

  if (width > 0 && height > 0)
    {
      LigmaItem     *item = LIGMA_ITEM (viewable);
      LigmaProgress *progress;
      GtkWidget    *progress_dialog = NULL;

      gtk_widget_destroy (dialog);

      if (width  == ligma_item_get_width  (item) &&
          height == ligma_item_get_height (item))
        return;

      if (display)
        {
          progress = LIGMA_PROGRESS (display);
        }
      else
        {
          progress_dialog = ligma_progress_dialog_new ();
          progress = LIGMA_PROGRESS (progress_dialog);
        }

      progress = ligma_progress_start (progress, FALSE, _("Scaling"));

      ligma_item_scale_by_origin (item,
                                 width, height, interpolation,
                                 progress, TRUE);

      if (progress)
        ligma_progress_end (progress);

      if (progress_dialog)
        gtk_widget_destroy (progress_dialog);

      ligma_image_flush (ligma_item_get_image (item));
    }
  else
    {
      g_warning ("Scale Error: "
                 "Both width and height must be greater than zero.");
    }
}

static void
layers_resize_callback (GtkWidget    *dialog,
                        LigmaViewable *viewable,
                        LigmaContext  *context,
                        gint          width,
                        gint          height,
                        LigmaUnit      unit,
                        gint          offset_x,
                        gint          offset_y,
                        gdouble       unused0,
                        gdouble       unused1,
                        LigmaUnit      unused2,
                        LigmaFillType  fill_type,
                        LigmaItemSet   unused3,
                        gboolean      unused4,
                        gpointer      user_data)
{
  layer_resize_unit = unit;

  if (width > 0 && height > 0)
    {
      LigmaItem         *item   = LIGMA_ITEM (viewable);
      LigmaImage        *image  = ligma_item_get_image (item);
      LigmaDialogConfig *config = LIGMA_DIALOG_CONFIG (image->ligma->config);

      g_object_set (config,
                    "layer-resize-fill-type", fill_type,
                    NULL);

      gtk_widget_destroy (dialog);

      if (width  == ligma_item_get_width  (item) &&
          height == ligma_item_get_height (item))
        return;

      ligma_item_resize (item, context, fill_type,
                        width, height, offset_x, offset_y);
      ligma_image_flush (ligma_item_get_image (item));
    }
  else
    {
      g_warning ("Resize Error: "
                 "Both width and height must be greater than zero.");
    }
}

static gint
layers_mode_index (LigmaLayerMode         layer_mode,
                   const LigmaLayerMode  *modes,
                   gint                  n_modes)
{
  gint i = 0;

  while (i < (n_modes - 1) && modes[i] != layer_mode)
    i++;

  return i;
}
