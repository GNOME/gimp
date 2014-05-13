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
#include "tests.h"
#include "units.h"


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
 * selected subset of the initialization happning in app_run().
 **/
Gimp *
gimp_init_for_testing (void)
{
  Gimp *gimp;

  gimp_log_init ();
  gegl_init (NULL, NULL);

  gimp = gimp_new ("Unit Tested GIMP", NULL, NULL, FALSE, TRUE, TRUE, TRUE,
                   FALSE, FALSE, TRUE, TRUE, FALSE);

  units_init (gimp);

  gimp_load_config (gimp, NULL, NULL);

  gimp_gegl_init (gimp);
  gimp_initialize (gimp, gimp_status_func_dummy);
  gimp_restore (gimp, gimp_status_func_dummy);

  return gimp;
}


#ifndef GIMP_CONSOLE_COMPILATION

static void
gimp_init_icon_theme_for_testing (void)
{
  const gchar *top_srcdir = g_getenv ("GIMP_TESTING_ABS_TOP_SRCDIR");
  gchar       *icon_root;
  gchar       *link_name;
  gchar       *link_target;
  gint         i;

  static const gchar *sizes[] = { "12x12", "16x16", "18x18", "20x20", "22x22",
                                  "24x24", "32x32", "48x48", "64x64" };

  if (! top_srcdir)
    {
      g_printerr ("*\n"
                  "*  The env var GIMP_TESTING_ABS_TOP_SRCDIR is not set,\n"
                  "*  you are probably running in a debugger.\n"
                  "*  Set it manually, e.g.:\n"
                  "*\n"
                  "*    set env GIMP_TESTING_ABS_TOP_SRCDIR=%s/source/gimp\n"
                  "*\n",
                  g_get_home_dir ());
      return;
    }

  icon_root = g_dir_make_tmp ("gimp-test-icon-theme-XXXXXX", NULL);
  if (! icon_root)
    return;

  for (i = 0; i < G_N_ELEMENTS (sizes); i++)
    {
      gchar *icon_dir;

      icon_dir = g_build_filename (icon_root, "hicolor", sizes[i], NULL);
      g_mkdir_with_parents (icon_dir, 0700);

      link_name   = g_build_filename (icon_dir, "apps", NULL);
      link_target = g_build_filename (top_srcdir, "icons", sizes[i] + 3, NULL);

      symlink (link_target, link_name);

      g_free (link_target);
      g_free (link_name);

      g_free (icon_dir);
    }

  link_name   = g_build_filename (icon_root, "hicolor", "index.theme", NULL);
  link_target = g_build_filename (top_srcdir, "icons", "index.theme", NULL);

  symlink (link_target, link_name);

  g_free (link_target);
  g_free (link_name);

  gtk_icon_theme_prepend_search_path (gtk_icon_theme_get_default (),
                                      icon_root);

  g_free (icon_root);
}

static Gimp *
gimp_init_for_gui_testing_internal (gboolean     show_gui,
                                    const gchar *gimprc)
{
  GimpSessionInfoClass *klass;
  Gimp                 *gimp;

  /* from main() */
  gimp_log_init ();
  gegl_init (NULL, NULL);

  /* Introduce an error margin for positions written to sessionrc */
  klass = g_type_class_ref (GIMP_TYPE_SESSION_INFO);
  gimp_session_info_class_set_position_accuracy (klass, 5);

  /* from app_run() */
  gimp = gimp_new ("Unit Tested GIMP", NULL, NULL, FALSE, TRUE, TRUE, !show_gui,
                   FALSE, FALSE, TRUE, TRUE, FALSE);
  gimp_set_show_gui (gimp, show_gui);
  units_init (gimp);
  gimp_load_config (gimp, gimprc, NULL);
  gimp_gegl_init (gimp);
  gui_init (gimp, TRUE);
  gimp_init_icon_theme_for_testing ();
  gimp_initialize (gimp, gimp_status_func_dummy);
  gimp_restore (gimp, gimp_status_func_dummy);

  g_type_class_unref (klass);

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
gimp_init_for_gui_testing_with_rc (gboolean     show_gui,
                                   const gchar *gimprc)
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
      exit (EXIT_SUCCESS);
    }
}
