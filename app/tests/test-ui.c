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
#include <string.h>

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

#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpdockable.h"
#include "widgets/gimpdockbook.h"
#include "widgets/gimpdocked.h"
#include "widgets/gimpdockwindow.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpsessioninfo.h"
#include "widgets/gimptoolbox.h"
#include "widgets/gimptooloptionseditor.h"
#include "widgets/gimpuimanager.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimptoolinfo.h"
#include "core/gimptooloptions.h"

#include "tests.h"

#include "gimp-app-test-utils.h"


#define GIMP_UI_WINDOW_POSITION_EPSILON 10
#define GIMP_UI_POSITION_EPSILON        1
#define GIMP_UI_ZOOM_EPSILON            0.01

#define ADD_TEST(function) \
  g_test_add ("/gimp-ui/" #function, \
              GimpTestFixture, \
              gimp, \
              NULL, \
              function, \
              NULL);


typedef gboolean (*GimpUiTestFunc) (GObject *object);


typedef struct
{
  int avoid_sizeof_zero;
} GimpTestFixture;


static GimpUIManager * gimp_ui_get_ui_manager                   (Gimp              *gimp);
static void            gimp_ui_synthesize_delete_event          (GtkWidget         *widget);
static void            gimp_ui_synthesize_key_event             (GtkWidget         *widget,
                                                                 guint              keyval);
static GtkWidget     * gimp_ui_find_dock_window                 (GimpDialogFactory *dialog_factory,
                                                                 GimpUiTestFunc     predicate);
static gboolean        gimp_ui_not_toolbox_window               (GObject           *object);
static gboolean        gimp_ui_multicolumn_not_toolbox_window   (GObject           *object);


/**
 * tool_options_editor_updates:
 * @fixture:
 * @data:
 *
 * Makes sure that the tool options editor is updated when the tool
 * changes.
 **/
static void
tool_options_editor_updates (GimpTestFixture *fixture,
                             gconstpointer    data)
{
  Gimp                  *gimp         = GIMP (data);
  GimpDisplay           *display      = GIMP_DISPLAY (gimp_get_empty_display (gimp));
  GimpDisplayShell      *shell        = gimp_display_get_shell (display);
  GtkWidget             *toplevel     = gtk_widget_get_toplevel (GTK_WIDGET (shell));
  GimpImageWindow       *image_window = GIMP_IMAGE_WINDOW (toplevel);
  GimpUIManager         *ui_manager   = gimp_image_window_get_ui_manager (image_window);
  GtkWidget             *dockable     = gimp_dialog_factory_dialog_new (gimp_dialog_factory_get_singleton (),
                                                                        gtk_widget_get_screen (toplevel),
                                                                        NULL /*ui_manager*/,
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

static GtkWidget *
gimp_ui_get_dialog (const gchar *identifier)
{
  GtkWidget *result = NULL;
  GList     *iter;

  for (iter = gimp_dialog_factory_get_open_dialogs (gimp_dialog_factory_get_singleton ());
       iter;
       iter = g_list_next (iter))
    {
      GtkWidget *dialog = GTK_WIDGET (iter->data);
      GimpDialogFactoryEntry *entry = NULL;

      gimp_dialog_factory_from_widget (dialog, &entry);

      if (strcmp (entry->identifier, identifier) == 0)
        {
          result = dialog;
          break;
        }
    }

  return result;
}

static void
automatic_tab_style (GimpTestFixture *fixture,
                     gconstpointer    data)
{
  GtkWidget    *channel_dockable = gimp_ui_get_dialog ("gimp-channel-list");
  GimpDockable *dockable;
  GimpUIManager *ui_manager;
  g_assert (channel_dockable != NULL);

  dockable = GIMP_DOCKABLE (channel_dockable);

  /* The channel dockable is the only dockable, it has enough space
   * for the icon-blurb
   */
  g_assert_cmpint (GIMP_TAB_STYLE_ICON_BLURB,
                   ==,
                   gimp_dockable_get_actual_tab_style (dockable));

  /* Add some dockables to the channel dockable dockbook */
  ui_manager =
    gimp_dockbook_get_ui_manager (gimp_dockable_get_dockbook (dockable));
  gimp_ui_manager_activate_action (ui_manager,
                                   "dockable",
                                   "dialogs-sample-points");
  gimp_ui_manager_activate_action (ui_manager,
                                   "dockable",
                                   "dialogs-vectors");
  gimp_test_run_mainloop_until_idle ();

  /* Now there is not enough space to have icon-blurb for channel
   * dockable, make sure it's just an icon now
   */
  g_assert_cmpint (GIMP_TAB_STYLE_ICON,
                   ==,
                   gimp_dockable_get_actual_tab_style (dockable));

  /* Close the two dockables we added */
  gimp_ui_manager_activate_action (ui_manager,
                                   "dockable",
                                   "dockable-close-tab");
  gimp_ui_manager_activate_action (ui_manager,
                                   "dockable",
                                   "dockable-close-tab");
  gimp_test_run_mainloop_until_idle ();

  /* We should now be back on icon-blurb */
  g_assert_cmpint (GIMP_TAB_STYLE_ICON_BLURB,
                   ==,
                   gimp_dockable_get_actual_tab_style (dockable));
}

static void
create_new_image_via_dialog (GimpTestFixture *fixture,
                             gconstpointer    data)
{
  Gimp              *gimp             = GIMP (data);
  GimpDisplay       *display          = GIMP_DISPLAY (gimp_get_empty_display (gimp));
  GimpDisplayShell  *shell            = gimp_display_get_shell (display);
  GtkWidget         *toplevel         = gtk_widget_get_toplevel (GTK_WIDGET (shell));
  GimpImageWindow   *image_window     = GIMP_IMAGE_WINDOW (toplevel);
  GimpUIManager     *ui_manager       = gimp_image_window_get_ui_manager (image_window);
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
    gimp_dialog_factory_dialog_raise (gimp_dialog_factory_get_singleton (),
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
keyboard_zoom_focus (GimpTestFixture *fixture,
                     gconstpointer    data)
{
  Gimp              *gimp    = GIMP (data);
  GimpDisplay       *display = GIMP_DISPLAY (gimp_get_display_iter (gimp)->data);
  GimpDisplayShell  *shell   = gimp_display_get_shell (display);
  GimpImageWindow   *window  = gimp_display_shell_get_window (shell);
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
                                   &shell_y_before_zoom,
                                   FALSE /*use_offsets*/);
  gimp_display_shell_push_zoom_focus_pointer_pos (shell,
                                                  shell_x_before_zoom,
                                                  shell_y_before_zoom);
  factor_before_zoom = gimp_zoom_model_get_factor (shell->zoom);

  /* Do the zoom */
  gimp_ui_synthesize_key_event (GTK_WIDGET (window), GDK_plus);
  gimp_test_run_mainloop_until_idle ();

  /* Make sure the zoom focus point remained fixed */
  gimp_display_shell_transform_xy (shell,
                                   image_x,
                                   image_y,
                                   &shell_x_after_zoom,
                                   &shell_y_after_zoom,
                                   FALSE /*use_offsets*/);
  factor_after_zoom = gimp_zoom_model_get_factor (shell->zoom);

  /* First of all make sure a zoom happend at all */
  g_assert_cmpfloat (fabs (factor_before_zoom - factor_after_zoom),
                     >=,
                     GIMP_UI_ZOOM_EPSILON);
  g_assert_cmpint (ABS (shell_x_after_zoom - shell_x_before_zoom),
                   <=,
                   GIMP_UI_POSITION_EPSILON);
  g_assert_cmpint (ABS (shell_y_after_zoom - shell_y_before_zoom),
                   <=,
                   GIMP_UI_POSITION_EPSILON);
}

static void
restore_recently_closed_multi_column_dock (GimpTestFixture *fixture,
                                           gconstpointer    data)
{
  Gimp      *gimp                          = GIMP (data);
  GtkWidget *dock_window                   = NULL;
  gint       n_session_infos_before_close  = -1;
  gint       n_session_infos_after_close   = -1;
  gint       n_session_infos_after_restore = -1;
  GList     *session_infos                 = NULL;

  /* Find a non-toolbox dock window */
  dock_window = gimp_ui_find_dock_window (gimp_dialog_factory_get_singleton (),
                                          gimp_ui_multicolumn_not_toolbox_window);
  g_assert (dock_window != NULL);

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

  /* Restore the (only avaiable) closed dock and make sure the session
   * infos in the global dock factory are increased again
   */
  gimp_ui_manager_activate_action (gimp_ui_get_ui_manager (gimp),
                                   "windows",
                                   /* FIXME: This is severly hardcoded */
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
 * @fixture:
 * @data:
 *
 * Makes sure that when dock windows are hidden with Tab and shown
 * again, their positions and sizes are not changed. We don't really
 * use Tab though, we only simulate its effect.
 **/
static void
tab_toggle_dont_change_dock_window_position (GimpTestFixture *fixture,
                                             gconstpointer    data)
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
  dock_window = gimp_ui_find_dock_window (gimp_dialog_factory_get_singleton (),
                                          gimp_ui_not_toolbox_window);
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
  gimp_ui_manager_activate_action (gimp_ui_get_ui_manager (gimp),
                                   "windows",
                                   "windows-hide-docks");
  gimp_test_run_mainloop_until_idle ();
  g_assert (! gtk_widget_get_visible (dock_window));

  /* Show them again */
  gimp_ui_manager_activate_action (gimp_ui_get_ui_manager (gimp),
                                   "windows",
                                   "windows-hide-docks");
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
  g_assert_cmpint ((int)abs (x_before_hide - x_after_show), <=, GIMP_UI_WINDOW_POSITION_EPSILON);
  g_assert_cmpint ((int)abs (y_before_hide - y_after_show), <=, GIMP_UI_WINDOW_POSITION_EPSILON);
  g_assert_cmpint ((int)abs (w_before_hide - w_after_show), <=, GIMP_UI_WINDOW_POSITION_EPSILON);
  g_assert_cmpint ((int)abs (h_before_hide - h_after_show), <=, GIMP_UI_WINDOW_POSITION_EPSILON);
}

static void
switch_to_single_window_mode (GimpTestFixture *fixture,
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
gimp_ui_toggle_docks_in_single_window_mode (Gimp *gimp)
{
  GimpDisplay      *display       = GIMP_DISPLAY (gimp_get_display_iter (gimp)->data);
  GimpDisplayShell *shell         = gimp_display_get_shell (display);
  GtkWidget        *toplevel      = GTK_WIDGET (gimp_display_shell_get_window (shell));
  gint              x_temp        = -1;
  gint              y_temp        = -1;
  gint              x_before_hide = -1;
  gint              y_before_hide = -1;
  gint              x_after_hide  = -1;
  gint              y_after_hide  = -1;
  g_assert (shell);
  g_assert (toplevel);

  /* Get toplevel coordinate of image origin */
  gimp_test_run_mainloop_until_idle ();
  gimp_display_shell_transform_xy (shell,
                                   0.0, 0.0,
                                   &x_temp, &y_temp,
                                   FALSE /*use_offsets*/);
  gtk_widget_translate_coordinates (GTK_WIDGET (shell),
                                    toplevel,
                                    x_temp, y_temp,
                                    &x_before_hide, &y_before_hide);

  /* Hide all dock windows */
  gimp_ui_manager_activate_action (gimp_ui_get_ui_manager (gimp),
                                   "windows",
                                   "windows-hide-docks");
  gimp_test_run_mainloop_until_idle ();

  /* Get toplevel coordinate of image origin */
  gimp_test_run_mainloop_until_idle ();
  gimp_display_shell_transform_xy (shell,
                                   0.0, 0.0,
                                   &x_temp, &y_temp,
                                   FALSE /*use_offsets*/);
  gtk_widget_translate_coordinates (GTK_WIDGET (shell),
                                    toplevel,
                                    x_temp, y_temp,
                                    &x_after_hide, &y_after_hide);

  g_assert_cmpint ((int)abs (x_after_hide - x_before_hide), <=, GIMP_UI_POSITION_EPSILON);
  g_assert_cmpint ((int)abs (y_after_hide - y_before_hide), <=, GIMP_UI_POSITION_EPSILON);
}

static void
hide_docks_in_single_window_mode (GimpTestFixture *fixture,
                                  gconstpointer   data)
{
  Gimp *gimp = GIMP (data);
  gimp_ui_toggle_docks_in_single_window_mode (gimp);
}

static void
show_docks_in_single_window_mode (GimpTestFixture *fixture,
                                  gconstpointer    data)
{
  Gimp *gimp = GIMP (data);
  gimp_ui_toggle_docks_in_single_window_mode (gimp);
}

static void
switch_back_to_multi_window_mode (GimpTestFixture *fixture,
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

static void
gimp_ui_synthesize_key_event (GtkWidget *widget,
                              guint      keyval)
{
  gdk_test_simulate_key (gtk_widget_get_window (widget),
                         -1, -1, /*x, y*/
                         keyval,
                         0 /*modifiers*/,
                         GDK_KEY_PRESS);
  gdk_test_simulate_key (gtk_widget_get_window (widget),
                         -1, -1, /*x, y*/
                         keyval,
                         0 /*modifiers*/,
                         GDK_KEY_RELEASE);
}

static GtkWidget *
gimp_ui_find_dock_window (GimpDialogFactory *dialog_factory,
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

      if (GIMP_IS_DOCK_WINDOW (widget) &&
          predicate (G_OBJECT (widget)))
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
  return ! gimp_dock_window_has_toolbox (GIMP_DOCK_WINDOW (object));
}

static gboolean
gimp_ui_multicolumn_not_toolbox_window (GObject *object)
{
  GimpDockWindow *dock_window = GIMP_DOCK_WINDOW (object);

  return (! gimp_dock_window_has_toolbox (dock_window) &&
          g_list_length (gimp_dock_window_get_docks (dock_window)) > 1);
}

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

  /* Add tests */
  ADD_TEST (tool_options_editor_updates);
  ADD_TEST (automatic_tab_style);
  ADD_TEST (create_new_image_via_dialog);
  ADD_TEST (keyboard_zoom_focus);
  ADD_TEST (restore_recently_closed_multi_column_dock);
  ADD_TEST (tab_toggle_dont_change_dock_window_position);
  ADD_TEST (switch_to_single_window_mode);
  ADD_TEST (hide_docks_in_single_window_mode);
  ADD_TEST (show_docks_in_single_window_mode);
  ADD_TEST (switch_back_to_multi_window_mode);

  /* Run the tests and return status */
  result = g_test_run ();

  /* Don't write files to the source dir */
  gimp_test_utils_set_gimp2_directory ("GIMP_TESTING_ABS_TOP_BUILDDIR",
                                       "app/tests/gimpdir-output");

  /* Exit properly so we don't break script-fu plug-in wire */
  gimp_exit (gimp, TRUE);

  return result;
}
