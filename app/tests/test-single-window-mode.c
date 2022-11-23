/* LIGMA - The GNU Image Manipulation Program
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
#include "widgets/ligmatoolbox.h"
#include "widgets/ligmatooloptionseditor.h"
#include "widgets/ligmauimanager.h"
#include "widgets/ligmawidgets-utils.h"

#include "core/ligma.h"
#include "core/ligmachannel.h"
#include "core/ligmacontext.h"
#include "core/ligmaimage.h"
#include "core/ligmalayer.h"
#include "core/ligmatoolinfo.h"
#include "core/ligmatooloptions.h"

#include "tests.h"

#include "ligma-app-test-utils.h"


#define ADD_TEST(function) \
  g_test_add_data_func ("/ligma-single-window-mode/" #function, ligma, function);


/* Put this in the code below when you want the test to pause so you
 * can do measurements of widgets on the screen for example
 */
#define LIGMA_PAUSE (g_usleep (20 * 1000 * 1000))


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
  Ligma              *ligma             = LIGMA (data);
  LigmaDialogFactory *factory          = ligma_dialog_factory_get_singleton ();
  gint               dialogs_before   = 0;
  gint               toplevels_before = 0;
  gint               dialogs_after    = 0;
  gint               toplevels_after  = 0;
  GList             *dialogs;
  GList             *iter;

  ligma_test_run_mainloop_until_idle ();

  /* Count dialogs before we create the dockable */
  dialogs        = ligma_dialog_factory_get_open_dialogs (factory);
  dialogs_before = g_list_length (dialogs);
  for (iter = dialogs; iter; iter = g_list_next (iter))
    {
      if (gtk_widget_is_toplevel (iter->data))
        toplevels_before++;
    }

  /* Create a dockable */
  ligma_ui_manager_activate_action (ligma_test_utils_get_ui_manager (ligma),
                                   "dialogs",
                                   "dialogs-undo-history");
  ligma_test_run_mainloop_until_idle ();

  /* Count dialogs after we created the dockable */
  dialogs       = ligma_dialog_factory_get_open_dialogs (factory);
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
  Ligma  *ligma   = NULL;
  gint   result = -1;

  ligma_test_bail_if_no_display ();
  gtk_test_init (&argc, &argv, NULL);

  ligma_test_utils_set_ligma3_directory ("LIGMA_TESTING_ABS_TOP_SRCDIR",
                                       "app/tests/ligmadir");
  ligma_test_utils_setup_menus_path ();

  /* Launch LIGMA in single-window mode */
  g_setenv ("LIGMA_TESTING_SESSIONRC_NAME", "sessionrc-2-8-single-window", TRUE /*overwrite*/);
  ligma = ligma_init_for_gui_testing (TRUE /*show_gui*/);
  ligma_test_run_mainloop_until_idle ();

  ADD_TEST (new_dockable_not_in_new_window);

  /* Run the tests and return status */
  result = g_test_run ();

  /* Don't write files to the source dir */
  ligma_test_utils_set_ligma3_directory ("LIGMA_TESTING_ABS_TOP_BUILDDIR",
                                       "app/tests/ligmadir-output");

  /* Exit properly so we don't break script-fu plug-in wire */
  ligma_exit (ligma, TRUE);

  return result;
}
