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
      const gchar *platform;
      const gchar *last_version   = NULL;
      const gchar *release_date   = NULL;
      JsonParser  *parser;
      JsonPath    *path;
      JsonNode    *result;
      JsonArray   *versions;
      JsonArray   *builds;
      gint         build_revision = 0;
      gint         major;
      gint         minor;
      gint         micro;
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
          g_printerr("%s: parsing of %s failed: %s\n", G_STRFUNC,
                     g_file_get_uri (G_FILE (source)), error->message);
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
           * platform is available.
           */
          version = json_array_get_object_element (versions, i);
          if (json_object_has_member (version, platform))
            {
              last_version = json_object_get_string_member (version, "version");
              release_date = json_object_get_string_member (version, "date");
              builds       = json_object_get_array_member (version, platform);
              break;
            }
        }

      /* If version is not properly parsed, something is wrong with
       * upstream version number or parsing.  This should not happen.
       */
      if (gimp_version_break (last_version, &major, &minor, &micro))
        {
          const gchar *build_date    = NULL;
          const gchar *build_comment = NULL;
          GDateTime   *datetime;
          gchar       *str;

          if (major < GIMP_MAJOR_VERSION ||
              (major == GIMP_MAJOR_VERSION && minor < GIMP_MINOR_VERSION) ||
              (major == GIMP_MAJOR_VERSION && minor == GIMP_MINOR_VERSION && micro < GIMP_MICRO_VERSION))
            {
              /* We are using a newer version than last one (somehow). */
              last_version = NULL;
            }
          else if (major == GIMP_MAJOR_VERSION &&
                   minor == GIMP_MINOR_VERSION &&
                   micro == GIMP_MICRO_VERSION)
            {
              for (i = 0; i < (gint) json_array_get_length (builds); i++)
                {
                  const gchar *build_id = NULL;
                  JsonObject  *build;

                  build = json_array_get_object_element (builds, i);
                  if (json_object_has_member (build, "build-id"))
                    build_id = json_object_get_string_member (build, "build-id");
                  if (g_strcmp0 (build_id, GIMP_BUILD_ID) == 0)
                    {
                      if (json_object_has_member (build, "revision"))
                        build_revision = json_object_get_int_member (build, "revision");
                      if (json_object_has_member (build, "date"))
                        build_date = json_object_get_string_member (build, "date");
                      if (json_object_has_member (build, "comment"))
                        build_comment = json_object_get_string_member (build, "comment");
                      break;
                    }
                }
              if (build_revision <= gimp_version_get_revision ())
                {
                  /* Already using the last officially released
                   * revision. */
                  last_version = NULL;
                }
            }
          str = g_strdup_printf ("%s 00:00:00Z", build_date ? build_date : release_date);
          datetime = g_date_time_new_from_iso8601 (str, NULL);
          g_free (str);
          if (datetime)
            {
              g_object_set (config,
                            "check-update-timestamp", g_get_real_time() / G_USEC_PER_SEC,
                            "last-release-timestamp", g_date_time_to_unix (datetime),
                            "last-known-release",     last_version,
                            "last-revision",          build_revision,
                            "last-release-comment",   build_comment,
                            NULL);
              g_date_time_unref (datetime);
            }
        }

      g_object_unref (path);
      g_object_unref (parser);
      g_free (file_contents);
    }
  else
    {
      g_printerr("%s: loading of %s failed: %s\n", G_STRFUNC,
                 g_file_get_uri (G_FILE (source)), error->message);
      g_clear_error (&error);
    }
}

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

/*
 * gimp_update_auto_check:
 * @config:
 *
 * Run the check for newer versions of GIMP if conditions are right.
 */
gboolean
gimp_update_auto_check (GimpCoreConfig *config)
{
  gint64 prev_update_timestamp;
  gint64 current_timestamp;

  /* Builds with update check deactivated just always return FALSE. */
#ifdef CHECK_UPDATE
  if (! config->check_updates)
#endif
    return FALSE;

  g_object_get (config,
                "check-update-timestamp", &prev_update_timestamp,
                NULL);
  current_timestamp = g_get_real_time() / G_USEC_PER_SEC;

  /* Get rid of invalid saved timestamps. */
  if (prev_update_timestamp > current_timestamp)
    prev_update_timestamp = -1;

#ifndef GIMP_UNSTABLE
  /* Do not check more than once a week. */
  if (current_timestamp - prev_update_timestamp < 3600L * 24L * 7L)
    return FALSE;
#endif

  g_signal_connect (config, "notify::last-known-release",
                    (GCallback) gimp_update_about_dialog,
                    NULL);

  return gimp_update_check (config);
}

/*
 * gimp_update_check:
 * @config:
 *
 * Run the check for newer versions of GIMP inconditionnally.
 */
gboolean
gimp_update_check (GimpCoreConfig *config)
{
  GFile *gimp_versions;

#ifdef GIMP_UNSTABLE
  if (g_getenv ("GIMP_DEV_VERSIONS_JSON"))
    gimp_versions = g_file_new_for_path (g_getenv ("GIMP_DEV_VERSIONS_JSON"));
  else
    gimp_versions = g_file_new_for_uri ("https://testing.gimp.org/gimp_versions.json");
#else
  gimp_versions = g_file_new_for_uri ("https://gimp.org/gimp_versions.json");
#endif
  g_file_load_contents_async (gimp_versions, NULL, gimp_check_updates_callback, config);
  g_object_unref (gimp_versions);

  return TRUE;
}
