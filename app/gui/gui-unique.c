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

#include <glib-object.h>

#if HAVE_DBUS_GLIB
#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#endif

#include "core/core-types.h"

#include "core/gimp.h"

#include "widgets/gimpdbusservice.h"

#include "gui-unique.h"


#if HAVE_DBUS_GLIB
static void  gui_dbus_service_init (Gimp *gimp);
static void  gui_dbus_service_exit (void);

static DBusGConnection *dbus_connection  = NULL;
#endif

#ifdef G_OS_WIN32
static void gui_unique_win32_init  (Gimp *gimp);
static void gui_unique_win32_exit  (void);

static Gimp            *unique_gimp      = NULL;
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
#if HAVE_DBUS_GLIB
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

LRESULT CALLBACK
gui_unique_win32_message_handler (HWND   hWnd,
				  UINT   uMsg,
				  WPARAM wParam,
				  LPARAM lParam)
{
  switch (uMsg)
    {
    case WM_COPYDATA:
      {
	COPYDATASTRUCT *copydata = (COPYDATASTRUCT *) lParam;

	if (copydata->cbData > 0)
	  file_open_from_command_line (unique_gimp,
				       copydata->lpData,
				       copydata->dwData != 0);
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
  HWND      window_handle;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (unique_gimp == NULL);

  unique_gimp = gimp;

  /* register window class for proxy window */
  memset (&wc, 0, sizeof (wc));

  wc.hInstance     = GetModuleHandle (NULL);
  wc.lpfnWndProc   = gui_unique_win32_message_handler;
  wc.lpszClassName = GIMP_UNIQUE_WIN32_WINDOW_CLASS;

  RegisterClassW (&wc);

  CreateWindowExW (0,
		   GIMP_UNIQUE_WIN32_WINDOW_CLASS,
		   GIMP_UNIQUE_WIN32_WINDOW_NAME,
		   WS_POPUP, 0, 0, 1, 1, NULL, NULL, wc.hInstance, NULL);
}

static void
gui_unique_win32_exit (void)
{
  g_return_if_fail (GIMP_IS_GIMP (unique_gimp));

  unique_gimp = NULL;
}


#endif  /* G_OS_WIN32 */
