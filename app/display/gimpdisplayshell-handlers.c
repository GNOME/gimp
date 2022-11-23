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

#include "libligmabase/ligmabase.h"
#include "libligmacolor/ligmacolor.h"
#include "libligmaconfig/ligmaconfig.h"
#include "libligmamath/ligmamath.h"
#include "libligmawidgets/ligmawidgets.h"

#include "display-types.h"

#include "config/ligmadisplayoptions.h"
#include "config/ligmaguiconfig.h"

#include "core/ligma.h"
#include "core/ligma-cairo.h"
#include "core/ligmaguide.h"
#include "core/ligmaimage.h"
#include "core/ligmaimage-grid.h"
#include "core/ligmaimage-guides.h"
#include "core/ligmaimage-quick-mask.h"
#include "core/ligmaimage-sample-points.h"
#include "core/ligmaitem.h"
#include "core/ligmaitemstack.h"
#include "core/ligmasamplepoint.h"
#include "core/ligmatreehandler.h"

#include "vectors/ligmavectors.h"

#include "widgets/ligmawidgets-utils.h"

#include "ligmacanvascanvasboundary.h"
#include "ligmacanvasguide.h"
#include "ligmacanvaslayerboundary.h"
#include "ligmacanvaspath.h"
#include "ligmacanvasproxygroup.h"
#include "ligmacanvassamplepoint.h"
#include "ligmadisplay.h"
#include "ligmadisplayshell.h"
#include "ligmadisplayshell-appearance.h"
#include "ligmadisplayshell-callbacks.h"
#include "ligmadisplayshell-expose.h"
#include "ligmadisplayshell-handlers.h"
#include "ligmadisplayshell-profile.h"
#include "ligmadisplayshell-render.h"
#include "ligmadisplayshell-rulers.h"
#include "ligmadisplayshell-scale.h"
#include "ligmadisplayshell-scroll.h"
#include "ligmadisplayshell-selection.h"
#include "ligmadisplayshell-title.h"
#include "ligmaimagewindow.h"
#include "ligmastatusbar.h"

#include "ligma-intl.h"


/*  local function prototypes  */

static void   ligma_display_shell_clean_dirty_handler        (LigmaImage        *image,
                                                             LigmaDirtyMask     dirty_mask,
                                                             LigmaDisplayShell *shell);
static void   ligma_display_shell_undo_event_handler         (LigmaImage        *image,
                                                             LigmaUndoEvent     event,
                                                             LigmaUndo         *undo,
                                                             LigmaDisplayShell *shell);
static void   ligma_display_shell_grid_notify_handler        (LigmaGrid         *grid,
                                                             GParamSpec       *pspec,
                                                             LigmaDisplayShell *shell);
static void   ligma_display_shell_name_changed_handler       (LigmaImage        *image,
                                                             LigmaDisplayShell *shell);
static void   ligma_display_shell_selection_invalidate_handler
                                                            (LigmaImage        *image,
                                                             LigmaDisplayShell *shell);
static void   ligma_display_shell_component_visibility_changed_handler
                                                            (LigmaImage        *image,
                                                             LigmaChannelType   channel,
                                                             LigmaDisplayShell *shell);
static void   ligma_display_shell_size_changed_detailed_handler
                                                            (LigmaImage        *image,
                                                             gint              previous_origin_x,
                                                             gint              previous_origin_y,
                                                             gint              previous_width,
                                                             gint              previous_height,
                                                             LigmaDisplayShell *shell);
static void   ligma_display_shell_resolution_changed_handler (LigmaImage        *image,
                                                             LigmaDisplayShell *shell);
static void   ligma_display_shell_quick_mask_changed_handler (LigmaImage        *image,
                                                             LigmaDisplayShell *shell);
static void   ligma_display_shell_guide_add_handler          (LigmaImage        *image,
                                                             LigmaGuide        *guide,
                                                             LigmaDisplayShell *shell);
static void   ligma_display_shell_guide_remove_handler       (LigmaImage        *image,
                                                             LigmaGuide        *guide,
                                                             LigmaDisplayShell *shell);
static void   ligma_display_shell_guide_move_handler         (LigmaImage        *image,
                                                             LigmaGuide        *guide,
                                                             LigmaDisplayShell *shell);
static void   ligma_display_shell_sample_point_add_handler   (LigmaImage        *image,
                                                             LigmaSamplePoint  *sample_point,
                                                             LigmaDisplayShell *shell);
static void   ligma_display_shell_sample_point_remove_handler(LigmaImage        *image,
                                                             LigmaSamplePoint  *sample_point,
                                                             LigmaDisplayShell *shell);
static void   ligma_display_shell_sample_point_move_handler  (LigmaImage        *image,
                                                             LigmaSamplePoint  *sample_point,
                                                             LigmaDisplayShell *shell);
static void   ligma_display_shell_mode_changed_handler       (LigmaImage        *image,
                                                             LigmaDisplayShell *shell);
static void   ligma_display_shell_precision_changed_handler  (LigmaImage        *image,
                                                             LigmaDisplayShell *shell);
static void   ligma_display_shell_profile_changed_handler    (LigmaColorManaged *image,
                                                             LigmaDisplayShell *shell);
static void   ligma_display_shell_saved_handler              (LigmaImage        *image,
                                                             GFile            *file,
                                                             LigmaDisplayShell *shell);
static void   ligma_display_shell_exported_handler           (LigmaImage        *image,
                                                             GFile            *file,
                                                             LigmaDisplayShell *shell);

static void   ligma_display_shell_active_vectors_handler     (LigmaImage        *image,
                                                             LigmaDisplayShell *shell);

static void   ligma_display_shell_vectors_freeze_handler     (LigmaVectors      *vectors,
                                                             LigmaDisplayShell *shell);
static void   ligma_display_shell_vectors_thaw_handler       (LigmaVectors      *vectors,
                                                             LigmaDisplayShell *shell);
static void   ligma_display_shell_vectors_visible_handler    (LigmaVectors      *vectors,
                                                             LigmaDisplayShell *shell);
static void   ligma_display_shell_vectors_add_handler        (LigmaContainer    *container,
                                                             LigmaVectors      *vectors,
                                                             LigmaDisplayShell *shell);
static void   ligma_display_shell_vectors_remove_handler     (LigmaContainer    *container,
                                                             LigmaVectors      *vectors,
                                                             LigmaDisplayShell *shell);

static void   ligma_display_shell_check_notify_handler       (GObject          *config,
                                                             GParamSpec       *param_spec,
                                                             LigmaDisplayShell *shell);
static void   ligma_display_shell_title_notify_handler       (GObject          *config,
                                                             GParamSpec       *param_spec,
                                                             LigmaDisplayShell *shell);
static void   ligma_display_shell_nav_size_notify_handler    (GObject          *config,
                                                             GParamSpec       *param_spec,
                                                             LigmaDisplayShell *shell);
static void   ligma_display_shell_monitor_res_notify_handler (GObject          *config,
                                                             GParamSpec       *param_spec,
                                                             LigmaDisplayShell *shell);
static void   ligma_display_shell_padding_notify_handler     (GObject          *config,
                                                             GParamSpec       *param_spec,
                                                             LigmaDisplayShell *shell);
static void   ligma_display_shell_ants_speed_notify_handler  (GObject          *config,
                                                             GParamSpec       *param_spec,
                                                             LigmaDisplayShell *shell);
static void   ligma_display_shell_quality_notify_handler     (GObject          *config,
                                                             GParamSpec       *param_spec,
                                                             LigmaDisplayShell *shell);
static void  ligma_display_shell_color_config_notify_handler (GObject          *config,
                                                             GParamSpec       *param_spec,
                                                             LigmaDisplayShell *shell);
static void  ligma_display_shell_display_changed_handler     (LigmaContext      *context,
                                                             LigmaDisplay      *display,
                                                             LigmaDisplayShell *shell);


/*  public functions  */

void
ligma_display_shell_connect (LigmaDisplayShell *shell)
{
  LigmaImage         *image;
  LigmaContainer     *vectors;
  LigmaDisplayConfig *config;
  LigmaColorConfig   *color_config;
  LigmaContext       *user_context;
  GList             *list;

  g_return_if_fail (LIGMA_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (LIGMA_IS_DISPLAY (shell->display));

  image = ligma_display_get_image (shell->display);

  g_return_if_fail (LIGMA_IS_IMAGE (image));

  vectors = ligma_image_get_vectors (image);

  config       = shell->display->config;
  color_config = LIGMA_CORE_CONFIG (config)->color_management;

  user_context = ligma_get_user_context (shell->display->ligma);

  g_signal_connect (image, "clean",
                    G_CALLBACK (ligma_display_shell_clean_dirty_handler),
                    shell);
  g_signal_connect (image, "dirty",
                    G_CALLBACK (ligma_display_shell_clean_dirty_handler),
                    shell);
  g_signal_connect (image, "undo-event",
                    G_CALLBACK (ligma_display_shell_undo_event_handler),
                    shell);

  g_signal_connect (ligma_image_get_grid (image), "notify",
                    G_CALLBACK (ligma_display_shell_grid_notify_handler),
                    shell);
  g_object_set (shell->grid, "grid", ligma_image_get_grid (image), NULL);

  g_signal_connect (image, "name-changed",
                    G_CALLBACK (ligma_display_shell_name_changed_handler),
                    shell);
  g_signal_connect (image, "selection-invalidate",
                    G_CALLBACK (ligma_display_shell_selection_invalidate_handler),
                    shell);
  g_signal_connect (image, "component-visibility-changed",
                    G_CALLBACK (ligma_display_shell_component_visibility_changed_handler),
                    shell);
  g_signal_connect (image, "size-changed-detailed",
                    G_CALLBACK (ligma_display_shell_size_changed_detailed_handler),
                    shell);
  g_signal_connect (image, "resolution-changed",
                    G_CALLBACK (ligma_display_shell_resolution_changed_handler),
                    shell);
  g_signal_connect (image, "quick-mask-changed",
                    G_CALLBACK (ligma_display_shell_quick_mask_changed_handler),
                    shell);

  g_signal_connect (image, "guide-added",
                    G_CALLBACK (ligma_display_shell_guide_add_handler),
                    shell);
  g_signal_connect (image, "guide-removed",
                    G_CALLBACK (ligma_display_shell_guide_remove_handler),
                    shell);
  g_signal_connect (image, "guide-moved",
                    G_CALLBACK (ligma_display_shell_guide_move_handler),
                    shell);
  for (list = ligma_image_get_guides (image);
       list;
       list = g_list_next (list))
    {
      ligma_display_shell_guide_add_handler (image, list->data, shell);
    }

  g_signal_connect (image, "sample-point-added",
                    G_CALLBACK (ligma_display_shell_sample_point_add_handler),
                    shell);
  g_signal_connect (image, "sample-point-removed",
                    G_CALLBACK (ligma_display_shell_sample_point_remove_handler),
                    shell);
  g_signal_connect (image, "sample-point-moved",
                    G_CALLBACK (ligma_display_shell_sample_point_move_handler),
                    shell);
  for (list = ligma_image_get_sample_points (image);
       list;
       list = g_list_next (list))
    {
      ligma_display_shell_sample_point_add_handler (image, list->data, shell);
    }

  g_signal_connect (image, "mode-changed",
                    G_CALLBACK (ligma_display_shell_mode_changed_handler),
                    shell);
  g_signal_connect (image, "precision-changed",
                    G_CALLBACK (ligma_display_shell_precision_changed_handler),
                    shell);
  g_signal_connect (image, "profile-changed",
                    G_CALLBACK (ligma_display_shell_profile_changed_handler),
                    shell);
  g_signal_connect_swapped (image, "simulation-profile-changed",
                            G_CALLBACK (ligma_display_shell_profile_update),
                            shell);
  g_signal_connect_swapped (image, "simulation-intent-changed",
                            G_CALLBACK (ligma_display_shell_profile_update),
                            shell);
  g_signal_connect_swapped (image, "simulation-bpc-changed",
                            G_CALLBACK (ligma_display_shell_profile_update),
                            shell);

  g_signal_connect (image, "saved",
                    G_CALLBACK (ligma_display_shell_saved_handler),
                    shell);
  g_signal_connect (image, "exported",
                    G_CALLBACK (ligma_display_shell_exported_handler),
                    shell);

  g_signal_connect (image, "selected-vectors-changed",
                    G_CALLBACK (ligma_display_shell_active_vectors_handler),
                    shell);

  shell->vectors_freeze_handler =
    ligma_tree_handler_connect (vectors, "freeze",
                               G_CALLBACK (ligma_display_shell_vectors_freeze_handler),
                               shell);
  shell->vectors_thaw_handler =
    ligma_tree_handler_connect (vectors, "thaw",
                               G_CALLBACK (ligma_display_shell_vectors_thaw_handler),
                               shell);
  shell->vectors_visible_handler =
    ligma_tree_handler_connect (vectors, "visibility-changed",
                               G_CALLBACK (ligma_display_shell_vectors_visible_handler),
                               shell);

  g_signal_connect (vectors, "add",
                    G_CALLBACK (ligma_display_shell_vectors_add_handler),
                    shell);
  g_signal_connect (vectors, "remove",
                    G_CALLBACK (ligma_display_shell_vectors_remove_handler),
                    shell);
  for (list = ligma_item_stack_get_item_iter (LIGMA_ITEM_STACK (vectors));
       list;
       list = g_list_next (list))
    {
      ligma_display_shell_vectors_add_handler (vectors, list->data, shell);
    }

  g_signal_connect (config,
                    "notify::transparency-size",
                    G_CALLBACK (ligma_display_shell_check_notify_handler),
                    shell);
  g_signal_connect (config,
                    "notify::transparency-type",
                    G_CALLBACK (ligma_display_shell_check_notify_handler),
                    shell);
  g_signal_connect (config,
                    "notify::transparency-custom-color1",
                    G_CALLBACK (ligma_display_shell_check_notify_handler),
                    shell);
  g_signal_connect (config,
                    "notify::transparency-custom-color2",
                    G_CALLBACK (ligma_display_shell_check_notify_handler),
                    shell);

  g_signal_connect (config,
                    "notify::image-title-format",
                    G_CALLBACK (ligma_display_shell_title_notify_handler),
                    shell);
  g_signal_connect (config,
                    "notify::image-status-format",
                    G_CALLBACK (ligma_display_shell_title_notify_handler),
                    shell);
  g_signal_connect (config,
                    "notify::navigation-preview-size",
                    G_CALLBACK (ligma_display_shell_nav_size_notify_handler),
                    shell);
  g_signal_connect (config,
                    "notify::monitor-resolution-from-windowing-system",
                    G_CALLBACK (ligma_display_shell_monitor_res_notify_handler),
                    shell);
  g_signal_connect (config,
                    "notify::monitor-xresolution",
                    G_CALLBACK (ligma_display_shell_monitor_res_notify_handler),
                    shell);
  g_signal_connect (config,
                    "notify::monitor-yresolution",
                    G_CALLBACK (ligma_display_shell_monitor_res_notify_handler),
                    shell);

  g_signal_connect (config->default_view,
                    "notify::padding-mode",
                    G_CALLBACK (ligma_display_shell_padding_notify_handler),
                    shell);
  g_signal_connect (config->default_view,
                    "notify::padding-color",
                    G_CALLBACK (ligma_display_shell_padding_notify_handler),
                    shell);
  g_signal_connect (config->default_fullscreen_view,
                    "notify::padding-mode",
                    G_CALLBACK (ligma_display_shell_padding_notify_handler),
                    shell);
  g_signal_connect (config->default_fullscreen_view,
                    "notify::padding-color",
                    G_CALLBACK (ligma_display_shell_padding_notify_handler),
                    shell);

  g_signal_connect (config,
                    "notify::marching-ants-speed",
                    G_CALLBACK (ligma_display_shell_ants_speed_notify_handler),
                    shell);

  g_signal_connect (config,
                    "notify::zoom-quality",
                    G_CALLBACK (ligma_display_shell_quality_notify_handler),
                    shell);

  g_signal_connect (color_config, "notify",
                    G_CALLBACK (ligma_display_shell_color_config_notify_handler),
                    shell);

  g_signal_connect (user_context, "display-changed",
                    G_CALLBACK (ligma_display_shell_display_changed_handler),
                    shell);

  ligma_display_shell_active_vectors_handler     (image, shell);
  ligma_display_shell_quick_mask_changed_handler (image, shell);
  ligma_display_shell_profile_changed_handler    (LIGMA_COLOR_MANAGED (image),
                                                 shell);
  ligma_display_shell_color_config_notify_handler (G_OBJECT (color_config),
                                                  NULL, /* sync all */
                                                  shell);

  ligma_canvas_layer_boundary_set_layers (LIGMA_CANVAS_LAYER_BOUNDARY (shell->layer_boundary),
                                         ligma_image_get_selected_layers (image));

  ligma_canvas_canvas_boundary_set_image (LIGMA_CANVAS_CANVAS_BOUNDARY (shell->canvas_boundary),
                                         image);

  if (shell->show_all)
    {
      ligma_image_inc_show_all_count (image);

      ligma_image_flush (image);
    }
}

void
ligma_display_shell_disconnect (LigmaDisplayShell *shell)
{
  LigmaImage         *image;
  LigmaContainer     *vectors;
  LigmaDisplayConfig *config;
  LigmaColorConfig   *color_config;
  LigmaContext       *user_context;
  GList             *list;

  g_return_if_fail (LIGMA_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (LIGMA_IS_DISPLAY (shell->display));

  image = ligma_display_get_image (shell->display);

  g_return_if_fail (LIGMA_IS_IMAGE (image));

  vectors = ligma_image_get_vectors (image);

  config       = shell->display->config;
  color_config = LIGMA_CORE_CONFIG (config)->color_management;

  user_context = ligma_get_user_context (shell->display->ligma);

  ligma_canvas_layer_boundary_set_layers (LIGMA_CANVAS_LAYER_BOUNDARY (shell->layer_boundary),
                                         NULL);

  ligma_canvas_canvas_boundary_set_image (LIGMA_CANVAS_CANVAS_BOUNDARY (shell->canvas_boundary),
                                         NULL);

  g_signal_handlers_disconnect_by_func (user_context,
                                        ligma_display_shell_display_changed_handler,
                                        shell);

  g_signal_handlers_disconnect_by_func (color_config,
                                        ligma_display_shell_color_config_notify_handler,
                                        shell);
  shell->color_config_set = FALSE;

  g_signal_handlers_disconnect_by_func (config,
                                        ligma_display_shell_quality_notify_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (config,
                                        ligma_display_shell_ants_speed_notify_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (config->default_fullscreen_view,
                                        ligma_display_shell_padding_notify_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (config->default_view,
                                        ligma_display_shell_padding_notify_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (config,
                                        ligma_display_shell_monitor_res_notify_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (config,
                                        ligma_display_shell_nav_size_notify_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (config,
                                        ligma_display_shell_title_notify_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (config,
                                        ligma_display_shell_check_notify_handler,
                                        shell);

  g_signal_handlers_disconnect_by_func (vectors,
                                        ligma_display_shell_vectors_remove_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (vectors,
                                        ligma_display_shell_vectors_add_handler,
                                        shell);

  ligma_tree_handler_disconnect (shell->vectors_visible_handler);
  shell->vectors_visible_handler = NULL;

  ligma_tree_handler_disconnect (shell->vectors_thaw_handler);
  shell->vectors_thaw_handler = NULL;

  ligma_tree_handler_disconnect (shell->vectors_freeze_handler);
  shell->vectors_freeze_handler = NULL;

  g_signal_handlers_disconnect_by_func (image,
                                        ligma_display_shell_active_vectors_handler,
                                        shell);

  for (list = ligma_item_stack_get_item_iter (LIGMA_ITEM_STACK (vectors));
       list;
       list = g_list_next (list))
    {
      ligma_canvas_proxy_group_remove_item (LIGMA_CANVAS_PROXY_GROUP (shell->vectors),
                                           list->data);
    }

  g_signal_handlers_disconnect_by_func (image,
                                        ligma_display_shell_exported_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (image,
                                        ligma_display_shell_saved_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (image,
                                        ligma_display_shell_profile_changed_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (image,
                                        ligma_display_shell_profile_update,
                                        shell);
  g_signal_handlers_disconnect_by_func (image,
                                        ligma_display_shell_precision_changed_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (image,
                                        ligma_display_shell_mode_changed_handler,
                                        shell);

  g_signal_handlers_disconnect_by_func (image,
                                        ligma_display_shell_guide_add_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (image,
                                        ligma_display_shell_guide_remove_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (image,
                                        ligma_display_shell_guide_move_handler,
                                        shell);
  for (list = ligma_image_get_guides (image);
       list;
       list = g_list_next (list))
    {
      ligma_canvas_proxy_group_remove_item (LIGMA_CANVAS_PROXY_GROUP (shell->guides),
                                           list->data);
    }

  g_signal_handlers_disconnect_by_func (image,
                                        ligma_display_shell_sample_point_add_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (image,
                                        ligma_display_shell_sample_point_remove_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (image,
                                        ligma_display_shell_sample_point_move_handler,
                                        shell);
  for (list = ligma_image_get_sample_points (image);
       list;
       list = g_list_next (list))
    {
      ligma_canvas_proxy_group_remove_item (LIGMA_CANVAS_PROXY_GROUP (shell->sample_points),
                                           list->data);
    }

  g_signal_handlers_disconnect_by_func (image,
                                        ligma_display_shell_quick_mask_changed_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (image,
                                        ligma_display_shell_resolution_changed_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (image,
                                        ligma_display_shell_component_visibility_changed_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (image,
                                        ligma_display_shell_size_changed_detailed_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (image,
                                        ligma_display_shell_selection_invalidate_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (image,
                                        ligma_display_shell_name_changed_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (ligma_image_get_grid (image),
                                        ligma_display_shell_grid_notify_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (image,
                                        ligma_display_shell_undo_event_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (image,
                                        ligma_display_shell_clean_dirty_handler,
                                        shell);

  if (shell->show_all)
    {
      ligma_image_dec_show_all_count (image);

      ligma_image_flush (image);
    }
}


/*  private functions  */

static void
ligma_display_shell_clean_dirty_handler (LigmaImage        *image,
                                        LigmaDirtyMask     dirty_mask,
                                        LigmaDisplayShell *shell)
{
  ligma_display_shell_title_update (shell);
}

static void
ligma_display_shell_undo_event_handler (LigmaImage        *image,
                                       LigmaUndoEvent     event,
                                       LigmaUndo         *undo,
                                       LigmaDisplayShell *shell)
{
  ligma_display_shell_title_update (shell);
}

static void
ligma_display_shell_grid_notify_handler (LigmaGrid         *grid,
                                        GParamSpec       *pspec,
                                        LigmaDisplayShell *shell)
{
  g_object_set (shell->grid, "grid", grid, NULL);
}

static void
ligma_display_shell_name_changed_handler (LigmaImage        *image,
                                         LigmaDisplayShell *shell)
{
  ligma_display_shell_title_update (shell);
}

static void
ligma_display_shell_selection_invalidate_handler (LigmaImage        *image,
                                                 LigmaDisplayShell *shell)
{
  ligma_display_shell_selection_undraw (shell);
}

static void
ligma_display_shell_resolution_changed_handler (LigmaImage        *image,
                                               LigmaDisplayShell *shell)
{
  ligma_display_shell_scale_update (shell);

  if (shell->dot_for_dot)
    {
      if (shell->unit != LIGMA_UNIT_PIXEL)
        {
          ligma_display_shell_rulers_update (shell);
        }

      ligma_display_shell_scaled (shell);
    }
  else
    {
      /* A resolution change has the same effect as a size change from
       * a display shell point of view. Force a redraw of the display
       * so that we don't get any display garbage.
       */

      LigmaDisplayConfig *config = shell->display->config;
      gboolean           resize_window;

      /* Resize windows only in multi-window mode */
      resize_window = (config->resize_windows_on_resize &&
                       ! LIGMA_GUI_CONFIG (config)->single_window_mode);

      ligma_display_shell_scale_resize (shell, resize_window, FALSE);
    }
}

static void
ligma_display_shell_quick_mask_changed_handler (LigmaImage        *image,
                                               LigmaDisplayShell *shell)
{
  GtkImage *gtk_image;
  gboolean  quick_mask_state;

  gtk_image = GTK_IMAGE (gtk_bin_get_child (GTK_BIN (shell->quick_mask_button)));

  g_signal_handlers_block_by_func (shell->quick_mask_button,
                                   ligma_display_shell_quick_mask_toggled,
                                   shell);

  quick_mask_state = ligma_image_get_quick_mask_state (image);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (shell->quick_mask_button),
                                quick_mask_state);

  if (quick_mask_state)
    gtk_image_set_from_icon_name (gtk_image, LIGMA_ICON_QUICK_MASK_ON,
                                  GTK_ICON_SIZE_MENU);
  else
    gtk_image_set_from_icon_name (gtk_image, LIGMA_ICON_QUICK_MASK_OFF,
                                  GTK_ICON_SIZE_MENU);

  g_signal_handlers_unblock_by_func (shell->quick_mask_button,
                                     ligma_display_shell_quick_mask_toggled,
                                     shell);
}

static void
ligma_display_shell_guide_add_handler (LigmaImage        *image,
                                      LigmaGuide        *guide,
                                      LigmaDisplayShell *shell)
{
  LigmaCanvasProxyGroup *group = LIGMA_CANVAS_PROXY_GROUP (shell->guides);
  LigmaCanvasItem       *item;
  LigmaGuideStyle        style;

  style = ligma_guide_get_style (guide);
  item = ligma_canvas_guide_new (shell,
                                ligma_guide_get_orientation (guide),
                                ligma_guide_get_position (guide),
                                style);

  ligma_canvas_proxy_group_add_item (group, guide, item);
  g_object_unref (item);
}

static void
ligma_display_shell_guide_remove_handler (LigmaImage        *image,
                                         LigmaGuide        *guide,
                                         LigmaDisplayShell *shell)
{
  LigmaCanvasProxyGroup *group = LIGMA_CANVAS_PROXY_GROUP (shell->guides);

  ligma_canvas_proxy_group_remove_item (group, guide);
}

static void
ligma_display_shell_guide_move_handler (LigmaImage        *image,
                                       LigmaGuide        *guide,
                                       LigmaDisplayShell *shell)
{
  LigmaCanvasProxyGroup *group = LIGMA_CANVAS_PROXY_GROUP (shell->guides);
  LigmaCanvasItem       *item;

  item = ligma_canvas_proxy_group_get_item (group, guide);

  ligma_canvas_guide_set (item,
                         ligma_guide_get_orientation (guide),
                         ligma_guide_get_position (guide));
}

static void
ligma_display_shell_sample_point_add_handler (LigmaImage        *image,
                                             LigmaSamplePoint  *sample_point,
                                             LigmaDisplayShell *shell)
{
  LigmaCanvasProxyGroup *group = LIGMA_CANVAS_PROXY_GROUP (shell->sample_points);
  LigmaCanvasItem       *item;
  GList                *list;
  gint                  x;
  gint                  y;
  gint                  i;

  ligma_sample_point_get_position (sample_point, &x, &y);

  item = ligma_canvas_sample_point_new (shell, x, y, 0, TRUE);

  ligma_canvas_proxy_group_add_item (group, sample_point, item);
  g_object_unref (item);

  for (list = ligma_image_get_sample_points (image), i = 1;
       list;
       list = g_list_next (list), i++)
    {
      LigmaSamplePoint *sample_point = list->data;

      item = ligma_canvas_proxy_group_get_item (group, sample_point);

      if (item)
        g_object_set (item,
                      "index", i,
                      NULL);
    }
}

static void
ligma_display_shell_sample_point_remove_handler (LigmaImage        *image,
                                                LigmaSamplePoint  *sample_point,
                                                LigmaDisplayShell *shell)
{
  LigmaCanvasProxyGroup *group = LIGMA_CANVAS_PROXY_GROUP (shell->sample_points);
  GList                *list;
  gint                  i;

  ligma_canvas_proxy_group_remove_item (group, sample_point);

  for (list = ligma_image_get_sample_points (image), i = 1;
       list;
       list = g_list_next (list), i++)
    {
      LigmaSamplePoint *sample_point = list->data;
      LigmaCanvasItem  *item;

      item = ligma_canvas_proxy_group_get_item (group, sample_point);

      if (item)
        g_object_set (item,
                      "index", i,
                      NULL);
    }
}

static void
ligma_display_shell_sample_point_move_handler (LigmaImage        *image,
                                              LigmaSamplePoint  *sample_point,
                                              LigmaDisplayShell *shell)
{
  LigmaCanvasProxyGroup *group = LIGMA_CANVAS_PROXY_GROUP (shell->sample_points);
  LigmaCanvasItem       *item;
  gint                  x;
  gint                  y;

  item = ligma_canvas_proxy_group_get_item (group, sample_point);

  ligma_sample_point_get_position (sample_point, &x, &y);

  ligma_canvas_sample_point_set (item, x, y);
}

static void
ligma_display_shell_component_visibility_changed_handler (LigmaImage        *image,
                                                         LigmaChannelType   channel,
                                                         LigmaDisplayShell *shell)
{
  if (channel == LIGMA_CHANNEL_ALPHA && shell->show_all)
    ligma_display_shell_expose_full (shell);
}

static void
ligma_display_shell_size_changed_detailed_handler (LigmaImage        *image,
                                                  gint              previous_origin_x,
                                                  gint              previous_origin_y,
                                                  gint              previous_width,
                                                  gint              previous_height,
                                                  LigmaDisplayShell *shell)
{
  LigmaDisplayConfig *config = shell->display->config;
  gboolean           resize_window;

  /* Resize windows only in multi-window mode */
  resize_window = (config->resize_windows_on_resize &&
                   ! LIGMA_GUI_CONFIG (config)->single_window_mode);

  if (resize_window)
    {
      LigmaImageWindow *window = ligma_display_shell_get_window (shell);

      if (window && ligma_image_window_get_active_shell (window) == shell)
        {
          /* If the window is resized just center the image in it when it
           * has change size
           */
          ligma_image_window_shrink_wrap (window, FALSE);
        }
    }
  else
    {
      LigmaImage *image      = ligma_display_get_image (shell->display);
      gint       new_width  = ligma_image_get_width  (image);
      gint       new_height = ligma_image_get_height (image);
      gint       scaled_previous_origin_x;
      gint       scaled_previous_origin_y;
      gboolean   horizontally;
      gboolean   vertically;

      scaled_previous_origin_x = SCALEX (shell, previous_origin_x);
      scaled_previous_origin_y = SCALEY (shell, previous_origin_y);

      horizontally = (SCALEX (shell, previous_width)  >  shell->disp_width  &&
                      SCALEX (shell, new_width)       <= shell->disp_width);
      vertically   = (SCALEY (shell, previous_height) >  shell->disp_height &&
                      SCALEY (shell, new_height)      <= shell->disp_height);

      ligma_display_shell_scroll_set_offset (shell,
                                            shell->offset_x + scaled_previous_origin_x,
                                            shell->offset_y + scaled_previous_origin_y);

      if (! ligma_display_shell_get_infinite_canvas (shell))
        {
          ligma_display_shell_scroll_center_image (shell,
                                                  horizontally, vertically);
        }

      /* The above calls might not lead to a call to
       * ligma_display_shell_scroll_clamp_and_update() and
       * ligma_display_shell_expose_full() in all cases because when
       * scaling the old and new scroll offset might be the same.
       *
       * We need them to be called in all cases, so simply call them
       * explicitly here at the end
       */
      ligma_display_shell_scroll_clamp_and_update (shell);

      ligma_display_shell_expose_full (shell);
      ligma_display_shell_render_invalidate_full (shell);
    }
}

static void
ligma_display_shell_mode_changed_handler (LigmaImage        *image,
                                         LigmaDisplayShell *shell)
{
  ligma_display_shell_profile_update (shell);
}

static void
ligma_display_shell_precision_changed_handler (LigmaImage        *image,
                                              LigmaDisplayShell *shell)
{
  ligma_display_shell_profile_update (shell);
}

static void
ligma_display_shell_profile_changed_handler (LigmaColorManaged *image,
                                            LigmaDisplayShell *shell)
{
  ligma_color_managed_profile_changed (LIGMA_COLOR_MANAGED (shell));
}

static void
ligma_display_shell_saved_handler (LigmaImage        *image,
                                  GFile            *file,
                                  LigmaDisplayShell *shell)
{
  LigmaStatusbar *statusbar = ligma_display_shell_get_statusbar (shell);

  ligma_statusbar_push_temp (statusbar, LIGMA_MESSAGE_INFO,
                            LIGMA_ICON_DOCUMENT_SAVE,
                            _("Image saved to '%s'"),
                            ligma_file_get_utf8_name (file));
}

static void
ligma_display_shell_exported_handler (LigmaImage        *image,
                                     GFile            *file,
                                     LigmaDisplayShell *shell)
{
  LigmaStatusbar *statusbar = ligma_display_shell_get_statusbar (shell);

  ligma_statusbar_push_temp (statusbar, LIGMA_MESSAGE_INFO,
                            LIGMA_ICON_DOCUMENT_SAVE,
                            _("Image exported to '%s'"),
                            ligma_file_get_utf8_name (file));
}

static void
ligma_display_shell_active_vectors_handler (LigmaImage        *image,
                                           LigmaDisplayShell *shell)
{
  LigmaCanvasProxyGroup *group    = LIGMA_CANVAS_PROXY_GROUP (shell->vectors);
  GList                *selected = ligma_image_get_selected_vectors (image);
  GList                *list;

  for (list = ligma_image_get_vectors_iter (image);
       list;
       list = g_list_next (list))
    {
      LigmaVectors    *vectors = list->data;
      LigmaCanvasItem *item;

      item = ligma_canvas_proxy_group_get_item (group, vectors);

      ligma_canvas_item_set_highlight (item,
                                      g_list_find (selected, vectors) != NULL);
    }
}

static void
ligma_display_shell_vectors_freeze_handler (LigmaVectors      *vectors,
                                           LigmaDisplayShell *shell)
{
  /* do nothing */
}

static void
ligma_display_shell_vectors_thaw_handler (LigmaVectors      *vectors,
                                         LigmaDisplayShell *shell)
{
  LigmaCanvasProxyGroup *group = LIGMA_CANVAS_PROXY_GROUP (shell->vectors);
  LigmaCanvasItem       *item;

  item = ligma_canvas_proxy_group_get_item (group, vectors);

  ligma_canvas_path_set (item, ligma_vectors_get_bezier (vectors));
}

static void
ligma_display_shell_vectors_visible_handler (LigmaVectors      *vectors,
                                            LigmaDisplayShell *shell)
{
  LigmaCanvasProxyGroup *group = LIGMA_CANVAS_PROXY_GROUP (shell->vectors);
  LigmaCanvasItem       *item;

  item = ligma_canvas_proxy_group_get_item (group, vectors);

  ligma_canvas_item_set_visible (item,
                                ligma_item_get_visible (LIGMA_ITEM (vectors)));
}

static void
ligma_display_shell_vectors_add_handler (LigmaContainer    *container,
                                        LigmaVectors      *vectors,
                                        LigmaDisplayShell *shell)
{
  LigmaCanvasProxyGroup *group = LIGMA_CANVAS_PROXY_GROUP (shell->vectors);
  LigmaCanvasItem       *item;

  item = ligma_canvas_path_new (shell,
                               ligma_vectors_get_bezier (vectors),
                               0, 0,
                               FALSE,
                               LIGMA_PATH_STYLE_VECTORS);
  ligma_canvas_item_set_visible (item,
                                ligma_item_get_visible (LIGMA_ITEM (vectors)));

  ligma_canvas_proxy_group_add_item (group, vectors, item);
  g_object_unref (item);
}

static void
ligma_display_shell_vectors_remove_handler (LigmaContainer    *container,
                                           LigmaVectors      *vectors,
                                           LigmaDisplayShell *shell)
{
  LigmaCanvasProxyGroup *group = LIGMA_CANVAS_PROXY_GROUP (shell->vectors);

  ligma_canvas_proxy_group_remove_item (group, vectors);
}

static void
ligma_display_shell_check_notify_handler (GObject          *config,
                                         GParamSpec       *param_spec,
                                         LigmaDisplayShell *shell)
{
  LigmaCanvasPaddingMode padding_mode;
  LigmaRGB               padding_color;

  g_clear_pointer (&shell->checkerboard, cairo_pattern_destroy);

  ligma_display_shell_get_padding (shell, &padding_mode, &padding_color);

  switch (padding_mode)
    {
    case LIGMA_CANVAS_PADDING_MODE_LIGHT_CHECK:
    case LIGMA_CANVAS_PADDING_MODE_DARK_CHECK:
      ligma_display_shell_set_padding (shell, padding_mode, &padding_color);
      break;

    default:
      break;
    }

  ligma_display_shell_expose_full (shell);
}

static void
ligma_display_shell_title_notify_handler (GObject          *config,
                                         GParamSpec       *param_spec,
                                         LigmaDisplayShell *shell)
{
  ligma_display_shell_title_update (shell);
}

static void
ligma_display_shell_nav_size_notify_handler (GObject          *config,
                                            GParamSpec       *param_spec,
                                            LigmaDisplayShell *shell)
{
  g_clear_pointer (&shell->nav_popup, gtk_widget_destroy);
}

static void
ligma_display_shell_monitor_res_notify_handler (GObject          *config,
                                               GParamSpec       *param_spec,
                                               LigmaDisplayShell *shell)
{
  if (LIGMA_DISPLAY_CONFIG (config)->monitor_res_from_gdk)
    {
      ligma_get_monitor_resolution (ligma_widget_get_monitor (GTK_WIDGET (shell)),
                                   &shell->monitor_xres,
                                   &shell->monitor_yres);
    }
  else
    {
      shell->monitor_xres = LIGMA_DISPLAY_CONFIG (config)->monitor_xres;
      shell->monitor_yres = LIGMA_DISPLAY_CONFIG (config)->monitor_yres;
    }

  ligma_display_shell_scale_update (shell);

  if (! shell->dot_for_dot)
    {
      ligma_display_shell_scroll_clamp_and_update (shell);

      ligma_display_shell_scaled (shell);

      ligma_display_shell_expose_full (shell);
      ligma_display_shell_render_invalidate_full (shell);
    }
}

static void
ligma_display_shell_padding_notify_handler (GObject          *config,
                                           GParamSpec       *param_spec,
                                           LigmaDisplayShell *shell)
{
  LigmaDisplayConfig     *display_config;
  LigmaImageWindow       *window;
  gboolean               fullscreen;
  LigmaCanvasPaddingMode  padding_mode;
  LigmaRGB                padding_color;

  display_config = shell->display->config;

  window = ligma_display_shell_get_window (shell);

  if (window)
    fullscreen = ligma_image_window_get_fullscreen (window);
  else
    fullscreen = FALSE;

  /*  if the user did not set the padding mode for this display explicitly  */
  if (! shell->fullscreen_options->padding_mode_set)
    {
      padding_mode  = display_config->default_fullscreen_view->padding_mode;
      padding_color = display_config->default_fullscreen_view->padding_color;

      if (fullscreen)
        {
          ligma_display_shell_set_padding (shell, padding_mode, &padding_color);
        }
      else
        {
          shell->fullscreen_options->padding_mode  = padding_mode;
          shell->fullscreen_options->padding_color = padding_color;
        }
    }

  /*  if the user did not set the padding mode for this display explicitly  */
  if (! shell->options->padding_mode_set)
    {
      padding_mode  = display_config->default_view->padding_mode;
      padding_color = display_config->default_view->padding_color;

      if (fullscreen)
        {
          shell->options->padding_mode  = padding_mode;
          shell->options->padding_color = padding_color;
        }
      else
        {
          ligma_display_shell_set_padding (shell, padding_mode, &padding_color);
        }
    }
}

static void
ligma_display_shell_ants_speed_notify_handler (GObject          *config,
                                              GParamSpec       *param_spec,
                                              LigmaDisplayShell *shell)
{
  ligma_display_shell_selection_pause (shell);
  ligma_display_shell_selection_resume (shell);
}

static void
ligma_display_shell_quality_notify_handler (GObject          *config,
                                           GParamSpec       *param_spec,
                                           LigmaDisplayShell *shell)
{
  ligma_display_shell_expose_full (shell);
  ligma_display_shell_render_invalidate_full (shell);
}

static void
ligma_display_shell_color_config_notify_handler (GObject          *config,
                                                GParamSpec       *param_spec,
                                                LigmaDisplayShell *shell)
{
  if (param_spec)
    {
      gboolean copy = TRUE;

      if (! strcmp (param_spec->name, "mode")                                 ||
          ! strcmp (param_spec->name, "display-rendering-intent")             ||
          ! strcmp (param_spec->name, "display-use-black-point-compensation") ||
          ! strcmp (param_spec->name, "simulation-gamut-check"))
        {
          if (shell->color_config_set)
            copy = FALSE;
        }

      if (copy)
        {
          GValue value = G_VALUE_INIT;

          g_value_init (&value, param_spec->value_type);

          g_object_get_property (config,
                                 param_spec->name, &value);
          g_object_set_property (G_OBJECT (shell->color_config),
                                 param_spec->name, &value);

          g_value_unset (&value);
        }
    }
  else
    {
      ligma_config_copy (LIGMA_CONFIG (config),
                        LIGMA_CONFIG (shell->color_config),
                        0);
      shell->color_config_set = FALSE;
    }
}

static void
ligma_display_shell_display_changed_handler (LigmaContext      *context,
                                            LigmaDisplay      *display,
                                            LigmaDisplayShell *shell)
{
  if (shell->display == display)
    ligma_display_shell_update_priority_rect (shell);
}
