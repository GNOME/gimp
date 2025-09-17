/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimp-update.c
 * Copyright (C) 2019 Jehan
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
#include <json-glib/json-glib.h>
#include <stdio.h>

#ifdef PLATFORM_OSX
#import <Foundation/Foundation.h>
#endif /* PLATFORM_OSX */

#ifndef GIMP_CONSOLE_COMPILATION
#include <gtk/gtk.h>
#endif

#include "libgimpbase/gimpbase.h"

#include "core/core-types.h"
#include "core/gimp.h"

#include "config/gimpcoreconfig.h"
#include "config/gimpguiconfig.h"

#ifndef GIMP_CONSOLE_COMPILATION
#include "dialogs/about-dialog.h"
#include "dialogs/welcome-dialog.h"
#endif

#include "gimp-intl.h"
#include "gimp-update.h"
#include "gimp-version.h"

static gboolean      gimp_update_known               (GimpCoreConfig   *config,
                                                      const gchar      *last_version,
                                                      gint64            release_timestamp,
                                                      gint              build_revision,
                                                      const gchar      *comment);
static void          gimp_check_updates_process      (const gchar      *source,
                                                      gchar            *file_contents,
                                                      gsize             file_length,
                                                      GimpCoreConfig   *config);
#ifndef PLATFORM_OSX
static void          gimp_check_updates_callback     (GObject          *source,
                                                      GAsyncResult     *result,
                                                      gpointer          user_data);
#endif /* PLATFORM_OSX */
static void          gimp_update_about_dialog        (GimpCoreConfig   *config,
                                                      const GParamSpec *pspec,
                                                      gpointer          user_data);

static gboolean      gimp_version_break              (const gchar      *v,
                                                      gint             *major,
                                                      gint             *minor,
                                                      gint             *micro,
                                                      gint             *rc,
                                                      gboolean         *is_git);
static gint          gimp_version_cmp                (const gchar      *v1,
                                                      const gchar      *v2);

static const gchar * gimp_get_version_url            (void);

#ifdef PLATFORM_OSX
static gint          gimp_check_updates_process_idle (gpointer          data);
#endif

/* Private Functions */

/**
 * gimp_update_known:
 * @config:
 * @last_version:
 * @release_timestamp: must be non-zero is @last_version is not %NULL.
 * @build_revision:
 * @build_comment:
 *
 * Compare @last_version with currently running version. If the checked
 * version is more recent than running version, then @config properties
 * are updated appropriately (which may trigger a dialog depending on
 * update policy settings).
 * If @last_version is %NULL, the currently stored "last known release"
 * is compared. Even if we haven't made any new remote checks, it is
 * important to always compare again stored last release, otherwise we
 * might warn of the same current version, or worse an older version.
 *
 * Returns: %TRUE is @last_version (or stored last known release if
 * @last_version was %NULL) is newer than running version.
 */
static gboolean
gimp_update_known (GimpCoreConfig *config,
                   const gchar    *last_version,
                   gint64          release_timestamp,
                   gint            build_revision,
                   const gchar    *build_comment)
{
  gboolean new_check = (last_version != NULL);

  if (last_version && release_timestamp == 0)
    {
      /* I don't exit with a g_return_val_if_fail() assert because this
       * is not necessarily a code bug. It may be data issues. So let's
       * just return with an error printed on stderr.
       */
      g_printerr ("%s: version %s with no release dates.\n",
                  G_STRFUNC, last_version);
      return FALSE;
    }

  if (last_version == NULL)
    {
      last_version      = config->last_known_release;
      release_timestamp = config->last_release_timestamp;
      build_revision    = config->last_revision;
      build_comment     = config->last_release_comment;
    }

  if (last_version &&
      (/* We are using a newer version than last check. This could
        * happen if updating the config files without having
        * re-checked the remote JSON file.
        */
       gimp_version_cmp (last_version, NULL) < 0 ||
       /* Already using the last officially released
        * revision. */
       (gimp_version_cmp (last_version, NULL) == 0 &&
        build_revision <= gimp_version_get_revision ())))
    {
      last_version = NULL;
    }

  if (last_version == NULL)
    {
      release_timestamp = 0;
      build_revision    = 0;
      build_comment     = NULL;
    }

  if (new_check)
    g_object_set (config,
                  "check-update-timestamp", g_get_real_time() / G_USEC_PER_SEC,
                  NULL);

  g_object_set (config,
                "last-release-timestamp", release_timestamp,
                "last-known-release",     last_version,
                "last-revision",          build_revision,
                "last-release-comment",   build_comment,
                NULL);

  /* Are we running an old GIMP? */
  return (last_version != NULL);
}

static gboolean
gimp_update_get_highest (JsonParser  *parser,
                         gchar      **highest_version,
                         gint64      *release_timestamp,
                         gint        *build_revision,
                         gchar      **build_comment,
                         gboolean     unstable)
{
  JsonPath    *path;
  JsonNode    *result;
  JsonArray   *versions;
  const gchar *platform;
  const gchar *path_str;
  const gchar *release_date = NULL;
  GError      *error        = NULL;
  gint         i;

  g_return_val_if_fail (highest_version   != NULL, FALSE);
  g_return_val_if_fail (release_timestamp != NULL, FALSE);
  g_return_val_if_fail (build_revision    != NULL, FALSE);
  g_return_val_if_fail (build_comment     != NULL, FALSE);

  *highest_version   = NULL;
  *release_timestamp = 0;
  *build_revision    = 0;
  *build_comment     = NULL;

  if (unstable)
    path_str = "$['DEVELOPMENT'][*]";
  else
    path_str = "$['STABLE'][*]";

  /* For Windows and macOS, let's look if installers are available.
   * For other platforms, let's just look for source release.
   */
  if (g_strcmp0 (GIMP_BUILD_PLATFORM_FAMILY, "windows") == 0 ||
      g_strcmp0 (GIMP_BUILD_PLATFORM_FAMILY, "macos") == 0)
    platform = GIMP_BUILD_PLATFORM_FAMILY;
  else
    platform = "source";

  path = json_path_new ();
  /* Ideally we could just use Json path filters like this to
   * retrieve only released binaries for a given platform:
   * g_strdup_printf ("$['STABLE'][?(@.%s)]['version']", platform);
   * json_array_get_string_element (result, 0);
   * And that would be it! We'd have our last release for given
   * platform.
   * Unfortunately json-glib does not support filter syntax, so we
   * end up looping through releases.
   */
  if (! json_path_compile (path, path_str, &error))
    {
      g_warning ("%s: path compilation failed: %s\n",
                 G_STRFUNC, error->message);
      g_clear_error (&error);
      g_object_unref (path);

      return FALSE;
    }
  result = json_path_match (path, json_parser_get_root (parser));
  if (! JSON_NODE_HOLDS_ARRAY (result))
    {
      g_printerr ("%s: match for \"%s\" is not a JSON array.\n",
                  G_STRFUNC, path_str);
      g_object_unref (path);

      return FALSE;
    }

  versions = json_node_get_array (result);
  for (i = 0; i < (gint) json_array_get_length (versions); i++)
    {
      JsonObject *version;

      /* Note that we don't actually look for the highest version,
       * but for the highest version for which a build for your
       * platform (and optional build-id) is available.
       *
       * So we loop through the version list then the build array
       * and break at first compatible release, since JSON arrays
       * are ordered.
       */
      version = json_array_get_object_element (versions, i);
      if (json_object_has_member (version, platform))
        {
          JsonArray *builds;
          gint       j;

          builds = json_object_get_array_member (version, platform);

          for (j = 0; j < (gint) json_array_get_length (builds); j++)
            {
              const gchar *build_id = NULL;
              JsonObject  *build;

              build = json_array_get_object_element (builds, j);
              if (json_object_has_member (build, "build-id"))
                build_id = json_object_get_string_member (build, "build-id");
              if (g_strcmp0 (build_id, GIMP_BUILD_ID) == 0 ||
                  g_strcmp0 (platform, "source") == 0)
                {
                  /* Release date is the build date if any set,
                   * otherwise the main version release date.
                   */
                  if (json_object_has_member (build, "date"))
                    release_date = json_object_get_string_member (build, "date");
                  else
                    release_date = json_object_get_string_member (version, "date");

                  /* These are optional data. */
                  if (json_object_has_member (build, "revision"))
                    {
                      if (g_strcmp0 (json_node_type_name (json_object_get_member (build, "revision")),
                                     "String") == 0)
                        *build_revision = g_ascii_strtoull (json_object_get_string_member (build, "revision"),
                                                            NULL, 10);
                      else
                        *build_revision = json_object_get_int_member (build, "revision");
                    }
                  if (json_object_has_member (build, "comment"))
                    *build_comment = g_strdup (json_object_get_string_member (build, "comment"));
                  break;
                }
            }

          if (release_date)
            {
              *highest_version = g_strdup (json_object_get_string_member (version, "version"));
              break;
            }
        }
    }

  if (*highest_version && *release_date)
    {
      GDateTime *datetime;
      gchar     *str;

      str = g_strdup_printf ("%s 00:00:00Z", release_date);
      datetime = g_date_time_new_from_iso8601 (str, NULL);
      g_free (str);

      if (datetime)
        {
          *release_timestamp = g_date_time_to_unix (datetime);
          g_date_time_unref (datetime);
        }
      else
        {
          /* JSON file data bug. */
          g_printerr ("%s: release date for version %s not properly formatted: %s\n",
                      G_STRFUNC, *highest_version, release_date);

          g_clear_pointer (highest_version, g_free);
          g_clear_pointer (build_comment, g_free);
          *build_revision = 0;
        }
    }

  json_node_unref (result);
  g_object_unref (path);

  return (*highest_version != NULL);
}

static void
gimp_check_updates_process (const gchar    *source,
                            gchar          *file_contents,
                            gsize           file_length,
                            GimpCoreConfig *config)
{
  gchar       *last_version      = NULL;
  gchar       *build_comment     = NULL;
  gint64       release_timestamp = 0;
  gint         build_revision    = 0;
  GError      *error             = NULL;
  JsonParser  *parser;

  parser = json_parser_new ();
  if (! json_parser_load_from_data (parser, file_contents, file_length, &error))
    {
      gchar *uri = g_file_get_uri (G_FILE (source));

      g_printerr ("%s: parsing of %s failed: %s\n", G_STRFUNC,
                  uri, error->message);

      g_free (uri);
      g_free (file_contents);
      g_clear_object (&parser);
      g_clear_error (&error);

      return;
    }

  gimp_update_get_highest (parser, &last_version, &release_timestamp,
                           &build_revision, &build_comment, FALSE);

#if defined(GIMP_UNSTABLE) || defined(GIMP_RC_VERSION)
    {
      gchar  *dev_version = NULL;
      gchar  *dev_comment      = NULL;
      gint64  dev_timestamp    = 0;
      gint    dev_revision     = 0;

      gimp_update_get_highest (parser, &dev_version, &dev_timestamp,
                               &dev_revision, &dev_comment, TRUE);
      if (dev_version)
        {
          if (! last_version || gimp_version_cmp (dev_version, last_version) > 0)
            {
              g_clear_pointer (&last_version, g_free);
              g_clear_pointer (&build_comment, g_free);
              last_version      = dev_version;
              build_comment     = dev_comment;
              release_timestamp = dev_timestamp;
              build_revision    = dev_revision;
            }
          else
            {
              g_clear_pointer (&dev_version, g_free);
              g_clear_pointer (&dev_comment, g_free);
            }
        }
    }
#endif

  gimp_update_known (config, last_version, release_timestamp, build_revision, build_comment);

  g_clear_pointer (&last_version, g_free);
  g_clear_pointer (&build_comment, g_free);
  g_object_unref (parser);
  g_free (file_contents);
}

#ifndef PLATFORM_OSX
static void
gimp_check_updates_callback (GObject      *source,
                             GAsyncResult *result,
                             gpointer      user_data)
{
  GimpCoreConfig *config        = user_data;
  char           *file_contents = NULL;
  gsize           file_length   = 0;
  GError         *error         = NULL;

  if (g_file_load_contents_finish (G_FILE (source), result,
                                   &file_contents, &file_length,
                                   NULL, &error))
    {
      gimp_check_updates_process (g_file_get_uri (G_FILE (source)), file_contents, file_length, config);
    }
  else
    {
      gchar *uri = g_file_get_uri (G_FILE (source));

      g_printerr ("%s: loading of %s failed: %s\n", G_STRFUNC,
                  uri, error->message);

      g_free (uri);
      g_clear_error (&error);
    }
}
#endif /* PLATFORM_OSX */

static void
gimp_update_about_dialog (GimpCoreConfig   *config,
                          const GParamSpec *pspec,
                          gpointer          user_data)
{
#ifndef GIMP_CONSOLE_COMPILATION
  Gimp *gimp = user_data;
#endif

  g_signal_handlers_disconnect_by_func (config,
                                        (GCallback) gimp_update_about_dialog,
                                        user_data);

  if (config->last_known_release != NULL)
    {
#ifndef GIMP_CONSOLE_COMPILATION
      gtk_widget_show (about_dialog_create (gimp, config));
#else
      g_printerr (_("A new version of GIMP (%s) was released.\n"
                    "It is recommended to update."),
                  config->last_known_release);
#endif
    }
}

static gboolean
gimp_version_break (const gchar *v,
                    gint        *major,
                    gint        *minor,
                    gint        *micro,
                    gint        *rc,
                    gboolean    *is_git)
{
  gchar **versions;

  *major  = 0;
  *minor  = 0;
  *micro  = 0;
  *rc     = 0;
  *is_git = FALSE;

  if (v == NULL)
    return FALSE;

  versions = g_strsplit_set (v, ".", 3);
  if (versions[0] != NULL)
    {
      *major = g_ascii_strtoll (versions[0], NULL, 10);
      if (versions[1] != NULL)
        {
          *minor = g_ascii_strtoll (versions[1], NULL, 10);
          if (versions[2] != NULL)
            {
              gchar **micro_rc_git;

              *micro = g_ascii_strtoll (versions[2], NULL, 10);

              micro_rc_git = g_strsplit_set (versions[2], "-", 2);

              if (g_strv_length (micro_rc_git) > 1 &&
                  strlen (micro_rc_git[1]) > 2     &&
                  micro_rc_git[1][0] == 'R'           &&
                  micro_rc_git[1][1] == 'C')
                {
                  gchar **rc_git;

                  *rc = g_ascii_strtoll (micro_rc_git[1] + 2, NULL, 10);

                  rc_git = g_strsplit_set (micro_rc_git[1], "+", 2);

                  if (g_strv_length (rc_git) > 1 &&
                      strlen (rc_git[1]) == 3    &&
                      rc_git[1][0] == 'g'        &&
                      rc_git[1][1] == 'i'        &&
                      rc_git[1][2] == 't')
                    {
                      *is_git = TRUE;
                    }

                  g_strfreev (rc_git);
                }

              g_strfreev (micro_rc_git);
            }
        }
    }

  g_strfreev (versions);

  return (*major > 0 || *minor > 0 || *micro > 0);
}

/**
 * gimp_version_cmp:
 * @v1: a string representing a version, ex. "2.10.22".
 * @v2: a string representing another version, ex. "2.99.2".
 *
 * If @v2 is %NULL, @v1 is compared to the currently running version.
 *
 * Returns: an integer less than, equal to, or greater than zero if @v1
 *          is found to represent a version respectively, lower than,
 *          matching, or greater than @v2.
 */
static gint
gimp_version_cmp (const gchar *v1,
                  const gchar *v2)
{
  gint     major1;
  gint     minor1;
  gint     micro1;
  gint     rc1;
  gboolean is_git1;
  gint     major2  = GIMP_MAJOR_VERSION;
  gint     minor2  = GIMP_MINOR_VERSION;
  gint     micro2  = GIMP_MICRO_VERSION;
  gint     rc2     = 0;
  gboolean is_git2 = FALSE;

#if defined(GIMP_RC_VERSION)
  rc2 = GIMP_RC_VERSION;
#if defined(GIMP_IS_RC_GIT)
  is_git2 = TRUE;
#endif
#endif

  g_return_val_if_fail (v1 != NULL, -1);

  if (! gimp_version_break (v1, &major1, &minor1, &micro1, &rc1, &is_git1))
    {
      /* If version is not properly parsed, something is wrong with
       * upstream version number or parsing. This should not happen.
       */
      g_printerr ("%s: version not properly formatted: %s\n",
                  G_STRFUNC, v1);

      return -1;
    }
  if (v2 && ! gimp_version_break (v2, &major2, &minor2, &micro2, &rc2, &is_git2))
    {
      g_printerr ("%s: version not properly formatted: %s\n",
                  G_STRFUNC, v2);

      return 1;
    }

  if (major1 == major2 && minor1 == minor2 && micro1 == micro2 &&
      rc1 == rc2 && is_git1 == is_git2)
    return 0;
  else if (major1 > major2                                                                    ||
           (major1 == major2 && minor1 > minor2)                                              ||
           (major1 == major2 && minor1 == minor2 && micro1 > micro2)                          ||
           /* RC 0 is the real release, so it's "higher" than any other. */
           (major1 == major2 && minor1 == minor2 && micro1 == micro2 && rc1 == 0 && rc2 > 0)  ||
           (major1 == major2 && minor1 == minor2 && micro1 == micro2 && rc1 > rc2 && rc2 > 0) ||
           (major1 == major2 && minor1 == minor2 && micro1 == micro2 && rc1 == rc2 && is_git1))
    return 1;
  else
    return -1;
}

static const gchar *
gimp_get_version_url (void)
{
#if defined(GIMP_RELEASE) && ! defined(GIMP_RC_VERSION)
  return "https://www.gimp.org/gimp_versions.json";
#else
  if (g_getenv ("GIMP_DEV_VERSIONS_JSON") &&
      g_strcmp0 (g_getenv ("GIMP_DEV_VERSIONS_JSON"), "") != 0)
    return g_getenv ("GIMP_DEV_VERSIONS_JSON");
  else
    return "https://testing.gimp.org/gimp_versions.json";
#endif
}

#ifdef PLATFORM_OSX
typedef struct _GimpCheckUpdatesData
{
  const gchar    *gimp_versions;
  gchar          *json_result;
  gsize           json_size;
  GimpCoreConfig *config;
} GimpCheckUpdatesData;

static int
gimp_check_updates_process_idle (gpointer data)
{
  GimpCheckUpdatesData *check_updates_data = (GimpCheckUpdatesData *) data;

  gimp_check_updates_process (check_updates_data->gimp_versions,
                              check_updates_data->json_result,
                              check_updates_data->json_size,
                              check_updates_data->config);

  g_free (check_updates_data);

  return FALSE; /* remove idle */
}
#endif /* PLATFORM_OSX */

/* Public Functions */

/*
 * gimp_update_auto_check:
 * @config:
 * @gimp:
 *
 * Run the check for newer versions of GIMP if conditions are right.
 *
 * Returns: %TRUE if a check was actually run.
 */
gboolean
gimp_update_auto_check (GimpCoreConfig *config,
                        Gimp           *gimp)
{
  gboolean is_update;
  gint64   prev_update_timestamp;
  gint64   current_timestamp;

  is_update = (config->config_version == NULL ||
               gimp_version_cmp (GIMP_VERSION,
                                 config->config_version) > 0);

  if (is_update || GIMP_GUI_CONFIG (config)->show_welcome_dialog)
    {
#ifndef GIMP_CONSOLE_COMPILATION
      /* If GIMP was just updated and this is the first time the new
       * version is run, display a welcome dialog and do not check for
       * updates right now. Otherwise, if the user has set the welcome
       * dialog to always appear on load, show the Create page on start.
       */
      if (! gimp->no_interface)
        gtk_widget_set_visible (welcome_dialog_create (gimp, is_update),
                                TRUE);

      /* Do not check for new updates when GIMP was just updated. */
      if (is_update)
        return FALSE;
#else
      g_log (G_LOG_DOMAIN, G_LOG_LEVEL_MESSAGE,
             "Welcome to GIMP %s!", GIMP_VERSION);
#endif
    }

  /* Builds with update check deactivated just always return FALSE. */
#ifdef CHECK_UPDATE
  /* Allows to disable updates at package level with a build having the
   * version check code built-in.
   * For instance, it allows to use the same bundling artifact for the
   * Microsoft Store (with update check disabled because it comes with
   * its own update channel).
   */
  if (! gimp_version_check_update () ||
      ! config->check_updates)
#endif
    return FALSE;

  g_object_get (config,
                "check-update-timestamp", &prev_update_timestamp,
                NULL);
  current_timestamp = g_get_real_time() / G_USEC_PER_SEC;

  /* Get rid of invalid saved timestamps. */
  if (prev_update_timestamp > current_timestamp)
    prev_update_timestamp = -1;

#ifdef GIMP_RELEASE
  /* Do not check more than once a week. */
  if (current_timestamp - prev_update_timestamp < 3600L * 24L * 7L)
    return FALSE;
#endif

  g_signal_connect (config, "notify::last-known-release",
                    (GCallback) gimp_update_about_dialog,
                    gimp);

  gimp_update_check (config);

  return TRUE;
}

/*
 * gimp_update_check:
 * @config:
 *
 * Run the check for newer versions of GIMP inconditionnally.
 */
void
gimp_update_check (GimpCoreConfig *config)
{
#ifdef PLATFORM_OSX
  const gchar *gimp_versions;

  gimp_versions = gimp_get_version_url ();

  NSMutableURLRequest *request = [[NSMutableURLRequest alloc] init];
  [request setURL:[NSURL URLWithString:@(gimp_versions)]];
  [request setHTTPMethod:@"GET"];

  NSURLSession *session = [NSURLSession sessionWithConfiguration:[NSURLSessionConfiguration defaultSessionConfiguration]];
  /* completionHandler is called on a background thread */
  [[session dataTaskWithRequest:request completionHandler:^(NSData *data, NSURLResponse *response, NSError *error) {
    NSString             *reply;
    gchar                *json_result;
    GimpCheckUpdatesData *update_results;

    if (error)
      {
        g_printerr ("%s: gimp_update_check failed to get update from %s, with error: %s\n",
                    G_STRFUNC,
                    gimp_versions,
                    [error.localizedDescription UTF8String]);
        return;
      }

    if ([response isKindOfClass:[NSHTTPURLResponse class]]) {
        NSInteger statusCode = [(NSHTTPURLResponse *)response statusCode];

        if (statusCode != 200)
          {
            g_printerr ("%s: gimp_update_check failed to get update from %s, with status code: %d\n",
                        G_STRFUNC,
                        gimp_versions,
                        (int)statusCode);
            return;
          }
    }

    reply       = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
    json_result = g_strdup ([reply UTF8String]); /* will be freed by gimp_check_updates_process */

    update_results = g_new (GimpCheckUpdatesData, 1);

    update_results->gimp_versions = gimp_versions;
    update_results->json_result   = json_result;
    update_results->json_size     = [reply lengthOfBytesUsingEncoding:NSUTF8StringEncoding];
    update_results->config        = config;

    g_idle_add ((GSourceFunc) gimp_check_updates_process_idle, (gpointer) update_results);
  }] resume];
#else
  GFile *gimp_versions;

  gimp_versions = g_file_new_for_uri (gimp_get_version_url ());

  g_file_load_contents_async (gimp_versions, NULL, gimp_check_updates_callback, config);
  g_object_unref (gimp_versions);
#endif /* PLATFORM_OSX */
}

/*
 * gimp_update_refresh:
 * @config:
 *
 * Do not execute a remote check, but refresh the known release data as
 * it may be outdated.
 */
void
gimp_update_refresh (GimpCoreConfig *config)
{
  gimp_update_known (config, NULL, 0, 0, NULL);
}
