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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
#include "git-version.h"

#include "gimp-intl.h"
#include "gimp-version.h"


static gchar * gimp_library_version          (const gchar *package,
                                              gint         build_time_major,
                                              gint         build_time_minor,
                                              gint         build_time_micro,
                                              gint         run_time_major,
                                              gint         run_time_minor,
                                              gint         run_time_micro,
                                              gboolean     localized);
static gchar * gimp_library_versions         (gboolean     localized);
static void    gimp_version_get_release_info (gint        *package_revision,
                                              gboolean    *package_check_update);

void
gimp_version_show (gboolean be_verbose)
{
  gchar *version = gimp_version (be_verbose, TRUE);

  g_print ("%s", version);

  g_free (version);
}

gchar *
gimp_version (gboolean be_verbose,
              gboolean localized)
{
  gchar *version;
  gchar *temp;

  version = g_strdup_printf (localized ? _("%s version %s") : "%s version %s",
                             GIMP_NAME, GIMP_VERSION);;
  temp = g_strconcat (version, "\n", NULL);
  g_free (version);
  version = temp;

  if (be_verbose)
    {
      gchar *verbose_info;
      gchar *lib_versions;
      gchar *flatpak_info = NULL;

      lib_versions = gimp_library_versions (localized);
      verbose_info = g_strdup_printf ("git-describe: %s\n"
                                      "Build: %s rev %d for %s\n"
                                      "# C compiler #\n%s\n"
                                      "# Libraries #\n%s",
                                      GIMP_GIT_VERSION,
                                      GIMP_BUILD_ID,
                                      gimp_version_get_revision (),
                                      GIMP_BUILD_PLATFORM_FAMILY,
                                      CC_VERSION,
                                      lib_versions);
      g_free (lib_versions);

      /* This file should be available at root path in a flatpak
       * environment. Just add its contents to the verbose output if it
       * exists. Silently ignore otherwise.
       */
      if (g_file_get_contents ("/.flatpak-info", &flatpak_info, NULL, NULL))
        {
          temp = g_strdup_printf ("\n# Flatpak info #\n%s",
                                  flatpak_info);
          g_free (flatpak_info);
          flatpak_info = temp;
        }
      temp = g_strconcat (version, verbose_info, flatpak_info, NULL);
      g_free (version);
      g_free (verbose_info);
      g_free (flatpak_info);

      version = temp;
    }

  return version;
}

gint
gimp_version_get_revision (void)
{
  gint revision;

  gimp_version_get_release_info (&revision, NULL);

  return revision;
}

gboolean
gimp_version_check_update (void)
{
  gboolean check_update;

  gimp_version_get_release_info (NULL, &check_update);

  return check_update;
}

static gchar *
gimp_library_version (const gchar *package,
                      gint         build_time_major,
                      gint         build_time_minor,
                      gint         build_time_micro,
                      gint         run_time_major,
                      gint         run_time_minor,
                      gint         run_time_micro,
                      gboolean     localized)
{
  gchar *lib_version;
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
  lib_version = g_strdup_printf (localized ?
                                 _("using %s version %s (compiled against version %s)") :
                                 "using %s version %s (compiled against version %s)",
                                 package, run_time_version, build_time_version);
  g_free (run_time_version);
  g_free (build_time_version);

  return lib_version;
}

static gchar *
gimp_library_versions (gboolean localized)
{
  gchar *lib_versions;
  gchar *lib_version;
  gchar *temp;
  gint   babl_major_version;
  gint   babl_minor_version;
  gint   babl_micro_version;
  gint   gegl_major_version;
  gint   gegl_minor_version;
  gint   gegl_micro_version;

  babl_get_version (&babl_major_version,
                    &babl_minor_version,
                    &babl_micro_version);

  lib_versions = gimp_library_version ("babl",
                                       BABL_MAJOR_VERSION,
                                       BABL_MINOR_VERSION,
                                       BABL_MICRO_VERSION,
                                       babl_major_version,
                                       babl_minor_version,
                                       babl_micro_version,
                                       localized);

  gegl_get_version (&gegl_major_version,
                    &gegl_minor_version,
                    &gegl_micro_version);

  lib_version = gimp_library_version ("GEGL",
                                       GEGL_MAJOR_VERSION,
                                       GEGL_MINOR_VERSION,
                                       GEGL_MICRO_VERSION,
                                       gegl_major_version,
                                       gegl_minor_version,
                                       gegl_micro_version,
                                       localized);

  temp = g_strdup_printf ("%s\n%s", lib_versions, lib_version);
  g_free (lib_versions);
  g_free (lib_version);
  lib_versions = temp;

  lib_version = gimp_library_version ("GLib",
                                      GLIB_MAJOR_VERSION,
                                      GLIB_MINOR_VERSION,
                                      GLIB_MICRO_VERSION,
                                      glib_major_version,
                                      glib_minor_version,
                                      glib_micro_version,
                                      localized);
  temp = g_strdup_printf ("%s\n%s", lib_versions, lib_version);
  g_free (lib_versions);
  g_free (lib_version);
  lib_versions = temp;

  lib_version = gimp_library_version ("GdkPixbuf",
                                      GDK_PIXBUF_MAJOR,
                                      GDK_PIXBUF_MINOR,
                                      GDK_PIXBUF_MICRO,
                                      gdk_pixbuf_major_version,
                                      gdk_pixbuf_minor_version,
                                      gdk_pixbuf_micro_version,
                                      localized);
  temp = g_strdup_printf ("%s\n%s", lib_versions, lib_version);
  g_free (lib_versions);
  g_free (lib_version);
  lib_versions = temp;

#ifndef GIMP_CONSOLE_COMPILATION
  lib_version = gimp_library_version ("GTK+",
                                      GTK_MAJOR_VERSION,
                                      GTK_MINOR_VERSION,
                                      GTK_MICRO_VERSION,
                                      gtk_major_version,
                                      gtk_minor_version,
                                      gtk_micro_version,
                                      localized);
  temp = g_strdup_printf ("%s\n%s", lib_versions, lib_version);
  g_free (lib_versions);
  g_free (lib_version);
  lib_versions = temp;
#endif

  lib_version = gimp_library_version ("Pango",
                                      PANGO_VERSION_MAJOR,
                                      PANGO_VERSION_MINOR,
                                      PANGO_VERSION_MICRO,
                                      pango_version () / 100 / 100,
                                      pango_version () / 100 % 100,
                                      pango_version () % 100,
                                      localized);
  temp = g_strdup_printf ("%s\n%s", lib_versions, lib_version);
  g_free (lib_versions);
  g_free (lib_version);
  lib_versions = temp;

  lib_version = gimp_library_version ("Fontconfig",
                                      FC_MAJOR, FC_MINOR, FC_REVISION,
                                      FcGetVersion () / 100 / 100,
                                      FcGetVersion () / 100 % 100,
                                      FcGetVersion () % 100,
                                      localized);
  temp = g_strdup_printf ("%s\n%s", lib_versions, lib_version);
  g_free (lib_versions);
  g_free (lib_version);
  lib_versions = temp;

  lib_version = g_strdup_printf (localized ?
                                 _("using %s version %s (compiled against version %s)") :
                                 "using %s version %s (compiled against version %s)",
                                 "Cairo", cairo_version_string (), CAIRO_VERSION_STRING);
  temp = g_strdup_printf ("%s\n%s\n", lib_versions, lib_version);
  g_free (lib_versions);
  g_free (lib_version);
  lib_versions = temp;

  return lib_versions;
}

static void
gimp_version_get_release_info (gint     *package_revision,
                               gboolean *package_check_update)
{
  static gint     revision     = -1;
  static gboolean check_update = FALSE;

  if (revision == -1)
    {
      GKeyFile *key_file;
      gchar    *gimp_release;

      revision = 0;
      key_file = g_key_file_new ();

      /* The gimp-release file is inspired by /etc/os-release and similar
       * distribution files. Right now its main use is to number the package
       * revision. This information is not a build variable because a new
       * package version does not necessarily imply a rebuild (maybe just
       * installed data or dependencies change).
       */
      gimp_release = g_build_filename (gimp_data_directory (), "gimp-release", NULL);
      /* Absence of the file is not an error. Actually most third-party
       * builds probably won't install such file.
       */
      if (g_key_file_load_from_file (key_file, gimp_release, G_KEY_FILE_NONE, NULL))
        {
          check_update = TRUE;

          if (g_key_file_has_key (key_file, "package", "revision", NULL))
            revision = g_key_file_get_integer (key_file, "package", "revision", NULL);

          if (g_key_file_has_key (key_file, "package", "check-update", NULL))
            check_update = g_key_file_get_boolean (key_file, "package", "check-update", NULL);
        }
      g_key_file_free (key_file);
      g_free (gimp_release);
    }

  if (package_revision)
    *package_revision = revision;
  if (package_check_update)
    *package_check_update = check_update;
}
