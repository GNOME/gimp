/* LIGMA - The GNU Image Manipulation Program
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

#include "widgets/ligmasessioninfo.h"

#include "config/ligmageglconfig.h"

#include "core/ligma.h"
#include "core/ligma-contexts.h"

#include "gegl/ligma-gegl.h"

#include "ligma-log.h"
#include "tests.h"

#ifdef GDK_WINDOWING_QUARTZ
#include <Cocoa/Cocoa.h>
#endif


static void
ligma_status_func_dummy (const gchar *text1,
                        const gchar *text2,
                        gdouble      percentage)
{
}

/**
 * ligma_init_for_testing:
 *
 * Initialize the LIGMA object system for unit testing. This is a
 * selected subset of the initialization happening in app_run().
 **/
Ligma *
ligma_init_for_testing (void)
{
  Ligma *ligma;

  ligma_log_init ();
  gegl_init (NULL, NULL);

  ligma = ligma_new ("Unit Tested LIGMA", NULL, NULL, FALSE, TRUE, TRUE, TRUE,
                   FALSE, FALSE, TRUE, FALSE, FALSE,
                   LIGMA_STACK_TRACE_QUERY, LIGMA_PDB_COMPAT_OFF);

  ligma_load_config (ligma, NULL, NULL);

  ligma_gegl_init (ligma);
  ligma_initialize (ligma, ligma_status_func_dummy);
  ligma_restore (ligma, ligma_status_func_dummy, NULL);

  return ligma;
}


#ifndef LIGMA_CONSOLE_COMPILATION

static void
ligma_init_icon_theme_for_testing (void)
{
  gchar       *icon_root;

  icon_root = g_test_build_filename (G_TEST_BUILT, "ligma-test-icon-theme", NULL);
  gtk_icon_theme_prepend_search_path (gtk_icon_theme_get_default (),
                                      icon_root);
  g_free (icon_root);
  return;
}

#ifdef GDK_WINDOWING_QUARTZ
static gboolean
ligma_osx_focus_window (gpointer user_data)
{
  [NSApp activateIgnoringOtherApps:YES];
  return FALSE;
}
#endif

static Ligma *
ligma_init_for_gui_testing_internal (gboolean  show_gui,
                                    GFile    *ligmarc)
{
  Ligma *ligma;

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
  ligma_log_init ();
  gegl_init (NULL, NULL);

  /* Introduce an error margin for positions written to sessionrc */
  ligma_session_info_set_position_accuracy (5);

  /* from app_run() */
  ligma = ligma_new ("Unit Tested LIGMA", NULL, NULL, FALSE, TRUE, TRUE, !show_gui,
                   FALSE, FALSE, TRUE, FALSE, FALSE,
                   LIGMA_STACK_TRACE_QUERY, LIGMA_PDB_COMPAT_OFF);

  ligma_set_show_gui (ligma, show_gui);
  ligma_load_config (ligma, ligmarc, NULL);
  ligma_gegl_init (ligma);
  gui_init (ligma, TRUE, NULL, g_getenv ("LIGMA_TESTING_ABS_TOP_SRCDIR"));
  ligma_init_icon_theme_for_testing ();
  ligma_initialize (ligma, ligma_status_func_dummy);
  ligma_restore (ligma, ligma_status_func_dummy, NULL);
#ifdef GDK_WINDOWING_QUARTZ
  g_idle_add (ligma_osx_focus_window, NULL);
#endif

  return ligma;
}

/**
 * ligma_init_for_gui_testing:
 * @show_gui:
 *
 * Initializes a #Ligma instance for use in test cases that rely on GUI
 * code to be initialized.
 *
 * Returns: The #Ligma instance.
 **/
Ligma *
ligma_init_for_gui_testing (gboolean show_gui)
{
  return ligma_init_for_gui_testing_internal (show_gui, NULL);
}

/**
 * ligma_init_for_gui_testing:
 * @show_gui:
 * @ligmarc:
 *
 * Like ligma_init_for_gui_testing(), but also allows a custom ligmarc
 * filename to be specified.
 *
 * Returns: The #Ligma instance.
 **/
Ligma *
ligma_init_for_gui_testing_with_rc (gboolean  show_gui,
                                   GFile    *ligmarc)
{
  return ligma_init_for_gui_testing_internal (show_gui, ligmarc);
}

#endif /* LIGMA_CONSOLE_COMPILATION */

static gboolean
ligma_tests_quit_mainloop (GMainLoop *loop)
{
  g_main_loop_quit (loop);

  return FALSE;
}

/**
 * ligma_test_run_temp_mainloop:
 * @running_time: The time to run the main loop.
 *
 * Helper function for tests that wants to run a main loop for a
 * while. Useful when you want LIGMA's state to settle before doing
 * tests.
 **/
void
ligma_test_run_temp_mainloop (guint32 running_time)
{
  GMainLoop *loop;
  loop = g_main_loop_new (NULL, FALSE);

  g_timeout_add (running_time,
                 (GSourceFunc) ligma_tests_quit_mainloop,
                 loop);

  g_main_loop_run (loop);

  g_main_loop_unref (loop);
}

/**
 * ligma_test_run_mainloop_until_idle:
 *
 * Creates and runs a main loop until it is idle, i.e. has no more
 * work to do.
 **/
void
ligma_test_run_mainloop_until_idle (void)
{
  GMainLoop *loop = g_main_loop_new (NULL, FALSE);

  g_idle_add ((GSourceFunc) ligma_tests_quit_mainloop, loop);

  g_main_loop_run (loop);

  g_main_loop_unref (loop);
}

/**
 * ligma_test_bail_if_no_display:
 * @void:
 *
 * If no DISPLAY is set, call exit(EXIT_SUCCESS). There is no use in
 * having UI tests failing in DISPLAY-less environments.
 **/
void
ligma_test_bail_if_no_display (void)
{
  if (! g_getenv ("DISPLAY"))
    {
      g_message ("No DISPLAY set, not running UI tests\n");
      exit (EXIT_SUCCESS);
    }
}
