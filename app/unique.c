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

#include <gdk-pixbuf/gdk-pixbuf.h>

#ifdef G_OS_WIN32
#include <windows.h>
#endif

#include "core/core-types.h"

#ifndef GIMP_CONSOLE_COMPILATION
/*  for the DBus service names  */
#include "gui/gimpdbusservice.h"
#endif

#include "unique.h"

#ifndef G_OS_WIN32
static gboolean  gimp_unique_dbus_open  (const gchar **filenames,
                                         gboolean      as_new);
static gboolean  gimp_unique_dbus_batch_run (const gchar  *batch_interpreter,
                                             const gchar **batch_commands);
#else
static gboolean  gimp_unique_win32_open (const gchar **filenames,
                                         gboolean      as_new);
#endif


gboolean
gimp_unique_open (const gchar **filenames,
                  gboolean      as_new)
{
#ifdef G_OS_WIN32
  return gimp_unique_win32_open (filenames, as_new);
#elif defined (PLATFORM_OSX)
  /* Opening files through "Open with" from other software is likely handled
   * instead by gui_unique_quartz_init() by gtkosx signal handling.
   *
   * Opening files through command lines will always create new process, because
   * dbus is usually not installed by default on macOS (and when it is, it may
   * not work properly). See !808 and #8997.
   */
  return FALSE;
#else
  return gimp_unique_dbus_open (filenames, as_new);
#endif
}

gboolean
gimp_unique_batch_run (const gchar  *batch_interpreter,
                       const gchar **batch_commands)
{
#ifdef G_OS_WIN32
  g_printerr ("Batch commands cannot be run in existing instance in Win32.\n");
  return FALSE;
#elif defined (PLATFORM_OSX)
  /* Running batch commands through command lines will always run in the new
   * process, because dbus is usually not installed by default on macOS (and
   * when it is, it may not work properly). See !808 and #8997.
   */
  return FALSE;
#else
  return gimp_unique_dbus_batch_run (batch_interpreter,
                                     batch_commands);
#endif
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
          gint    i;

          for (i = 0; filenames[i]; i++)
            {
              GFile *file;
              file = g_file_new_for_commandline_arg_and_cwd (filenames[i], cwd);

              if (file)
                {
                  gchar *uri = g_file_get_uri (file);

                  copydata.lpData = uri;
                  copydata.cbData = strlen (uri) + 1;  /* size in bytes   */
                  copydata.dwData = (long) as_new;

                  SendMessage (window_handle,
                               WM_COPYDATA, (WPARAM) window_handle, (LPARAM) &copydata);

                  g_free (uri);
                  g_object_unref (file);
                }
              else
                {
                  g_printerr ("conversion to uri failed for '%s'\n",
                              filenames[i]);
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

#else

static gboolean
gimp_unique_dbus_open (const gchar **filenames,
                       gboolean      as_new)
{
#ifndef GIMP_CONSOLE_COMPILATION

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
              GFile *file;

              file = g_file_new_for_commandline_arg_and_cwd (filenames[i], cwd);

              if (file)
                {
                  GVariant *result;
                  gchar    *uri = g_file_get_uri (file);

                  result = g_dbus_connection_call_sync (connection,
                                                        GIMP_DBUS_SERVICE_NAME,
                                                        GIMP_DBUS_SERVICE_PATH,
                                                        GIMP_DBUS_INTERFACE_NAME,
                                                        method,
                                                        g_variant_new ("(s)",
                                                                       uri),
                                                        NULL,
                                                        G_DBUS_CALL_FLAGS_NO_AUTO_START,
                                                        -1,
                                                        NULL, NULL);

                  g_free (uri);

                  if (result)
                    g_variant_unref (result);
                  else
                    success = FALSE;

                  g_object_unref (file);
                }
              else
                {
                  g_printerr ("conversion to uri failed for '%s'\n",
                              filenames[i]);
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
                                                GIMP_DBUS_INTERFACE_NAME,
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
#endif

  return FALSE;
}


static gboolean
gimp_unique_dbus_batch_run (const gchar  *batch_interpreter,
                            const gchar **batch_commands)
{
#ifndef GIMP_CONSOLE_COMPILATION

  GDBusConnection *connection;
  GError          *error = NULL;

  connection = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &error);

  if (connection)
    {
      const gchar *method = "BatchRun";
      gboolean     success = TRUE;
      gint         i;

      for (i = 0; batch_commands[i] && success; i++)
        {
          GVariant    *result;
          const gchar *interpreter;

          /* NULL is not a valid string GVariant. */
          interpreter = batch_interpreter ? batch_interpreter : "";

          result = g_dbus_connection_call_sync (connection,
                                                GIMP_DBUS_SERVICE_NAME,
                                                GIMP_DBUS_SERVICE_PATH,
                                                GIMP_DBUS_INTERFACE_NAME,
                                                method,
                                                g_variant_new ("(ss)",
                                                               interpreter,
                                                               batch_commands[i]),
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
#endif

  return FALSE;
}
#endif  /* G_OS_WIN32 */
