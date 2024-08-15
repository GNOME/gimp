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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <stdlib.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "gui/gui-types.h"

#include "gui/gui.h"

#include "actions/actions.h"

#include "menus/menus.h"

#include "widgets/gimpsessioninfo.h"

#include "config/gimpgeglconfig.h"

#include "core/gimp.h"
#include "core/gimp-contexts.h"

#include "gegl/gimp-gegl.h"

#include "gimp-log.h"
#include "gimpcoreapp.h"

#include "gimp-app-test-utils.h"
#include "tests.h"

#ifdef GDK_WINDOWING_QUARTZ
#include <Cocoa/Cocoa.h>
#endif


static void
gimp_status_func_dummy (const gchar *text1,
                        const gchar *text2,
                        gdouble      percentage)
{
}

/**
 * gimp_init_for_testing:
 *
 * Initialize the GIMP object system for unit testing. This is a
 * selected subset of the initialization happening in app_run().
 **/
Gimp *
gimp_init_for_testing (void)
{
  Gimp *gimp;

  gimp_log_init ();
  gegl_init (NULL, NULL);

  gimp = gimp_new ("Unit Tested GIMP", NULL, NULL, FALSE, TRUE, TRUE, TRUE,
                   FALSE, FALSE, TRUE, FALSE, FALSE,
                   GIMP_STACK_TRACE_QUERY, GIMP_PDB_COMPAT_OFF);

  gimp_load_config (gimp, NULL, NULL);

  gimp_gegl_init (gimp);
  gimp_initialize (gimp, gimp_status_func_dummy);
  gimp_restore (gimp, gimp_status_func_dummy, NULL);

  return gimp;
}


#ifndef GIMP_CONSOLE_COMPILATION

static void
gimp_init_icon_theme_for_testing (void)
{
  gchar       *icon_root;

  icon_root = g_test_build_filename (G_TEST_BUILT, "gimp-test-icon-theme", NULL);
  gtk_icon_theme_prepend_search_path (gtk_icon_theme_get_default (),
                                      icon_root);
  g_free (icon_root);
  return;
}

#ifdef GDK_WINDOWING_QUARTZ
static gboolean
gimp_osx_focus_window (gpointer user_data)
{
  [NSApp activateIgnoringOtherApps:YES];
  return FALSE;
}
#endif

static void
gimp_test_app_activate_callback (GimpCoreApp *app,
                                 gpointer     user_data)
{
  Gimp *gimp = NULL;

  g_return_if_fail (GIMP_IS_CORE_APP (app));

  gimp = gimp_core_app_get_gimp (app);

  gimp_core_app_set_exit_status (app, EXIT_SUCCESS);

  gui_init (gimp, TRUE, NULL, g_getenv ("GIMP_TESTING_ABS_TOP_SRCDIR"), "System Language");
  gimp_init_icon_theme_for_testing ();
  gimp_initialize (gimp, gimp_status_func_dummy);
  gimp_restore (gimp, gimp_status_func_dummy, NULL);
#ifdef GDK_WINDOWING_QUARTZ
  g_idle_add (gimp_osx_focus_window, NULL);
#endif
  gimp->initialized = TRUE;
  gimp_core_app_set_exit_status (app, g_test_run ());

  /* Don't write files to the source dir */
  gimp_test_utils_set_gimp3_directory ("GIMP_TESTING_ABS_TOP_BUILDDIR",
                                       "app/tests/gimpdir-output");

  gimp_exit (gimp, TRUE);
}

static Gimp *
gimp_init_for_gui_testing_internal (gboolean  show_gui,
                                    GFile    *gimprc)
{
  Gimp *gimp;

  /* Load configuration from the source dir */
  gimp_test_utils_set_gimp3_directory ("GIMP_TESTING_ABS_TOP_SRCDIR",
                                       "app/tests/gimpdir");

#if defined (G_OS_WIN32)
  /* g_test_init() sets warnings always fatal, which is a usually a good
     testing default. Nevertheless the Windows platform may have a few
     quirks generating warnings, yet we want to finish tests. So we
     allow some relaxed rules on this platform. */

  GLogLevelFlags fatal_mask;

  fatal_mask = (GLogLevelFlags) (G_LOG_FATAL_MASK | G_LOG_LEVEL_CRITICAL);
  g_log_set_always_fatal (fatal_mask);
#endif

  /* from main() */
  gimp_log_init ();
  gegl_init (NULL, NULL);

  /* Introduce an error margin for positions written to sessionrc */
  gimp_session_info_set_position_accuracy (5);

  /* from app_run() */
  gimp = gimp_new ("Unit Tested GIMP", NULL, NULL, FALSE, TRUE, TRUE, !show_gui,
                   FALSE, FALSE, TRUE, FALSE, FALSE,
                   GIMP_STACK_TRACE_QUERY, GIMP_PDB_COMPAT_OFF);
  gimp->app = gimp_app_new (gimp, TRUE, FALSE, FALSE, NULL, NULL, NULL);

  gimp_set_show_gui (gimp, show_gui);
  gimp_load_config (gimp, gimprc, NULL);
  gimp_gegl_init (gimp);

  g_signal_connect (gimp->app, "activate",
                    G_CALLBACK (gimp_test_app_activate_callback),
                    NULL);
  return gimp;
}

/**
 * gimp_init_for_gui_testing:
 * @show_gui:
 *
 * Initializes a #Gimp instance for use in test cases that rely on GUI
 * code to be initialized.
 *
 * Returns: The #Gimp instance.
 **/
Gimp *
gimp_init_for_gui_testing (gboolean show_gui)
{
  return gimp_init_for_gui_testing_internal (show_gui, NULL);
}

/**
 * gimp_init_for_gui_testing:
 * @show_gui:
 * @gimprc:
 *
 * Like gimp_init_for_gui_testing(), but also allows a custom gimprc
 * filename to be specified.
 *
 * Returns: The #Gimp instance.
 **/
Gimp *
gimp_init_for_gui_testing_with_rc (gboolean  show_gui,
                                   GFile    *gimprc)
{
  return gimp_init_for_gui_testing_internal (show_gui, gimprc);
}

#endif /* GIMP_CONSOLE_COMPILATION */

static gboolean
gimp_tests_quit_mainloop (GMainLoop *loop)
{
  g_main_loop_quit (loop);

  return FALSE;
}

/**
 * gimp_test_run_temp_mainloop:
 * @running_time: The time to run the main loop.
 *
 * Helper function for tests that wants to run a main loop for a
 * while. Useful when you want GIMP's state to settle before doing
 * tests.
 **/
void
gimp_test_run_temp_mainloop (guint32 running_time)
{
  GMainLoop *loop;
  loop = g_main_loop_new (NULL, FALSE);

  g_timeout_add (running_time,
                 (GSourceFunc) gimp_tests_quit_mainloop,
                 loop);

  g_main_loop_run (loop);

  g_main_loop_unref (loop);
}

/**
 * gimp_test_run_mainloop_until_idle:
 *
 * Creates and runs a main loop until it is idle, i.e. has no more
 * work to do.
 **/
void
gimp_test_run_mainloop_until_idle (void)
{
  GMainLoop *loop = g_main_loop_new (NULL, FALSE);

  g_idle_add ((GSourceFunc) gimp_tests_quit_mainloop, loop);

  g_main_loop_run (loop);

  g_main_loop_unref (loop);
}

/**
 * gimp_test_bail_if_no_display:
 * @void:
 *
 * If no DISPLAY is set, call exit(EXIT_SUCCESS). There is no use in
 * having UI tests failing in DISPLAY-less environments.
 **/
void
gimp_test_bail_if_no_display (void)
{
  if (! g_getenv ("DISPLAY"))
    {
      g_message ("No DISPLAY set, not running UI tests\n");
      exit (GIMP_EXIT_TEST_SKIPPED);
    }
}
