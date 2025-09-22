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
#include "core/gimp-utils.h"

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
static gboolean      gimp_update_get_highest         (JsonParser       *parser,
                                                      gchar           **highest_version,
                                                      gint64           *release_timestamp,
                                                      gint             *build_revision,
                                                      gchar           **build_comment,
                                                      gboolean          unstable);
static gboolean      gimp_update_get_messages        (JsonParser       *parser,
                                                      gchar           **top_message_id,
                                                      gchar          ***titles,
                                                      gchar          ***messages,
                                                      gchar          ***images,
                                                      gchar          ***dates,
                                                      gint             *new_messages);
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
static void          gimp_update_welcome_dialog      (GimpCoreConfig   *config,
                                                      const GParamSpec *pspec,
                                                      gpointer          user_data);

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

static gboolean
gimp_update_get_messages (JsonParser   *parser,
                          gchar       **top_message_id,
                          gchar      ***titles,
                          gchar      ***messages,
                          gchar      ***images,
                          gchar      ***dates,
                          gint         *new_messages)
{
  GStrvBuilder *titles_builder;
  GStrvBuilder *messages_builder;
  GStrvBuilder *images_builder;
  GStrvBuilder *dates_builder;
  JsonPath     *path;
  JsonNode     *result;
  JsonArray    *messages_array;
  gchar        *new_top_message_id = NULL;
  const gchar  *path_str;
  const gchar  *build_category;
  GError       *error              = NULL;
  gint          i;
  gboolean      is_new_message     = TRUE;

  g_return_val_if_fail (top_message_id != NULL, FALSE);
  g_return_val_if_fail (titles         != NULL, FALSE);
  g_return_val_if_fail (messages       != NULL, FALSE);
  g_return_val_if_fail (images         != NULL, FALSE);
  g_return_val_if_fail (dates          != NULL, FALSE);
  g_return_val_if_fail (new_messages   != NULL, FALSE);

  *titles       = NULL;
  *messages     = NULL;
  *images       = NULL;
  *dates        = NULL;
  *new_messages = 0;

  path_str = "$['MESSAGES'][*]";
  path     = json_path_new ();
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

  titles_builder   = g_strv_builder_new ();
  messages_builder = g_strv_builder_new ();
  images_builder   = g_strv_builder_new ();
  dates_builder    = g_strv_builder_new ();

#if defined(GIMP_RELEASE)
#if defined(GIMP_UNSTABLE)
  build_category = "devel";
#else
  build_category = "stable";
#endif
#else
  build_category = "nightly";
#endif

  messages_array = json_node_get_array (result);
  for (i = 0; i < (gint) json_array_get_length (messages_array); i++)
    {
      JsonObject *msg;

      msg = json_array_get_object_element (messages_array, i);
      if (! json_object_has_member (msg, "title")   ||
          ! json_object_has_member (msg, "message") ||
          ! json_object_has_member (msg, "id")      ||
          ! json_object_has_member (msg, "date"))
        {
          /* JSON file data bug. */
          g_printerr ("%s: message %d misses mandatory elements.\n",
                      G_STRFUNC, i);
          continue;
        }
      else
        {
          const gchar *id;
          const gchar *date;
          GDateTime   *datetime;
          gchar       *str;

          id = json_object_get_string_member (msg, "id");

          if (json_object_has_member (msg, "conditions"))
            {
              JsonArray *conditions;
              gboolean   valid_message = FALSE;
              gint       j;

              conditions = json_object_get_array_member (msg, "conditions");
              /* The "conditions" is a list where if one condition
               * matches, then the message is considered valid.
               */
              for (j = 0; j < (gint) json_array_get_length (conditions); j++)
                {
                  gchar *condition;
                  gchar *token;

                  condition = g_strdup (json_array_get_string_element (conditions, j));
                  /* The format of conditions is:
                   * "[os]:[build-id]:[stable|devel|nightly]:[ver]"
                   * - Any part can be replaced by "*" to mean it
                   *   matches any value.
                   * - Any missing section means "*" (so for instance,
                   *   you could just have a
                   *   "windows:org.gimp.GIMP_official" condition.
                   * - The "[ver]" can be an exact version, e.g.
                   *   "3.0.4", but it can also ">3.0.4" or "<=3.0.4" or
                   *   again ">=3.0.0,<=3.0.4" for any version in between
                   *   these. I.e. a comma-separated list of version
                   *   comparisons.
                   */
                  token = strtok (condition, ":");

                  if (g_strcmp0 (token, "*") == 0 ||
                      g_strcmp0 (token, GIMP_BUILD_PLATFORM_FAMILY) == 0)
                    {
                      token = strtok (NULL, ":");
                      if (token == NULL)
                        {
                          valid_message = TRUE;
                          break;
                        }

                      if (g_strcmp0 (token, "*") == 0 ||
                          g_strcmp0 (token, GIMP_BUILD_ID) == 0)
                        {
                          token = strtok (NULL, ":");
                          if (token == NULL)
                            {
                              valid_message = TRUE;
                              break;
                            }

                          if (g_strcmp0 (token, "*") == 0 ||
                              g_strcmp0 (token, build_category) == 0)
                            {
                              gchar *vertok;

                              token = strtok (NULL, ":");
                              if (token == NULL && g_strcmp0 (token, "*") == 0)
                                {
                                  valid_message = TRUE;
                                  break;
                                }

                              valid_message = TRUE;

                              vertok = strtok (token, ",");
                              while (vertok != NULL)
                                {
                                  if (strlen (vertok) > 2 &&
                                      vertok[1] == '=')
                                    {
                                      if (vertok[0] == '>' &&
                                          gimp_version_cmp (vertok + 2, NULL) > 0)
                                        {
                                          valid_message = FALSE;
                                          break;
                                        }
                                      else if (vertok[0] == '<' &&
                                          gimp_version_cmp (vertok + 2, NULL) < 0)
                                        {
                                          valid_message = FALSE;
                                          break;
                                        }
                                    }
                                  else if (strlen (vertok) > 1 &&
                                           (vertok[0] == '>' || vertok[0] == '<'))
                                    {
                                      if (vertok[0] == '>' &&
                                          gimp_version_cmp (vertok + 1, NULL) >= 0)
                                        {
                                          valid_message = FALSE;
                                          break;
                                        }
                                      else if (vertok[0] == '<' &&
                                               gimp_version_cmp (vertok + 1, NULL) <= 0)
                                        {
                                          valid_message = FALSE;
                                          break;
                                        }
                                    }
                                  else if (strlen (vertok) > 0 &&
                                           gimp_version_cmp (vertok, NULL) != 0)
                                    {
                                      valid_message = FALSE;
                                      break;
                                    }

                                  if (! valid_message)
                                    break;

                                  vertok = strtok (NULL, ",");
                                }
                            }
                        }
                    }

                  g_free (condition);

                  if (valid_message)
                    break;
                }

              if (! valid_message)
                continue;
            }

          if (*top_message_id &&
              is_new_message  &&
              g_strcmp0 (id, *top_message_id) == 0)
            /* Below messages were all shown in previous runs. */
            is_new_message = FALSE;

          if (is_new_message)
            (*new_messages)++;

          date = json_object_get_string_member (msg, "date");
          str = g_strdup_printf ("%s 00:00:00Z", date);
          datetime = g_date_time_new_from_iso8601 (str, NULL);
          g_free (str);

          if (datetime)
            {
              gchar *date = g_date_time_format (datetime, "%x");
              g_strv_builder_add (dates_builder, date);

              g_free (date);
            }
          else
            {
              /* JSON file data bug. */
              g_printerr ("%s: date for message %d not properly formatted: %s\n",
                          G_STRFUNC, i, date);
              continue;
            }

          if (new_top_message_id == NULL)
            new_top_message_id = g_strdup (id);
        }

      g_strv_builder_add (titles_builder, json_object_get_string_member (msg, "title"));
      g_strv_builder_add (messages_builder, json_object_get_string_member (msg, "message"));
      if (json_object_has_member (msg, "image"))
        g_strv_builder_add (images_builder, json_object_get_string_member (msg, "image"));
      else
        g_strv_builder_add (images_builder, "");
    }

  *titles   = g_strv_builder_end (titles_builder);
  *messages = g_strv_builder_end (messages_builder);
  *images   = g_strv_builder_end (images_builder);
  *dates    = g_strv_builder_end (dates_builder);

  json_node_unref (result);
  g_object_unref (path);
  g_strv_builder_unref (titles_builder);
  g_strv_builder_unref (messages_builder);
  g_strv_builder_unref (images_builder);
  g_strv_builder_unref (dates_builder);

  if (new_top_message_id)
    {
      g_free (*top_message_id);
      *top_message_id = new_top_message_id;
    }

  return (g_strv_length (*titles) != 0);
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

  gchar       *last_message_id   = NULL;
  gchar      **titles            = NULL;
  gchar      **messages          = NULL;
  gchar      **images            = NULL;
  gchar      **dates             = NULL;
  gint         new_messages      = 0;

  GError      *error             = NULL;
  JsonParser  *parser;

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

  g_object_get (config,
                "last-message-id", &last_message_id,
                NULL);

  if (gimp_update_get_messages (parser, &last_message_id,
                                &titles, &messages, &images, &dates,
                                &new_messages))
    {
      g_object_set (config,
                    "messages",       messages,
                    "message-titles", titles,
                    "message-images", images,
                    "message-dates",  dates,
                    "n-new-messages", new_messages,
                    NULL);

      if (config->last_known_release == NULL)
        /* Only update the last message ID if we didn't pop-up the About
         * dialog to announce a new release. Indeed in such a case, we
         * won't display the new message(s) and therefore the user
         * likely won't get knowledge of it (unless they were to later
         * open the Welcome dialog on their own.
         */
        g_object_set (config,
                      "last-message-id", last_message_id,
                      NULL);
    }

  g_clear_pointer (&last_version, g_free);
  g_clear_pointer (&build_comment, g_free);
  g_clear_pointer (&last_message_id, g_free);
  g_strfreev (titles);
  g_strfreev (messages);
  g_strfreev (images);

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

static void
gimp_update_welcome_dialog (GimpCoreConfig   *config,
                            const GParamSpec *pspec,
                            gpointer          user_data)
{
#ifndef GIMP_CONSOLE_COMPILATION
  Gimp *gimp = user_data;
#endif

  g_signal_handlers_disconnect_by_func (config,
                                        (GCallback) gimp_update_welcome_dialog,
                                        user_data);

  if (config->last_known_release == NULL &&
      config->n_new_messages > 0)
    {
#ifndef GIMP_CONSOLE_COMPILATION
      gtk_widget_show (welcome_dialog_create (gimp, TRUE));
#else
      g_printerr (_("You have new messages from the GIMP team."));
#endif
    }
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
  g_signal_connect (config, "notify::n-new-messages",
                    (GCallback) gimp_update_welcome_dialog,
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
