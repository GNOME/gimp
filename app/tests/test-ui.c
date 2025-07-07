/* GIMP - The GNU Image Manipulation Program
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

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "dialogs/dialogs-types.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-scale.h"
#include "display/gimpdisplayshell-transform.h"
#include "display/gimpimagewindow.h"

#include "menus/menus.h"

#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpdock.h"
#include "widgets/gimpdockable.h"
#include "widgets/gimpdockbook.h"
#include "widgets/gimpdockcontainer.h"
#include "widgets/gimpdocked.h"
#include "widgets/gimpdockwindow.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpsessioninfo.h"
#include "widgets/gimpsessioninfo-aux.h"
#include "widgets/gimpsessionmanaged.h"
#include "widgets/gimptoolbox.h"
#include "widgets/gimptooloptionseditor.h"
#include "widgets/gimpuimanager.h"
#include "widgets/gimpwidgets-utils.h"

#include "core/gimp.h"
#include "core/gimpchannel.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimplayer.h"
#include "core/gimplayer-new.h"
#include "core/gimptoolinfo.h"
#include "core/gimptooloptions.h"

#include "gimpcoreapp.h"

#include "gimp-app-test-utils.h"
#include "tests.h"


#define GIMP_UI_WINDOW_POSITION_EPSILON 30
#define GIMP_UI_POSITION_EPSILON        1
#define GIMP_UI_ZOOM_EPSILON            0.01

#define ADD_TEST(function) \
  g_test_add_data_func ("/gimp-ui/" #function, gimp, function);


/* Put this in the code below when you want the test to pause so you
 * can do measurements of widgets on the screen for example
 */
#define GIMP_PAUSE (g_usleep (20 * 1000 * 1000))


typedef gboolean (*GimpUiTestFunc) (GObject *object);


static void            gimp_ui_synthesize_delete_event          (GtkWidget         *widget);
static GtkWidget     * gimp_ui_find_window                      (GimpDialogFactory *dialog_factory,
                                                                 GimpUiTestFunc     predicate);
static gboolean        gimp_ui_not_toolbox_window               (GObject           *object);
static gboolean        gimp_ui_multicolumn_not_toolbox_window   (GObject           *object);
static void            gimp_ui_switch_window_mode               (Gimp              *gimp);


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
  Gimp                  *gimp         = GIMP (data);
  GimpDisplay           *display      = GIMP_DISPLAY (gimp_get_empty_display (gimp));
  GimpDisplayShell      *shell        = gimp_display_get_shell (display);
  GtkWidget             *toplevel     = gtk_widget_get_toplevel (GTK_WIDGET (shell));
  GimpUIManager         *ui_manager   = menus_get_image_manager_singleton (gimp);
  GtkWidget             *dockable     = gimp_dialog_factory_dialog_new (gimp_dialog_factory_get_singleton (),
                                                                        gimp_widget_get_monitor (toplevel),
                                                                        NULL /*ui_manager*/,
                                                                        toplevel,
                                                                        "gimp-tool-options",
                                                                        -1 /*view_size*/,
                                                                        FALSE /*present*/);
  GimpToolOptionsEditor *editor       = GIMP_TOOL_OPTIONS_EDITOR (gtk_bin_get_child (GTK_BIN (dockable)));

  /* First select the rect select tool */
  gimp_ui_manager_activate_action (ui_manager,
                                   "tools",
                                   "tools-rect-select");
  g_assert_cmpstr (GIMP_HELP_TOOL_RECT_SELECT,
                   ==,
                   gimp_tool_options_editor_get_tool_options (editor)->
                   tool_info->help_id);

  /* Change tool and make sure the change is taken into account by the
   * tool options editor
   */
  gimp_ui_manager_activate_action (ui_manager,
                                   "tools",
                                   "tools-ellipse-select");
  g_assert_cmpstr (GIMP_HELP_TOOL_ELLIPSE_SELECT,
                   ==,
                   gimp_tool_options_editor_get_tool_options (editor)->
                   tool_info->help_id);
}

static void
create_new_image_via_dialog (gconstpointer data)
{
  Gimp      *gimp = GIMP (data);
  GimpImage *image;
  GimpLayer *layer;

  image = gimp_test_utils_create_image_from_dialog (gimp);

  /* Add a layer to the image to make it more useful in later tests */
  layer = gimp_layer_new (image,
                          gimp_image_get_width (image),
                          gimp_image_get_height (image),
                          gimp_image_get_layer_format (image, TRUE),
                          "Layer for testing",
                          GIMP_OPACITY_OPAQUE,
                          GIMP_LAYER_MODE_NORMAL);

  gimp_image_add_layer (image, layer,
                        GIMP_IMAGE_ACTIVE_PARENT, -1, TRUE);
  gimp_test_run_mainloop_until_idle ();
}

static void
keyboard_zoom_focus (gconstpointer data)
{
  Gimp              *gimp    = GIMP (data);
  GimpDisplay       *display = GIMP_DISPLAY (gimp_get_display_iter (gimp)->data);
  GimpDisplayShell  *shell   = gimp_display_get_shell (display);
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
  gimp_display_shell_transform_xy (shell,
                                   image_x,
                                   image_y,
                                   &shell_x_before_zoom,
                                   &shell_y_before_zoom);
  gimp_display_shell_push_zoom_focus_pointer_pos (shell,
                                                  shell_x_before_zoom,
                                                  shell_y_before_zoom);
  factor_before_zoom = gimp_zoom_model_get_factor (shell->zoom);

  /* Do the zoom */
  gimp_test_utils_synthesize_key_event (GTK_WIDGET (shell), GDK_KEY_plus);
  gimp_test_run_mainloop_until_idle ();

  /* Make sure the zoom focus point remained fixed */
  gimp_display_shell_transform_xy (shell,
                                   image_x,
                                   image_y,
                                   &shell_x_after_zoom,
                                   &shell_y_after_zoom);
  factor_after_zoom = gimp_zoom_model_get_factor (shell->zoom);

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
                     GIMP_UI_ZOOM_EPSILON);
}

static void
restore_recently_closed_multi_column_dock (gconstpointer data)
{
  Gimp      *gimp                          = GIMP (data);
  GtkWidget *dock_window                   = NULL;
  gint       n_session_infos_before_close  = -1;
  gint       n_session_infos_after_close   = -1;
  gint       n_session_infos_after_restore = -1;
  GList     *session_infos                 = NULL;

  /* Find a non-toolbox dock window */
  dock_window = gimp_ui_find_window (gimp_dialog_factory_get_singleton (),
                                     gimp_ui_multicolumn_not_toolbox_window);
  g_assert_true (dock_window != NULL);

  /* Count number of docks */
  session_infos = gimp_dialog_factory_get_session_infos (gimp_dialog_factory_get_singleton ());
  n_session_infos_before_close = g_list_length (session_infos);

  /* Close one of the dock windows */
  gimp_ui_synthesize_delete_event (GTK_WIDGET (dock_window));
  gimp_test_run_mainloop_until_idle ();

  /* Make sure the number of session infos went down */
  session_infos = gimp_dialog_factory_get_session_infos (gimp_dialog_factory_get_singleton ());
  n_session_infos_after_close = g_list_length (session_infos);
  g_assert_cmpint (n_session_infos_before_close,
                   >,
                   n_session_infos_after_close);

  /* Restore the (only available) closed dock and make sure the session
   * infos in the global dock factory are increased again
   */
  gimp_ui_manager_activate_action (gimp_test_utils_get_ui_manager (gimp),
                                   "windows",
                                   /* FIXME: This is severely hardcoded */
                                   "windows-recent-0003");
  gimp_test_run_mainloop_until_idle ();
  session_infos = gimp_dialog_factory_get_session_infos (gimp_dialog_factory_get_singleton ());
  n_session_infos_after_restore = g_list_length (session_infos);
  g_assert_cmpint (n_session_infos_after_close,
                   <,
                   n_session_infos_after_restore);
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
  Gimp      *gimp          = GIMP (data);
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
  dock_window = gimp_ui_find_window (gimp_dialog_factory_get_singleton (),
                                     gimp_ui_not_toolbox_window);
  g_assert_true (dock_window != NULL);
  g_assert_true (gtk_widget_get_visible (dock_window));

  /* Get the position and size */
  gimp_test_run_mainloop_until_idle ();
  gtk_window_get_position (GTK_WINDOW (dock_window),
                           &x_before_hide,
                           &y_before_hide);
  gtk_window_get_size (GTK_WINDOW (dock_window),
                       &w_before_hide,
                       &h_before_hide);

  /* Hide all dock windows */
  gimp_ui_manager_activate_action (gimp_test_utils_get_ui_manager (gimp),
                                   "windows",
                                   "windows-hide-docks");
  gimp_test_run_mainloop_until_idle ();
  g_assert_true (! gtk_widget_get_visible (dock_window));

  /* Show them again */
  gimp_ui_manager_activate_action (gimp_test_utils_get_ui_manager (gimp),
                                   "windows",
                                   "windows-hide-docks");
  gimp_test_run_mainloop_until_idle ();
  g_assert_true (gtk_widget_get_visible (dock_window));

  /* Get the position and size again and make sure it's the same as
   * before
   */
  gtk_window_get_position (GTK_WINDOW (dock_window),
                           &x_after_show,
                           &y_after_show);
  gtk_window_get_size (GTK_WINDOW (dock_window),
                       &w_after_show,
                       &h_after_show);
  g_assert_cmpint ((int)abs (x_before_hide - x_after_show), <=, GIMP_UI_WINDOW_POSITION_EPSILON);
  g_assert_cmpint ((int)abs (y_before_hide - y_after_show), <=, GIMP_UI_WINDOW_POSITION_EPSILON);
  g_assert_cmpint ((int)abs (w_before_hide - w_after_show), <=, GIMP_UI_WINDOW_POSITION_EPSILON);
  g_assert_cmpint ((int)abs (h_before_hide - h_after_show), <=, GIMP_UI_WINDOW_POSITION_EPSILON);
}

static void
switch_to_single_window_mode (gconstpointer data)
{
  Gimp *gimp = GIMP (data);

  /* Switch to single-window mode. We consider this test as passed if
   * we don't get any GLib warnings/errors
   */
  gimp_ui_switch_window_mode (gimp);
}

static void
switch_back_to_multi_window_mode (gconstpointer data)
{
  Gimp *gimp = GIMP (data);

  /* Switch back to multi-window mode. We consider this test as passed
   * if we don't get any GLib warnings/errors
   */
  gimp_ui_switch_window_mode (gimp);
}

static void
close_image (gconstpointer data)
{
  Gimp *gimp       = GIMP (data);
  int   undo_count = 4;

  /* Undo all changes so we don't need to find the 'Do you want to
   * save?'-dialog and its 'No' button
   */
  while (undo_count--)
    {
      gimp_ui_manager_activate_action (gimp_test_utils_get_ui_manager (gimp),
                                       "edit",
                                       "edit-undo");
      gimp_test_run_mainloop_until_idle ();
    }

  /* Close the image */
  gimp_ui_manager_activate_action (gimp_test_utils_get_ui_manager (gimp),
                                   "view",
                                   "view-close");
  gimp_test_run_mainloop_until_idle ();

  /* Did it really disappear? */
  g_assert_cmpint (g_list_length (gimp_get_image_iter (gimp)), ==, 0);
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
  Gimp              *gimp           = GIMP (data);
  GimpDisplay       *display        = GIMP_DISPLAY (gimp_get_display_iter (gimp)->data);
  GimpDisplayShell  *shell          = gimp_display_get_shell (display);
  GimpImageWindow   *window         = gimp_display_shell_get_window (shell);
  GdkMonitor        *monitor        = NULL;
  GtkWidget         *dock           = NULL;
  GtkWidget         *toolbox        = NULL;
  GimpDockWindow    *dock_window    = NULL;
  GimpDockWindow    *toolbox_window = NULL;

  monitor        = gdk_display_get_monitor_at_window(gdk_display_get_default (), gtk_widget_get_window(GTK_WIDGET(window)));

  dock           = gimp_dock_with_window_new (gimp_dialog_factory_get_singleton (),
                                              monitor,
                                              FALSE /*toolbox*/);
  toolbox        = gimp_dock_with_window_new (gimp_dialog_factory_get_singleton (),
                                              monitor,
                                              TRUE /*toolbox*/);
  dock_window    = gimp_dock_window_from_dock (GIMP_DOCK (dock));
  toolbox_window = gimp_dock_window_from_dock (GIMP_DOCK (toolbox));

  g_assert_cmpint (g_str_has_prefix (gtk_window_get_role (GTK_WINDOW (dock_window)), "gimp-dock-"), ==,
                   TRUE);
  g_assert_cmpint (g_str_has_prefix (gtk_window_get_role (GTK_WINDOW (toolbox_window)), "gimp-toolbox-"), ==,
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
  Gimp         *gimp         = GIMP (data);
  GimpContext  *user_context = gimp_get_user_context (gimp);
  GimpToolInfo *tool_info    = gimp_context_get_tool (user_context);

  g_assert_cmpstr (tool_info->help_id,
                   ==,
                   "gimp-tool-paintbrush");
}

/**
 * gimp_ui_synthesize_delete_event:
 * @widget:
 *
 * Synthesize a delete event to @widget.
 **/
static void
gimp_ui_synthesize_delete_event (GtkWidget *widget)
{
  GdkWindow *window = NULL;
  GdkEvent *event = NULL;

  window = gtk_widget_get_window (widget);
  g_assert_true (window);

  event = gdk_event_new (GDK_DELETE);
  event->any.window     = g_object_ref (window);
  event->any.send_event = TRUE;
  gtk_main_do_event (event);
  gdk_event_free (event);
}

static GtkWidget *
gimp_ui_find_window (GimpDialogFactory *dialog_factory,
                     GimpUiTestFunc     predicate)
{
  GList     *iter        = NULL;
  GtkWidget *dock_window = NULL;

  g_return_val_if_fail (predicate != NULL, NULL);

  for (iter = gimp_dialog_factory_get_session_infos (dialog_factory);
       iter;
       iter = g_list_next (iter))
    {
      GtkWidget *widget = gimp_session_info_get_widget (iter->data);

      if (predicate (G_OBJECT (widget)))
        {
          dock_window = widget;
          break;
        }
    }

  return dock_window;
}

static gboolean
gimp_ui_not_toolbox_window (GObject *object)
{
  return (GIMP_IS_DOCK_WINDOW (object) &&
          ! gimp_dock_window_has_toolbox (GIMP_DOCK_WINDOW (object)));
}

static gboolean
gimp_ui_multicolumn_not_toolbox_window (GObject *object)
{
  gboolean           not_toolbox_window;
  GimpDockWindow    *dock_window;
  GimpDockContainer *dock_container;
  GList             *docks;

  if (! GIMP_IS_DOCK_WINDOW (object))
    return FALSE;

  dock_window    = GIMP_DOCK_WINDOW (object);
  dock_container = GIMP_DOCK_CONTAINER (object);
  docks          = gimp_dock_container_get_docks (dock_container);

  not_toolbox_window = (! gimp_dock_window_has_toolbox (dock_window) &&
                        g_list_length (docks) > 1);

  g_list_free (docks);

  return not_toolbox_window;
}

static void
gimp_ui_switch_window_mode (Gimp *gimp)
{
  gimp_ui_manager_activate_action (gimp_test_utils_get_ui_manager (gimp),
                                   "windows",
                                   "windows-use-single-window-mode");
  gimp_test_run_mainloop_until_idle ();

  /* Add a small sleep to let things stabilize */
  g_usleep (500 * 1000);
  gimp_test_run_mainloop_until_idle ();
}

int main(int argc, char **argv)
{
  Gimp *gimp   = NULL;
  gint  result = -1;

  gimp_test_bail_if_no_display ();
  gtk_test_init (&argc, &argv, NULL);

  gimp_test_utils_setup_menus_path ();

  /* Start up GIMP */
  gimp = gimp_init_for_gui_testing (TRUE /*show_gui*/);
  gimp_test_run_mainloop_until_idle ();

  /* Add tests. Note that the order matters. For example,
   * 'paintbrush_is_standard_tool' can't be after
   * 'tool_options_editor_updates'
   */
  ADD_TEST (paintbrush_is_standard_tool);
  ADD_TEST (tool_options_editor_updates);
  ADD_TEST (create_new_image_via_dialog);
  ADD_TEST (keyboard_zoom_focus);
  ADD_TEST (restore_recently_closed_multi_column_dock);
  ADD_TEST (tab_toggle_dont_change_dock_window_position);
  ADD_TEST (switch_to_single_window_mode);
  ADD_TEST (switch_back_to_multi_window_mode);
  ADD_TEST (close_image);
  ADD_TEST (window_roles);

  /* Run the tests and return status */
  g_application_run (gimp->app, 0, NULL);
  result = gimp_core_app_get_exit_status (GIMP_CORE_APP (gimp->app));

  g_application_quit (G_APPLICATION (gimp->app));
  g_clear_object (&gimp->app);

  return result;
}
