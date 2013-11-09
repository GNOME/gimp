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

#include <gtk/gtk.h>

#ifdef G_OS_WIN32
#include <windows.h>
#endif

#ifdef GDK_WINDOWING_QUARTZ
#import <AppKit/AppKit.h>
#include <gtkosxapplication.h>
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

#include "file/file-open.h"

#include "gimpdbusservice.h"
#include "gui-unique.h"


#if HAVE_DBUS_GLIB
static void  gui_dbus_service_init (Gimp *gimp);
static void  gui_dbus_service_exit (void);

static DBusGConnection *dbus_connection  = NULL;
#endif

#ifdef G_OS_WIN32
static void  gui_unique_win32_init (Gimp *gimp);
static void  gui_unique_win32_exit (void);

static Gimp            *unique_gimp      = NULL;
static HWND             proxy_window     = NULL;
#endif

#ifdef GDK_WINDOWING_QUARTZ
static void gui_unique_quartz_init (Gimp *gimp);
static void gui_unique_quartz_exit (void);

@interface GimpAppleEventHandler : NSObject {}
- (void) handleEvent:(NSAppleEventDescriptor *) inEvent
        andReplyWith:(NSAppleEventDescriptor *) replyEvent;
@end

static Gimp                   *unique_gimp   = NULL;
static GimpAppleEventHandler  *event_handler = NULL;
#endif


void
gui_unique_init (Gimp *gimp)
{
#ifdef G_OS_WIN32
  gui_unique_win32_init (gimp);
#elif defined (GDK_WINDOWING_QUARTZ)
  gui_unique_quartz_init (gimp);
#elif defined (HAVE_DBUS_GLIB)
  gui_dbus_service_init (gimp);
#endif
}

void
gui_unique_exit (void)
{
#ifdef G_OS_WIN32
  gui_unique_win32_exit ();
#elif defined (GDK_WINDOWING_QUARTZ)
  gui_unique_quartz_exit ();
#elif defined (HAVE_DBUS_GLIB)
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

#endif  /* G_OS_WIN32 */


#ifdef GDK_WINDOWING_QUARTZ

static gboolean
gui_unique_quartz_idle_open (gchar *path)
{
  /*  We want to be called again later in case that GIMP is not fully
   *  started yet.
   */
  if (! gimp_is_restored (unique_gimp))
    return TRUE;

  if (path)
    {
      file_open_from_command_line (unique_gimp, path, FALSE);
    }

  return FALSE;
}

static gboolean
gui_unique_quartz_nsopen_file_callback (GtkosxApplication *osx_app,
                                        gchar             *path,
                                        gpointer           user_data)
{
  gchar    *callback_path;
  GSource  *source;
  GClosure *closure;

  callback_path = g_strdup (path);

  closure = g_cclosure_new (G_CALLBACK (gui_unique_quartz_idle_open),
                            (gpointer) callback_path,
                            (GClosureNotify) g_free);

  g_object_watch_closure (G_OBJECT (unique_gimp), closure);

  source = g_idle_source_new ();

  g_source_set_priority (source, G_PRIORITY_LOW);
  g_source_set_closure (source, closure);
  g_source_attach (source, NULL);
  g_source_unref (source);

  return TRUE;
}

@implementation GimpAppleEventHandler
- (void) handleEvent: (NSAppleEventDescriptor *) inEvent
        andReplyWith: (NSAppleEventDescriptor *) replyEvent
{
  const gchar       *path;
  NSURL             *url;
  NSAutoreleasePool *urlpool;
  NSInteger          count;
  NSInteger          i;

  urlpool = [[NSAutoreleasePool alloc] init];

  count = [inEvent numberOfItems];

  for (i = 1; i <= count; i++)
    {
      gchar    *callback_path;
      GSource  *source;
      GClosure *closure;

      url = [NSURL URLWithString: [[inEvent descriptorAtIndex: i] stringValue]];
      path = [[url path] UTF8String];

      callback_path = g_strdup (path);
      closure = g_cclosure_new (G_CALLBACK (gui_unique_quartz_idle_open),
                                (gpointer) callback_path,
                                (GClosureNotify) g_free);

      g_object_watch_closure (G_OBJECT (unique_gimp), closure);

      source = g_idle_source_new ();
      g_source_set_priority (source, G_PRIORITY_LOW);
      g_source_set_closure (source, closure);
      g_source_attach (source, NULL);
      g_source_unref (source);
    }

  [urlpool drain];
}
@end

static void
gui_unique_quartz_init (Gimp *gimp)
{
  GtkosxApplication *osx_app;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (unique_gimp == NULL);

  osx_app = gtkosx_application_get ();

  unique_gimp = gimp;

  g_signal_connect (osx_app, "NSApplicationOpenFile",
                    G_CALLBACK (gui_unique_quartz_nsopen_file_callback),
                    gimp);

  /* Using the event handler is a hack, it is neccesary becuase
   * gtkosx_application will drop the file open events if any
   * event processing is done before gtkosx_application_ready is
   * called, which we unfortuantly can't avoid doing right now.
   */
  event_handler = [[GimpAppleEventHandler alloc] init];

  [[NSAppleEventManager sharedAppleEventManager]
      setEventHandler: event_handler
          andSelector: @selector (handleEvent: andReplyWith:)
        forEventClass: kCoreEventClass
           andEventID: kAEOpenDocuments];
}

static void
gui_unique_quartz_exit (void)
{
  g_return_if_fail (GIMP_IS_GIMP (unique_gimp));

  unique_gimp = NULL;

  [[NSAppleEventManager sharedAppleEventManager]
      removeEventHandlerForEventClass: kCoreEventClass
                           andEventID: kAEOpenDocuments];

  [event_handler release];

  event_handler = NULL;
}

#endif /* GDK_WINDOWING_QUARTZ */
