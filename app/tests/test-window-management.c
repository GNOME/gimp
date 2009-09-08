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

#include "core/gimp.h"
#include "core/gimpcontext.h"

#include "tests.h"


typedef struct
{
  int dummy;
} GimpTestFixture;


static void gimp_test_window_roles   (GimpTestFixture *fixture,
                                      gconstpointer    data);


static Gimp *gimp = NULL;


int main(int argc, char **argv)
{
  g_type_init ();
  gtk_init (&argc, &argv);
  g_test_init (&argc, &argv, NULL);

  /* We share the same application instance across all tests */
  gimp = gimp_init_for_gui_testing (FALSE);

  /* Setup the tests */
  g_test_add ("/gimp-window-management/window-roles",
              GimpTestFixture,
              NULL,
              NULL,
              gimp_test_window_roles,
              NULL);

  /* Run the tests and return status */
  return g_test_run ();
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
  GtkWidget *dock    = gimp_dialog_factory_dock_new (global_dock_factory,
                                                     gdk_screen_get_default ());
  GtkWidget *toolbox = gimp_dialog_factory_dock_new (global_toolbox_factory,
                                                     gdk_screen_get_default ());

  g_assert_cmpstr (gtk_window_get_role (GTK_WINDOW (toolbox)), ==,
                   "gimp-toolbox");
  g_assert_cmpstr (gtk_window_get_role (GTK_WINDOW (dock)), ==,
                   "gimp-dock");

  /* When we get here we have a ref count of one, but the signals we
   * emit cause the reference count to become less than zero for some
   * reason. So we're lazy and simply ignore to unref these
  g_object_unref (toolbox);
  g_object_unref (dock);
   */
}
