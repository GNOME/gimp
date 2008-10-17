/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include <gtk/gtk.h>

#ifdef G_OS_WIN32
#include <windows.h>
#endif

#if HAVE_DBUS_GLIB
#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#endif

#include "gui/gui-types.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"

#include "display/gimpdisplay.h"

#include "gimpdbusservice.h"
#include "gui-unique.h"


#if HAVE_DBUS_GLIB
static void  gui_dbus_service_init (Gimp *gimp);
static void  gui_dbus_service_exit (void);

static DBusGConnection *dbus_connection  = NULL;
#endif

#ifdef G_OS_WIN32
#include "file/file-open.h"

static void  gui_unique_win32_init (Gimp *gimp);
static void  gui_unique_win32_exit (void);

static Gimp            *unique_gimp      = NULL;
static HWND             proxy_window     = NULL;
#endif


void
gui_unique_init (Gimp *gimp)
{
#ifdef G_OS_WIN32
  gui_unique_win32_init (gimp);
#elif HAVE_DBUS_GLIB
  gui_dbus_service_init (gimp);
#endif
}

void
gui_unique_exit (void)
{
#ifdef G_OS_WIN32
  gui_unique_win32_exit ();
#elif HAVE_DBUS_GLIB
  gui_dbus_service_exit ();
#endif
}


#if HAVE_DBUS_GLIB

static void
gui_dbus_service_init (Gimp *gimp)
{
  GError  *error = NULL;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (dbus_connection == NULL);

  dbus_connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);

  if (dbus_connection)
    {
      GObject *service = gimp_dbus_service_new (gimp);

      dbus_bus_request_name (dbus_g_connection_get_connection (dbus_connection),
                             GIMP_DBUS_SERVICE_NAME, 0, NULL);

      dbus_g_connection_register_g_object (dbus_connection,
                                           GIMP_DBUS_SERVICE_PATH, service);
    }
  else
    {
      g_printerr ("%s\n", error->message);
      g_error_free (error);
    }
}

static void
gui_dbus_service_exit (void)
{
  if (dbus_connection)
    {
      dbus_g_connection_unref (dbus_connection);
      dbus_connection = NULL;
    }
}

#endif  /* HAVE_DBUS_GLIB */


#ifdef G_OS_WIN32

typedef struct
{
  gchar    *name;
  gboolean  as_new;
} IdleOpenData;


static IdleOpenData *
idle_open_data_new (const gchar *name,
                    gint         len,
                    gboolean     as_new)
{
  IdleOpenData *data = g_slice_new0 (IdleOpenData);

  if (len > 0)
    {
      data->name   = g_strdup (name);
      data->as_new = as_new;
    }

  return data;
}

static void
idle_open_data_free (IdleOpenData *data)
{
  g_free (data->name);
  g_slice_free (IdleOpenData, data);
}

static gboolean
gui_unique_win32_idle_open (IdleOpenData *data)
{
  /*  We want to be called again later in case that GIMP is not fully
   *  started yet.
   */
  if (! gimp_is_restored (unique_gimp))
    return TRUE;

  if (data->name)
    {
      file_open_from_command_line (unique_gimp, data->name, data->as_new);
    }
  else
    {
      /*  raise the first display  */
      GimpObject *display;

      display = gimp_container_get_first_child (unique_gimp->displays);

      gtk_window_present (GTK_WINDOW (GIMP_DISPLAY (display)->shell));
    }

  return FALSE;
}


static LRESULT CALLBACK
gui_unique_win32_message_handler (HWND   hWnd,
                                  UINT   uMsg,
                                  WPARAM wParam,
                                  LPARAM lParam)
{
  switch (uMsg)
    {
    case WM_COPYDATA:
      if (unique_gimp)
        {
          COPYDATASTRUCT *copydata = (COPYDATASTRUCT *) lParam;
          GSource        *source;
          GClosure       *closure;
          IdleOpenData   *data;

          data = idle_open_data_new (copydata->lpData,
                                     copydata->cbData,
                                     copydata->dwData != 0);

          closure = g_cclosure_new (G_CALLBACK (gui_unique_win32_idle_open),
                                    data,
                                    (GClosureNotify) idle_open_data_free);

          g_object_watch_closure (unique_gimp, closure);

          source = g_idle_source_new ();
          g_source_set_priority (source, G_PRIORITY_LOW);
          g_source_set_closure (source, closure);
          g_source_attach (source, NULL);
          g_source_unref (source);
        }
      return TRUE;

    default:
      return DefWindowProcW (hWnd, uMsg, wParam, lParam);
    }
}

static void
gui_unique_win32_init (Gimp *gimp)
{
  WNDCLASSW wc;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (unique_gimp == NULL);

  unique_gimp = gimp;

  /* register window class for proxy window */
  memset (&wc, 0, sizeof (wc));

  wc.hInstance     = GetModuleHandle (NULL);
  wc.lpfnWndProc   = gui_unique_win32_message_handler;
  wc.lpszClassName = GIMP_UNIQUE_WIN32_WINDOW_CLASS;

  RegisterClassW (&wc);

  proxy_window = CreateWindowExW (0,
                                  GIMP_UNIQUE_WIN32_WINDOW_CLASS,
                                  GIMP_UNIQUE_WIN32_WINDOW_NAME,
                                  WS_POPUP, 0, 0, 1, 1, NULL, NULL, wc.hInstance, NULL);
}

static void
gui_unique_win32_exit (void)
{
  g_return_if_fail (GIMP_IS_GIMP (unique_gimp));

  unique_gimp = NULL;

  DestroyWindow (proxy_window);
}


#endif  /* G_OS_WIN32 */
