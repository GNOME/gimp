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

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libgimpbase/gimpbase.h"

#include "dialogs/dialogs-types.h"

#include "dialogs/dialogs.h"

#include "display/gimpdisplay.h"
#include "display/gimpimagewindow.h"

#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpdocked.h"
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


typedef struct
{
  int avoid_sizeof_zero;
} GimpTestFixture;


static void            gimp_ui_tool_options_editor_updates      (GimpTestFixture *fixture,
                                                                 gconstpointer    data);
static void            gimp_ui_switch_to_single_window_mode     (GimpTestFixture *fixture,
                                                                 gconstpointer    data);
static void            gimp_ui_switch_back_to_multi_window_mode (GimpTestFixture *fixture,
                                                                 gconstpointer    data);
static void            gimp_ui_create_new_image_via_dialog      (GimpTestFixture *fixture,
                                                                 gconstpointer    data);
static GimpUIManager * gimp_ui_get_ui_manager                   (Gimp            *gimp);


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
  g_test_add ("/gimp-ui/create-new-image-via-dialog",
              GimpTestFixture,
              gimp,
              NULL,
              gimp_ui_create_new_image_via_dialog,
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

static GimpUIManager *
gimp_ui_get_ui_manager (Gimp *gimp)
{
  GimpDisplay       *display          = GIMP_DISPLAY (gimp_get_empty_display (gimp));
  GimpDisplayShell  *shell            = gimp_display_get_shell (display);
  GtkWidget         *toplevel         = gtk_widget_get_toplevel (GTK_WIDGET (shell));
  GimpImageWindow   *image_window     = GIMP_IMAGE_WINDOW (toplevel);
  GimpUIManager     *ui_manager       = gimp_image_window_get_ui_manager (image_window);

  return ui_manager;
}
