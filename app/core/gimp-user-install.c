/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimp-user-install.c
 * Copyright (C) 2000-2008 Michael Natterer and Sven Neumann
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

/* This file contains functions to help migrate the settings from a
 * previous GIMP version to be used with the current (newer) version.
 */

#include "config.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef PLATFORM_OSX
#include <AppKit/AppKit.h>
#endif

#include <gio/gio.h>
#include <glib/gstdio.h>

#ifdef G_OS_WIN32
#include <libgimpbase/gimpwin32-io.h>
#endif

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "config/gimpconfig-file.h"
#include "config/gimprc.h"

#include "gimp-templates.h"
#include "gimp-tags.h"
#include "gimp-user-install.h"

#include "gimp-intl.h"


struct _GimpUserInstall
{
  GObject                 *gimp;

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
  { "gimp.css",        USER_INSTALL_COPY  },
  { "menurc",          USER_INSTALL_COPY  },
  { "brushes",         USER_INSTALL_MKDIR },
  { "dynamics",        USER_INSTALL_MKDIR },
  { "fonts",           USER_INSTALL_MKDIR },
  { "gradients",       USER_INSTALL_MKDIR },
  { "palettes",        USER_INSTALL_MKDIR },
  { "patterns",        USER_INSTALL_MKDIR },
  { "tool-presets",    USER_INSTALL_MKDIR },
  { "plug-ins",        USER_INSTALL_MKDIR },
  { "modules",         USER_INSTALL_MKDIR },
  { "interpreters",    USER_INSTALL_MKDIR },
  { "environ",         USER_INSTALL_MKDIR },
  { "scripts",         USER_INSTALL_MKDIR },
  { "templates",       USER_INSTALL_MKDIR },
  { "themes",          USER_INSTALL_MKDIR },
  { "icons",           USER_INSTALL_MKDIR },
  { "tmp",             USER_INSTALL_MKDIR },
  { "curves",          USER_INSTALL_MKDIR },
  { "levels",          USER_INSTALL_MKDIR },
  { "filters",         USER_INSTALL_MKDIR },
  { "fractalexplorer", USER_INSTALL_MKDIR },
  { "gfig",            USER_INSTALL_MKDIR },
  { "gflare",          USER_INSTALL_MKDIR },
  { "gimpressionist",  USER_INSTALL_MKDIR }
};


static gboolean  user_install_detect_old         (GimpUserInstall    *install,
                                                  const gchar        *gimp_dir);
static gchar *   user_install_old_style_gimpdir  (void);

static void      user_install_log                (GimpUserInstall    *install,
                                                  const gchar        *format,
                                                  ...) G_GNUC_PRINTF (2, 3);
static void      user_install_log_newline        (GimpUserInstall    *install);
static void      user_install_log_error          (GimpUserInstall    *install,
                                                  GError            **error);

static gboolean  user_install_mkdir              (GimpUserInstall    *install,
                                                  const gchar        *dirname);
static gboolean  user_install_mkdir_with_parents (GimpUserInstall    *install,
                                                  const gchar        *dirname);
static gboolean  user_install_file_copy          (GimpUserInstall    *install,
                                                  const gchar        *source,
                                                  const gchar        *dest,
                                                  const gchar        *old_options_regexp,
                                                  GRegexEvalCallback  update_callback);
static gboolean  user_install_dir_copy           (GimpUserInstall    *install,
                                                  const gchar        *source,
                                                  const gchar        *base);

static gboolean  user_install_create_files       (GimpUserInstall    *install);
static gboolean  user_install_migrate_files      (GimpUserInstall    *install);


/*  public functions  */

GimpUserInstall *
gimp_user_install_new (GObject  *gimp,
                       gboolean  verbose)
{
  GimpUserInstall *install = g_slice_new0 (GimpUserInstall);

  install->gimp    = gimp;
  install->verbose = verbose;

  user_install_detect_old (install, gimp_directory ());

#ifdef PLATFORM_OSX
  /* The config path on OSX has for a very short time frame (2.8.2 only)
     been "~/Library/GIMP". It changed to "~/Library/Application Support"
     in 2.8.4 and was in the home folder (as was other UNIX) before. */

  if (! install->old_dir)
    {
      gchar             *dir;
      NSAutoreleasePool *pool;
      NSArray           *path;
      NSString          *library_dir;

      pool = [[NSAutoreleasePool alloc] init];

      path = NSSearchPathForDirectoriesInDomains (NSLibraryDirectory,
                                                  NSUserDomainMask, YES);
      library_dir = [path objectAtIndex:0];

      dir = g_build_filename ([library_dir UTF8String],
                              GIMPDIR, GIMP_USER_VERSION, NULL);

      [pool drain];

      user_install_detect_old (install, dir);
      g_free (dir);
    }

#endif

  if (! install->old_dir)
    {
      /* if the default XDG-style config directory was not found, try
       * the "old-style" path in the home folder.
       */
      gchar *dir = user_install_old_style_gimpdir ();
      user_install_detect_old (install, dir);
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
    if (! user_install_migrate_files (install))
      return FALSE;

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

static gboolean
user_install_detect_old (GimpUserInstall *install,
                         const gchar     *gimp_dir)
{
  gchar    *dir     = g_strdup (gimp_dir);
  gchar    *version;
  gboolean  migrate = FALSE;

  version = strstr (dir, GIMP_APP_VERSION);

  if (version)
    {
      gint i;

      for (i = (GIMP_MINOR_VERSION & ~1); i >= 0; i -= 2)
        {
          /*  we assume that GIMP_APP_VERSION is in the form '2.x'  */
          g_snprintf (version + 2, 2, "%d", i);

          migrate = g_file_test (dir, G_FILE_TEST_IS_DIR);

          if (migrate)
            {
#ifdef GIMP_UNSTABLE
              g_printerr ("gimp-user-install: migrating from %s\n", dir);
#endif
              install->old_major = 2;
              install->old_minor = i;

              break;
            }
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

  return migrate;
}

static gchar *
user_install_old_style_gimpdir (void)
{
  const gchar *home_dir = g_get_home_dir ();
  gchar       *gimp_dir = NULL;

  if (home_dir)
    {
      gimp_dir = g_build_filename (home_dir, ".gimp-" GIMP_APP_VERSION, NULL);
    }
  else
    {
      gchar *user_name = g_strdup (g_get_user_name ());
      gchar *subdir_name;

#ifdef G_OS_WIN32
      gchar *p = user_name;

      while (*p)
        {
          /* Replace funny characters in the user name with an
           * underscore. The code below also replaces some
           * characters that in fact are legal in file names, but
           * who cares, as long as the definitely illegal ones are
           * caught.
           */
          if (!g_ascii_isalnum (*p) && !strchr ("-.,@=", *p))
            *p = '_';
          p++;
        }
#endif

#ifndef G_OS_WIN32
      g_message ("warning: no home directory.");
#endif
      subdir_name = g_strconcat (".gimp-" GIMP_APP_VERSION ".", user_name, NULL);
      gimp_dir = g_build_filename (gimp_data_directory (),
                                   subdir_name,
                                   NULL);
      g_free (user_name);
      g_free (subdir_name);
    }

  return gimp_dir;
}

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
user_install_file_copy (GimpUserInstall    *install,
                        const gchar        *source,
                        const gchar        *dest,
                        const gchar        *old_options_regexp,
                        GRegexEvalCallback  update_callback)
{
  GError   *error = NULL;
  gboolean  success;

  user_install_log (install, _("Copying file '%s' from '%s'..."),
                    gimp_filename_to_utf8 (dest),
                    gimp_filename_to_utf8 (source));

  success = gimp_config_file_copy (source, dest, old_options_regexp, update_callback, &error);

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

/* The regexp pattern of all options changed from menurc of GIMP 2.8.
 * Add any pattern that we want to recognize for replacement in the menurc of
 * the next release
 */
#define MENURC_OVER20_UPDATE_PATTERN \
  "\"<Actions>/file/file-export-to\""           "|" \
  "\"<Actions>/file/file-export\""              "|" \
  "\"<Actions>/edit/edit-paste-as-new\""        "|" \
  "\"<Actions>/buffers/buffers-paste-as-new\""  "|" \
  "\"<Actions>/tools/tools-value-[1-4]-.*\""

/**
 * callback to use for updating a menurc from GIMP over 2.0.
 * data is unused (always NULL).
 * The updated value will be matched line by line.
 */
static gboolean
user_update_menurc_over20 (const GMatchInfo *matched_value,
                           GString          *new_value,
                           gpointer          data)
{
  gchar *match = g_match_info_fetch (matched_value, 0);

  /* file-export-* changes to follow file-save-* patterns.  Actions
   * available since GIMP 2.8, changed for 2.10 in commit 4b14ed2.
   */
  if (g_strcmp0 (match, "\"<Actions>/file/file-export\"") == 0)
    {
      g_string_append (new_value, "\"<Actions>/file/file-export-as\"");
    }
  else if (g_strcmp0 (match, "\"<Actions>/file/file-export-to\"") == 0)
    {
      g_string_append (new_value, "\"<Actions>/file/file-export\"");
    }
  /* "*-paste-as-new" renamed to "*-paste-as-new-image"
   */
  else if (g_strcmp0 (match, "\"<Actions>/edit/edit-paste-as-new\"") == 0)
    {
      g_string_append (new_value, "\"<Actions>/edit/edit-paste-as-new-image\"");
    }
  else if (g_strcmp0 (match, "\"<Actions>/buffers/buffers-paste-as-new\"") == 0)
    {
      g_string_append (new_value, "\"<Actions>/buffers/buffers-paste-as-new-image\"");
    }
  /* Tools settings renamed more user-friendly.  Actions available
   * since GIMP 2.4, changed for 2.10 in commit 0bdb747.
   */
  else if (g_str_has_prefix (match, "\"<Actions>/tools/tools-value-1-"))
    {
      g_string_append (new_value, "\"<Actions>/tools/tools-opacity-");
      g_string_append (new_value, match + 31);
    }
  else if (g_str_has_prefix (match, "\"<Actions>/tools/tools-value-2-"))
    {
      g_string_append (new_value, "\"<Actions>/tools/tools-size-");
      g_string_append (new_value, match + 31);
    }
  else if (g_str_has_prefix (match, "\"<Actions>/tools/tools-value-3-"))
    {
      g_string_append (new_value, "\"<Actions>/tools/tools-aspect-");
      g_string_append (new_value, match + 31);
    }
  else if (g_str_has_prefix (match, "\"<Actions>/tools/tools-value-4-"))
    {
      g_string_append (new_value, "\"<Actions>/tools/tools-angle-");
      g_string_append (new_value, match + 31);
    }
  /* Should not happen. Just in case we match something unexpected by
   * mistake.
   */
  else
    {
      g_string_append (new_value, match);
    }

  g_free (match);
  return FALSE;
}

#define CONTROLLERRC_UPDATE_PATTERN \
  "\\(map \"(scroll|cursor)-[^\"]*\\bcontrol\\b[^\"]*\""

static gboolean
user_update_controllerrc (const GMatchInfo *matched_value,
                          GString          *new_value,
                          gpointer          data)
{
  gchar  *original;
  gchar  *replacement;
  GRegex *regexp = NULL;

  /* No need of a complicated pattern here.
   * CONTROLLERRC_UPDATE_PATTERN took care of it first.
   */
  regexp   = g_regex_new ("\\bcontrol\\b", 0, 0, NULL);
  original = g_match_info_fetch (matched_value, 0);

  replacement = g_regex_replace (regexp, original, -1, 0,
                                 "primary", 0, NULL);
  g_string_append (new_value, replacement);

  g_free (original);
  g_free (replacement);
  g_regex_unref (regexp);

  return FALSE;
}

#define GIMPRC_UPDATE_PATTERN \
  "\\(theme [^)]*\\)"    "|" \
  "\\(.*-path [^)]*\\)"

static gboolean
user_update_gimprc (const GMatchInfo *matched_value,
                    GString          *new_value,
                    gpointer          data)
{
  /* Do not migrate paths and themes from GIMP < 2.10. */
  return FALSE;
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

          if (! user_install_file_copy (install, name, dest, NULL, NULL))
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
  gchar dest[1024];
  gchar source[1024];
  gint  i;

  for (i = 0; i < G_N_ELEMENTS (gimp_user_install_items); i++)
    {
      g_snprintf (dest, sizeof (dest), "%s%c%s",
                  gimp_directory (),
                  G_DIR_SEPARATOR,
                  gimp_user_install_items[i].name);

      if (g_file_test (dest, G_FILE_TEST_EXISTS))
        continue;

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

          if (! user_install_file_copy (install, source, dest, NULL, NULL))
            return FALSE;
          break;
        }
    }

  g_snprintf (dest, sizeof (dest), "%s%c%s",
              gimp_directory (), G_DIR_SEPARATOR, "tags.xml");

  if (! g_file_test (dest, G_FILE_TEST_IS_REGULAR))
    {
      /* if there was no tags.xml, install it with default tag set.
       */
      if (! gimp_tags_user_install ())
        {
          return FALSE;
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
          const gchar        *update_pattern = NULL;
          GRegexEvalCallback  update_callback = NULL;

          /*  skip these files for all old versions  */
          if (strcmp (basename, "documents") == 0      ||
              g_str_has_prefix (basename, "gimpswap.") ||
              strcmp (basename, "pluginrc") == 0       ||
              strcmp (basename, "themerc") == 0        ||
              strcmp (basename, "toolrc") == 0         ||
              strcmp (basename, "gtkrc") == 0)
            {
              goto next_file;
            }
          else if (strcmp (basename, "menurc") == 0)
            {
              switch (install->old_minor)
                {
                case 0:
                  /*  skip menurc for gimp 2.0 as the format has changed  */
                  goto next_file;
                  break;
                default:
                  update_pattern  = MENURC_OVER20_UPDATE_PATTERN;
                  update_callback = user_update_menurc_over20;
                  break;
                }
            }
          else if (strcmp (basename, "controllerrc") == 0)
            {
              update_pattern  = CONTROLLERRC_UPDATE_PATTERN;
              update_callback = user_update_controllerrc;
            }
          else if (strcmp (basename, "gimprc") == 0)
            {
              update_pattern  = GIMPRC_UPDATE_PATTERN;
              update_callback = user_update_gimprc;
            }

          g_snprintf (dest, sizeof (dest), "%s%c%s",
                      gimp_directory (), G_DIR_SEPARATOR, basename);

          user_install_file_copy (install, source, dest,
                                  update_pattern, update_callback);
        }
      else if (g_file_test (source, G_FILE_TEST_IS_DIR))
        {
          /*  skip these directories for all old versions  */
          if (strcmp (basename, "tmp") == 0          ||
              strcmp (basename, "tool-options") == 0 ||
              strcmp (basename, "themes") == 0)
            {
              goto next_file;
            }

          user_install_dir_copy (install, source, gimp_directory ());
        }

    next_file:
      g_free (source);
    }

  /*  create the tmp directory that was explicitly not copied  */

  g_snprintf (dest, sizeof (dest), "%s%c%s",
              gimp_directory (), G_DIR_SEPARATOR, "tmp");

  user_install_mkdir (install, dest);
  g_dir_close (dir);

  gimp_templates_migrate (install->old_dir);

  gimprc = gimp_rc_new (install->gimp, NULL, NULL, FALSE);
  gimp_rc_migrate (gimprc);
  gimp_rc_save (gimprc);
  g_object_unref (gimprc);

  return TRUE;
}
