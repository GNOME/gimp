/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include <glib.h>
#include <cairo.h>
#include <fontconfig/fontconfig.h>
#include <pango/pango.h>
#include <pango/pangoft2.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#ifndef GIMP_CONSOLE_COMPILATION
#include <gtk/gtk.h>
#endif

#include "libgimpbase/gimpbase.h"

#include "about.h"
#include "version.h"
#include "git-version.h"

#include "gimp-intl.h"


static void
gimp_show_library_version (const gchar *package,
                           gint         build_time_major,
                           gint         build_time_minor,
                           gint         build_time_micro,
                           gint         run_time_major,
                           gint         run_time_minor,
                           gint         run_time_micro)
{
  gchar *build_time_version;
  gchar *run_time_version;

  build_time_version = g_strdup_printf ("%d.%d.%d",
                                        build_time_major,
                                        build_time_minor,
                                        build_time_micro);
  run_time_version = g_strdup_printf ("%d.%d.%d",
                                      run_time_major,
                                      run_time_minor,
                                      run_time_micro);

  /* show versions of libraries used by GIMP */
  g_print (_("using %s version %s (compiled against version %s)"),
           package, run_time_version, build_time_version);
  g_print ("\n");

  g_free (run_time_version);
  g_free (build_time_version);
}

static void
gimp_show_library_versions (void)
{
  gint gegl_major_version;
  gint gegl_minor_version;
  gint gegl_micro_version;

  gegl_get_version (&gegl_major_version,
                    &gegl_minor_version,
                    &gegl_micro_version);

  gimp_show_library_version ("GEGL",
                             GEGL_MAJOR_VERSION,
                             GEGL_MINOR_VERSION,
                             GEGL_MICRO_VERSION,
                             gegl_major_version,
                             gegl_minor_version,
                             gegl_micro_version);

  gimp_show_library_version ("GLib",
                             GLIB_MAJOR_VERSION,
                             GLIB_MINOR_VERSION,
                             GLIB_MICRO_VERSION,
                             glib_major_version,
                             glib_minor_version,
                             glib_micro_version);

  gimp_show_library_version ("GdkPixbuf",
                             GDK_PIXBUF_MAJOR,
                             GDK_PIXBUF_MINOR,
                             GDK_PIXBUF_MICRO,
                             gdk_pixbuf_major_version,
                             gdk_pixbuf_minor_version,
                             gdk_pixbuf_micro_version);

#ifndef GIMP_CONSOLE_COMPILATION
  gimp_show_library_version ("GTK+",
                             GTK_MAJOR_VERSION,
                             GTK_MINOR_VERSION,
                             GTK_MICRO_VERSION,
                             gtk_major_version,
                             gtk_minor_version,
                             gtk_micro_version);
#endif

  gimp_show_library_version ("Pango",
                             PANGO_VERSION_MAJOR,
                             PANGO_VERSION_MINOR,
                             PANGO_VERSION_MICRO,
                             pango_version () / 100 / 100,
                             pango_version () / 100 % 100,
                             pango_version () % 100);

  gimp_show_library_version ("Fontconfig",
                             FC_MAJOR, FC_MINOR, FC_REVISION,
                             FcGetVersion () / 100 / 100,
                             FcGetVersion () / 100 % 100,
                             FcGetVersion () % 100);

  g_print (_("using %s version %s (compiled against version %s)"),
           "Cairo", cairo_version_string (), CAIRO_VERSION_STRING);
  g_print ("\n");
}

void
gimp_version_show (gboolean be_verbose)
{
  g_print (_("%s version %s"), GIMP_NAME, GIMP_VERSION);
  g_print ("\n");

  if (be_verbose)
    {
      g_print ("git-describe: %s", GIMP_GIT_VERSION);
      g_print ("\n");
      g_print ("C compiler:\n%s", CC_VERSION);

      g_print ("\n");
      gimp_show_library_versions ();
    }
}
