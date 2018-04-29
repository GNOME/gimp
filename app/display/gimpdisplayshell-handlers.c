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

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "display-types.h"

#include "config/gimpdisplayoptions.h"
#include "config/gimpguiconfig.h"

#include "core/gimp.h"
#include "core/gimp-cairo.h"
#include "core/gimpguide.h"
#include "core/gimpimage.h"
#include "core/gimpimage-grid.h"
#include "core/gimpimage-guides.h"
#include "core/gimpimage-quick-mask.h"
#include "core/gimpimage-sample-points.h"
#include "core/gimpitem.h"
#include "core/gimpitemstack.h"
#include "core/gimpsamplepoint.h"
#include "core/gimptreehandler.h"

#include "vectors/gimpvectors.h"

#include "widgets/gimpwidgets-utils.h"

#include "gimpcanvasguide.h"
#include "gimpcanvaslayerboundary.h"
#include "gimpcanvaspath.h"
#include "gimpcanvasproxygroup.h"
#include "gimpcanvassamplepoint.h"
#include "gimpdisplay.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-appearance.h"
#include "gimpdisplayshell-callbacks.h"
#include "gimpdisplayshell-expose.h"
#include "gimpdisplayshell-handlers.h"
#include "gimpdisplayshell-icon.h"
#include "gimpdisplayshell-profile.h"
#include "gimpdisplayshell-rulers.h"
#include "gimpdisplayshell-scale.h"
#include "gimpdisplayshell-scroll.h"
#include "gimpdisplayshell-selection.h"
#include "gimpdisplayshell-title.h"
#include "gimpimagewindow.h"
#include "gimpstatusbar.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   gimp_display_shell_clean_dirty_handler        (GimpImage        *image,
                                                             GimpDirtyMask     dirty_mask,
                                                             GimpDisplayShell *shell);
static void   gimp_display_shell_undo_event_handler         (GimpImage        *image,
                                                             GimpUndoEvent     event,
                                                             GimpUndo         *undo,
                                                             GimpDisplayShell *shell);
static void   gimp_display_shell_grid_notify_handler        (GimpGrid         *grid,
                                                             GParamSpec       *pspec,
                                                             GimpDisplayShell *shell);
static void   gimp_display_shell_name_changed_handler       (GimpImage        *image,
                                                             GimpDisplayShell *shell);
static void   gimp_display_shell_selection_invalidate_handler
                                                            (GimpImage        *image,
                                                             GimpDisplayShell *shell);
static void   gimp_display_shell_size_changed_detailed_handler
                                                            (GimpImage        *image,
                                                             gint              previous_origin_x,
                                                             gint              previous_origin_y,
                                                             gint              previous_width,
                                                             gint              previous_height,
                                                             GimpDisplayShell *shell);
static void   gimp_display_shell_resolution_changed_handler (GimpImage        *image,
                                                             GimpDisplayShell *shell);
static void   gimp_display_shell_quick_mask_changed_handler (GimpImage        *image,
                                                             GimpDisplayShell *shell);
static void   gimp_display_shell_guide_add_handler          (GimpImage        *image,
                                                             GimpGuide        *guide,
                                                             GimpDisplayShell *shell);
static void   gimp_display_shell_guide_remove_handler       (GimpImage        *image,
                                                             GimpGuide        *guide,
                                                             GimpDisplayShell *shell);
static void   gimp_display_shell_guide_move_handler         (GimpImage        *image,
                                                             GimpGuide        *guide,
                                                             GimpDisplayShell *shell);
static void   gimp_display_shell_sample_point_add_handler   (GimpImage        *image,
                                                             GimpSamplePoint  *sample_point,
                                                             GimpDisplayShell *shell);
static void   gimp_display_shell_sample_point_remove_handler(GimpImage        *image,
                                                             GimpSamplePoint  *sample_point,
                                                             GimpDisplayShell *shell);
static void   gimp_display_shell_sample_point_move_handler  (GimpImage        *image,
                                                             GimpSamplePoint  *sample_point,
                                                             GimpDisplayShell *shell);
static void   gimp_display_shell_invalidate_preview_handler (GimpImage        *image,
                                                             GimpDisplayShell *shell);
static void   gimp_display_shell_precision_changed_handler  (GimpImage        *image,
                                                             GimpDisplayShell *shell);
static void   gimp_display_shell_profile_changed_handler    (GimpColorManaged *image,
                                                             GimpDisplayShell *shell);
static void   gimp_display_shell_saved_handler              (GimpImage        *image,
                                                             GFile            *file,
                                                             GimpDisplayShell *shell);
static void   gimp_display_shell_exported_handler           (GimpImage        *image,
                                                             GFile            *file,
                                                             GimpDisplayShell *shell);

static void   gimp_display_shell_active_vectors_handler     (GimpImage        *image,
                                                             GimpDisplayShell *shell);

static void   gimp_display_shell_vectors_freeze_handler     (GimpVectors      *vectors,
                                                             GimpDisplayShell *shell);
static void   gimp_display_shell_vectors_thaw_handler       (GimpVectors      *vectors,
                                                             GimpDisplayShell *shell);
static void   gimp_display_shell_vectors_visible_handler    (GimpVectors      *vectors,
                                                             GimpDisplayShell *shell);
static void   gimp_display_shell_vectors_add_handler        (GimpContainer    *container,
                                                             GimpVectors      *vectors,
                                                             GimpDisplayShell *shell);
static void   gimp_display_shell_vectors_remove_handler     (GimpContainer    *container,
                                                             GimpVectors      *vectors,
                                                             GimpDisplayShell *shell);

static void   gimp_display_shell_check_notify_handler       (GObject          *config,
                                                             GParamSpec       *param_spec,
                                                             GimpDisplayShell *shell);
static void   gimp_display_shell_title_notify_handler       (GObject          *config,
                                                             GParamSpec       *param_spec,
                                                             GimpDisplayShell *shell);
static void   gimp_display_shell_nav_size_notify_handler    (GObject          *config,
                                                             GParamSpec       *param_spec,
                                                             GimpDisplayShell *shell);
static void   gimp_display_shell_monitor_res_notify_handler (GObject          *config,
                                                             GParamSpec       *param_spec,
                                                             GimpDisplayShell *shell);
static void   gimp_display_shell_padding_notify_handler     (GObject          *config,
                                                             GParamSpec       *param_spec,
                                                             GimpDisplayShell *shell);
static void   gimp_display_shell_ants_speed_notify_handler  (GObject          *config,
                                                             GParamSpec       *param_spec,
                                                             GimpDisplayShell *shell);
static void   gimp_display_shell_quality_notify_handler     (GObject          *config,
                                                             GParamSpec       *param_spec,
                                                             GimpDisplayShell *shell);
static void  gimp_display_shell_color_config_notify_handler (GObject          *config,
                                                             GParamSpec       *param_spec,
                                                             GimpDisplayShell *shell);


/*  public functions  */

void
gimp_display_shell_connect (GimpDisplayShell *shell)
{
  GimpImage         *image;
  GimpContainer     *vectors;
  GimpDisplayConfig *config;
  GimpColorConfig   *color_config;
  GList             *list;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (GIMP_IS_DISPLAY (shell->display));

  image = gimp_display_get_image (shell->display);

  g_return_if_fail (GIMP_IS_IMAGE (image));

  vectors = gimp_image_get_vectors (image);

  config       = shell->display->config;
  color_config = GIMP_CORE_CONFIG (config)->color_management;

  g_signal_connect (image, "clean",
                    G_CALLBACK (gimp_display_shell_clean_dirty_handler),
                    shell);
  g_signal_connect (image, "dirty",
                    G_CALLBACK (gimp_display_shell_clean_dirty_handler),
                    shell);
  g_signal_connect (image, "undo-event",
                    G_CALLBACK (gimp_display_shell_undo_event_handler),
                    shell);

  g_signal_connect (gimp_image_get_grid (image), "notify",
                    G_CALLBACK (gimp_display_shell_grid_notify_handler),
                    shell);
  g_object_set (shell->grid, "grid", gimp_image_get_grid (image), NULL);

  g_signal_connect (image, "name-changed",
                    G_CALLBACK (gimp_display_shell_name_changed_handler),
                    shell);
  g_signal_connect (image, "selection-invalidate",
                    G_CALLBACK (gimp_display_shell_selection_invalidate_handler),
                    shell);
  g_signal_connect (image, "size-changed-detailed",
                    G_CALLBACK (gimp_display_shell_size_changed_detailed_handler),
                    shell);
  g_signal_connect (image, "resolution-changed",
                    G_CALLBACK (gimp_display_shell_resolution_changed_handler),
                    shell);
  g_signal_connect (image, "quick-mask-changed",
                    G_CALLBACK (gimp_display_shell_quick_mask_changed_handler),
                    shell);

  g_signal_connect (image, "guide-added",
                    G_CALLBACK (gimp_display_shell_guide_add_handler),
                    shell);
  g_signal_connect (image, "guide-removed",
                    G_CALLBACK (gimp_display_shell_guide_remove_handler),
                    shell);
  g_signal_connect (image, "guide-moved",
                    G_CALLBACK (gimp_display_shell_guide_move_handler),
                    shell);
  for (list = gimp_image_get_guides (image);
       list;
       list = g_list_next (list))
    {
      gimp_display_shell_guide_add_handler (image, list->data, shell);
    }

  g_signal_connect (image, "sample-point-added",
                    G_CALLBACK (gimp_display_shell_sample_point_add_handler),
                    shell);
  g_signal_connect (image, "sample-point-removed",
                    G_CALLBACK (gimp_display_shell_sample_point_remove_handler),
                    shell);
  g_signal_connect (image, "sample-point-moved",
                    G_CALLBACK (gimp_display_shell_sample_point_move_handler),
                    shell);
  for (list = gimp_image_get_sample_points (image);
       list;
       list = g_list_next (list))
    {
      gimp_display_shell_sample_point_add_handler (image, list->data, shell);
    }

  g_signal_connect (image, "invalidate-preview",
                    G_CALLBACK (gimp_display_shell_invalidate_preview_handler),
                    shell);
  g_signal_connect (image, "precision-changed",
                    G_CALLBACK (gimp_display_shell_precision_changed_handler),
                    shell);
  g_signal_connect (image, "profile-changed",
                    G_CALLBACK (gimp_display_shell_profile_changed_handler),
                    shell);
  g_signal_connect (image, "precision-changed",
                    G_CALLBACK (gimp_display_shell_precision_changed_handler),
                    shell);
  g_signal_connect (image, "saved",
                    G_CALLBACK (gimp_display_shell_saved_handler),
                    shell);
  g_signal_connect (image, "exported",
                    G_CALLBACK (gimp_display_shell_exported_handler),
                    shell);

  g_signal_connect (image, "active-vectors-changed",
                    G_CALLBACK (gimp_display_shell_active_vectors_handler),
                    shell);

  shell->vectors_freeze_handler =
    gimp_tree_handler_connect (vectors, "freeze",
                               G_CALLBACK (gimp_display_shell_vectors_freeze_handler),
                               shell);
  shell->vectors_thaw_handler =
    gimp_tree_handler_connect (vectors, "thaw",
                               G_CALLBACK (gimp_display_shell_vectors_thaw_handler),
                               shell);
  shell->vectors_visible_handler =
    gimp_tree_handler_connect (vectors, "visibility-changed",
                               G_CALLBACK (gimp_display_shell_vectors_visible_handler),
                               shell);

  g_signal_connect (vectors, "add",
                    G_CALLBACK (gimp_display_shell_vectors_add_handler),
                    shell);
  g_signal_connect (vectors, "remove",
                    G_CALLBACK (gimp_display_shell_vectors_remove_handler),
                    shell);
  for (list = gimp_item_stack_get_item_iter (GIMP_ITEM_STACK (vectors));
       list;
       list = g_list_next (list))
    {
      gimp_display_shell_vectors_add_handler (vectors, list->data, shell);
    }

  g_signal_connect (config,
                    "notify::transparency-size",
                    G_CALLBACK (gimp_display_shell_check_notify_handler),
                    shell);
  g_signal_connect (config,
                    "notify::transparency-type",
                    G_CALLBACK (gimp_display_shell_check_notify_handler),
                    shell);

  g_signal_connect (config,
                    "notify::image-title-format",
                    G_CALLBACK (gimp_display_shell_title_notify_handler),
                    shell);
  g_signal_connect (config,
                    "notify::image-status-format",
                    G_CALLBACK (gimp_display_shell_title_notify_handler),
                    shell);
  g_signal_connect (config,
                    "notify::navigation-preview-size",
                    G_CALLBACK (gimp_display_shell_nav_size_notify_handler),
                    shell);
  g_signal_connect (config,
                    "notify::monitor-resolution-from-windowing-system",
                    G_CALLBACK (gimp_display_shell_monitor_res_notify_handler),
                    shell);
  g_signal_connect (config,
                    "notify::monitor-xresolution",
                    G_CALLBACK (gimp_display_shell_monitor_res_notify_handler),
                    shell);
  g_signal_connect (config,
                    "notify::monitor-yresolution",
                    G_CALLBACK (gimp_display_shell_monitor_res_notify_handler),
                    shell);

  g_signal_connect (config->default_view,
                    "notify::padding-mode",
                    G_CALLBACK (gimp_display_shell_padding_notify_handler),
                    shell);
  g_signal_connect (config->default_view,
                    "notify::padding-color",
                    G_CALLBACK (gimp_display_shell_padding_notify_handler),
                    shell);
  g_signal_connect (config->default_fullscreen_view,
                    "notify::padding-mode",
                    G_CALLBACK (gimp_display_shell_padding_notify_handler),
                    shell);
  g_signal_connect (config->default_fullscreen_view,
                    "notify::padding-color",
                    G_CALLBACK (gimp_display_shell_padding_notify_handler),
                    shell);

  g_signal_connect (config,
                    "notify::marching-ants-speed",
                    G_CALLBACK (gimp_display_shell_ants_speed_notify_handler),
                    shell);

  g_signal_connect (config,
                    "notify::zoom-quality",
                    G_CALLBACK (gimp_display_shell_quality_notify_handler),
                    shell);

  g_signal_connect (color_config, "notify",
                    G_CALLBACK (gimp_display_shell_color_config_notify_handler),
                    shell);

  gimp_display_shell_active_vectors_handler     (image, shell);
  gimp_display_shell_invalidate_preview_handler (image, shell);
  gimp_display_shell_quick_mask_changed_handler (image, shell);
  gimp_display_shell_profile_changed_handler    (GIMP_COLOR_MANAGED (image),
                                                 shell);
  gimp_display_shell_color_config_notify_handler (G_OBJECT (color_config),
                                                  NULL, /* sync all */
                                                  shell);

  gimp_canvas_layer_boundary_set_layer (GIMP_CANVAS_LAYER_BOUNDARY (shell->layer_boundary),
                                        gimp_image_get_active_layer (image));
}

void
gimp_display_shell_disconnect (GimpDisplayShell *shell)
{
  GimpImage         *image;
  GimpContainer     *vectors;
  GimpDisplayConfig *config;
  GimpColorConfig   *color_config;
  GList             *list;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (GIMP_IS_DISPLAY (shell->display));

  image = gimp_display_get_image (shell->display);

  g_return_if_fail (GIMP_IS_IMAGE (image));

  vectors = gimp_image_get_vectors (image);

  config       = shell->display->config;
  color_config = GIMP_CORE_CONFIG (config)->color_management;

  gimp_display_shell_icon_update_stop (shell);

  gimp_canvas_layer_boundary_set_layer (GIMP_CANVAS_LAYER_BOUNDARY (shell->layer_boundary),
                                        NULL);

  g_signal_handlers_disconnect_by_func (color_config,
                                        gimp_display_shell_color_config_notify_handler,
                                        shell);
  shell->color_config_set = FALSE;

  g_signal_handlers_disconnect_by_func (config,
                                        gimp_display_shell_quality_notify_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (config,
                                        gimp_display_shell_ants_speed_notify_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (config->default_fullscreen_view,
                                        gimp_display_shell_padding_notify_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (config->default_view,
                                        gimp_display_shell_padding_notify_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (config,
                                        gimp_display_shell_monitor_res_notify_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (config,
                                        gimp_display_shell_nav_size_notify_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (config,
                                        gimp_display_shell_title_notify_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (config,
                                        gimp_display_shell_check_notify_handler,
                                        shell);

  g_signal_handlers_disconnect_by_func (vectors,
                                        gimp_display_shell_vectors_remove_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (vectors,
                                        gimp_display_shell_vectors_add_handler,
                                        shell);

  gimp_tree_handler_disconnect (shell->vectors_visible_handler);
  shell->vectors_visible_handler = NULL;

  gimp_tree_handler_disconnect (shell->vectors_thaw_handler);
  shell->vectors_thaw_handler = NULL;

  gimp_tree_handler_disconnect (shell->vectors_freeze_handler);
  shell->vectors_freeze_handler = NULL;

  g_signal_handlers_disconnect_by_func (image,
                                        gimp_display_shell_active_vectors_handler,
                                        shell);

  for (list = gimp_item_stack_get_item_iter (GIMP_ITEM_STACK (vectors));
       list;
       list = g_list_next (list))
    {
      gimp_canvas_proxy_group_remove_item (GIMP_CANVAS_PROXY_GROUP (shell->vectors),
                                           list->data);
    }

  g_signal_handlers_disconnect_by_func (image,
                                        gimp_display_shell_exported_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (image,
                                        gimp_display_shell_saved_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (image,
                                        gimp_display_shell_profile_changed_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (image,
                                        gimp_display_shell_precision_changed_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (image,
                                        gimp_display_shell_invalidate_preview_handler,
                                        shell);

  g_signal_handlers_disconnect_by_func (image,
                                        gimp_display_shell_guide_add_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (image,
                                        gimp_display_shell_guide_remove_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (image,
                                        gimp_display_shell_guide_move_handler,
                                        shell);
  for (list = gimp_image_get_guides (image);
       list;
       list = g_list_next (list))
    {
      gimp_canvas_proxy_group_remove_item (GIMP_CANVAS_PROXY_GROUP (shell->guides),
                                           list->data);
    }

  g_signal_handlers_disconnect_by_func (image,
                                        gimp_display_shell_sample_point_add_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (image,
                                        gimp_display_shell_sample_point_remove_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (image,
                                        gimp_display_shell_sample_point_move_handler,
                                        shell);
  for (list = gimp_image_get_sample_points (image);
       list;
       list = g_list_next (list))
    {
      gimp_canvas_proxy_group_remove_item (GIMP_CANVAS_PROXY_GROUP (shell->sample_points),
                                           list->data);
    }

  g_signal_handlers_disconnect_by_func (image,
                                        gimp_display_shell_quick_mask_changed_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (image,
                                        gimp_display_shell_resolution_changed_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (image,
                                        gimp_display_shell_size_changed_detailed_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (image,
                                        gimp_display_shell_selection_invalidate_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (image,
                                        gimp_display_shell_name_changed_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (gimp_image_get_grid (image),
                                        gimp_display_shell_grid_notify_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (image,
                                        gimp_display_shell_undo_event_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (image,
                                        gimp_display_shell_clean_dirty_handler,
                                        shell);
}


/*  private functions  */

static void
gimp_display_shell_clean_dirty_handler (GimpImage        *image,
                                        GimpDirtyMask     dirty_mask,
                                        GimpDisplayShell *shell)
{
  gimp_display_shell_title_update (shell);
}

static void
gimp_display_shell_undo_event_handler (GimpImage        *image,
                                       GimpUndoEvent     event,
                                       GimpUndo         *undo,
                                       GimpDisplayShell *shell)
{
  gimp_display_shell_title_update (shell);
}

static void
gimp_display_shell_grid_notify_handler (GimpGrid         *grid,
                                        GParamSpec       *pspec,
                                        GimpDisplayShell *shell)
{
  g_object_set (shell->grid, "grid", grid, NULL);
}

static void
gimp_display_shell_name_changed_handler (GimpImage        *image,
                                         GimpDisplayShell *shell)
{
  gimp_display_shell_title_update (shell);
}

static void
gimp_display_shell_selection_invalidate_handler (GimpImage        *image,
                                                 GimpDisplayShell *shell)
{
  gimp_display_shell_selection_undraw (shell);
}

static void
gimp_display_shell_resolution_changed_handler (GimpImage        *image,
                                               GimpDisplayShell *shell)
{
  gimp_display_shell_scale_update (shell);

  if (shell->dot_for_dot)
    {
      if (shell->unit != GIMP_UNIT_PIXEL)
        {
          gimp_display_shell_rulers_update (shell);
        }

      gimp_display_shell_scaled (shell);
    }
  else
    {
      /* A resolution change has the same effect as a size change from
       * a display shell point of view. Force a redraw of the display
       * so that we don't get any display garbage.
       */

      GimpDisplayConfig *config = shell->display->config;
      gboolean           resize_window;

      /* Resize windows only in multi-window mode */
      resize_window = (config->resize_windows_on_resize &&
                       ! GIMP_GUI_CONFIG (config)->single_window_mode);

      gimp_display_shell_scale_resize (shell, resize_window, FALSE);
    }
}

static void
gimp_display_shell_quick_mask_changed_handler (GimpImage        *image,
                                               GimpDisplayShell *shell)
{
  GtkImage *gtk_image;
  gboolean  quick_mask_state;

  gtk_image = GTK_IMAGE (gtk_bin_get_child (GTK_BIN (shell->quick_mask_button)));

  g_signal_handlers_block_by_func (shell->quick_mask_button,
                                   gimp_display_shell_quick_mask_toggled,
                                   shell);

  quick_mask_state = gimp_image_get_quick_mask_state (image);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (shell->quick_mask_button),
                                quick_mask_state);

  if (quick_mask_state)
    gtk_image_set_from_icon_name (gtk_image, GIMP_ICON_QUICK_MASK_ON,
                                  GTK_ICON_SIZE_MENU);
  else
    gtk_image_set_from_icon_name (gtk_image, GIMP_ICON_QUICK_MASK_OFF,
                                  GTK_ICON_SIZE_MENU);

  g_signal_handlers_unblock_by_func (shell->quick_mask_button,
                                     gimp_display_shell_quick_mask_toggled,
                                     shell);
}

static void
gimp_display_shell_guide_add_handler (GimpImage        *image,
                                      GimpGuide        *guide,
                                      GimpDisplayShell *shell)
{
  GimpCanvasProxyGroup *group = GIMP_CANVAS_PROXY_GROUP (shell->guides);
  GimpCanvasItem       *item;
  GimpGuideStyle        style;

  style = gimp_guide_get_style (guide);
  item = gimp_canvas_guide_new (shell,
                                gimp_guide_get_orientation (guide),
                                gimp_guide_get_position (guide),
                                style);

  gimp_canvas_proxy_group_add_item (group, guide, item);
  g_object_unref (item);
}

static void
gimp_display_shell_guide_remove_handler (GimpImage        *image,
                                         GimpGuide        *guide,
                                         GimpDisplayShell *shell)
{
  GimpCanvasProxyGroup *group = GIMP_CANVAS_PROXY_GROUP (shell->guides);

  gimp_canvas_proxy_group_remove_item (group, guide);
}

static void
gimp_display_shell_guide_move_handler (GimpImage        *image,
                                       GimpGuide        *guide,
                                       GimpDisplayShell *shell)
{
  GimpCanvasProxyGroup *group = GIMP_CANVAS_PROXY_GROUP (shell->guides);
  GimpCanvasItem       *item;

  item = gimp_canvas_proxy_group_get_item (group, guide);

  gimp_canvas_guide_set (item,
                         gimp_guide_get_orientation (guide),
                         gimp_guide_get_position (guide));
}

static void
gimp_display_shell_sample_point_add_handler (GimpImage        *image,
                                             GimpSamplePoint  *sample_point,
                                             GimpDisplayShell *shell)
{
  GimpCanvasProxyGroup *group = GIMP_CANVAS_PROXY_GROUP (shell->sample_points);
  GimpCanvasItem       *item;
  GList                *list;
  gint                  x;
  gint                  y;
  gint                  i;

  gimp_sample_point_get_position (sample_point, &x, &y);

  item = gimp_canvas_sample_point_new (shell, x, y, 0, TRUE);

  gimp_canvas_proxy_group_add_item (group, sample_point, item);
  g_object_unref (item);

  for (list = gimp_image_get_sample_points (image), i = 1;
       list;
       list = g_list_next (list), i++)
    {
      GimpSamplePoint *sample_point = list->data;

      item = gimp_canvas_proxy_group_get_item (group, sample_point);

      if (item)
        g_object_set (item,
                      "index", i,
                      NULL);
    }
}

static void
gimp_display_shell_sample_point_remove_handler (GimpImage        *image,
                                                GimpSamplePoint  *sample_point,
                                                GimpDisplayShell *shell)
{
  GimpCanvasProxyGroup *group = GIMP_CANVAS_PROXY_GROUP (shell->sample_points);
  GList                *list;
  gint                  i;

  gimp_canvas_proxy_group_remove_item (group, sample_point);

  for (list = gimp_image_get_sample_points (image), i = 1;
       list;
       list = g_list_next (list), i++)
    {
      GimpSamplePoint *sample_point = list->data;
      GimpCanvasItem  *item;

      item = gimp_canvas_proxy_group_get_item (group, sample_point);

      if (item)
        g_object_set (item,
                      "index", i,
                      NULL);
    }
}

static void
gimp_display_shell_sample_point_move_handler (GimpImage        *image,
                                              GimpSamplePoint  *sample_point,
                                              GimpDisplayShell *shell)
{
  GimpCanvasProxyGroup *group = GIMP_CANVAS_PROXY_GROUP (shell->sample_points);
  GimpCanvasItem       *item;
  gint                  x;
  gint                  y;

  item = gimp_canvas_proxy_group_get_item (group, sample_point);

  gimp_sample_point_get_position (sample_point, &x, &y);

  gimp_canvas_sample_point_set (item, x, y);
}

static void
gimp_display_shell_size_changed_detailed_handler (GimpImage        *image,
                                                  gint              previous_origin_x,
                                                  gint              previous_origin_y,
                                                  gint              previous_width,
                                                  gint              previous_height,
                                                  GimpDisplayShell *shell)
{
  GimpDisplayConfig *config = shell->display->config;
  gboolean           resize_window;

  /* Resize windows only in multi-window mode */
  resize_window = (config->resize_windows_on_resize &&
                   ! GIMP_GUI_CONFIG (config)->single_window_mode);

  if (resize_window)
    {
      GimpImageWindow *window = gimp_display_shell_get_window (shell);

      if (window && gimp_image_window_get_active_shell (window) == shell)
        {
          /* If the window is resized just center the image in it when it
           * has change size
           */
          gimp_image_window_shrink_wrap (window, FALSE);
        }
    }
  else
    {
      GimpImage *image      = gimp_display_get_image (shell->display);
      gint       new_width  = gimp_image_get_width  (image);
      gint       new_height = gimp_image_get_height (image);
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

      gimp_display_shell_scroll_set_offset (shell,
                                            shell->offset_x + scaled_previous_origin_x,
                                            shell->offset_y + scaled_previous_origin_y);

      gimp_display_shell_scroll_center_image (shell, horizontally, vertically);

      /* The above calls might not lead to a call to
       * gimp_display_shell_scroll_clamp_and_update() and
       * gimp_display_shell_expose_full() in all cases because when
       * scaling the old and new scroll offset might be the same.
       *
       * We need them to be called in all cases, so simply call them
       * explicitly here at the end
       */
      gimp_display_shell_scroll_clamp_and_update (shell);

      gimp_display_shell_expose_full (shell);
    }
}

static void
gimp_display_shell_invalidate_preview_handler (GimpImage        *image,
                                               GimpDisplayShell *shell)
{
  gimp_display_shell_icon_update (shell);
}

static void
gimp_display_shell_precision_changed_handler (GimpImage        *image,
                                              GimpDisplayShell *shell)
{
  gimp_display_shell_profile_update (shell);
}

static void
gimp_display_shell_profile_changed_handler (GimpColorManaged *image,
                                            GimpDisplayShell *shell)
{
  gimp_color_managed_profile_changed (GIMP_COLOR_MANAGED (shell));
}

static void
gimp_display_shell_saved_handler (GimpImage        *image,
                                  GFile            *file,
                                  GimpDisplayShell *shell)
{
  GimpStatusbar *statusbar = gimp_display_shell_get_statusbar (shell);

  gimp_statusbar_push_temp (statusbar, GIMP_MESSAGE_INFO,
                            GIMP_ICON_DOCUMENT_SAVE,
                            _("Image saved to '%s'"),
                            gimp_file_get_utf8_name (file));
}

static void
gimp_display_shell_exported_handler (GimpImage        *image,
                                     GFile            *file,
                                     GimpDisplayShell *shell)
{
  GimpStatusbar *statusbar = gimp_display_shell_get_statusbar (shell);

  gimp_statusbar_push_temp (statusbar, GIMP_MESSAGE_INFO,
                            GIMP_ICON_DOCUMENT_SAVE,
                            _("Image exported to '%s'"),
                            gimp_file_get_utf8_name (file));
}

static void
gimp_display_shell_active_vectors_handler (GimpImage        *image,
                                           GimpDisplayShell *shell)
{
  GimpCanvasProxyGroup *group  = GIMP_CANVAS_PROXY_GROUP (shell->vectors);
  GimpVectors          *active = gimp_image_get_active_vectors (image);
  GList                *list;

  for (list = gimp_image_get_vectors_iter (image);
       list;
       list = g_list_next (list))
    {
      GimpVectors    *vectors = list->data;
      GimpCanvasItem *item;

      item = gimp_canvas_proxy_group_get_item (group, vectors);

      gimp_canvas_item_set_highlight (item, vectors == active);
    }
}

static void
gimp_display_shell_vectors_freeze_handler (GimpVectors      *vectors,
                                           GimpDisplayShell *shell)
{
  /* do nothing */
}

static void
gimp_display_shell_vectors_thaw_handler (GimpVectors      *vectors,
                                         GimpDisplayShell *shell)
{
  GimpCanvasProxyGroup *group = GIMP_CANVAS_PROXY_GROUP (shell->vectors);
  GimpCanvasItem       *item;

  item = gimp_canvas_proxy_group_get_item (group, vectors);

  gimp_canvas_path_set (item, gimp_vectors_get_bezier (vectors));
}

static void
gimp_display_shell_vectors_visible_handler (GimpVectors      *vectors,
                                            GimpDisplayShell *shell)
{
  GimpCanvasProxyGroup *group = GIMP_CANVAS_PROXY_GROUP (shell->vectors);
  GimpCanvasItem       *item;

  item = gimp_canvas_proxy_group_get_item (group, vectors);

  gimp_canvas_item_set_visible (item,
                                gimp_item_get_visible (GIMP_ITEM (vectors)));
}

static void
gimp_display_shell_vectors_add_handler (GimpContainer    *container,
                                        GimpVectors      *vectors,
                                        GimpDisplayShell *shell)
{
  GimpCanvasProxyGroup *group = GIMP_CANVAS_PROXY_GROUP (shell->vectors);
  GimpCanvasItem       *item;

  item = gimp_canvas_path_new (shell,
                               gimp_vectors_get_bezier (vectors),
                               0, 0,
                               FALSE,
                               GIMP_PATH_STYLE_VECTORS);
  gimp_canvas_item_set_visible (item,
                                gimp_item_get_visible (GIMP_ITEM (vectors)));

  gimp_canvas_proxy_group_add_item (group, vectors, item);
  g_object_unref (item);
}

static void
gimp_display_shell_vectors_remove_handler (GimpContainer    *container,
                                           GimpVectors      *vectors,
                                           GimpDisplayShell *shell)
{
  GimpCanvasProxyGroup *group = GIMP_CANVAS_PROXY_GROUP (shell->vectors);

  gimp_canvas_proxy_group_remove_item (group, vectors);
}

static void
gimp_display_shell_check_notify_handler (GObject          *config,
                                         GParamSpec       *param_spec,
                                         GimpDisplayShell *shell)
{
  GimpCanvasPaddingMode padding_mode;
  GimpRGB               padding_color;

  g_clear_pointer (&shell->checkerboard, cairo_pattern_destroy);

  gimp_display_shell_get_padding (shell, &padding_mode, &padding_color);

  switch (padding_mode)
    {
    case GIMP_CANVAS_PADDING_MODE_LIGHT_CHECK:
    case GIMP_CANVAS_PADDING_MODE_DARK_CHECK:
      gimp_display_shell_set_padding (shell, padding_mode, &padding_color);
      break;

    default:
      break;
    }

  gimp_display_shell_expose_full (shell);
}

static void
gimp_display_shell_title_notify_handler (GObject          *config,
                                         GParamSpec       *param_spec,
                                         GimpDisplayShell *shell)
{
  gimp_display_shell_title_update (shell);
}

static void
gimp_display_shell_nav_size_notify_handler (GObject          *config,
                                            GParamSpec       *param_spec,
                                            GimpDisplayShell *shell)
{
  g_clear_pointer (&shell->nav_popup, gtk_widget_destroy);
}

static void
gimp_display_shell_monitor_res_notify_handler (GObject          *config,
                                               GParamSpec       *param_spec,
                                               GimpDisplayShell *shell)
{
  if (GIMP_DISPLAY_CONFIG (config)->monitor_res_from_gdk)
    {
      gimp_get_monitor_resolution (gimp_widget_get_monitor (GTK_WIDGET (shell)),
                                   &shell->monitor_xres,
                                   &shell->monitor_yres);
    }
  else
    {
      shell->monitor_xres = GIMP_DISPLAY_CONFIG (config)->monitor_xres;
      shell->monitor_yres = GIMP_DISPLAY_CONFIG (config)->monitor_yres;
    }

  gimp_display_shell_scale_update (shell);

  if (! shell->dot_for_dot)
    {
      gimp_display_shell_scroll_clamp_and_update (shell);

      gimp_display_shell_scaled (shell);

      gimp_display_shell_expose_full (shell);
    }
}

static void
gimp_display_shell_padding_notify_handler (GObject          *config,
                                           GParamSpec       *param_spec,
                                           GimpDisplayShell *shell)
{
  GimpDisplayConfig     *display_config;
  GimpImageWindow       *window;
  gboolean               fullscreen;
  GimpCanvasPaddingMode  padding_mode;
  GimpRGB                padding_color;

  display_config = shell->display->config;

  window = gimp_display_shell_get_window (shell);

  if (window)
    fullscreen = gimp_image_window_get_fullscreen (window);
  else
    fullscreen = FALSE;

  /*  if the user did not set the padding mode for this display explicitly  */
  if (! shell->fullscreen_options->padding_mode_set)
    {
      padding_mode  = display_config->default_fullscreen_view->padding_mode;
      padding_color = display_config->default_fullscreen_view->padding_color;

      if (fullscreen)
        {
          gimp_display_shell_set_padding (shell, padding_mode, &padding_color);
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
          gimp_display_shell_set_padding (shell, padding_mode, &padding_color);
        }
    }
}

static void
gimp_display_shell_ants_speed_notify_handler (GObject          *config,
                                              GParamSpec       *param_spec,
                                              GimpDisplayShell *shell)
{
  gimp_display_shell_selection_pause (shell);
  gimp_display_shell_selection_resume (shell);
}

static void
gimp_display_shell_quality_notify_handler (GObject          *config,
                                           GParamSpec       *param_spec,
                                           GimpDisplayShell *shell)
{
  gimp_display_shell_expose_full (shell);
}

static void
gimp_display_shell_color_config_notify_handler (GObject          *config,
                                                GParamSpec       *param_spec,
                                                GimpDisplayShell *shell)
{
  if (param_spec)
    {
      gboolean copy = TRUE;

      if (! strcmp (param_spec->name, "mode")                                 ||
          ! strcmp (param_spec->name, "display-rendering-intent")             ||
          ! strcmp (param_spec->name, "display-use-black-point-compensation") ||
          ! strcmp (param_spec->name, "printer-profile")                      ||
          ! strcmp (param_spec->name, "simulation-rendering-intent")          ||
          ! strcmp (param_spec->name, "simulation-use-black-point-compensation") ||
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
      gimp_config_copy (GIMP_CONFIG (config),
                        GIMP_CONFIG (shell->color_config),
                        0);
      shell->color_config_set = FALSE;
    }
}
