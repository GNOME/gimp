/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 2009 Martin Nordholts <martinn@src.gnome.org>
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

#include <stdlib.h>
#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libligmabase/ligmabase.h"
#include "libligmamath/ligmamath.h"
#include "libligmawidgets/ligmawidgets.h"

#include "dialogs/dialogs-types.h"

#include "display/ligmadisplay.h"
#include "display/ligmadisplayshell.h"
#include "display/ligmadisplayshell-scale.h"
#include "display/ligmadisplayshell-transform.h"
#include "display/ligmaimagewindow.h"

#include "widgets/ligmadialogfactory.h"
#include "widgets/ligmadock.h"
#include "widgets/ligmadockable.h"
#include "widgets/ligmadockbook.h"
#include "widgets/ligmadockcontainer.h"
#include "widgets/ligmadocked.h"
#include "widgets/ligmadockwindow.h"
#include "widgets/ligmahelp-ids.h"
#include "widgets/ligmasessioninfo.h"
#include "widgets/ligmasessioninfo-aux.h"
#include "widgets/ligmasessionmanaged.h"
#include "widgets/ligmatoolbox.h"
#include "widgets/ligmatooloptionseditor.h"
#include "widgets/ligmauimanager.h"
#include "widgets/ligmawidgets-utils.h"

#include "core/ligma.h"
#include "core/ligmachannel.h"
#include "core/ligmacontext.h"
#include "core/ligmaimage.h"
#include "core/ligmalayer.h"
#include "core/ligmalayer-new.h"
#include "core/ligmatoolinfo.h"
#include "core/ligmatooloptions.h"

#include "tests.h"

#include "ligma-app-test-utils.h"


#define LIGMA_UI_WINDOW_POSITION_EPSILON 30
#define LIGMA_UI_POSITION_EPSILON        1
#define LIGMA_UI_ZOOM_EPSILON            0.01

#define ADD_TEST(function) \
  g_test_add_data_func ("/ligma-ui/" #function, ligma, function);


/* Put this in the code below when you want the test to pause so you
 * can do measurements of widgets on the screen for example
 */
#define LIGMA_PAUSE (g_usleep (20 * 1000 * 1000))


typedef gboolean (*LigmaUiTestFunc) (GObject *object);


static void            ligma_ui_synthesize_delete_event          (GtkWidget         *widget);
static gboolean        ligma_ui_synthesize_click                 (GtkWidget         *widget,
                                                                 gint               x,
                                                                 gint               y,
                                                                 gint               button,
                                                                 GdkModifierType    modifiers);
static GtkWidget     * ligma_ui_find_window                      (LigmaDialogFactory *dialog_factory,
                                                                 LigmaUiTestFunc     predicate);
static gboolean        ligma_ui_not_toolbox_window               (GObject           *object);
static gboolean        ligma_ui_multicolumn_not_toolbox_window   (GObject           *object);
static gboolean        ligma_ui_is_ligma_layer_list               (GObject           *object);
static int             ligma_ui_aux_data_eqiuvalent              (gconstpointer      _a,
                                                                 gconstpointer      _b);
static void            ligma_ui_switch_window_mode               (Ligma              *ligma);


/**
 * tool_options_editor_updates:
 * @data:
 *
 * Makes sure that the tool options editor is updated when the tool
 * changes.
 **/
static void
tool_options_editor_updates (gconstpointer data)
{
  Ligma                  *ligma         = LIGMA (data);
  LigmaDisplay           *display      = LIGMA_DISPLAY (ligma_get_empty_display (ligma));
  LigmaDisplayShell      *shell        = ligma_display_get_shell (display);
  GtkWidget             *toplevel     = gtk_widget_get_toplevel (GTK_WIDGET (shell));
  LigmaImageWindow       *image_window = LIGMA_IMAGE_WINDOW (toplevel);
  LigmaUIManager         *ui_manager   = ligma_image_window_get_ui_manager (image_window);
  GtkWidget             *dockable     = ligma_dialog_factory_dialog_new (ligma_dialog_factory_get_singleton (),
                                                                        ligma_widget_get_monitor (toplevel),
                                                                        NULL /*ui_manager*/,
                                                                        toplevel,
                                                                        "ligma-tool-options",
                                                                        -1 /*view_size*/,
                                                                        FALSE /*present*/);
  LigmaToolOptionsEditor *editor       = LIGMA_TOOL_OPTIONS_EDITOR (gtk_bin_get_child (GTK_BIN (dockable)));

  /* First select the rect select tool */
  ligma_ui_manager_activate_action (ui_manager,
                                   "tools",
                                   "tools-rect-select");
  g_assert_cmpstr (LIGMA_HELP_TOOL_RECT_SELECT,
                   ==,
                   ligma_tool_options_editor_get_tool_options (editor)->
                   tool_info->help_id);

  /* Change tool and make sure the change is taken into account by the
   * tool options editor
   */
  ligma_ui_manager_activate_action (ui_manager,
                                   "tools",
                                   "tools-ellipse-select");
  g_assert_cmpstr (LIGMA_HELP_TOOL_ELLIPSE_SELECT,
                   ==,
                   ligma_tool_options_editor_get_tool_options (editor)->
                   tool_info->help_id);
}

static void
create_new_image_via_dialog (gconstpointer data)
{
  Ligma      *ligma = LIGMA (data);
  LigmaImage *image;
  LigmaLayer *layer;

  image = ligma_test_utils_create_image_from_dialog (ligma);

  /* Add a layer to the image to make it more useful in later tests */
  layer = ligma_layer_new (image,
                          ligma_image_get_width (image),
                          ligma_image_get_height (image),
                          ligma_image_get_layer_format (image, TRUE),
                          "Layer for testing",
                          LIGMA_OPACITY_OPAQUE,
                          LIGMA_LAYER_MODE_NORMAL);

  ligma_image_add_layer (image, layer,
                        LIGMA_IMAGE_ACTIVE_PARENT, -1, TRUE);
  ligma_test_run_mainloop_until_idle ();
}

static void
keyboard_zoom_focus (gconstpointer data)
{
  Ligma              *ligma    = LIGMA (data);
  LigmaDisplay       *display = LIGMA_DISPLAY (ligma_get_display_iter (ligma)->data);
  LigmaDisplayShell  *shell   = ligma_display_get_shell (display);
  LigmaImageWindow   *window  = ligma_display_shell_get_window (shell);
  gint               image_x;
  gint               image_y;
  gint               shell_x_before_zoom;
  gint               shell_y_before_zoom;
  gdouble            factor_before_zoom;
  gint               shell_x_after_zoom;
  gint               shell_y_after_zoom;
  gdouble            factor_after_zoom;

  /* We need to use a point that is within the visible (exposed) part
   * of the canvas
   */
  image_x = 400;
  image_y = 50;

  /* Setup zoom focus on the bottom right part of the image. We avoid
   * 0,0 because that's essentially a particularly easy special case.
   */
  ligma_display_shell_transform_xy (shell,
                                   image_x,
                                   image_y,
                                   &shell_x_before_zoom,
                                   &shell_y_before_zoom);
  ligma_display_shell_push_zoom_focus_pointer_pos (shell,
                                                  shell_x_before_zoom,
                                                  shell_y_before_zoom);
  factor_before_zoom = ligma_zoom_model_get_factor (shell->zoom);

  /* Do the zoom */
  ligma_test_utils_synthesize_key_event (GTK_WIDGET (window), GDK_KEY_plus);
  ligma_test_run_mainloop_until_idle ();

  /* Make sure the zoom focus point remained fixed */
  ligma_display_shell_transform_xy (shell,
                                   image_x,
                                   image_y,
                                   &shell_x_after_zoom,
                                   &shell_y_after_zoom);
  factor_after_zoom = ligma_zoom_model_get_factor (shell->zoom);

  /* First of all make sure a zoom happened at all. If this assert
   * fails, it means that the zoom didn't happen. Possible causes:
   *
   *  * gdk_test_simulate_key() failed to map 'GDK_KEY_plus' to the proper
   *    'plus' X keysym, probably because it is mapped to a keycode
   *    with modifiers like 'shift'. Run "xmodmap -pk | grep plus" to
   *    find out. Make sure 'plus' is the first keysym for the given
   *    keycode. If not, use "xmodmap <keycode> = plus" to correct it.
   */
  g_assert_cmpfloat (fabs (factor_before_zoom - factor_after_zoom),
                     >=,
                     LIGMA_UI_ZOOM_EPSILON);

#ifdef __GNUC__
#warning disabled zoom test, it fails randomly, no clue how to fix it
#endif
#if 0
  g_assert_cmpint (ABS (shell_x_after_zoom - shell_x_before_zoom),
                   <=,
                   LIGMA_UI_POSITION_EPSILON);
  g_assert_cmpint (ABS (shell_y_after_zoom - shell_y_before_zoom),
                   <=,
                   LIGMA_UI_POSITION_EPSILON);
#endif
}

/**
 * alt_click_is_layer_to_selection:
 * @data:
 *
 * Makes sure that we can alt-click on a layer to do
 * layer-to-selection. Also makes sure that the layer clicked on is
 * not set as the active layer.
 **/
static void
alt_click_is_layer_to_selection (gconstpointer data)
{
#if __GNUC__
#warning FIXME: please fix alt_click_is_layer_to_selection test
#endif
#if 0
  Ligma        *ligma      = LIGMA (data);
  LigmaImage   *image     = LIGMA_IMAGE (ligma_get_image_iter (ligma)->data);
  LigmaChannel *selection = ligma_image_get_mask (image);
  GList       *selected_layers;
  GList       *iter;
  GtkWidget   *dockable;
  GtkWidget   *gtk_tree_view;
  gint         assumed_layer_x;
  gint         assumed_empty_layer_y;
  gint         assumed_background_layer_y;

  /* Hardcode assumptions of where the layers are in the
   * GtkTreeView. Doesn't feel worth adding proper API for this. One
   * can just use LIGMA_PAUSE and re-measure new coordinates if we
   * start to layout layers in the GtkTreeView differently
   */
  assumed_layer_x            = 96;
  assumed_empty_layer_y      = 16;
  assumed_background_layer_y = 42;

  /* Store the active layer, it shall not change during the execution
   * of this test
   */
  selected_layers = ligma_image_get_selected_layers (image);
  selected_layers = g_list_copy (selected_layers);

  /* Find the layer tree view to click in. Note that there is a
   * potential problem with gtk_test_find_widget and GtkNotebook: it
   * will return e.g. a GtkTreeView from another page if that page is
   * "on top" of the reference label.
   */
  dockable = ligma_ui_find_window (ligma_dialog_factory_get_singleton (),
                                  ligma_ui_is_ligma_layer_list);
  gtk_tree_view = gtk_test_find_widget (dockable,
                                        "Lock:",
                                        GTK_TYPE_TREE_VIEW);

  /* First make sure there is no selection */
  g_assert (ligma_channel_is_empty (selection));

  /* Now simulate alt-click on the background layer */
  g_assert (ligma_ui_synthesize_click (gtk_tree_view,
                                      assumed_layer_x,
                                      assumed_background_layer_y,
                                      1 /*button*/,
                                      GDK_MOD1_MASK));
  ligma_test_run_mainloop_until_idle ();

  /* Make sure we got a selection and that the active layer didn't
   * change
   */
  g_assert (! ligma_channel_is_empty (selection));
  g_assert (g_list_length (ligma_image_get_selected_layers (image)) ==
            g_list_length (selected_layers));
  for (iter = selected_layers; iter; iter = iter->next)
    g_assert (g_list_find (ligma_image_get_selected_layers (image), iter->data));

  /* Now simulate alt-click on the empty layer */
  g_assert (ligma_ui_synthesize_click (gtk_tree_view,
                                      assumed_layer_x,
                                      assumed_empty_layer_y,
                                      1 /*button*/,
                                      GDK_MOD1_MASK));
  ligma_test_run_mainloop_until_idle ();

  /* Make sure that emptied the selection and that the active layer
   * still didn't change
   */
  g_assert (ligma_channel_is_empty (selection));
  g_assert (g_list_length (ligma_image_get_selected_layers (image)) ==
            g_list_length (selected_layers));
  for (iter = selected_layers; iter; iter = iter->next)
    g_assert (g_list_find (ligma_image_get_selected_layers (image), iter->data));

  g_list_free (selected_layers);
#endif
}

static void
restore_recently_closed_multi_column_dock (gconstpointer data)
{
  Ligma      *ligma                          = LIGMA (data);
  GtkWidget *dock_window                   = NULL;
  gint       n_session_infos_before_close  = -1;
  gint       n_session_infos_after_close   = -1;
  gint       n_session_infos_after_restore = -1;
  GList     *session_infos                 = NULL;

  /* Find a non-toolbox dock window */
  dock_window = ligma_ui_find_window (ligma_dialog_factory_get_singleton (),
                                     ligma_ui_multicolumn_not_toolbox_window);
  g_assert (dock_window != NULL);

  /* Count number of docks */
  session_infos = ligma_dialog_factory_get_session_infos (ligma_dialog_factory_get_singleton ());
  n_session_infos_before_close = g_list_length (session_infos);

  /* Close one of the dock windows */
  ligma_ui_synthesize_delete_event (GTK_WIDGET (dock_window));
  ligma_test_run_mainloop_until_idle ();

  /* Make sure the number of session infos went down */
  session_infos = ligma_dialog_factory_get_session_infos (ligma_dialog_factory_get_singleton ());
  n_session_infos_after_close = g_list_length (session_infos);
  g_assert_cmpint (n_session_infos_before_close,
                   >,
                   n_session_infos_after_close);

#ifdef __GNUC__
#warning FIXME test disabled until we depend on GTK+ >= 2.24.11
#endif
#if 0
  /* Restore the (only available) closed dock and make sure the session
   * infos in the global dock factory are increased again
   */
  ligma_ui_manager_activate_action (ligma_test_utils_get_ui_manager (ligma),
                                   "windows",
                                   /* FIXME: This is severely hardcoded */
                                   "windows-recent-0003");
  ligma_test_run_mainloop_until_idle ();
  session_infos = ligma_dialog_factory_get_session_infos (ligma_dialog_factory_get_singleton ());
  n_session_infos_after_restore = g_list_length (session_infos);
  g_assert_cmpint (n_session_infos_after_close,
                   <,
                   n_session_infos_after_restore);
#endif
}

/**
 * tab_toggle_dont_change_dock_window_position:
 * @data:
 *
 * Makes sure that when dock windows are hidden with Tab and shown
 * again, their positions and sizes are not changed. We don't really
 * use Tab though, we only simulate its effect.
 **/
static void
tab_toggle_dont_change_dock_window_position (gconstpointer data)
{
  Ligma      *ligma          = LIGMA (data);
  GtkWidget *dock_window   = NULL;
  gint       x_before_hide = -1;
  gint       y_before_hide = -1;
  gint       w_before_hide = -1;
  gint       h_before_hide = -1;
  gint       x_after_show  = -1;
  gint       y_after_show  = -1;
  gint       w_after_show  = -1;
  gint       h_after_show  = -1;

  /* Find a non-toolbox dock window */
  dock_window = ligma_ui_find_window (ligma_dialog_factory_get_singleton (),
                                     ligma_ui_not_toolbox_window);
  g_assert (dock_window != NULL);
  g_assert (gtk_widget_get_visible (dock_window));

  /* Get the position and size */
  ligma_test_run_mainloop_until_idle ();
  gtk_window_get_position (GTK_WINDOW (dock_window),
                           &x_before_hide,
                           &y_before_hide);
  gtk_window_get_size (GTK_WINDOW (dock_window),
                       &w_before_hide,
                       &h_before_hide);

  /* Hide all dock windows */
  ligma_ui_manager_activate_action (ligma_test_utils_get_ui_manager (ligma),
                                   "windows",
                                   "windows-hide-docks");
  ligma_test_run_mainloop_until_idle ();
  g_assert (! gtk_widget_get_visible (dock_window));

  /* Show them again */
  ligma_ui_manager_activate_action (ligma_test_utils_get_ui_manager (ligma),
                                   "windows",
                                   "windows-hide-docks");
  ligma_test_run_mainloop_until_idle ();
  g_assert (gtk_widget_get_visible (dock_window));

  /* Get the position and size again and make sure it's the same as
   * before
   */
  gtk_window_get_position (GTK_WINDOW (dock_window),
                           &x_after_show,
                           &y_after_show);
  gtk_window_get_size (GTK_WINDOW (dock_window),
                       &w_after_show,
                       &h_after_show);
  g_assert_cmpint ((int)abs (x_before_hide - x_after_show), <=, LIGMA_UI_WINDOW_POSITION_EPSILON);
  g_assert_cmpint ((int)abs (y_before_hide - y_after_show), <=, LIGMA_UI_WINDOW_POSITION_EPSILON);
  g_assert_cmpint ((int)abs (w_before_hide - w_after_show), <=, LIGMA_UI_WINDOW_POSITION_EPSILON);
  g_assert_cmpint ((int)abs (h_before_hide - h_after_show), <=, LIGMA_UI_WINDOW_POSITION_EPSILON);
}

static void
switch_to_single_window_mode (gconstpointer data)
{
  Ligma *ligma = LIGMA (data);

  /* Switch to single-window mode. We consider this test as passed if
   * we don't get any GLib warnings/errors
   */
  ligma_ui_switch_window_mode (ligma);
}

static void
ligma_ui_toggle_docks_in_single_window_mode (Ligma *ligma)
{
  LigmaDisplay      *display       = LIGMA_DISPLAY (ligma_get_display_iter (ligma)->data);
  LigmaDisplayShell *shell         = ligma_display_get_shell (display);
  GtkWidget        *toplevel      = GTK_WIDGET (ligma_display_shell_get_window (shell));
  gint              x_temp        = -1;
  gint              y_temp        = -1;
  gint              x_before_hide = -1;
  gint              y_before_hide = -1;
  gint              x_after_hide  = -1;
  gint              y_after_hide  = -1;
  g_assert (shell);
  g_assert (toplevel);

  /* Get toplevel coordinate of image origin */
  ligma_test_run_mainloop_until_idle ();
  ligma_display_shell_transform_xy (shell,
                                   0.0, 0.0,
                                   &x_temp, &y_temp);
  gtk_widget_translate_coordinates (GTK_WIDGET (shell),
                                    toplevel,
                                    x_temp, y_temp,
                                    &x_before_hide, &y_before_hide);

  /* Hide all dock windows */
  ligma_ui_manager_activate_action (ligma_test_utils_get_ui_manager (ligma),
                                   "windows",
                                   "windows-hide-docks");
  ligma_test_run_mainloop_until_idle ();

  /* Get toplevel coordinate of image origin */
  ligma_test_run_mainloop_until_idle ();
  ligma_display_shell_transform_xy (shell,
                                   0.0, 0.0,
                                   &x_temp, &y_temp);
  gtk_widget_translate_coordinates (GTK_WIDGET (shell),
                                    toplevel,
                                    x_temp, y_temp,
                                    &x_after_hide, &y_after_hide);

  g_assert_cmpint ((int)abs (x_after_hide - x_before_hide), <=, LIGMA_UI_POSITION_EPSILON);
  g_assert_cmpint ((int)abs (y_after_hide - y_before_hide), <=, LIGMA_UI_POSITION_EPSILON);
}

static void
hide_docks_in_single_window_mode (gconstpointer data)
{
  Ligma *ligma = LIGMA (data);
  ligma_ui_toggle_docks_in_single_window_mode (ligma);
}

static void
show_docks_in_single_window_mode (gconstpointer data)
{
  Ligma *ligma = LIGMA (data);
  ligma_ui_toggle_docks_in_single_window_mode (ligma);
}

static void
maximize_state_in_aux_data (gconstpointer data)
{
  Ligma               *ligma    = LIGMA (data);
  LigmaDisplay        *display = LIGMA_DISPLAY (ligma_get_display_iter (ligma)->data);
  LigmaDisplayShell   *shell   = ligma_display_get_shell (display);
  LigmaImageWindow    *window  = ligma_display_shell_get_window (shell);
  gint                i;

  for (i = 0; i < 2; i++)
    {
      GList              *aux_info = NULL;
      LigmaSessionInfoAux *target_info;
      gboolean            target_max_state;

      if (i == 0)
        {
          target_info = ligma_session_info_aux_new ("maximized" , "yes");
          target_max_state = TRUE;
        }
      else
        {
          target_info = ligma_session_info_aux_new ("maximized", "no");
          target_max_state = FALSE;
        }

      /* Set the aux info to out target data */
      aux_info = g_list_append (aux_info, target_info);
      ligma_session_managed_set_aux_info (LIGMA_SESSION_MANAGED (window), aux_info);
      g_list_free (aux_info);

      /* Give the WM a chance to maximize/unmaximize us */
      ligma_test_run_mainloop_until_idle ();
      g_usleep (500 * 1000);
      ligma_test_run_mainloop_until_idle ();

      /* Make sure the maximize/unmaximize happened */
      g_assert (ligma_image_window_is_maximized (window) == target_max_state);

      /* Make sure we can read out the window state again */
      aux_info = ligma_session_managed_get_aux_info (LIGMA_SESSION_MANAGED (window));
      g_assert (g_list_find_custom (aux_info, target_info, ligma_ui_aux_data_eqiuvalent));
      g_list_free_full (aux_info,
                        (GDestroyNotify) ligma_session_info_aux_free);

      ligma_session_info_aux_free (target_info);
    }
}

static void
switch_back_to_multi_window_mode (gconstpointer data)
{
  Ligma *ligma = LIGMA (data);

  /* Switch back to multi-window mode. We consider this test as passed
   * if we don't get any GLib warnings/errors
   */
  ligma_ui_switch_window_mode (ligma);
}

static void
close_image (gconstpointer data)
{
  Ligma *ligma       = LIGMA (data);
  int   undo_count = 4;

  /* Undo all changes so we don't need to find the 'Do you want to
   * save?'-dialog and its 'No' button
   */
  while (undo_count--)
    {
      ligma_ui_manager_activate_action (ligma_test_utils_get_ui_manager (ligma),
                                       "edit",
                                       "edit-undo");
      ligma_test_run_mainloop_until_idle ();
    }

  /* Close the image */
  ligma_ui_manager_activate_action (ligma_test_utils_get_ui_manager (ligma),
                                   "view",
                                   "view-close");
  ligma_test_run_mainloop_until_idle ();

  /* Did it really disappear? */
  g_assert_cmpint (g_list_length (ligma_get_image_iter (ligma)), ==, 0);
}

/**
 * repeatedly_switch_window_mode:
 * @data:
 *
 * Makes sure that the size of the image window is properly handled
 * when repeatedly switching between window modes.
 **/
static void
repeatedly_switch_window_mode (gconstpointer data)
{
#ifdef __GNUC__
#warning FIXME: plesase fix repeatedly_switch_window_mode test
#endif
#if 0
  Ligma             *ligma     = LIGMA (data);
  LigmaDisplay      *display  = LIGMA_DISPLAY (ligma_get_empty_display (ligma));
  LigmaDisplayShell *shell    = ligma_display_get_shell (display);
  GtkWidget        *toplevel = gtk_widget_get_toplevel (GTK_WIDGET (shell));

  gint expected_initial_height;
  gint expected_initial_width;
  gint expected_second_height;
  gint expected_second_width;
  gint initial_width;
  gint initial_height;
  gint second_width;
  gint second_height;

  /* We need this for some reason */
  ligma_test_run_mainloop_until_idle ();

  /* Remember the multi-window mode size */
  gtk_window_get_size (GTK_WINDOW (toplevel),
                       &expected_initial_width,
                       &expected_initial_height);

  /* Switch to single-window mode */
  ligma_ui_switch_window_mode (ligma);

  /* Remember the single-window mode size */
  gtk_window_get_size (GTK_WINDOW (toplevel),
                       &expected_second_width,
                       &expected_second_height);

  /* Make sure they differ, otherwise the test is pointless */
  g_assert_cmpint (expected_initial_width,  !=, expected_second_width);
  g_assert_cmpint (expected_initial_height, !=, expected_second_height);

  /* Switch back to multi-window mode */
  ligma_ui_switch_window_mode (ligma);

  /* Make sure the size is the same as before */
  gtk_window_get_size (GTK_WINDOW (toplevel), &initial_width, &initial_height);
  g_assert_cmpint (expected_initial_width,  ==, initial_width);
  g_assert_cmpint (expected_initial_height, ==, initial_height);

  /* Switch to single-window mode again... */
  ligma_ui_switch_window_mode (ligma);

  /* Make sure the size is the same as before */
  gtk_window_get_size (GTK_WINDOW (toplevel), &second_width, &second_height);
  g_assert_cmpint (expected_second_width,  ==, second_width);
  g_assert_cmpint (expected_second_height, ==, second_height);

  /* Finally switch back to multi-window mode since that was the mode
   * when we started
   */
  ligma_ui_switch_window_mode (ligma);
#endif
}

/**
 * window_roles:
 * @data:
 *
 * Makes sure that different windows have the right roles specified.
 **/
static void
window_roles (gconstpointer data)
{
  GdkMonitor     *monitor        = NULL;
  GtkWidget      *dock           = NULL;
  GtkWidget      *toolbox        = NULL;
  LigmaDockWindow *dock_window    = NULL;
  LigmaDockWindow *toolbox_window = NULL;

  monitor        = gdk_display_get_primary_monitor (gdk_display_get_default ());
  dock           = ligma_dock_with_window_new (ligma_dialog_factory_get_singleton (),
                                              monitor,
                                              FALSE /*toolbox*/);
  toolbox        = ligma_dock_with_window_new (ligma_dialog_factory_get_singleton (),
                                              monitor,
                                              TRUE /*toolbox*/);
  dock_window    = ligma_dock_window_from_dock (LIGMA_DOCK (dock));
  toolbox_window = ligma_dock_window_from_dock (LIGMA_DOCK (toolbox));

  g_assert_cmpint (g_str_has_prefix (gtk_window_get_role (GTK_WINDOW (dock_window)), "ligma-dock-"), ==,
                   TRUE);
  g_assert_cmpint (g_str_has_prefix (gtk_window_get_role (GTK_WINDOW (toolbox_window)), "ligma-toolbox-"), ==,
                   TRUE);

  /* When we get here we have a ref count of one, but the signals we
   * emit cause the reference count to become less than zero for some
   * reason. So we're lazy and simply ignore to unref these
  g_object_unref (toolbox);
  g_object_unref (dock);
   */
}

static void
paintbrush_is_standard_tool (gconstpointer data)
{
  Ligma         *ligma         = LIGMA (data);
  LigmaContext  *user_context = ligma_get_user_context (ligma);
  LigmaToolInfo *tool_info    = ligma_context_get_tool (user_context);

  g_assert_cmpstr (tool_info->help_id,
                   ==,
                   "ligma-tool-paintbrush");
}

/**
 * ligma_ui_synthesize_delete_event:
 * @widget:
 *
 * Synthesize a delete event to @widget.
 **/
static void
ligma_ui_synthesize_delete_event (GtkWidget *widget)
{
  GdkWindow *window = NULL;
  GdkEvent *event = NULL;

  window = gtk_widget_get_window (widget);
  g_assert (window);

  event = gdk_event_new (GDK_DELETE);
  event->any.window     = g_object_ref (window);
  event->any.send_event = TRUE;
  gtk_main_do_event (event);
  gdk_event_free (event);
}

static gboolean
ligma_ui_synthesize_click (GtkWidget       *widget,
                          gint             x,
                          gint             y,
                          gint             button, /*1..3*/
                          GdkModifierType  modifiers)
{
  return (gdk_test_simulate_button (gtk_widget_get_window (widget),
                                    x, y,
                                    button,
                                    modifiers,
                                    GDK_BUTTON_PRESS) &&
          gdk_test_simulate_button (gtk_widget_get_window (widget),
                                    x, y,
                                    button,
                                    modifiers,
                                    GDK_BUTTON_RELEASE));
}

static GtkWidget *
ligma_ui_find_window (LigmaDialogFactory *dialog_factory,
                     LigmaUiTestFunc     predicate)
{
  GList     *iter        = NULL;
  GtkWidget *dock_window = NULL;

  g_return_val_if_fail (predicate != NULL, NULL);

  for (iter = ligma_dialog_factory_get_session_infos (dialog_factory);
       iter;
       iter = g_list_next (iter))
    {
      GtkWidget *widget = ligma_session_info_get_widget (iter->data);

      if (predicate (G_OBJECT (widget)))
        {
          dock_window = widget;
          break;
        }
    }

  return dock_window;
}

static gboolean
ligma_ui_not_toolbox_window (GObject *object)
{
  return (LIGMA_IS_DOCK_WINDOW (object) &&
          ! ligma_dock_window_has_toolbox (LIGMA_DOCK_WINDOW (object)));
}

static gboolean
ligma_ui_multicolumn_not_toolbox_window (GObject *object)
{
  gboolean           not_toolbox_window;
  LigmaDockWindow    *dock_window;
  LigmaDockContainer *dock_container;
  GList             *docks;

  if (! LIGMA_IS_DOCK_WINDOW (object))
    return FALSE;

  dock_window    = LIGMA_DOCK_WINDOW (object);
  dock_container = LIGMA_DOCK_CONTAINER (object);
  docks          = ligma_dock_container_get_docks (dock_container);

  not_toolbox_window = (! ligma_dock_window_has_toolbox (dock_window) &&
                        g_list_length (docks) > 1);

  g_list_free (docks);

  return not_toolbox_window;
}

static gboolean
ligma_ui_is_ligma_layer_list (GObject *object)
{
  LigmaDialogFactoryEntry *entry = NULL;

  if (! GTK_IS_WIDGET (object))
    return FALSE;

  ligma_dialog_factory_from_widget (GTK_WIDGET (object), &entry);

  return strcmp (entry->identifier, "ligma-layer-list") == 0;
}

static int
ligma_ui_aux_data_eqiuvalent (gconstpointer _a, gconstpointer _b)
{
  LigmaSessionInfoAux *a = (LigmaSessionInfoAux*) _a;
  LigmaSessionInfoAux *b = (LigmaSessionInfoAux*) _b;
  return (strcmp (a->name, b->name) || strcmp (a->value, b->value));
}

static void
ligma_ui_switch_window_mode (Ligma *ligma)
{
  ligma_ui_manager_activate_action (ligma_test_utils_get_ui_manager (ligma),
                                   "windows",
                                   "windows-use-single-window-mode");
  ligma_test_run_mainloop_until_idle ();

  /* Add a small sleep to let things stabilize */
  g_usleep (500 * 1000);
  ligma_test_run_mainloop_until_idle ();
}

int main(int argc, char **argv)
{
  Ligma *ligma   = NULL;
  gint  result = -1;

  ligma_test_bail_if_no_display ();
  gtk_test_init (&argc, &argv, NULL);

  ligma_test_utils_set_ligma3_directory ("LIGMA_TESTING_ABS_TOP_SRCDIR",
                                       "app/tests/ligmadir");
  ligma_test_utils_setup_menus_path ();

  /* Start up LIGMA */
  ligma = ligma_init_for_gui_testing (TRUE /*show_gui*/);
  ligma_test_run_mainloop_until_idle ();

  /* Add tests. Note that the order matters. For example,
   * 'paintbrush_is_standard_tool' can't be after
   * 'tool_options_editor_updates'
   */
  ADD_TEST (paintbrush_is_standard_tool);
  ADD_TEST (tool_options_editor_updates);
  ADD_TEST (create_new_image_via_dialog);
  ADD_TEST (keyboard_zoom_focus);
  ADD_TEST (alt_click_is_layer_to_selection);
  ADD_TEST (restore_recently_closed_multi_column_dock);
  ADD_TEST (tab_toggle_dont_change_dock_window_position);
  ADD_TEST (switch_to_single_window_mode);
#warning FIXME: hide/show docks doesn't work when running make check
#if 0
  ADD_TEST (hide_docks_in_single_window_mode);
  ADD_TEST (show_docks_in_single_window_mode);
#endif
#warning FIXME: maximize_state_in_aux_data doesn't work without WM
#if 0
  ADD_TEST (maximize_state_in_aux_data);
#endif
  ADD_TEST (switch_back_to_multi_window_mode);
  ADD_TEST (close_image);
  ADD_TEST (repeatedly_switch_window_mode);
  ADD_TEST (window_roles);

  /* Run the tests and return status */
  result = g_test_run ();

  /* Don't write files to the source dir */
  ligma_test_utils_set_ligma3_directory ("LIGMA_TESTING_ABS_TOP_BUILDDIR",
                                       "app/tests/ligmadir-output");

  /* Exit properly so we don't break script-fu plug-in wire */
  ligma_exit (ligma, TRUE);

  return result;
}
