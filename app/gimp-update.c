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

static const gchar *
gimp_version_max (const gchar *v1,
                  const gchar *v2)
{
  gint major1;
  gint minor1;
  gint micro1;
  gint major2;
  gint minor2;
  gint micro2;

  if (v1 == NULL)
    return v2;
  else if (v2 == NULL)
    return v1;
  else if (gimp_version_break (v1, &major1, &minor1, &micro1) &&
           gimp_version_break (v2, &major2, &minor2, &micro2))
    {
      if (major1 > major2 ||
          (major1 == major2 && minor1 > minor2) ||
          (major1 == major2 && minor1 == minor2 && micro1 > micro2))
        return v1;
      else
        return v2;
    }

  return NULL;
}

static void
gimp_check_updates_callback (GObject      *source,
                             GAsyncResult *result,
                             gpointer      user_data)
{
  GFileInputStream *stream;
  GimpCoreConfig   *config = user_data;
  GError           *error  = NULL;

  stream = g_file_read_finish (G_FILE (source), result, &error);
  if (stream)
    {
      JsonParser       *parser  = NULL;
      JsonPath         *path;
      JsonNode         *result;
      JsonObject       *versions;
      GList            *members;
      GList            *iter;

      const gchar      *last_version   = NULL;
      gint              major;
      gint              minor;
      gint              micro;

      parser = json_parser_new ();
      if (! json_parser_load_from_stream (parser, G_INPUT_STREAM (stream), NULL, &error))
        {
          g_clear_object (&stream);
          g_clear_object (&parser);

          return;
        }

      path = json_path_new ();
      json_path_compile (path, "$['STABLE']", &error);
      result = json_path_match (path, json_parser_get_root (parser));
      versions = json_array_get_object_element (json_node_get_array (result), 0);
      members = json_object_get_members (versions);

      for (iter = members; iter; iter = iter->next)
        last_version = gimp_version_max (last_version, iter->data);

      /* If version is not properly parsed, something is wrong with
       * upstream version number or parsing.  This should not happen.
       */
      if (gimp_version_break (last_version, &major, &minor, &micro))
        {
          g_object_set (config,
                        "check-update-timestamp", g_get_real_time() / G_USEC_PER_SEC,
                        "last-known-release",
                        (major > GIMP_MAJOR_VERSION ||
                         (major == GIMP_MAJOR_VERSION && minor > GIMP_MINOR_VERSION) ||
                         (major == GIMP_MAJOR_VERSION && minor == GIMP_MINOR_VERSION && micro > GIMP_MICRO_VERSION)) ?
                        last_version : NULL,
                        NULL);
        }

      g_list_free (members);
      json_node_unref (result);
      g_object_unref (path);
      g_object_unref (parser);
      g_object_unref (stream);
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

  if (! config->check_updates)
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
  g_file_read_async (gimp_versions, 0, NULL, gimp_check_updates_callback, config);
  g_object_unref (gimp_versions);

  return TRUE;
}
