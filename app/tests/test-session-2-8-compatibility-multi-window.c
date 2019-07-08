/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 2011 Martin Nordholts
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

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "dialogs/dialogs-types.h"

#include "tests.h"

#include "gimp-test-session-utils.h"
#include "gimp-app-test-utils.h"


#define ADD_TEST(function) \
  g_test_add_func ("/gimp-session-2-8-compatibility-multi-window/" #function, \
                   function);

#define SKIP_TEST(function) \
  g_test_add_func ("/gimp-session-2-8-compatibility-multi-window/subprocess/" #function, \
                   function);


/**
 * Tests that a single-window sessionrc in GIMP 2.8 format is loaded
 * and written (thus also interpreted) like we expect.
 **/
static void
read_and_write_session_files (void)
{
  gimp_test_session_load_and_write_session_files ("sessionrc-2-8-multi-window",
                                                  "dockrc-2-8",
                                                  "sessionrc-expected-multi-window",
                                                  "dockrc-expected",
                                                  FALSE /*single_window_mode*/);
}

int main(int argc, char **argv)
{
  gimp_test_bail_if_no_display ();
  gtk_test_init (&argc, &argv, NULL);

#ifdef HAVE_XVFB_RUN
  ADD_TEST (read_and_write_session_files);
#else
  SKIP_TEST (read_and_write_session_files);
#endif

  /* Don't bother freeing stuff, the process is short-lived */
#ifdef HAVE_XVFB_RUN
  return g_test_run ();
#else
  return GIMP_EXIT_TEST_SKIPPED;
#endif
}
