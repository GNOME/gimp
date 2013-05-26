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

#include <gegl.h>
#include <gtk/gtk.h>

#ifdef G_OS_WIN32
#include <windows.h>
#endif

#ifdef GDK_WINDOWING_QUARTZ
#include <Carbon/Carbon.h>
#include <sys/param.h>
#endif

#include "gui/gui-types.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"

#include "display/gimpdisplay.h"

#include "file/file-open.h"

#include "gimpdbusservice.h"
#include "gui-unique.h"


#ifdef G_OS_WIN32

static void  gui_unique_win32_init (Gimp *gimp);
static void  gui_unique_win32_exit (void);

static Gimp *unique_gimp  = NULL;
static HWND  proxy_window = NULL;

#elif defined (GDK_WINDOWING_QUARTZ)

static void  gui_unique_mac_init (Gimp *gimp);
static void  gui_unique_mac_exit (void);

static Gimp       *unique_gimp = NULL;
AEEventHandlerUPP  open_document_callback_proc;

#else

static void  gui_dbus_service_init (Gimp *gimp);
static void  gui_dbus_service_exit (void);

static GDBusObjectManagerServer *dbus_manager = NULL;
static guint                     dbus_name_id = 0;

#endif


void
gui_unique_init (Gimp *gimp)
{
#ifdef G_OS_WIN32
  gui_unique_win32_init (gimp);
#elif defined (GDK_WINDOWING_QUARTZ)
  gui_unique_mac_init (gimp);
#else
  gui_dbus_service_init (gimp);
#endif
}

void
gui_unique_exit (void)
{
#ifdef G_OS_WIN32
  gui_unique_win32_exit ();
#elif defined (GDK_WINDOWING_QUARTZ)
  gui_unique_mac_exit ();
#else
  gui_dbus_service_exit ();
#endif
}


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

      gimp_display_shell_present (gimp_display_get_shell (GIMP_DISPLAY (display)));
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

#elif defined (GDK_WINDOWING_QUARTZ)

static gboolean
gui_unique_mac_idle_open (gchar *data)
{
  /*  We want to be called again later in case that GIMP is not fully
   *  started yet.
   */
  if (! gimp_is_restored (unique_gimp))
    return TRUE;

  if (data)
    {
      file_open_from_command_line (unique_gimp, data, FALSE);
    }

  return FALSE;
}

/* Handle the kAEOpenDocuments Apple events. This will register
 * an idle source callback for each filename in the event.
 */
static pascal OSErr
gui_unique_mac_open_documents (const AppleEvent *inAppleEvent,
                               AppleEvent       *outAppleEvent,
                               long              handlerRefcon)
{
  OSStatus    status;
  AEDescList  documents;
  gchar       path[MAXPATHLEN];

  status = AEGetParamDesc (inAppleEvent,
                           keyDirectObject, typeAEList,
                           &documents);
  if (status == noErr)
    {
      long count = 0;
      int  i;

      AECountItems (&documents, &count);

      for (i = 0; i < count; i++)
        {
          FSRef    ref;
          gchar    *callback_path;
          GSource  *source;
          GClosure *closure;

          status = AEGetNthPtr (&documents, i + 1, typeFSRef,
                                0, 0, &ref, sizeof (ref),
                                0);
          if (status != noErr)
            continue;

          FSRefMakePath (&ref, (UInt8 *) path, MAXPATHLEN);

          callback_path = g_strdup (path);

          closure = g_cclosure_new (G_CALLBACK (gui_unique_mac_idle_open),
                                    (gpointer) callback_path,
                                    (GClosureNotify) g_free);

          g_object_watch_closure (G_OBJECT (unique_gimp), closure);

          source = g_idle_source_new ();
          g_source_set_priority (source, G_PRIORITY_LOW);
          g_source_set_closure (source, closure);
          g_source_attach (source, NULL);
          g_source_unref (source);
        }
    }

    return status;
}

static void
gui_unique_mac_init (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (unique_gimp == NULL);

  unique_gimp = gimp;

  open_document_callback_proc = NewAEEventHandlerUPP(gui_unique_mac_open_documents);

  AEInstallEventHandler (kCoreEventClass, kAEOpenDocuments,
                         open_document_callback_proc,
                         0L, TRUE);
}

static void
gui_unique_mac_exit (void)
{
  unique_gimp = NULL;

  AERemoveEventHandler (kCoreEventClass, kAEOpenDocuments,
                        open_document_callback_proc, TRUE);

  DisposeAEEventHandlerUPP(open_document_callback_proc);
}

#else

static void
gui_dbus_bus_acquired (GDBusConnection *connection,
                       const gchar     *name,
                       Gimp            *gimp)
{
  GDBusObjectSkeleton *object;
  GObject             *service;

  /* this should use GIMP_DBUS_SERVICE_PATH, but that's historically wrong */
  dbus_manager = g_dbus_object_manager_server_new ("/org/gimp/GIMP");

  object = g_dbus_object_skeleton_new (GIMP_DBUS_INTERFACE_PATH);

  service = gimp_dbus_service_new (gimp);
  g_dbus_object_skeleton_add_interface (object,
                                        G_DBUS_INTERFACE_SKELETON (service));
  g_object_unref (service);

  g_dbus_object_manager_server_export (dbus_manager, object);
  g_object_unref (object);

  g_dbus_object_manager_server_set_connection (dbus_manager, connection);
}

static void
gui_dbus_name_acquired (GDBusConnection *connection,
                        const gchar     *name,
                        Gimp            *gimp)
{
}

static void
gui_dbus_name_lost (GDBusConnection *connection,
                    const gchar     *name,
                    Gimp            *gimp)
{
}

static void
gui_dbus_service_init (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (dbus_name_id == 0);

  dbus_name_id = g_bus_own_name (G_BUS_TYPE_SESSION,
                                 GIMP_DBUS_SERVICE_NAME,
                                 G_BUS_NAME_OWNER_FLAGS_NONE,
                                 (GBusAcquiredCallback) gui_dbus_bus_acquired,
                                 (GBusNameAcquiredCallback) gui_dbus_name_acquired,
                                 (GBusNameLostCallback) gui_dbus_name_lost,
                                 gimp, NULL);
}

static void
gui_dbus_service_exit (void)
{
  g_bus_unown_name (dbus_name_id);
  g_object_unref (dbus_manager);
}

#endif
