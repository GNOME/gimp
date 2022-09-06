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

#include "config/gimpcoreconfig.h"

#ifndef GIMP_CONSOLE_COMPILATION
#include "dialogs/about-dialog.h"
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
                                                      gint             *micro);

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
  gint     major;
  gint     minor;
  gint     micro;

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

  if (last_version)
    {
      if (gimp_version_break (last_version, &major, &minor, &micro))
        {
          if (/* We are using a newer version than last check. This could
               * happen if updating the config files without having
               * re-checked the remote JSON file.
               */
              (major < GIMP_MAJOR_VERSION ||
               (major == GIMP_MAJOR_VERSION && minor < GIMP_MINOR_VERSION) ||
               (major == GIMP_MAJOR_VERSION && minor == GIMP_MINOR_VERSION && micro < GIMP_MICRO_VERSION)) ||
              /* Already using the last officially released
               * revision. */
              (major == GIMP_MAJOR_VERSION &&
               minor == GIMP_MINOR_VERSION &&
               micro == GIMP_MICRO_VERSION &&
               build_revision <= gimp_version_get_revision ()))
            {
              last_version      = NULL;
            }
        }
      else
        {
          /* If version is not properly parsed, something is wrong with
           * upstream version number or parsing. This should not happen.
           */
          g_printerr ("%s: version not properly formatted: %s\n",
                      G_STRFUNC, last_version);

          return FALSE;
        }
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
gimp_version_break (const gchar *v,
                    gint        *major,
                    gint        *minor,
                    gint        *micro)
{
  gchar **versions;

  *major = 0;
  *minor = 0;
  *micro = 0;

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
              *micro = g_ascii_strtoll (versions[2], NULL, 10);
              return TRUE;
            }
        }
    }
  g_strfreev (versions);

  return (*major > 0);
}

static void
gimp_check_updates_process (const gchar    *source,
                            gchar          *file_contents,
                            gsize           file_length,
                            GimpCoreConfig *config)
{
  const gchar *platform;
  const gchar *last_version      = NULL;
  const gchar *release_date      = NULL;
  const gchar *build_comment     = NULL;
  GError      *error             = NULL;
  gint64       release_timestamp = 0;
  gint         build_revision    = 0;
  JsonParser  *parser;
  JsonPath    *path;
  JsonNode    *result;
  JsonArray   *versions;
  gint         i;

  /* For Windows and macOS, let's look if installers are available.
    * For other platforms, let's just look for source release.
    */
  if (g_strcmp0 (GIMP_BUILD_PLATFORM_FAMILY, "windows") == 0 ||
      g_strcmp0 (GIMP_BUILD_PLATFORM_FAMILY, "macos") == 0)
    platform = GIMP_BUILD_PLATFORM_FAMILY;
  else
    platform = "source";

  parser = json_parser_new ();
  if (! json_parser_load_from_data (parser, file_contents, file_length, &error))
    {
      g_printerr ("%s: parsing of %s failed: %s\n", G_STRFUNC,
                  source, error->message);
      g_free (file_contents);
      g_clear_object (&parser);
      g_clear_error (&error);

      return;
    }

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
  if (! json_path_compile (path, "$['STABLE'][*]", &error))
    {
#ifdef GIMP_UNSTABLE
      g_printerr("Path compilation failed: %s\n", error->message);
#endif
      g_free (file_contents);
      g_clear_object (&parser);
      g_clear_error (&error);

      return;
    }
  result = json_path_match (path, json_parser_get_root (parser));
  g_return_if_fail (JSON_NODE_HOLDS_ARRAY (result));

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
                    build_revision = json_object_get_int_member (build, "revision");
                  if (json_object_has_member (build, "comment"))
                    build_comment = json_object_get_string_member (build, "comment");
                  break;
                }
            }

          if (release_date)
            {
              last_version = json_object_get_string_member (version, "version");
              break;
            }
        }
    }

  if (last_version && release_date)
    {
      GDateTime *datetime;
      gchar     *str;

      str = g_strdup_printf ("%s 00:00:00Z", release_date);
      datetime = g_date_time_new_from_iso8601 (str, NULL);
      g_free (str);

      if (datetime)
        {
          release_timestamp = g_date_time_to_unix (datetime);
          g_date_time_unref (datetime);
        }
      else
        {
          /* JSON file data bug. */
          g_printerr ("%s: release date for version %s not properly formatted: %s\n",
                      G_STRFUNC, last_version, release_date);

          last_version   = NULL;
          release_date   = NULL;
          build_revision = 0;
          build_comment  = NULL;
        }
    }
  gimp_update_known (config, last_version, release_timestamp, build_revision, build_comment);

  g_object_unref (path);
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
      g_printerr("%s: loading of %s failed: %s\n", G_STRFUNC,
                 g_file_get_uri (G_FILE (source)), error->message);
      g_clear_error (&error);
    }
}
#endif /* PLATFORM_OSX */

static void
gimp_update_about_dialog (GimpCoreConfig   *config,
                          const GParamSpec *pspec,
                          gpointer          user_data)
{
  g_signal_handlers_disconnect_by_func (config,
                                        (GCallback) gimp_update_about_dialog,
                                        NULL);

  if (config->last_known_release != NULL)
    {
#ifndef GIMP_CONSOLE_COMPILATION
      gtk_widget_show (about_dialog_create (config));
#else
      g_warning (_("A new version of GIMP (%s) was released.\n"
                   "It is recommended to update."),
                 config->last_known_release);
#endif
    }
}

static const gchar *
gimp_get_version_url ()
{
#ifdef GIMP_RELEASE
  return "https://www.gimp.org/gimp_versions.json";
#else
  if (g_getenv ("GIMP_DEV_VERSIONS_JSON"))
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
 *
 * Run the check for newer versions of GIMP if conditions are right.
 *
 * Returns: %TRUE if a check was actually run.
 */
gboolean
gimp_update_auto_check (GimpCoreConfig *config)
{
  gint64 prev_update_timestamp;
  gint64 current_timestamp;

  /* Builds with update check deactivated just always return FALSE. */
#ifdef CHECK_UPDATE
  /* Allows to disable updates at package level with a build having the
   * version check code built-in.
   * For instance, it would allow to use the same Windows installer for
   * the Windows Store (with update check disabled because it comes with
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
                    NULL);

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
