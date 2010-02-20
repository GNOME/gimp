/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 2009 Martin Nordholts
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

#include <gegl.h>
#include <gtk/gtk.h>

#include "dialogs/dialogs-types.h"

#include "dialogs/dialogs.h"

#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpdock.h"
#include "widgets/gimpdockwindow.h"
#include "widgets/gimpwidgets-utils.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"

#include "tests.h"

#include "gimp-app-test-utils.h"


typedef struct
{
  int dummy;
} GimpTestFixture;


static void gimp_test_window_roles   (GimpTestFixture *fixture,
                                      gconstpointer    data);


static Gimp *gimp = NULL;


int main(int argc, char **argv)
{
  int test_result;

  g_type_init ();
  gtk_init (&argc, &argv);
  g_test_init (&argc, &argv, NULL);

  gimp_test_utils_set_gimp2_directory ("GIMP_TESTING_ABS_TOP_SRCDIR",
                                       "app/tests/gimpdir-empty");

  /* We share the same application instance across all tests */
  gimp = gimp_init_for_gui_testing (FALSE, FALSE);

  /* Setup the tests */
  g_test_add ("/gimp-window-management/window-roles",
              GimpTestFixture,
              NULL,
              NULL,
              gimp_test_window_roles,
              NULL);

  /* Run the tests and return status */
  test_result = g_test_run ();

  /* Don't write files to the source dir */
  gimp_test_utils_set_gimp2_directory ("GIMP_TESTING_ABS_TOP_BUILDDIR",
                                       "app/tests/gimpdir-output");

  /* Exit somewhat properly to avoid annoying warnings */
  gimp_exit (gimp, TRUE);

  return test_result;
}

/**
 * gimp_test_window_roles:
 * @fixture:
 * @data:
 *
 * Makes sure that different windows have the right roles specified.
 **/
static void
gimp_test_window_roles (GimpTestFixture *fixture,
                        gconstpointer    data)
{
  GtkWidget      *dock           = NULL;
  GtkWidget      *toolbox        = NULL;
  GimpDockWindow *dock_window    = NULL;
  GimpDockWindow *toolbox_window = NULL;

  dock           = gimp_dock_with_window_new (global_dialog_factory,
                                              gdk_screen_get_default (),
                                              FALSE /*toolbox*/);
  toolbox        = gimp_dock_with_window_new (global_dialog_factory,
                                              gdk_screen_get_default (),
                                              TRUE /*toolbox*/);
  dock_window    = gimp_dock_window_from_dock (GIMP_DOCK (dock));
  toolbox_window = gimp_dock_window_from_dock (GIMP_DOCK (toolbox));

  g_assert_cmpstr (gtk_window_get_role (GTK_WINDOW (dock_window)), ==,
                   "gimp-dock");
  g_assert_cmpstr (gtk_window_get_role (GTK_WINDOW (toolbox_window)), ==,
                   "gimp-toolbox");

  /* When we get here we have a ref count of one, but the signals we
   * emit cause the reference count to become less than zero for some
   * reason. So we're lazy and simply ignore to unref these
  g_object_unref (toolbox);
  g_object_unref (dock);
   */
}
