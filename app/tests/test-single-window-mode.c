/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * test-single-window-mode.c
 * Copyright (C) 2011 Martin Nordholts <martinn@src.gnome.org>
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

#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpdock.h"
#include "widgets/gimpdockable.h"
#include "widgets/gimpdockbook.h"
#include "widgets/gimpdockcontainer.h"
#include "widgets/gimpdocked.h"
#include "widgets/gimpdockwindow.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpsessioninfo.h"
#include "widgets/gimptoolbox.h"
#include "widgets/gimptooloptionseditor.h"
#include "widgets/gimpuimanager.h"
#include "widgets/gimpwidgets-utils.h"

#include "core/gimp.h"
#include "core/gimpchannel.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimplayer.h"
#include "core/gimptoolinfo.h"
#include "core/gimptooloptions.h"

#include "gimpcoreapp.h"

#include "gimp-app-test-utils.h"
#include "tests.h"


#define ADD_TEST(function) \
  g_test_add_data_func ("/gimp-single-window-mode/" #function, gimp, function);


/* Put this in the code below when you want the test to pause so you
 * can do measurements of widgets on the screen for example
 */
#define GIMP_PAUSE (g_usleep (20 * 1000 * 1000))


/**
 * new_dockable_not_in_new_window:
 * @data:
 *
 * Test that in single-window mode, new dockables are not put in new
 * windows (they should end up in the single image window).
 **/
static void
new_dockable_not_in_new_window (gconstpointer data)
{
  Gimp              *gimp             = GIMP (data);
  GimpDialogFactory *factory          = gimp_dialog_factory_get_singleton ();
  gint               dialogs_before   = 0;
  gint               toplevels_before = 0;
  gint               dialogs_after    = 0;
  gint               toplevels_after  = 0;
  GList             *dialogs;
  GList             *iter;

  gimp_test_run_mainloop_until_idle ();

  /* Count dialogs before we create the dockable */
  dialogs        = gimp_dialog_factory_get_open_dialogs (factory);
  dialogs_before = g_list_length (dialogs);
  for (iter = dialogs; iter; iter = g_list_next (iter))
    {
      if (gtk_widget_is_toplevel (iter->data))
        toplevels_before++;
    }

  /* Create a dockable */
  gimp_ui_manager_activate_action (gimp_test_utils_get_ui_manager (gimp),
                                   "dialogs",
                                   "dialogs-undo-history");
  gimp_test_run_mainloop_until_idle ();

  /* Count dialogs after we created the dockable */
  dialogs       = gimp_dialog_factory_get_open_dialogs (factory);
  dialogs_after = g_list_length (dialogs);
  for (iter = dialogs; iter; iter = g_list_next (iter))
    {
      if (gtk_widget_is_toplevel (iter->data))
        toplevels_after++;
    }

  /* We got one more session managed dialog ... */
  g_assert_cmpint (dialogs_before + 1, ==, dialogs_after);
  /* ... but no new toplevels */
  g_assert_cmpint (toplevels_before, ==, toplevels_after);
}

int main(int argc, char **argv)
{
  Gimp  *gimp   = NULL;
  gint   result = -1;

  gimp_test_bail_if_no_display ();
  gtk_test_init (&argc, &argv, NULL);

  gimp_test_utils_setup_menus_path ();

  /* Launch GIMP in single-window mode */
  g_setenv ("GIMP_TESTING_SESSIONRC_NAME", "sessionrc-2-8-single-window", TRUE /*overwrite*/);
  gimp = gimp_init_for_gui_testing (TRUE /*show_gui*/);
  gimp_test_run_mainloop_until_idle ();

  ADD_TEST (new_dockable_not_in_new_window);

  /* Run the tests and return status */
  g_application_run (gimp->app, 0, NULL);
  result = gimp_core_app_get_exit_status (GIMP_CORE_APP (gimp->app));

  /* Exit properly so we don't break script-fu plug-in wire */
  g_application_quit (G_APPLICATION (gimp->app));
  g_clear_object (&gimp->app);

  return result;
}
