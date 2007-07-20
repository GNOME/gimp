/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimp-user-install.c
 * Copyright (C) 2000-2006 Michael Natterer and Sven Neumann
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <glib/gstdio.h>

#include <glib-object.h>

#ifdef G_OS_WIN32
#include <libgimpbase/gimpwin32-io.h>
#endif

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "config/gimpconfig-file.h"
#include "config/gimprc.h"

#include "gimp-templates.h"
#include "gimp-user-install.h"

#include "gimp-intl.h"


struct _GimpUserInstall
{
  gboolean                verbose;

  gchar                  *old_dir;
  gint                    old_major;
  gint                    old_minor;

  const gchar            *migrate;

  GimpUserInstallLogFunc  log;
  gpointer                log_data;
};

typedef enum
{
  USER_INSTALL_MKDIR, /* Create the directory        */
  USER_INSTALL_COPY   /* Copy from sysconf directory */
} GimpUserInstallAction;

static const struct
{
  const gchar           *name;
  GimpUserInstallAction  action;
}
gimp_user_install_items[] =
{
  { "gtkrc",           USER_INSTALL_COPY  },
  { "menurc",          USER_INSTALL_COPY  },
  { "brushes",         USER_INSTALL_MKDIR },
  { "fonts",           USER_INSTALL_MKDIR },
  { "gradients",       USER_INSTALL_MKDIR },
  { "palettes",        USER_INSTALL_MKDIR },
  { "patterns",        USER_INSTALL_MKDIR },
  { "plug-ins",        USER_INSTALL_MKDIR },
  { "modules",         USER_INSTALL_MKDIR },
  { "interpreters",    USER_INSTALL_MKDIR },
  { "environ",         USER_INSTALL_MKDIR },
  { "scripts",         USER_INSTALL_MKDIR },
  { "templates",       USER_INSTALL_MKDIR },
  { "themes",          USER_INSTALL_MKDIR },
  { "tmp",             USER_INSTALL_MKDIR },
  { "tool-options",    USER_INSTALL_MKDIR },
  { "curves",          USER_INSTALL_MKDIR },
  { "levels",          USER_INSTALL_MKDIR },
  { "fractalexplorer", USER_INSTALL_MKDIR },
  { "gfig",            USER_INSTALL_MKDIR },
  { "gflare",          USER_INSTALL_MKDIR },
  { "gimpressionist",  USER_INSTALL_MKDIR }
};


static void      user_install_log                (GimpUserInstall  *install,
                                                  const gchar      *format,
                                                  ...) G_GNUC_PRINTF (2, 3);
static void      user_install_log_newline        (GimpUserInstall  *install);
static void      user_install_log_error          (GimpUserInstall  *install,
                                                  GError          **error);

static gboolean  user_install_mkdir              (GimpUserInstall  *install,
                                                  const gchar      *dirname);
static gboolean  user_install_mkdir_with_parents (GimpUserInstall  *install,
                                                  const gchar      *dirname);
static gboolean  user_install_file_copy          (GimpUserInstall  *install,
                                                  const gchar      *source,
                                                  const gchar      *dest);
static gboolean  user_install_dir_copy           (GimpUserInstall  *install,
                                                  const gchar      *source,
                                                  const gchar      *base);

static gboolean  user_install_create_files       (GimpUserInstall  *install);
static gboolean  user_install_migrate_files      (GimpUserInstall  *install);



GimpUserInstall *
gimp_user_install_new (gboolean verbose)
{
  GimpUserInstall *install = g_slice_new0 (GimpUserInstall);
  gchar           *dir;
  gchar           *version;
  gboolean         migrate;

  install->verbose = verbose;

  dir = g_strdup (gimp_directory ());

  version = strstr (dir, GIMP_APP_VERSION);

  if (! version)
    {
      g_free (dir);
      return install;
    }

  /*  we assume that GIMP_APP_VERSION is in the form '2.x'  */
  version[2] = '2';

  migrate = g_file_test (dir, G_FILE_TEST_IS_DIR);

  if (migrate)
    {
      install->old_major = 2;
      install->old_minor = 2;
    }
  else
    {
      version[2] = '0';

      migrate = g_file_test (dir, G_FILE_TEST_IS_DIR);

      if (migrate)
        {
          install->old_major = 2;
          install->old_minor = 0;
        }
    }

  if (migrate)
    {
      install->old_dir = dir;
      install->migrate = (const gchar *) version;
    }
  else
    {
      g_free (dir);
    }

  return install;
}

gboolean
gimp_user_install_run (GimpUserInstall *install)
{
  gchar *dirname;

  g_return_val_if_fail (install != NULL, FALSE);

  dirname = g_filename_display_name (gimp_directory ());

  if (install->migrate)
    user_install_log (install,
		      _("It seems you have used GIMP %s before.  "
			"GIMP will now migrate your user settings to '%s'."),
		      install->migrate, dirname);
  else
    user_install_log (install,
		      _("It appears that you are using GIMP for the "
			"first time.  GIMP will now create a folder "
			"named '%s' and copy some files to it."),
			dirname);

  g_free (dirname);

  user_install_log_newline (install);

  if (! user_install_mkdir_with_parents (install, gimp_directory ()))
    return FALSE;

  if (install->migrate)
    return user_install_migrate_files (install);
  else
    return user_install_create_files (install);
}

void
gimp_user_install_free (GimpUserInstall *install)
{
  g_return_if_fail (install != NULL);

  g_free (install->old_dir);

  g_slice_free (GimpUserInstall, install);
}

void
gimp_user_install_set_log_handler (GimpUserInstall        *install,
                                   GimpUserInstallLogFunc  log,
                                   gpointer                user_data)
{
  g_return_if_fail (install != NULL);

  install->log      = log;
  install->log_data = user_data;
}


/*  Local functions  */

static void
user_install_log (GimpUserInstall *install,
                  const gchar     *format,
                  ...)
{
  va_list args;

  va_start (args, format);

  if (format)
    {
      gchar *message = g_strdup_vprintf (format, args);

      if (install->verbose)
        g_print ("%s\n", message);

      if (install->log)
        install->log (message, FALSE, install->log_data);

      g_free (message);
    }

  va_end (args);
}

static void
user_install_log_newline (GimpUserInstall *install)
{
  if (install->verbose)
    g_print ("\n");

  if (install->log)
    install->log (NULL, FALSE, install->log_data);
}

static void
user_install_log_error (GimpUserInstall  *install,
                        GError          **error)
{
  if (error && *error)
    {
      const gchar *message = ((*error)->message ?
                              (*error)->message : "(unknown error)");

      if (install->log)
        install->log (message, TRUE, install->log_data);
      else
        g_print ("error: %s\n", message);

      g_clear_error (error);
    }
}

static gboolean
user_install_file_copy (GimpUserInstall *install,
                        const gchar     *source,
                        const gchar     *dest)
{
  GError   *error = NULL;
  gboolean  success;

  user_install_log (install, _("Copying file '%s' from '%s'..."),
                    gimp_filename_to_utf8 (dest),
                    gimp_filename_to_utf8 (source));

  success = gimp_config_file_copy (source, dest, &error);

  user_install_log_error (install, &error);

  return success;
}

static gboolean
user_install_mkdir (GimpUserInstall *install,
                    const gchar     *dirname)
{
  user_install_log (install, _("Creating folder '%s'..."),
                    gimp_filename_to_utf8 (dirname));

  if (g_mkdir (dirname,
               S_IRUSR | S_IWUSR | S_IXUSR |
               S_IRGRP | S_IXGRP |
               S_IROTH | S_IXOTH) == -1)
    {
      GError *error = NULL;

      g_set_error (&error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Cannot create folder '%s': %s"),
                   gimp_filename_to_utf8 (dirname), g_strerror (errno));

      user_install_log_error (install, &error);

      return FALSE;
    }

  return TRUE;
}

static gboolean
user_install_mkdir_with_parents (GimpUserInstall *install,
                                 const gchar     *dirname)
{
  user_install_log (install, _("Creating folder '%s'..."),
                    gimp_filename_to_utf8 (dirname));

  if (g_mkdir_with_parents (dirname,
                            S_IRUSR | S_IWUSR | S_IXUSR |
                            S_IRGRP | S_IXGRP |
                            S_IROTH | S_IXOTH) == -1)
    {
      GError *error = NULL;

      g_set_error (&error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Cannot create folder '%s': %s"),
                   gimp_filename_to_utf8 (dirname), g_strerror (errno));

      user_install_log_error (install, &error);

      return FALSE;
    }

  return TRUE;
}

static gboolean
user_install_dir_copy (GimpUserInstall *install,
                       const gchar     *source,
                       const gchar     *base)
{
  GDir        *source_dir = NULL;
  GDir        *dest_dir   = NULL;
  gchar        dest[1024];
  const gchar *basename;
  gchar       *dirname;
  gboolean     success;
  GError      *error = NULL;

  {
    gchar *basename = g_path_get_basename (source);

    dirname = g_build_filename (base, basename, NULL);
    g_free (basename);
  }

  success = user_install_mkdir (install, dirname);
  if (! success)
    goto error;

  success = (dest_dir = g_dir_open (dirname, 0, &error)) != NULL;
  if (! success)
    goto error;

  success = (source_dir = g_dir_open (source, 0, &error)) != NULL;
  if (! success)
    goto error;

  while ((basename = g_dir_read_name (source_dir)) != NULL)
    {
      gchar *name = g_build_filename (source, basename, NULL);

      if (g_file_test (name, G_FILE_TEST_IS_REGULAR))
        {
          g_snprintf (dest, sizeof (dest), "%s%c%s",
                      dirname, G_DIR_SEPARATOR, basename);

          if (! user_install_file_copy (install, name, dest))
            {
              g_free (name);
              goto error;
            }
        }

      g_free (name);
    }

 error:
  user_install_log_error (install, &error);

  if (source_dir)
    g_dir_close (source_dir);

  if (dest_dir)
    g_dir_close (dest_dir);

  g_free (dirname);

  return success;
}

static gboolean
user_install_create_files (GimpUserInstall *install)
{
  gchar     dest[1024];
  gchar     source[1024];
  gint      i;

  for (i = 0; i < G_N_ELEMENTS (gimp_user_install_items); i++)
    {
      g_snprintf (dest, sizeof (dest), "%s%c%s",
                  gimp_directory (),
                  G_DIR_SEPARATOR,
                  gimp_user_install_items[i].name);

      switch (gimp_user_install_items[i].action)
        {
        case USER_INSTALL_MKDIR:
          if (! user_install_mkdir (install, dest))
            return FALSE;
          break;

        case USER_INSTALL_COPY:
          g_snprintf (source, sizeof (source), "%s%c%s",
                      gimp_sysconf_directory (), G_DIR_SEPARATOR,
                      gimp_user_install_items[i].name);

          if (! user_install_file_copy (install, source, dest))
            return FALSE;
          break;
        }
    }

  return TRUE;
}

static gboolean
user_install_migrate_files (GimpUserInstall *install)
{
  GDir        *dir;
  const gchar *basename;
  gchar        dest[1024];
  GimpRc      *gimprc;
  GError      *error = NULL;

  dir = g_dir_open (install->old_dir, 0, &error);

  if (! dir)
    {
      user_install_log_error (install, &error);
      return FALSE;
    }

  while ((basename = g_dir_read_name (dir)) != NULL)
    {
      gchar *source = g_build_filename (install->old_dir, basename, NULL);

      if (g_file_test (source, G_FILE_TEST_IS_REGULAR))
        {
          /*  skip these files for all old versions  */
          if (g_str_has_prefix (basename, "gimpswap.") ||
              g_str_has_prefix (basename, "pluginrc")  ||
              g_str_has_prefix (basename, "themerc")   ||
              g_str_has_prefix (basename, "toolrc"))
            {
              goto next_file;
            }

          /*  skip menurc for gimp 2.0 since the format has changed  */
          if (install->old_minor == 0 && g_str_has_prefix (basename, "menurc"))
            {
              goto next_file;
            }

          g_snprintf (dest, sizeof (dest), "%s%c%s",
                      gimp_directory (), G_DIR_SEPARATOR, basename);

          user_install_file_copy (install, source, dest);
        }
      else if (g_file_test (source, G_FILE_TEST_IS_DIR))
        {
          /*  skip these directories for all old versions  */
          if (strcmp (basename, "tmp") == 0 ||
              strcmp (basename, "tool-options") == 0)
            {
              goto next_file;
            }

          user_install_dir_copy (install, source, gimp_directory ());
        }

    next_file:
      g_free (source);
    }

  /*  create the tmp directory that was explicitely not copied  */

  g_snprintf (dest, sizeof (dest), "%s%c%s",
              gimp_directory (), G_DIR_SEPARATOR, "tmp");

  user_install_mkdir (install, dest);
  g_dir_close (dir);

  gimp_templates_migrate (install->old_dir);

  gimprc = gimp_rc_new (NULL, NULL, FALSE);
  gimp_rc_migrate (gimprc);
  gimp_rc_save (gimprc);
  g_object_unref (gimprc);

  return TRUE;
}
