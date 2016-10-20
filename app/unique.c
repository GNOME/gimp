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

#include <gio/gio.h>

#ifdef G_OS_WIN32
#include <windows.h>
#endif

#include "core/core-types.h"

#include "file/file-utils.h"

#include "unique.h"


static gboolean  gimp_unique_dbus_open  (const gchar **filenames,
					 gboolean      as_new);
#ifdef G_OS_WIN32
static gboolean  gimp_unique_win32_open (const gchar **filenames,
					 gboolean      as_new);
#endif

gboolean
gimp_unique_open (const gchar **filenames,
		  gboolean      as_new)
{
#ifdef G_OS_WIN32
  return gimp_unique_win32_open (filenames, as_new);
#else
  return gimp_unique_dbus_open (filenames, as_new);
#endif
}

#ifndef GIMP_CONSOLE_COMPILATION
static gchar *
gimp_unique_filename_to_uri (const gchar  *filename,
			     const gchar  *cwd,
			     GError      **error)
{
  gchar *uri = NULL;

  if (file_utils_filename_is_uri (filename, error))
    {
      uri = g_strdup (filename);
    }
  else if (! *error)
    {
      if (! g_path_is_absolute (filename))
	{
	  gchar *absolute = g_build_filename (cwd, filename, NULL);

	  uri = g_filename_to_uri (absolute, NULL, error);

	  g_free (absolute);
	}
      else
	{
	  uri = g_filename_to_uri (filename, NULL, error);
	}
    }

  return uri;
}
#endif


static gboolean
gimp_unique_dbus_open (const gchar **filenames,
		       gboolean      as_new)
{
#ifndef GIMP_CONSOLE_COMPILATION
#if HAVE_DBUS_GLIB

/*  for the DBus service names  */
#include "gui/gimpdbusservice.h"

  GDBusConnection *connection;
  GError          *error = NULL;

  connection = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &error);

  if (connection)
    {
      gboolean success = TRUE;

      if (filenames)
        {
          const gchar *method = as_new ? "OpenAsNew" : "Open";
          gchar       *cwd    = g_get_current_dir ();
          gint         i;

          for (i = 0; filenames[i] && success; i++)
            {
              GError *error = NULL;
	      gchar  *uri   = gimp_unique_filename_to_uri (filenames[i],
                                                           cwd, &error);

              if (uri)
                {
                  GVariant *result;

                  result = g_dbus_connection_call_sync (connection,
                                                        GIMP_DBUS_SERVICE_NAME,
                                                        GIMP_DBUS_SERVICE_PATH,
                                                        GIMP_DBUS_SERVICE_INTERFACE,
                                                        method,
                                                        g_variant_new ("(s)",
                                                                       uri),
                                                        NULL,
                                                        G_DBUS_CALL_FLAGS_NO_AUTO_START,
                                                        -1,
                                                        NULL, NULL);
                  if (result)
                    g_variant_unref (result);
                  else
                    success = FALSE;

                  g_free (uri);
                }
              else
                {
                  g_printerr ("conversion to uri failed: %s\n", error->message);
                  g_clear_error (&error);
                }
            }

          g_free (cwd);
        }
      else
        {
          GVariant *result;

          result = g_dbus_connection_call_sync (connection,
                                                GIMP_DBUS_SERVICE_NAME,
                                                GIMP_DBUS_SERVICE_PATH,
                                                GIMP_DBUS_SERVICE_INTERFACE,
                                                "Activate",
                                                NULL,
                                                NULL,
                                                G_DBUS_CALL_FLAGS_NO_AUTO_START,
                                                -1,
                                                NULL, NULL);
          if (result)
            g_variant_unref (result);
          else
            success = FALSE;
        }

      g_object_unref (connection);

      return success;
    }
  else
    {
      g_printerr ("%s\n", error->message);
      g_clear_error (&error);
    }
#endif /* GIMP_CONSOLE_COMPILATION */
#endif /* HAVE_DBUS_GLIB */

  return FALSE;
}

#ifdef G_OS_WIN32

static gboolean
gimp_unique_win32_open (const gchar **filenames,
			gboolean      as_new)
{
#ifndef GIMP_CONSOLE_COMPILATION

/*  for the proxy window names  */
#include "gui/gui-unique.h"

  HWND  window_handle = FindWindowW (GIMP_UNIQUE_WIN32_WINDOW_CLASS,
				     GIMP_UNIQUE_WIN32_WINDOW_NAME);

  if (window_handle)
    {
      COPYDATASTRUCT  copydata = { 0, };

      if (filenames)
        {
          gchar  *cwd   = g_get_current_dir ();
          GError *error = NULL;
          gint    i;

          for (i = 0; filenames[i]; i++)
            {
              gchar *uri;

              uri = gimp_unique_filename_to_uri (filenames[i], cwd, &error);

              if (uri)
                {
                  copydata.lpData = uri;
                  copydata.cbData = strlen (uri) + 1;  /* size in bytes   */
                  copydata.dwData = (long) as_new;

                  SendMessage (window_handle,
                               WM_COPYDATA, (WPARAM) window_handle, (LPARAM) &copydata);
                }
              else
                {
                  g_printerr ("conversion to uri failed: %s\n", error->message);
                  g_clear_error (&error);
                }
            }

          g_free (cwd);
        }
      else
        {
          SendMessage (window_handle,
                       WM_COPYDATA, (WPARAM) window_handle, (LPARAM) &copydata);
        }

      return TRUE;
    }

#endif

  return FALSE;
}

#endif  /* G_OS_WIN32 */
