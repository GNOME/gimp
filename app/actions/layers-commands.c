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
#include "core/gimpchannel-combine.h"
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
#include "core/gimplink.h"
#include "core/gimplinklayer.h"
#include "core/gimplist.h"
#include "core/gimppickable.h"
#include "core/gimppickable-auto-shrink.h"
#include "core/gimprasterizable.h"
#include "core/gimptoolinfo.h"
#include "core/gimpundostack.h"
#include "core/gimpprogress.h"

#include "path/gimppath.h"
#include "path/gimppath-warp.h"
#include "path/gimpstroke.h"
#include "path/gimpvectorlayer.h"
#include "path/gimpvectorlayeroptions.h"

#include "text/gimptext.h"
#include "text/gimptext-path.h"
#include "text/gimptextlayer.h"

#include "widgets/gimpaction.h"
#include "widgets/gimpdock.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpopendialog.h"
#include "widgets/gimpprogressdialog.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpimagewindow.h"

#include "tools/gimppathtool.h"
#include "tools/gimptexttool.h"
#include "tools/tool_manager.h"

#include "dialogs/dialogs.h"
#include "dialogs/layer-add-mask-dialog.h"
#include "dialogs/layer-options-dialog.h"
#include "dialogs/resize-dialog.h"
#include "dialogs/scale-dialog.h"
#include "dialogs/vector-layer-options-dialog.h"

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
                                               GimpLink              *link,
                                               gint                   layer_width,
                                               gint                   layer_height,
                                               gint                   layer_offset_x,
                                               gint                   layer_offset_y,
                                               gboolean               layer_visible,
                                               GimpColorTag           layer_color_tag,
                                               gboolean               layer_lock_pixels,
                                               gboolean               layer_lock_position,
                                               gboolean               layer_lock_visibility,
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
                                               GimpLink              *link,
                                               gint                   layer_width,
                                               gint                   layer_height,
                                               gint                   layer_offset_x,
                                               gint                   layer_offset_y,
                                               gboolean               layer_visible,
                                               GimpColorTag           layer_color_tag,
                                               gboolean               layer_lock_pixels,
                                               gboolean               layer_lock_position,
                                               gboolean               layer_lock_visibility,
                                               gboolean               layer_lock_alpha,
                                               gboolean               rename_text_layer,
                                               gpointer               user_data);
static void   layers_add_mask_callback        (GtkWidget             *dialog,
                                               GList                 *layers,
                                               GimpAddMaskType        add_mask_type,
                                               GimpChannel           *channel,
                                               gboolean               invert,
                                               gboolean               edit_mask,
                                               gpointer               user_data);
static void   layers_scale_callback           (GtkWidget             *dialog,
                                               GimpViewable          *viewable,
                                               gint                   width,
                                               gint                   height,
                                               GimpUnit              *unit,
                                               GimpInterpolationType  interpolation,
                                               gdouble                xresolution,
                                               gdouble                yresolution,
                                               GimpUnit              *resolution_unit,
                                               gpointer               user_data);
static void   layers_resize_callback          (GtkWidget             *dialog,
                                               GimpViewable          *viewable,
                                               GimpContext           *context,
                                               gint                   width,
                                               gint                   height,
                                               GimpUnit              *unit,
                                               gint                   offset_x,
                                               gint                   offset_y,
                                               gdouble                unused0,
                                               gdouble                unused1,
                                               GimpUnit              *unused2,
                                               GimpFillType           fill_type,
                                               GimpItemSet            unused3,
                                               gboolean               unused4,
                                               gpointer               data);

static gint   layers_mode_index               (GimpLayerMode          layer_mode,
                                               const GimpLayerMode   *modes,
                                               gint                   n_modes);

static void   layers_vector_show_fill_stroke  (GimpAction            *action,
                                               GVariant              *value,
                                               gpointer               data);


/*  private variables  */

static GimpUnit              *layer_resize_unit   = NULL;
static GimpUnit              *layer_scale_unit    = NULL;
static GimpInterpolationType  layer_scale_interp  = -1;


/*  public functions  */

void
layers_edit_cmd_callback (GimpAction *action,
                          GVariant   *value,
                          gpointer    data)
{
  GimpImage *image;
  GList     *layers;
  GtkWidget *widget;
  return_if_no_layers (image, layers, data);
  return_if_no_widget (widget, data);

  if (g_list_length (layers) != 1)
    return;

  if (gimp_item_is_text_layer (GIMP_ITEM (layers->data)))
    {
      layers_edit_text_cmd_callback (action, value, data);
    }
  else if (gimp_item_is_vector_layer (GIMP_ITEM (layers->data)))
    {
      layers_vector_show_fill_stroke (action, value, data);
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
  GList     *layers;
  GtkWidget *widget;
  GimpTool  *active_tool;
  return_if_no_layers (image, layers, data);
  return_if_no_widget (widget, data);

  if (g_list_length (layers) != 1)
    return;

  layer = layers->data;
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
layers_edit_vector_cmd_callback (GimpAction *action,
                                 GVariant   *value,
                                 gpointer    data)
{
  GimpImage *image;
  GimpLayer *layer;
  GList     *layers;
  GtkWidget *widget;
  GimpTool  *active_tool;
  return_if_no_layers (image, layers, data);
  return_if_no_widget (widget, data);

  if (g_list_length (layers) != 1)
    return;

  layer = layers->data;

  if (! gimp_item_is_vector_layer (GIMP_ITEM (layer)))
    {
      layers_edit_attributes_cmd_callback (action, value, data);
      return;
    }

  active_tool = tool_manager_get_active (image->gimp);

  if (! GIMP_IS_PATH_TOOL (active_tool))
    {
      GimpToolInfo *tool_info;

      tool_info = (GimpToolInfo *)
        gimp_container_get_child_by_name (image->gimp->tool_info_list,
                                          "gimp-path-tool");

      if (GIMP_IS_TOOL_INFO (tool_info))
        {
          gimp_context_set_tool (action_data_get_context (data), tool_info);
          active_tool = tool_manager_get_active (image->gimp);
        }
    }

  if (GIMP_IS_PATH_TOOL (active_tool))
    gimp_path_tool_set_path (GIMP_PATH_TOOL (active_tool), GIMP_VECTOR_LAYER (layer), NULL);
}

void
layers_edit_attributes_cmd_callback (GimpAction *action,
                                     GVariant   *value,
                                     gpointer    data)
{
  GimpImage *image;
  GimpLayer *layer;
  GList     *layers;
  GtkWidget *widget;
  GtkWidget *dialog;
  return_if_no_layers (image, layers, data);
  return_if_no_widget (widget, data);

  if (g_list_length (layers) != 1)
    return;

  layer = layers->data;

  if (gimp_layer_is_floating_sel (layer))
    return;

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
                                         gimp_item_get_color_tag (item),
                                         gimp_item_get_lock_content (item),
                                         gimp_item_get_lock_position (item),
                                         gimp_item_get_lock_visibility (item),
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
      const gchar      *title;
      gchar            *desc;
      gint              n_layers;
      GimpLayerMode     layer_mode = config->layer_new_mode;

      n_layers = g_list_length (gimp_image_get_selected_layers (image));
      title = ngettext ("New Layer", "New Layers", n_layers > 0 ? n_layers : 1);
      desc  = ngettext ("Create a New Layer", "Create %d New Layers", n_layers > 0 ? n_layers : 1);
      desc  = g_strdup_printf (desc, n_layers > 0 ? n_layers : 1);

      if (layer_mode == GIMP_LAYER_MODE_NORMAL ||
          layer_mode == GIMP_LAYER_MODE_NORMAL_LEGACY)
        {
          layer_mode = gimp_image_get_default_new_layer_mode (image);
        }

      dialog = layer_options_dialog_new (image, NULL,
                                         action_data_get_context (data),
                                         widget,
                                         title,
                                         "gimp-layer-new",
                                         GIMP_ICON_LAYER,
                                         desc,
                                         GIMP_HELP_LAYER_NEW,
                                         config->layer_new_name,
                                         layer_mode,
                                         config->layer_new_blend_space,
                                         config->layer_new_composite_space,
                                         config->layer_new_composite_mode,
                                         config->layer_new_opacity,
                                         config->layer_new_fill_type,
                                         TRUE,
                                         GIMP_COLOR_TAG_NONE,
                                         FALSE,
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
layers_new_last_vals_cmd_callback (GimpAction *action,
                                   GVariant   *value,
                                   gpointer    data)
{
  GimpImage        *image;
  GtkWidget        *widget;
  GimpLayer        *layer;
  GimpDialogConfig *config;
  GList            *layers;
  GList            *new_layers = NULL;
  GList            *iter;
  GimpLayerMode     layer_mode;
  gint              n_layers;
  gboolean          run_once;

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

  layers   = gimp_image_get_selected_layers (image);
  layers   = g_list_copy (layers);
  n_layers = g_list_length (layers);
  run_once = (n_layers == 0);

  gimp_image_undo_group_start (image,
                               GIMP_UNDO_GROUP_LAYER_ADD,
                               ngettext ("New layer",
                                         "New layers",
                                         n_layers > 0 ? n_layers : 1));

  for (iter = layers; iter || run_once ; iter = iter ? iter->next : NULL)
    {
      GimpLayer *parent;
      gint       position;

      run_once = FALSE;
      if (iter)
        {
          if (gimp_viewable_get_children (GIMP_VIEWABLE (iter->data)))
            {
              parent   = iter->data;
              position = 0;
            }
          else
            {
              parent   = GIMP_LAYER (gimp_item_get_parent (iter->data));
              position = gimp_item_get_index (iter->data);
            }
        }
      else /* run_once */
        {
          parent   = NULL;
          position = -1;
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

      gimp_image_add_layer (image, layer, parent, position, TRUE);
      new_layers = g_list_prepend (new_layers, layer);
    }

  gimp_image_set_selected_layers (image, new_layers);

  g_list_free (layers);
  g_list_free (new_layers);

  gimp_image_undo_group_end (image);
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
  GList     *new_layers = NULL;
  GList     *layers;
  GList     *iter;
  gint       n_layers;
  gboolean   run_once;

  return_if_no_image (image, data);

  layers     = gimp_image_get_selected_layers (image);
  layers     = g_list_copy (layers);
  n_layers   = g_list_length (layers);
  run_once   = (n_layers == 0);

  gimp_image_undo_group_start (image,
                               GIMP_UNDO_GROUP_LAYER_ADD,
                               ngettext ("New layer group",
                                         "New layer groups",
                                         n_layers > 0 ? n_layers : 1));

  for (iter = layers; iter || run_once ; iter = iter ? iter->next : NULL)
    {
      GimpLayer *layer;
      GimpLayer *parent;
      gint       position;

      run_once = FALSE;
      if (iter)
        {
          if (gimp_viewable_get_children (GIMP_VIEWABLE (iter->data)))
            {
              parent   = iter->data;
              position = 0;
            }
          else
            {
              parent   = GIMP_LAYER (gimp_item_get_parent (iter->data));
              position = gimp_item_get_index (iter->data);
            }
        }
      else /* run_once */
        {
          parent   = NULL;
          position = -1;
        }
      layer = gimp_group_layer_new (image);

      gimp_image_add_layer (image, layer, parent, position, TRUE);
      new_layers = g_list_prepend (new_layers, layer);
    }

  gimp_image_set_selected_layers (image, new_layers);

  g_list_free (layers);
  g_list_free (new_layers);

  gimp_image_undo_group_end (image);
  gimp_image_flush (image);

}

void
layers_select_cmd_callback (GimpAction *action,
                            GVariant   *value,
                            gpointer    data)
{
  GimpImage            *image;
  GList                *new_layers = NULL;
  GList                *layers;
  GList                *iter;
  GimpActionSelectType  select_type;
  gboolean              run_once;
  return_if_no_image (image, data);

  select_type = (GimpActionSelectType) g_variant_get_int32 (value);

  layers   = gimp_image_get_selected_layers (image);
  run_once = (g_list_length (layers) == 0);

  for (iter = layers; iter || run_once; iter = iter ? iter->next : NULL)
    {
      GimpLayer     *new_layer;
      GimpContainer *container;

      if (iter)
        {
          container = gimp_item_get_container (GIMP_ITEM (iter->data));
        }
      else /* run_once */
        {
          container = gimp_image_get_layers (image);
          run_once  = FALSE;
        }
      new_layer = (GimpLayer *) action_select_object (select_type,
                                                      container,
                                                      iter ? iter->data : NULL);
      if (new_layer)
        new_layers = g_list_prepend (new_layers, new_layer);
    }

  if (new_layers)
    {
      gimp_image_set_selected_layers (image, new_layers);
      gimp_image_flush (image);
    }

  g_list_free (new_layers);
}

void
layers_raise_cmd_callback (GimpAction *action,
                           GVariant   *value,
                           gpointer    data)
{
  GimpImage *image;
  GList     *layers;
  GList     *iter;
  GList     *raised_layers = NULL;
  return_if_no_layers (image, layers, data);

  for (iter = layers; iter; iter = iter->next)
    {
      gint index;

      index = gimp_item_get_index (iter->data);
      if (index > 0)
        {
          raised_layers = g_list_prepend (raised_layers, iter->data);
        }
      else
        {
          g_list_free (raised_layers);
          return;
        }
    }

  gimp_image_undo_group_start (image,
                               GIMP_UNDO_GROUP_ITEM_DISPLACE,
                               ngettext ("Raise Layer",
                                         "Raise Layers",
                                         g_list_length (raised_layers)));

  raised_layers = g_list_reverse (raised_layers);
  for (iter = raised_layers; iter; iter = iter->next)
    gimp_image_raise_item (image, iter->data, NULL);
  g_list_free (raised_layers);

  gimp_image_undo_group_end (image);
  gimp_image_flush (image);
}

void
layers_raise_to_top_cmd_callback (GimpAction *action,
                                  GVariant   *value,
                                  gpointer    data)
{
  GimpImage *image;
  GList     *layers;
  GList     *iter;
  GList     *raised_layers = NULL;
  return_if_no_layers (image, layers, data);

  for (iter = layers; iter; iter = iter->next)
    {
      gint index;

      index = gimp_item_get_index (iter->data);
      if (index > 0)
        raised_layers = g_list_prepend (raised_layers, iter->data);
    }

  gimp_image_undo_group_start (image,
                               GIMP_UNDO_GROUP_ITEM_DISPLACE,
                               ngettext ("Raise Layer to Top",
                                         "Raise Layers to Top",
                                         g_list_length (raised_layers)));

  for (iter = raised_layers; iter; iter = iter->next)
    gimp_image_raise_item_to_top (image, iter->data);

  g_list_free (raised_layers);

  gimp_image_undo_group_end (image);
  gimp_image_flush (image);
}

void
layers_lower_cmd_callback (GimpAction *action,
                           GVariant   *value,
                           gpointer    data)
{
  GimpImage *image;
  GList     *layers;
  GList     *iter;
  GList     *lowered_layers = NULL;
  return_if_no_layers (image, layers, data);

  for (iter = layers; iter; iter = iter->next)
    {
      GList *layer_list;
      gint   index;

      layer_list = gimp_item_get_container_iter (GIMP_ITEM (iter->data));
      index = gimp_item_get_index (iter->data);
      if (index < g_list_length (layer_list) - 1)
        {
          lowered_layers = g_list_prepend (lowered_layers, iter->data);
        }
      else
        {
          g_list_free (lowered_layers);
          return;
        }
    }

  gimp_image_undo_group_start (image,
                               GIMP_UNDO_GROUP_ITEM_DISPLACE,
                               ngettext ("Lower Layer",
                                         "Lower Layers",
                                         g_list_length (lowered_layers)));

  for (iter = lowered_layers; iter; iter = iter->next)
    gimp_image_lower_item (image, iter->data, NULL);

  g_list_free (lowered_layers);

  gimp_image_undo_group_end (image);
  gimp_image_flush (image);
}

void
layers_lower_to_bottom_cmd_callback (GimpAction *action,
                                     GVariant   *value,
                                     gpointer    data)
{
  GimpImage *image;
  GList     *layers;
  GList     *iter;
  GList     *lowered_layers = NULL;
  return_if_no_layers (image, layers, data);

  for (iter = layers; iter; iter = iter->next)
    {
      GList *layer_list;
      gint   index;

      layer_list = gimp_item_get_container_iter (GIMP_ITEM (iter->data));
      index = gimp_item_get_index (iter->data);
      if (index < g_list_length (layer_list) - 1)
        lowered_layers = g_list_prepend (lowered_layers, iter->data);
    }

  gimp_image_undo_group_start (image,
                               GIMP_UNDO_GROUP_ITEM_DISPLACE,
                               ngettext ("Lower Layer to Bottom",
                                         "Lower Layers to Bottom",
                                         g_list_length (lowered_layers)));

  for (iter = lowered_layers; iter; iter = iter->next)
    gimp_image_lower_item_to_bottom (image, iter->data);

  g_list_free (lowered_layers);

  gimp_image_undo_group_end (image);
  gimp_image_flush (image);
}

void
layers_duplicate_cmd_callback (GimpAction *action,
                               GVariant   *value,
                               gpointer    data)
{
  GimpImage *image;
  GList     *layers;
  GList     *new_layers = NULL;
  GList     *iter;
  return_if_no_layers (image, layers, data);

  layers = g_list_copy (layers);
  gimp_image_undo_group_start (image,
                               GIMP_UNDO_GROUP_LAYER_ADD,
                               _("Duplicate layers"));
  for (iter = layers; iter; iter = iter->next)
    {
      GimpLayer *new_layer;

      new_layer = GIMP_LAYER (gimp_item_duplicate (GIMP_ITEM (iter->data),
                                                   G_TYPE_FROM_INSTANCE (iter->data)));

      /*  use the actual parent here, not GIMP_IMAGE_ACTIVE_PARENT because
       *  the latter would add a duplicated group inside itself instead of
       *  above it
       */
      gimp_image_add_layer (image, new_layer,
                            gimp_layer_get_parent (iter->data),
                            gimp_item_get_index (iter->data),
                            TRUE);
      gimp_drawable_enable_resize_undo (GIMP_DRAWABLE (new_layer));
      new_layers = g_list_prepend (new_layers, new_layer);
    }

  gimp_image_set_selected_layers (image, new_layers);
  g_list_free (layers);
  g_list_free (new_layers);

  gimp_image_undo_group_end (image);
  gimp_image_flush (image);
}

void
layers_anchor_cmd_callback (GimpAction *action,
                            GVariant   *value,
                            gpointer    data)
{
  GimpImage *image;
  GList     *layers;
  return_if_no_layers (image, layers, data);

  if (g_list_length (layers) == 1 &&
      gimp_layer_is_floating_sel (layers->data))
    {
      floating_sel_anchor (layers->data);
      gimp_image_flush (image);
    }
}

void
layers_merge_down_cmd_callback (GimpAction *action,
                                GVariant   *value,
                                gpointer    data)
{
  GimpImage   *image;
  GList       *layers;
  GimpDisplay *display;
  GError      *error = NULL;

  return_if_no_layers (image, layers, data);
  return_if_no_display (display, data);

  layers = gimp_image_merge_down (image, layers, action_data_get_context (data),
                                  GIMP_EXPAND_AS_NECESSARY,
                                  GIMP_PROGRESS (display), &error);

  if (error)
    {
      gimp_message_literal (image->gimp,
                            G_OBJECT (display), GIMP_MESSAGE_WARNING,
                            error->message);
      g_clear_error (&error);
      return;
    }

  gimp_image_set_selected_layers (image, layers);
  g_list_free (layers);

  gimp_image_flush (image);
}

void
layers_merge_group_cmd_callback (GimpAction *action,
                                 GVariant   *value,
                                 gpointer    data)
{
  GimpImage *image;
  GList     *layers;
  GList     *merge_layers = NULL;
  GList     *iter;
  return_if_no_layers (image, layers, data);

  for (iter = layers; iter; iter = iter->next)
    {
      if (gimp_viewable_get_children (GIMP_VIEWABLE (iter->data)))
        {
          GList *iter2;

          for (iter2 = layers; iter2; iter2 = iter2->next)
            {
              if (iter->data == iter2->data)
                continue;

              /* Do not merge a layer when we already merge one of its
               * ancestors.
               */
              if (gimp_viewable_is_ancestor (iter2->data, iter->data))
                break;

              /* Do not merge a layer which has a little sister (same
               * parent and smaller index) or a little cousin (one of
               * its ancestors is a little sister) of a pass-through
               * group layer.
               * These will be rendered and merged through the
               * pass-through by definition.
               */
              if (gimp_viewable_get_children (GIMP_VIEWABLE (iter2->data)) &&
                  gimp_layer_get_mode (iter2->data) == GIMP_LAYER_MODE_PASS_THROUGH)
                {
                  GimpLayer *pass_through_parent = gimp_layer_get_parent (iter2->data);
                  GimpLayer *cousin              = iter->data;
                  gboolean   ignore = FALSE;

                  do
                    {
                      GimpLayer *cousin_parent = gimp_layer_get_parent (cousin);

                      if (pass_through_parent == cousin_parent &&
                          gimp_item_get_index (GIMP_ITEM (iter2->data)) < gimp_item_get_index (GIMP_ITEM (cousin)))
                        {
                          ignore = TRUE;
                          break;
                        }

                      cousin = cousin_parent;
                    }
                  while (cousin != NULL);

                  if (ignore)
                    break;
                }
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
      gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_IMAGE_LAYERS_MERGE,
                                   undo_name);
      g_free (undo_name);
    }

  for (iter = merge_layers; iter; iter = iter->next)
    gimp_image_merge_group_layer (image, GIMP_GROUP_LAYER (iter->data));

  if (g_list_length (merge_layers) > 1)
    gimp_image_undo_group_end (image);

  g_list_free (merge_layers);

  gimp_image_flush (image);
}

void
layers_delete_cmd_callback (GimpAction *action,
                            GVariant   *value,
                            gpointer    data)
{
  GimpImage *image;
  GList     *layers;
  GList     *removed_layers;
  GList     *iter;
  GList     *iter2;

  return_if_no_image (image, data);

  layers = gimp_image_get_selected_layers (image);

  /*TODO: we should have a failsafe to determine when we are going to
   * delete all layers (i.e. all layers of first level at least) and
   * forbid it. */

  /* Copy of the original selection. */
  removed_layers = g_list_copy (layers);

  /* Removing children layers (they will be removed anyway by removing
   * the parent).
   */
  for (iter = removed_layers; iter; iter = iter->next)
    {
      for (iter2 = removed_layers; iter2; iter2 = iter2->next)
        {
          if (iter->data != iter2->data &&
              gimp_viewable_is_ancestor (iter2->data, iter->data))
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
      gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_IMAGE_ITEM_REMOVE,
                                   undo_name);
    }

  for (iter = removed_layers; iter; iter = iter->next)
    gimp_image_remove_layer (image, iter->data, TRUE, NULL);

  if (g_list_length (removed_layers) > 1)
    gimp_image_undo_group_end (image);

  g_list_free (removed_layers);

  gimp_image_flush (image);
}

void
layers_rasterize_cmd_callback (GimpAction *action,
                               GVariant   *value,
                               gpointer    data)
{
  GimpImage *image;
  GList     *layers;
  GList     *iter;

  return_if_no_layers (image, layers, data);

  gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_ITEM_PROPERTIES,
                               _("Rasterize Layers"));

  for (iter = layers; iter; iter = iter->next)
    {
      if (GIMP_IS_RASTERIZABLE (iter->data) && ! gimp_rasterizable_is_rasterized (iter->data))
        gimp_rasterizable_rasterize (GIMP_RASTERIZABLE (iter->data));
    }

  gimp_image_undo_group_end (image);
}

void
layers_revert_rasterize_cmd_callback (GimpAction *action,
                                      GVariant   *value,
                                      gpointer    data)
{
  GimpImage *image;
  GList     *layers;
  GList     *iter;

  return_if_no_layers (image, layers, data);

  gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_ITEM_PROPERTIES,
                               _("Revert Rasterize"));

  for (iter = layers; iter; iter = iter->next)
    {
      if (GIMP_IS_RASTERIZABLE (iter->data) && gimp_rasterizable_is_rasterized (iter->data))
        gimp_rasterizable_restore (GIMP_RASTERIZABLE (iter->data));
    }

  gimp_image_undo_group_end (image);
}

void
layers_text_to_path_cmd_callback (GimpAction *action,
                                  GVariant   *value,
                                  gpointer    data)
{
  GimpImage *image;
  GList     *layers;
  GList     *iter;
  gboolean   path_added = FALSE;
  return_if_no_layers (image, layers, data);

  /* TODO: have the proper undo group. */
  gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_PATHS_IMPORT,
                               _("Add Paths"));

  for (iter = layers; iter; iter = iter->next)
    {
      GimpLayer *layer = iter->data;

      if (GIMP_IS_TEXT_LAYER (layer))
        {
          GimpPath *path;
          gint      x, y;

          path = gimp_text_path_new (image, GIMP_TEXT_LAYER (layer)->text);

          gimp_item_get_offset (GIMP_ITEM (layer), &x, &y);
          gimp_item_translate (GIMP_ITEM (path), x, y, FALSE);

          gimp_image_add_path (image, path,
                               GIMP_IMAGE_ACTIVE_PARENT, -1, TRUE);
          path_added = TRUE;
        }
    }

  gimp_image_undo_group_end (image);

  if (path_added)
    gimp_image_flush (image);
}

void
layers_text_along_path_cmd_callback (GimpAction *action,
                                     GVariant   *value,
                                     gpointer    data)
{
  GimpImage   *image;
  GList       *layers;
  GList       *paths;
  GimpLayer   *layer;
  GimpPath    *path;
  return_if_no_layers (image, layers, data);
  return_if_no_paths (image, paths, data);

  if (g_list_length (layers) != 1 || g_list_length (paths) != 1)
    return;

  layer = layers->data;
  path  = paths->data;
  if (GIMP_IS_TEXT_LAYER (layer))
    {
      gdouble   box_width;
      gdouble   box_height;
      GimpPath *new_path;
      gdouble   offset;

      box_width  = gimp_item_get_width  (GIMP_ITEM (layer));
      box_height = gimp_item_get_height (GIMP_ITEM (layer));

      new_path = gimp_text_path_new (image, GIMP_TEXT_LAYER (layer)->text);

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

            while ((stroke = gimp_path_stroke_get_next (new_path, stroke)))
              {
                gimp_stroke_rotate (stroke, 0, 0, 270);
                gimp_stroke_translate (stroke, 0, box_width);
              }
          }
          offset = 0.5 * box_width;
          break;
        }

      gimp_path_warp_path (path, new_path, offset);

      gimp_item_set_visible (GIMP_ITEM (new_path), TRUE, FALSE);

      gimp_image_add_path (image, new_path,
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
  GList     *layers;
  GtkWidget *widget;
  GtkWidget *dialog;
  return_if_no_layers (image, layers, data);
  return_if_no_widget (widget, data);

#define RESIZE_DIALOG_KEY "gimp-resize-dialog"

  g_return_if_fail (g_list_length (layers) == 1);

  layer = layers->data;

  dialog = dialogs_get_dialog (G_OBJECT (layer), RESIZE_DIALOG_KEY);

  if (! dialog)
    {
      GimpDialogConfig *config  = GIMP_DIALOG_CONFIG (image->gimp->config);
      GimpDisplay      *display = NULL;

      if (GIMP_IS_IMAGE_WINDOW (data))
        display = action_data_get_display (data);

      if (layer_resize_unit == NULL)
        layer_resize_unit = gimp_unit_pixel ();

      if (layer_resize_unit != gimp_unit_percent () && display)
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
  GList     *layers;
  GList     *iter;
  return_if_no_layers (image, layers, data);

  if (g_list_length (layers) > 1)
    gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_ITEM_RESIZE,
                                 _("Layers to Image Size"));

  for (iter = layers; iter; iter = iter->next)
    gimp_layer_resize_to_image (iter->data,
                                action_data_get_context (data),
                                GIMP_FILL_TRANSPARENT);

  if (g_list_length (layers) > 1)
    gimp_image_undo_group_end (image);

  gimp_image_flush (image);
}

void
layers_scale_cmd_callback (GimpAction *action,
                           GVariant   *value,
                           gpointer    data)
{
  GimpImage *image;
  GList     *layers;
  GimpLayer *layer;
  GtkWidget *widget;
  GtkWidget *dialog;
  return_if_no_layers (image, layers, data);
  return_if_no_widget (widget, data);

#define SCALE_DIALOG_KEY "gimp-scale-dialog"

  g_return_if_fail (g_list_length (layers) == 1);

  layer = layers->data;

  dialog = dialogs_get_dialog (G_OBJECT (layer), SCALE_DIALOG_KEY);

  if (! dialog)
    {
      GimpDisplay *display = NULL;

      if (GIMP_IS_IMAGE_WINDOW (data))
        display = action_data_get_display (data);

      if (layer_scale_unit == NULL)
        layer_scale_unit = gimp_unit_pixel ();;

      if (layer_scale_unit != gimp_unit_percent () && display)
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
  GList     *layers;
  GList     *iter;
  GtkWidget *widget;
  gchar     *desc;
  gint       x, y;
  gint       width, height;
  return_if_no_layers (image, layers, data);
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

  desc = g_strdup_printf (ngettext ("Crop Layer to Selection",
                                    "Crop %d Layers to Selection",
                                    g_list_length (layers)),
                          g_list_length (layers));
  gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_ITEM_RESIZE, desc);
  g_free (desc);

  for (iter = layers; iter; iter = iter->next)
    {
      gint off_x, off_y;

      gimp_item_get_offset (GIMP_ITEM (iter->data), &off_x, &off_y);
      off_x -= x;
      off_y -= y;

      gimp_item_resize (GIMP_ITEM (iter->data),
                        action_data_get_context (data), GIMP_FILL_TRANSPARENT,
                        width, height, off_x, off_y);
    }

  gimp_image_undo_group_end (image);
  gimp_image_flush (image);
}

void
layers_crop_to_content_cmd_callback (GimpAction *action,
                                     GVariant   *value,
                                     gpointer    data)
{
  GimpImage *image;
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
      switch (gimp_pickable_auto_shrink (GIMP_PICKABLE (iter->data),
                                         0, 0,
                                         gimp_item_get_width  (iter->data),
                                         gimp_item_get_height (iter->data),
                                         &x, &y, &width, &height))
        {
        case GIMP_AUTO_SHRINK_SHRINK:
          n_croppable++;
          break;

        case GIMP_AUTO_SHRINK_EMPTY:
          /* Cannot crop because the layer has no content. */
        case GIMP_AUTO_SHRINK_UNSHRINKABLE:
          /* Cannot crop because the active layer is already cropped to
           * its content. */
          break;
        }
    }

  if (n_croppable == 0)
    {
      gimp_message_literal (image->gimp,
                            G_OBJECT (widget), GIMP_MESSAGE_INFO,
                            _("Cannot crop because none of the selected"
                              " layers have content or they are already"
                              " cropped to their content."));
      return;
    }

  desc = g_strdup_printf (ngettext ("Crop Layer to Content",
                                    "Crop %d Layers to Content",
                                    n_croppable),
                          n_croppable);
  gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_ITEM_RESIZE, desc);
  g_free (desc);

  for (iter = layers; iter; iter = iter->next)
    {
      switch (gimp_pickable_auto_shrink (GIMP_PICKABLE (iter->data),
                                         0, 0,
                                         gimp_item_get_width  (iter->data),
                                         gimp_item_get_height (iter->data),
                                         &x, &y, &width, &height))
        {
        case GIMP_AUTO_SHRINK_SHRINK:
          gimp_item_resize (iter->data,
                            action_data_get_context (data), GIMP_FILL_TRANSPARENT,
                            width, height, -x, -y);

          break;
        default:
          break;
        }
    }

  gimp_image_undo_group_end (image);
  gimp_image_flush (image);
}

void
layers_mask_add_cmd_callback (GimpAction *action,
                              GVariant   *value,
                              gpointer    data)
{
  GimpImage *image;
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
      g_return_if_fail (GIMP_IS_LAYER (iter->data));

      if (! gimp_layer_get_mask (iter->data))
        {
          update_layers = g_list_prepend (update_layers, iter->data);
          n_channels++;
        }
    }
  if (n_channels == 0)
    /* No layers or they all have masks already. */
    return;

#define ADD_MASK_DIALOG_KEY "gimp-add-mask-dialog"

  for (iter = update_layers; iter; iter = iter->next)
    {
      dialog = dialogs_get_dialog (G_OBJECT (iter->data), ADD_MASK_DIALOG_KEY);
      if (dialog)
        break;
    }

  if (! dialog)
    {
      GimpDialogConfig *config = GIMP_DIALOG_CONFIG (image->gimp->config);

      dialog = layer_add_mask_dialog_new (update_layers, action_data_get_context (data),
                                          widget,
                                          config->layer_add_mask_type,
                                          config->layer_add_mask_invert,
                                          config->layer_add_mask_edit_mask,
                                          layers_add_mask_callback,
                                          NULL);

      for (iter = update_layers; iter; iter = iter->next)
        dialogs_attach_dialog (G_OBJECT (iter->data), ADD_MASK_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

void
layers_mask_add_last_vals_cmd_callback (GimpAction *action,
                                        GVariant   *value,
                                        gpointer    data)
{
  GimpImage        *image;
  GList            *layers;
  GList            *iter;
  GtkWidget        *widget;
  GimpDialogConfig *config;
  GimpChannel      *channel = NULL;
  GimpLayerMask    *mask;
  return_if_no_layers (image, layers, data);
  return_if_no_widget (widget, data);

  config = GIMP_DIALOG_CONFIG (image->gimp->config);

  if (config->layer_add_mask_type == GIMP_ADD_MASK_CHANNEL)
    {
      GList *selected_channels;

      selected_channels = gimp_image_get_selected_channels (image);

      if (selected_channels)
        {
          channel = selected_channels->data;
        }
      else
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

  for (iter = layers; iter; iter = iter->next)
    {
      if (! gimp_layer_get_mask (iter->data))
        break;
    }
  if (iter == NULL)
    /* No layers or they all have masks already. */
    return;

  gimp_image_undo_group_start (image,
                               GIMP_UNDO_GROUP_LAYER_ADD,
                               _("Add Layer Masks"));
  for (iter = layers; iter; iter = iter->next)
    {
      if (gimp_layer_get_mask (iter->data))
        continue;

      mask = gimp_layer_create_mask (iter->data,
                                     config->layer_add_mask_type,
                                     channel);

      if (config->layer_add_mask_invert)
        gimp_channel_invert (GIMP_CHANNEL (mask), FALSE);

      gimp_layer_add_mask (iter->data, mask,
                           config->layer_add_mask_edit_mask,
                           TRUE, NULL);
    }

  gimp_image_undo_group_end (image);
  gimp_image_flush (image);
}

void
layers_mask_apply_cmd_callback (GimpAction *action,
                                GVariant   *value,
                                gpointer    data)
{
  GimpMaskApplyMode  mode;
  GimpImage         *image;
  GList             *layers;
  GList             *iter;
  gchar             *undo_text = NULL;
  GimpUndoType       undo_type = GIMP_UNDO_GROUP_NONE;

  return_if_no_layers (image, layers, data);

  mode = (GimpMaskApplyMode) g_variant_get_int32 (value);
  for (iter = layers; iter; iter = iter->next)
    {
      if (gimp_layer_get_mask (iter->data) &&
          (mode != GIMP_MASK_APPLY ||
           (! gimp_viewable_get_children (GIMP_VIEWABLE (iter->data)) &&
            ! gimp_item_is_content_locked (GIMP_ITEM (iter->data), NULL))))
        break;
    }

  if (iter == NULL)
    /* No layers or none have applicable masks. */
    return;

  switch (mode)
    {
    case GIMP_MASK_APPLY:
      undo_type = GIMP_UNDO_GROUP_MASK;
      undo_text = _("Apply Layer Masks");
      break;
    case GIMP_MASK_DISCARD:
      undo_type = GIMP_UNDO_GROUP_MASK;
      undo_text = _("Delete Layer Masks");
      break;
    default:
      g_warning ("%s: unhandled GimpMaskApplyMode %d\n",
                 G_STRFUNC, mode);
      break;
    }

  if (undo_type != GIMP_UNDO_GROUP_NONE)
    gimp_image_undo_group_start (image, undo_type, undo_text);

  for (iter = layers; iter; iter = iter->next)
    {
      if (gimp_layer_get_mask (iter->data))
        {
          if (mode == GIMP_MASK_APPLY &&
              (gimp_viewable_get_children (GIMP_VIEWABLE (iter->data)) ||
               gimp_item_is_content_locked (GIMP_ITEM (iter->data), NULL)))
            /* Layer groups cannot apply masks. Neither can
             * content-locked layers.
             */
            continue;

          gimp_layer_apply_mask (iter->data, mode, TRUE);
        }
    }

  if (undo_type != GIMP_UNDO_GROUP_NONE)
    gimp_image_undo_group_end (image);

  gimp_image_flush (image);
}

void
layers_mask_edit_cmd_callback (GimpAction *action,
                               GVariant   *value,
                               gpointer    data)
{
  GimpImage *image;
  GList     *layers;
  GList     *iter;
  gboolean   active = g_variant_get_boolean (value);
  return_if_no_layers (image, layers, data);

  /* Multiple-layer selection cannot edit masks. */
  active = active && (g_list_length (layers) == 1);

  for (iter = layers; iter; iter = iter->next)
    if (gimp_layer_get_mask (iter->data))
      gimp_layer_set_edit_mask (iter->data, active);

  gimp_image_flush (image);
}

void
layers_mask_show_cmd_callback (GimpAction *action,
                               GVariant   *value,
                               gpointer    data)
{
  GimpImage *image;
  GList     *layers;
  GList     *iter;
  gboolean   active  = g_variant_get_boolean (value);
  gint       n_masks = 0;
  return_if_no_layers (image, layers, data);

  for (iter = layers; iter; iter = iter->next)
    {
      if (gimp_layer_get_mask (iter->data))
        {
          if (active && gimp_layer_get_show_mask (iter->data))
            {
              /* if switching "show mask" on, and any selected layer's
               * mask is already visible, bail out because that's
               * exactly the logic we use in the ui for multiple
               * visible layer masks.
               */
              return;
            }

          if (gimp_layer_get_show_mask (iter->data) != active)
            n_masks++;
        }
    }

  if (n_masks == 0)
    return;

  if (n_masks > 1)
    gimp_image_undo_group_start (image,
                                 GIMP_UNDO_GROUP_LAYER_ADD,
                                 _("Show Layer Masks"));

  for (iter = layers; iter; iter = iter->next)
    {
      if (gimp_layer_get_mask (iter->data))
        gimp_layer_set_show_mask (iter->data, active, TRUE);
    }

  if (n_masks > 1)
    gimp_image_undo_group_end (image);

  gimp_image_flush (image);
}

void
layers_mask_disable_cmd_callback (GimpAction *action,
                                  GVariant   *value,
                                  gpointer    data)
{
  GimpImage *image;
  GList     *layers;
  GList     *iter;
  gboolean   active  = g_variant_get_boolean (value);
  gint       n_masks = 0;
  return_if_no_layers (image, layers, data);

  for (iter = layers; iter; iter = iter->next)
    {
      if (gimp_layer_get_mask (iter->data))
        {
          if (active && ! gimp_layer_get_apply_mask (iter->data))
            {
              /* if switching "disable mask" on, and any selected
               * layer's mask is already disabled, bail out because
               * that's exactly the logic we use in the ui for multiple
               * disabled layer masks.
               */
              return;
            }

          if ((! gimp_layer_get_apply_mask (iter->data)) != active)
            n_masks++;
        }
    }

  if (n_masks == 0)
    return;

  if (n_masks > 1)
    gimp_image_undo_group_start (image,
                                 GIMP_UNDO_GROUP_LAYER_ADD,
                                 _("Disable Layer Masks"));

  for (iter = layers; iter; iter = iter->next)
    {
      if (gimp_layer_get_mask (iter->data))
        gimp_layer_set_apply_mask (iter->data, ! active, TRUE);
    }

  if (n_masks > 1)
    gimp_image_undo_group_end (image);

  gimp_image_flush (image);
}

void
layers_mask_to_selection_cmd_callback (GimpAction *action,
                                       GVariant   *value,
                                       gpointer    data)
{
  GimpImage     *image;
  GList         *layers;
  GList         *iter;
  GList         *masks = NULL;
  return_if_no_layers (image, layers, data);

  for (iter = layers; iter; iter = iter->next)
    {
      if (gimp_layer_get_mask (iter->data))
        masks = g_list_prepend (masks, gimp_layer_get_mask (iter->data));
    }

  if (masks)
    {
      GimpChannelOps operation = (GimpChannelOps) g_variant_get_int32 (value);

      switch (operation)
        {
        case GIMP_CHANNEL_OP_REPLACE:
          gimp_channel_push_undo (gimp_image_get_mask (image),
                                  C_("undo-type", "Masks to Selection"));
          break;
        case GIMP_CHANNEL_OP_ADD:
          gimp_channel_push_undo (gimp_image_get_mask (image),
                                  C_("undo-type", "Add Masks to Selection"));
          break;
        case GIMP_CHANNEL_OP_SUBTRACT:
          gimp_channel_push_undo (gimp_image_get_mask (image),
                                  C_("undo-type", "Subtract Masks from Selection"));
          break;
        case GIMP_CHANNEL_OP_INTERSECT:
          gimp_channel_push_undo (gimp_image_get_mask (image),
                                  C_("undo-type", "Intersect Masks with Selection"));
          break;
        }

      gimp_channel_combine_items (gimp_image_get_mask (image),
                                  masks, operation);
      g_list_free (masks);

      gimp_image_flush (image);
    }
}

void
layers_alpha_add_cmd_callback (GimpAction *action,
                               GVariant   *value,
                               gpointer    data)
{
  GimpImage *image;
  GList     *layers;
  GList     *iter;
  return_if_no_layers (image, layers, data);

  gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_LAYER_ADD_ALPHA,
                               _("Add Alpha Channel"));

  for (iter = layers; iter; iter = iter->next)
    if (! gimp_drawable_has_alpha (iter->data))
      gimp_layer_add_alpha (iter->data);

  gimp_image_undo_group_end (image);
  gimp_image_flush (image);
}

void
layers_alpha_remove_cmd_callback (GimpAction *action,
                                  GVariant   *value,
                                  gpointer    data)
{
  GimpImage *image;
  GList     *layers;
  GList     *iter;
  return_if_no_layers (image, layers, data);

  gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_LAYER_ADD_ALPHA,
                               _("Remove Alpha Channel"));

  for (iter = layers; iter; iter = iter->next)
    if (gimp_drawable_has_alpha (iter->data))
      gimp_layer_remove_alpha (iter->data, action_data_get_context (data));

  gimp_image_undo_group_end (image);
  gimp_image_flush (image);
}

void
layers_alpha_to_selection_cmd_callback (GimpAction *action,
                                        GVariant   *value,
                                        gpointer    data)
{
  GimpImage      *image;
  GimpDisplay    *display;
  GList          *layers;
  GimpChannelOps  operation;
  return_if_no_layers (image, layers, data);
  return_if_no_display (display, data);

  operation = (GimpChannelOps) g_variant_get_int32 (value);

  switch (operation)
    {
    case GIMP_CHANNEL_OP_REPLACE:
      gimp_channel_push_undo (gimp_image_get_mask (image),
                              C_("undo-type", "Alpha to Selection"));
      break;
    case GIMP_CHANNEL_OP_ADD:
      gimp_channel_push_undo (gimp_image_get_mask (image),
                              C_("undo-type", "Add Alpha to Selection"));
      break;
    case GIMP_CHANNEL_OP_SUBTRACT:
      gimp_channel_push_undo (gimp_image_get_mask (image),
                              C_("undo-type", "Subtract Alpha from Selection"));
      break;
    case GIMP_CHANNEL_OP_INTERSECT:
      gimp_channel_push_undo (gimp_image_get_mask (image),
                              C_("undo-type", "Intersect Alpha with Selection"));
      break;
    }

  gimp_channel_combine_items (gimp_image_get_mask (image),
                              layers, operation);

  gimp_image_flush (image);

  if (gimp_channel_is_empty (gimp_image_get_mask (image)))
    {
      gimp_message_literal (image->gimp, G_OBJECT (display),
                            GIMP_MESSAGE_WARNING,
                            _("Empty Selection"));
    }
}

void
layers_opacity_cmd_callback (GimpAction *action,
                             GVariant   *value,
                             gpointer    data)
{
  GimpImage            *image;
  GList                *layers;
  GList                *iter;
  gdouble               opacity;
  GimpUndo             *undo;
  GimpActionSelectType  select_type;
  gboolean        push_undo = TRUE;
  return_if_no_layers (image, layers, data);

  select_type = (GimpActionSelectType) g_variant_get_int32 (value);
  if (g_list_length (layers) == 1)
    {
      undo = gimp_image_undo_can_compress (image, GIMP_TYPE_ITEM_UNDO,
                                           GIMP_UNDO_LAYER_OPACITY);

      if (undo && GIMP_ITEM_UNDO (undo)->item == GIMP_ITEM (layers->data))
        push_undo = FALSE;
    }

  if (g_list_length (layers) > 1)
    gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_LAYER_OPACITY,
                                 _("Set layers opacity"));

  for (iter = layers; iter; iter = iter->next)
    {
      opacity = action_select_value (select_type,
                                     gimp_layer_get_opacity (iter->data),
                                     0.0, 1.0, 1.0,
                                     1.0 / 255.0, 0.01, 0.1, 0.0, FALSE);
      gimp_layer_set_opacity (iter->data, opacity, push_undo);
    }

  if (g_list_length (layers) > 1)
    gimp_image_undo_group_end (image);

  gimp_image_flush (image);
}

void
layers_mode_cmd_callback (GimpAction *action,
                          GVariant   *value,
                          gpointer    data)
{
  GimpImage            *image;
  GList                *layers;
  GList                *iter;
  GimpActionSelectType  select_type;
  gboolean              push_undo = TRUE;
  return_if_no_layers (image, layers, data);

  select_type = (GimpActionSelectType) g_variant_get_int32 (value);

  if (g_list_length (layers) == 1)
    {
      GimpUndo *undo;

      undo = gimp_image_undo_can_compress (image, GIMP_TYPE_ITEM_UNDO,
                                           GIMP_UNDO_LAYER_MODE);

      if (undo && GIMP_ITEM_UNDO (undo)->item == GIMP_ITEM (layers->data))
        push_undo = FALSE;
    }

  if (g_list_length (layers) > 1)
    gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_LAYER_OPACITY,
                                 _("Set layers opacity"));

  for (iter = layers; iter; iter = iter->next)
    {
      GimpLayerMode *modes;
      gint           n_modes;
      GimpLayerMode  layer_mode;
      gint           index;

      layer_mode = gimp_layer_get_mode (iter->data);

      modes = gimp_layer_mode_get_context_array (layer_mode,
                                                 GIMP_LAYER_MODE_CONTEXT_LAYER,
                                                 &n_modes);
      index = layers_mode_index (layer_mode, modes, n_modes);
      index = action_select_value (select_type,
                                   index, 0, n_modes - 1, 0,
                                   0.0, 1.0, 1.0, 0.0, FALSE);
      layer_mode = modes[index];
      g_free (modes);

      gimp_layer_set_mode (iter->data, layer_mode, push_undo);
    }

  if (g_list_length (layers) > 1)
    gimp_image_undo_group_end (image);

  gimp_image_flush (image);
}

void
layers_blend_space_cmd_callback (GimpAction *action,
                                 GVariant   *value,
                                 gpointer    data)
{
  GimpImage           *image;
  GList               *layers;
  GList               *update_layers = NULL;
  GList               *iter;
  GimpLayerColorSpace  blend_space;
  gboolean             push_undo = TRUE;
  return_if_no_layers (image, layers, data);

  blend_space = (GimpLayerColorSpace) g_variant_get_int32 (value);

  for (iter = layers; iter; iter = iter->next)
    {
      GimpLayerMode mode;

      mode = gimp_layer_get_mode (iter->data);
      if (gimp_layer_mode_is_blend_space_mutable (mode) &&
          blend_space != gimp_layer_get_blend_space (iter->data))
        update_layers = g_list_prepend (update_layers, iter->data);
    }

  if (g_list_length (update_layers) == 1)
    {
      GimpUndo *undo;

      undo = gimp_image_undo_can_compress (image, GIMP_TYPE_LAYER_PROP_UNDO,
                                           GIMP_UNDO_LAYER_MODE);

      if (undo && GIMP_ITEM_UNDO (undo)->item == GIMP_ITEM (update_layers->data))
        push_undo = FALSE;
    }

  if (update_layers)
    {
      if (g_list_length (update_layers) > 1)
        gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_LAYER_MODE,
                                     _("Set layers' blend space"));

      for (iter = update_layers; iter; iter = iter->next)
        gimp_layer_set_blend_space (iter->data, blend_space, push_undo);

      if (g_list_length (update_layers) > 1)
        gimp_image_undo_group_end (image);

      g_list_free (update_layers);

      gimp_image_flush (image);
    }
}

void
layers_composite_space_cmd_callback (GimpAction *action,
                                     GVariant   *value,
                                     gpointer    data)
{
  GimpImage           *image;
  GList               *layers;
  GList               *update_layers = NULL;
  GList               *iter;
  GimpLayerColorSpace  composite_space;
  gboolean             push_undo = TRUE;
  return_if_no_layers (image, layers, data);

  composite_space = (GimpLayerColorSpace) g_variant_get_int32 (value);

  for (iter = layers; iter; iter = iter->next)
    {
      GimpLayerMode mode;

      mode = gimp_layer_get_mode (iter->data);
      if (gimp_layer_mode_is_composite_space_mutable (mode) &&
          composite_space != gimp_layer_get_composite_space (iter->data))
        update_layers = g_list_prepend (update_layers, iter->data);
    }

  if (g_list_length (update_layers) == 1)
    {
      GimpUndo *undo;

      undo = gimp_image_undo_can_compress (image, GIMP_TYPE_LAYER_PROP_UNDO,
                                           GIMP_UNDO_LAYER_MODE);

      if (undo && GIMP_ITEM_UNDO (undo)->item == GIMP_ITEM (update_layers->data))
        push_undo = FALSE;
    }

  if (update_layers)
    {
      if (g_list_length (update_layers) > 1)
        gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_LAYER_MODE,
                                     _("Set layers' composite space"));

      for (iter = update_layers; iter; iter = iter->next)
        gimp_layer_set_composite_space (iter->data, composite_space, push_undo);

      if (g_list_length (update_layers) > 1)
        gimp_image_undo_group_end (image);

      g_list_free (update_layers);

      gimp_image_flush (image);
    }
}

void
layers_composite_mode_cmd_callback (GimpAction *action,
                                    GVariant   *value,
                                    gpointer    data)
{
  GimpImage              *image;
  GList                  *layers;
  GList                  *update_layers = NULL;
  GList                  *iter;
  GimpLayerCompositeMode  composite_mode;
  gboolean             push_undo = TRUE;
  return_if_no_layers (image, layers, data);

  composite_mode = (GimpLayerCompositeMode) g_variant_get_int32 (value);

  for (iter = layers; iter; iter = iter->next)
    {
      GimpLayerMode mode;

      mode = gimp_layer_get_mode (iter->data);
      if (gimp_layer_mode_is_composite_mode_mutable (mode) &&
          composite_mode != gimp_layer_get_composite_mode (iter->data))
        update_layers = g_list_prepend (update_layers, iter->data);
    }

  if (g_list_length (update_layers) == 1)
    {
      GimpUndo *undo;

      undo = gimp_image_undo_can_compress (image, GIMP_TYPE_LAYER_PROP_UNDO,
                                           GIMP_UNDO_LAYER_MODE);

      if (undo && GIMP_ITEM_UNDO (undo)->item == GIMP_ITEM (update_layers->data))
        push_undo = FALSE;
    }

  if (update_layers)
    {
      if (g_list_length (update_layers) > 1)
        gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_LAYER_MODE,
                                     _("Set layers' composite mode"));

      for (iter = update_layers; iter; iter = iter->next)
        gimp_layer_set_composite_mode (iter->data, composite_mode, push_undo);

      if (g_list_length (update_layers) > 1)
        gimp_image_undo_group_end (image);

      g_list_free (update_layers);

      gimp_image_flush (image);
    }
}

void
layers_visible_cmd_callback (GimpAction *action,
                             GVariant   *value,
                             gpointer    data)
{
  GimpImage *image;
  GList     *layers;
  return_if_no_layers (image, layers, data);

  items_visible_cmd_callback (action, value, image, layers);
}

void
layers_lock_content_cmd_callback (GimpAction *action,
                                  GVariant   *value,
                                  gpointer    data)
{
  GimpImage *image;
  GList     *layers;
  return_if_no_layers (image, layers, data);

  items_lock_content_cmd_callback (action, value, image, layers);
}

void
layers_lock_position_cmd_callback (GimpAction *action,
                                   GVariant   *value,
                                   gpointer    data)
{
  GimpImage *image;
  GList     *layers;
  return_if_no_layers (image, layers, data);

  items_lock_position_cmd_callback (action, value, image, layers);
}

void
layers_lock_alpha_cmd_callback (GimpAction *action,
                                GVariant   *value,
                                gpointer    data)
{
  GimpImage *image;
  GList     *layers;
  GList     *iter;
  gboolean   lock_alpha;
  gboolean   lock_change = FALSE;
  return_if_no_layers (image, layers, data);

  lock_alpha = g_variant_get_boolean (value);

  for (iter = layers; iter; iter = iter->next)
    {
      if (gimp_layer_can_lock_alpha (iter->data))
        {
          /* Similar trick as in layers_mask_show_cmd_callback().
           * When unlocking, we expect all selected layers to be locked,
           * otherwise SET_ACTIVE() calls in layers-actions.c will
           * trigger lock updates.
           */
          if (! lock_alpha && ! gimp_layer_get_lock_alpha (iter->data))
            return;
          if (lock_alpha != gimp_layer_get_lock_alpha (iter->data))
            lock_change = TRUE;
        }
    }
  if (! lock_change)
    /* No layer locks would be changed. */
    return;

  gimp_image_undo_group_start (image,
                               GIMP_UNDO_GROUP_LAYER_LOCK_ALPHA,
                               lock_alpha ? _("Lock alpha channels") : _("Unlock alpha channels"));

  for (iter = layers; iter; iter = iter->next)
    {
      if (gimp_layer_can_lock_alpha (iter->data))
        {
          if (lock_alpha != gimp_layer_get_lock_alpha (iter->data))
            gimp_layer_set_lock_alpha (iter->data, lock_alpha, TRUE);
        }
    }

  gimp_image_undo_group_end (image);
  gimp_image_flush (image);
}

void
layers_color_tag_cmd_callback (GimpAction *action,
                               GVariant   *value,
                               gpointer    data)
{
  GimpImage    *image;
  GList        *layers;
  GimpColorTag  color_tag;
  return_if_no_layers (image, layers, data);

  color_tag = (GimpColorTag) g_variant_get_int32 (value);

  items_color_tag_cmd_callback (action, image, layers, color_tag);
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
                     GimpLink               *link,
                     gint                    layer_width,
                     gint                    layer_height,
                     gint                    layer_offset_x,
                     gint                    layer_offset_y,
                     gboolean                layer_visible,
                     GimpColorTag            layer_color_tag,
                     gboolean                layer_lock_pixels,
                     gboolean                layer_lock_position,
                     gboolean                layer_lock_visibility,
                     gboolean                layer_lock_alpha,
                     gboolean                rename_text_layer, /* unused */
                     gpointer                user_data)
{
  GimpDialogConfig *config     = GIMP_DIALOG_CONFIG (image->gimp->config);
  GList            *layers     = gimp_image_get_selected_layers (image);
  GList            *new_layers = NULL;
  GList            *iter;
  gint              n_layers   = g_list_length (layers);
  gboolean          run_once   = (n_layers == 0);

  g_return_if_fail (link == NULL);

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
  gimp_image_undo_group_start (image,
                               GIMP_UNDO_GROUP_LAYER_ADD,
                               ngettext ("New layer",
                                         "New layers",
                                         n_layers > 0 ? n_layers : 1));
  for (iter = layers; iter || run_once; iter = iter ? iter->next : NULL)
    {
      GimpLayer *parent;
      gint       position;

      run_once = FALSE;
      if (iter)
        {
          if (gimp_viewable_get_children (GIMP_VIEWABLE (iter->data)))
            {
              parent   = iter->data;
              position = 0;
            }
          else
            {
              parent   = GIMP_LAYER (gimp_item_get_parent (iter->data));
              position = gimp_item_get_index (iter->data);
            }
        }
      else /* run_once */
        {
          parent = NULL;
          position = 0;
        }

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
          gimp_item_set_color_tag (GIMP_ITEM (layer), layer_color_tag, FALSE);
          gimp_item_set_lock_content (GIMP_ITEM (layer), layer_lock_pixels,
                                      FALSE);
          gimp_item_set_lock_position (GIMP_ITEM (layer), layer_lock_position,
                                       FALSE);
          gimp_item_set_lock_visibility (GIMP_ITEM (layer), layer_lock_visibility,
                                         FALSE);
          gimp_layer_set_lock_alpha (layer, layer_lock_alpha, FALSE);
          gimp_layer_set_blend_space (layer, layer_blend_space, FALSE);
          gimp_layer_set_composite_space (layer, layer_composite_space, FALSE);
          gimp_layer_set_composite_mode (layer, layer_composite_mode, FALSE);

          gimp_image_add_layer (image, layer, parent, position, TRUE);
          gimp_image_flush (image);

          new_layers = g_list_prepend (new_layers, layer);
        }
      else
        {
          g_warning ("%s: could not allocate new layer", G_STRFUNC);
        }
    }

  gimp_image_undo_group_end (image);
  gimp_image_set_selected_layers (image, new_layers);

  g_list_free (layers);
  g_list_free (new_layers);
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
                                 GimpLink               *link,
                                 gint                    unused2,
                                 gint                    unused3,
                                 gint                    layer_offset_x,
                                 gint                    layer_offset_y,
                                 gboolean                layer_visible,
                                 GimpColorTag            layer_color_tag,
                                 gboolean                layer_lock_pixels,
                                 gboolean                layer_lock_position,
                                 gboolean                layer_lock_visibility,
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
      layer_color_tag       != gimp_item_get_color_tag (item)         ||
      layer_lock_pixels     != gimp_item_get_lock_content (item)      ||
      layer_lock_position   != gimp_item_get_lock_position (item)     ||
      layer_lock_visibility != gimp_item_get_lock_visibility (item)   ||
      layer_lock_alpha      != gimp_layer_get_lock_alpha (layer)      ||
      link)
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

      if (layer_color_tag != gimp_item_get_color_tag (item))
        gimp_item_set_color_tag (item, layer_color_tag, TRUE);

      if (layer_lock_pixels != gimp_item_get_lock_content (item))
        gimp_item_set_lock_content (item, layer_lock_pixels, TRUE);

      if (layer_lock_position != gimp_item_get_lock_position (item))
        gimp_item_set_lock_position (item, layer_lock_position, TRUE);

      if (layer_lock_visibility != gimp_item_get_lock_visibility (item))
        gimp_item_set_lock_visibility (item, layer_lock_visibility, TRUE);

      if (layer_lock_alpha != gimp_layer_get_lock_alpha (layer))
        gimp_layer_set_lock_alpha (layer, layer_lock_alpha, TRUE);

      if (GIMP_IS_LINK_LAYER (layer) && link)
        gimp_link_layer_set_link (GIMP_LINK_LAYER (layer), link, TRUE);

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
                          GList           *layers,
                          GimpAddMaskType  add_mask_type,
                          GimpChannel     *channel,
                          gboolean         invert,
                          gboolean         edit_mask,
                          gpointer         user_data)
{
  GimpImage        *image  = gimp_item_get_image (GIMP_ITEM (layers->data));
  GimpDialogConfig *config = GIMP_DIALOG_CONFIG (image->gimp->config);
  GimpLayerMask    *mask;
  GList            *iter;
  GError           *error = NULL;

  g_object_set (config,
                "layer-add-mask-type",      add_mask_type,
                "layer-add-mask-invert",    invert,
                "layer-add-mask-edit-mask", edit_mask,
                NULL);

  gimp_image_undo_group_start (image,
                               GIMP_UNDO_GROUP_LAYER_ADD,
                               _("Add Layer Masks"));

  for (iter = layers; iter; iter = iter->next)
    {
      mask = gimp_layer_create_mask (iter->data,
                                     config->layer_add_mask_type,
                                     channel);

      if (config->layer_add_mask_invert)
        gimp_channel_invert (GIMP_CHANNEL (mask), FALSE);

      if (! gimp_layer_add_mask (iter->data, mask, edit_mask, TRUE, &error))
        {
          gimp_message_literal (image->gimp,
                                G_OBJECT (dialog), GIMP_MESSAGE_WARNING,
                                error->message);
          g_object_unref (mask);
          g_clear_error (&error);
          return;
        }
    }

  gimp_image_undo_group_end (image);
  gimp_image_flush (image);

  gtk_widget_destroy (dialog);
}

static void
layers_scale_callback (GtkWidget             *dialog,
                       GimpViewable          *viewable,
                       gint                   width,
                       gint                   height,
                       GimpUnit              *unit,
                       GimpInterpolationType  interpolation,
                       gdouble                xresolution,    /* unused */
                       gdouble                yresolution,    /* unused */
                       GimpUnit              *resolution_unit,/* unused */
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
                        GimpUnit     *unit,
                        gint          offset_x,
                        gint          offset_y,
                        gdouble       unused0,
                        gdouble       unused1,
                        GimpUnit     *unused2,
                        GimpFillType  fill_type,
                        GimpItemSet   unused3,
                        gboolean      unused4,
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

static void
layers_vector_show_fill_stroke (GimpAction *action,
                                GVariant   *value,
                                gpointer    data)
{
  GimpImage *image;
  GimpLayer *layer;
  GList     *layers;
  GtkWidget *widget;
  return_if_no_layers (image, layers, data);
  return_if_no_widget (widget, data);

  if (g_list_length (layers) != 1)
    return;

  layer = layers->data;

  if (GIMP_IS_VECTOR_LAYER (layer))
    {
      GtkWidget *dialog;

      dialog = vector_layer_options_dialog_new (GIMP_VECTOR_LAYER (layer),
                                                action_data_get_context (data),
                                                _("Fill / Stroke"),
                                                "gimp-vector-layer-stroke",
                                                GIMP_HELP_LAYER_VECTOR_FILL_STROKE,
                                                widget);
      gtk_widget_show (dialog);
    }
}
