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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"

#include "dialogs/dialogs-types.h"

#include "dialogs/dialogs.h"

#include "display/gimpdisplay.h"
#include "display/gimpimagewindow.h"

#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpdocked.h"
#include "widgets/gimpdockwindow.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpsessioninfo.h"
#include "widgets/gimptooloptionseditor.h"
#include "widgets/gimpuimanager.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimptoolinfo.h"
#include "core/gimptooloptions.h"

#include "tests.h"

#include "gimp-app-test-utils.h"


#define GIMP_UI_POSITION_EPSILON 10


typedef struct
{
  int avoid_sizeof_zero;
} GimpTestFixture;


static void            gimp_ui_tool_options_editor_updates      (GimpTestFixture   *fixture,
                                                                 gconstpointer      data);
static void            gimp_ui_create_new_image_via_dialog      (GimpTestFixture   *fixture,
                                                                 gconstpointer      data);
static void            gimp_ui_restore_recently_closed_dock     (GimpTestFixture   *fixture,
                                                                 gconstpointer      data);
static void            gimp_ui_tab_toggle_dont_change_position  (GimpTestFixture   *fixture,
                                                                 gconstpointer      data);
static void            gimp_ui_switch_to_single_window_mode     (GimpTestFixture   *fixture,
                                                                 gconstpointer      data);
static void            gimp_ui_switch_back_to_multi_window_mode (GimpTestFixture   *fixture,
                                                                 gconstpointer      data);
static GimpUIManager * gimp_ui_get_ui_manager                   (Gimp              *gimp);
static void            gimp_ui_synthesize_delete_event          (GtkWidget         *widget);
static GtkWidget     * gimp_ui_find_non_toolbox_dock_window     (GimpDialogFactory *dialog_factory);


int main(int argc, char **argv)
{
  Gimp *gimp   = NULL;
  gint  result = -1;

  g_type_init ();
  gtk_init (&argc, &argv);
  g_test_init (&argc, &argv, NULL);

  gimp_test_utils_set_gimp2_directory ("GIMP_TESTING_ABS_TOP_SRCDIR",
                                       "app/tests/gimpdir");
  gimp_test_utils_setup_menus_dir ();

  /* Start up GIMP */
  gimp = gimp_init_for_gui_testing (FALSE, TRUE);
  gimp_test_run_mainloop_until_idle ();

  /* Setup the tests */
  g_test_add ("/gimp-ui/tool-options-editor-updates",
              GimpTestFixture,
              gimp,
              NULL,
              gimp_ui_tool_options_editor_updates,
              NULL);
  g_test_add ("/gimp-ui/create-new-image-via-dialog",
              GimpTestFixture,
              gimp,
              NULL,
              gimp_ui_create_new_image_via_dialog,
              NULL);
  g_test_add ("/gimp-ui/restore-recently-closed-dock",
              GimpTestFixture,
              gimp,
              NULL,
              gimp_ui_restore_recently_closed_dock,
              NULL);
  g_test_add ("/gimp-ui/tab-toggle-dont-change-dock-window-position",
              GimpTestFixture,
              gimp,
              NULL,
              gimp_ui_tab_toggle_dont_change_position,
              NULL);
  g_test_add ("/gimp-ui/switch-to-single-window-mode",
              GimpTestFixture,
              gimp,
              NULL,
              gimp_ui_switch_to_single_window_mode,
              NULL);
  g_test_add ("/gimp-ui/switch-back-to-multi-window-mode",
              GimpTestFixture,
              gimp,
              NULL,
              gimp_ui_switch_back_to_multi_window_mode,
              NULL);

  /* Run the tests and return status */
  result = g_test_run ();

  /* Don't write files to the source dir */
  gimp_test_utils_set_gimp2_directory ("GIMP_TESTING_ABS_TOP_BUILDDIR",
                                       "app/tests/gimpdir-output");

  /* Exit properly so we don't break script-fu plug-in wire */
  gimp_exit (gimp, TRUE);

  return result;
}

/**
 * gimp_ui_tool_options_editor_updates:
 * @fixture:
 * @data:
 *
 * Makes sure that the tool options editor is updated when the tool
 * changes.
 **/
static void
gimp_ui_tool_options_editor_updates (GimpTestFixture *fixture,
                                     gconstpointer    data)
{
  Gimp                  *gimp         = GIMP (data);
  GimpDisplay           *display      = GIMP_DISPLAY (gimp_get_empty_display (gimp));
  GimpDisplayShell      *shell        = gimp_display_get_shell (display);
  GtkWidget             *toplevel     = gtk_widget_get_toplevel (GTK_WIDGET (shell));
  GimpImageWindow       *image_window = GIMP_IMAGE_WINDOW (toplevel);
  GimpUIManager         *ui_manager   = gimp_image_window_get_ui_manager (image_window);
  GimpDialogFactory     *dock_factory = gimp_dialog_factory_from_name ("dock");
  GtkWidget             *dockable     = gimp_dialog_factory_dialog_new (dock_factory,
                                                                        gtk_widget_get_screen (toplevel),
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
                   tool_info->
                   help_id);

  /* Change tool and make sure the change is taken into account by the
   * tool options editor
   */
  gimp_ui_manager_activate_action (ui_manager,
                                   "tools",
                                   "tools-ellipse-select");
  g_assert_cmpstr (GIMP_HELP_TOOL_ELLIPSE_SELECT,
                   ==,
                   gimp_tool_options_editor_get_tool_options (editor)->
                   tool_info->
                   help_id);
}

static void
gimp_ui_create_new_image_via_dialog (GimpTestFixture *fixture,
                                     gconstpointer    data)
{
  Gimp              *gimp             = GIMP (data);
  GimpDisplay       *display          = GIMP_DISPLAY (gimp_get_empty_display (gimp));
  GimpDisplayShell  *shell            = gimp_display_get_shell (display);
  GtkWidget         *toplevel         = gtk_widget_get_toplevel (GTK_WIDGET (shell));
  GimpImageWindow   *image_window     = GIMP_IMAGE_WINDOW (toplevel);
  GimpUIManager     *ui_manager       = gimp_image_window_get_ui_manager (image_window);
  GimpDialogFactory *dialog_factory   = gimp_dialog_factory_from_name ("toplevel");
  GtkWidget         *new_image_dialog = NULL;
  guint              n_initial_images = g_list_length (gimp_get_image_iter (gimp));
  guint              n_images         = -1;
  gint               tries_left       = 100;

  /* Bring up the new image dialog */
  gimp_ui_manager_activate_action (ui_manager,
                                   "image",
                                   "image-new");
  gimp_test_run_mainloop_until_idle ();

  /* Get the GtkWindow of the dialog */
  new_image_dialog =
    gimp_dialog_factory_dialog_raise (dialog_factory,
                                      gtk_widget_get_screen (GTK_WIDGET (shell)),
                                      "gimp-image-new-dialog",
                                      -1 /*view_size*/);

  /* Press the focused widget, it should be the Ok button. It will
   * take a while for the image to be created to loop for a while
   */
  gtk_widget_activate (gtk_window_get_focus (GTK_WINDOW (new_image_dialog)));
  do
    {
      g_usleep (20 * 1000);
      gimp_test_run_mainloop_until_idle ();
      n_images = g_list_length (gimp_get_image_iter (gimp));
    }
  while (tries_left-- &&
         n_images != n_initial_images + 1);

  /* Make sure there now is one image more than initially */
  g_assert_cmpint (n_images,
                   ==,
                   n_initial_images + 1);
}

static void
gimp_ui_restore_recently_closed_dock (GimpTestFixture *fixture,
                                      gconstpointer    data)
{
  Gimp      *gimp                          = GIMP (data);
  GtkWidget *dock_window                   = NULL;
  gint       n_session_infos_before_close  = -1;
  gint       n_session_infos_after_close   = -1;
  gint       n_session_infos_after_restore = -1;
  GList     *session_infos                 = NULL;

  /* Find a non-toolbox dock window */
  dock_window = gimp_ui_find_non_toolbox_dock_window (global_dock_factory);
  g_assert (dock_window != NULL);

  /* Count number of docks */
  session_infos = gimp_dialog_factory_get_session_infos (global_dock_factory);
  n_session_infos_before_close = g_list_length (session_infos);

  /* Close one of the dock windows */
  gimp_ui_synthesize_delete_event (GTK_WIDGET (dock_window));
  gimp_test_run_mainloop_until_idle ();

  /* Make sure the number of session infos went down */
  session_infos = gimp_dialog_factory_get_session_infos (global_dock_factory);
  n_session_infos_after_close = g_list_length (session_infos);
  g_assert_cmpint (n_session_infos_before_close,
                   >,
                   n_session_infos_after_close);

  /* Restore the (only avaiable) closed dock and make sure the session
   * infos in the global dock factory are increased again
   */
  gimp_ui_manager_activate_action (gimp_ui_get_ui_manager (gimp),
                                   "windows",
                                   /* FIXME: This is severly hardcoded */
                                   "windows-recent-0003");
  gimp_test_run_mainloop_until_idle ();
  session_infos = gimp_dialog_factory_get_session_infos (global_dock_factory);
  n_session_infos_after_restore = g_list_length (session_infos);
  g_assert_cmpint (n_session_infos_after_close,
                   <,
                   n_session_infos_after_restore);
}

/**
 * gimp_ui_tab_toggle_dont_change_position:
 * @fixture:
 * @data:
 *
 * Makes sure that when dock windows are hidden with Tab and shown
 * again, their positions and sizes are not changed. We don't really
 * use Tab though, we only simulate its effect.
 **/
static void
gimp_ui_tab_toggle_dont_change_position (GimpTestFixture *fixture,
                                         gconstpointer    data)
{
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
  dock_window = gimp_ui_find_non_toolbox_dock_window (global_dock_factory);
  g_assert (dock_window != NULL);
  g_assert (gtk_widget_get_visible (dock_window));

  /* Get the position and size */
  gimp_test_run_mainloop_until_idle ();
  gtk_window_get_position (GTK_WINDOW (dock_window),
                           &x_before_hide,
                           &y_before_hide);
  gtk_window_get_size (GTK_WINDOW (dock_window),
                       &w_before_hide,
                       &h_before_hide);

  /* Hide all dock windows */
  gimp_dialog_factories_toggle ();
  gimp_test_run_mainloop_until_idle ();
  g_assert (! gtk_widget_get_visible (dock_window));

  /* Show them again */
  gimp_dialog_factories_toggle ();
  gimp_test_run_mainloop_until_idle ();
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
  g_assert_cmpint ((int)abs (x_before_hide - x_after_show), <, GIMP_UI_POSITION_EPSILON);
  g_assert_cmpint ((int)abs (y_before_hide - y_after_show), <, GIMP_UI_POSITION_EPSILON);
  g_assert_cmpint ((int)abs (w_before_hide - w_after_show), <, GIMP_UI_POSITION_EPSILON);
  g_assert_cmpint ((int)abs (h_before_hide - h_after_show), <, GIMP_UI_POSITION_EPSILON);
}

static void
gimp_ui_switch_to_single_window_mode (GimpTestFixture *fixture,
                                      gconstpointer    data)
{
  Gimp *gimp = GIMP (data);

  /* Switch to single-window mode. We consider this test as passed if
   * we don't get any GLib warnings/errors
   */
  gimp_ui_manager_activate_action (gimp_ui_get_ui_manager (gimp),
                                   "windows",
                                   "windows-use-single-window-mode");
  gimp_test_run_mainloop_until_idle ();
}

static void
gimp_ui_switch_back_to_multi_window_mode (GimpTestFixture *fixture,
                                          gconstpointer    data)
{
  Gimp *gimp = GIMP (data);

  /* Switch back to multi-window mode. We consider this test as passed
   * if we don't get any GLib warnings/errors
   */
  gimp_ui_manager_activate_action (gimp_ui_get_ui_manager (gimp),
                                   "windows",
                                   "windows-use-single-window-mode");
  gimp_test_run_mainloop_until_idle ();
}

static GimpUIManager *
gimp_ui_get_ui_manager (Gimp *gimp)
{
  GimpDisplay       *display      = NULL;
  GimpDisplayShell  *shell        = NULL;
  GtkWidget         *toplevel     = NULL;
  GimpImageWindow   *image_window = NULL;
  GimpUIManager     *ui_manager   = NULL;

  display = GIMP_DISPLAY (gimp_get_empty_display (gimp));

  /* If there were not empty display, assume that there is at least
   * one image display and use that
   */
  if (! display)
    display = GIMP_DISPLAY (gimp_get_display_iter (gimp)->data);

  shell            = gimp_display_get_shell (display);
  toplevel         = gtk_widget_get_toplevel (GTK_WIDGET (shell));
  image_window     = GIMP_IMAGE_WINDOW (toplevel);
  ui_manager       = gimp_image_window_get_ui_manager (image_window);

  return ui_manager;
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
  g_assert (window);

  event = gdk_event_new (GDK_DELETE);
  event->any.window     = g_object_ref (window);
  event->any.send_event = TRUE;
  gtk_main_do_event (event);
  gdk_event_free (event);
}

static GtkWidget *
gimp_ui_find_non_toolbox_dock_window (GimpDialogFactory *dialog_factory)
{
  GList     *iter        = NULL;
  GtkWidget *dock_window = NULL;

  for (iter = gimp_dialog_factory_get_session_infos (dialog_factory);
       iter;
       iter = g_list_next (iter))
    {
      GtkWidget *widget = gimp_session_info_get_widget (iter->data);

      if (GIMP_IS_DOCK_WINDOW (widget) &&
          ! gimp_dock_window_has_toolbox (GIMP_DOCK_WINDOW (widget)))
        {
          dock_window = widget;
          break;
        }
    }

  return dock_window;
}
