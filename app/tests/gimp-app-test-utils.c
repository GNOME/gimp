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

#include <glib.h>

#include "gimp-app-test-utils.h"

/**
 * gimp_test_utils_set_gimp2_directory:
 * @subdir:
 *
 * Sets GIMP2_DIRECTORY to the source dir ./app/tests/@subdir. Make
 * sure to run it before using any of the GIMP functions.
 **/
void
gimp_test_utils_set_gimp2_directory (const gchar *subdir)
{
  gchar *gimpdir = NULL;

  /* GIMP_TESTING_ABS_TOP_SRCDIR is set by the automake test runner,
   * see Makefile.am
   */
  gimpdir = g_build_filename (g_getenv ("GIMP_TESTING_ABS_TOP_SRCDIR"),
                              "app/tests",
                              subdir,
                              NULL);

  g_setenv ("GIMP2_DIRECTORY", gimpdir, TRUE);

  g_free (gimpdir);
}

/**
 * gimp_test_utils_setup_menus_dir:
 *
 * Sets GIMP_TESTING_MENUS_DIR to "$top_srcdir/menus".
 **/
void
gimp_test_utils_setup_menus_dir (void)
{
  gchar *menus_dir = NULL;

  menus_dir = gimp_test_utils_get_source_subdir ("menus");

  g_setenv ("GIMP_TESTING_MENUS_DIR", menus_dir, TRUE);

  g_free (menus_dir);
}

/**
 * gimp_test_utils_get_source_subdir:
 * @subdir:
 *
 * Returns: The source directory $top_srcdir/@subdir.
 **/
gchar *
gimp_test_utils_get_source_subdir (const gchar *subdir)
{
  /* GIMP_TESTING_ABS_TOP_SRCDIR is set by the automake test runner,
   * see Makefile.am
   */
  return g_build_filename (g_getenv ("GIMP_TESTING_ABS_TOP_SRCDIR"),
                           subdir,
                           NULL);
}
