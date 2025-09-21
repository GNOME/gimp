/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gtk/gtk.h>

#ifdef GDK_WINDOWING_WIN32
#include <gdk/gdkwin32.h>
#endif

#ifdef GDK_WINDOWING_X11
#include <gdk/gdkx.h>
#endif

#ifdef GDK_WINDOWING_QUARTZ
#include <Cocoa/Cocoa.h>
#endif

#ifdef GDK_WINDOWING_WAYLAND
#include <gdk/gdkwayland.h>
#endif

#include "gimp.h"
#include "gimpui.h"

#include "libgimpmodule/gimpmodule.h"

#include "libgimpwidgets/gimpwidgets.h"
#include "libgimpwidgets/gimpwidgets-private.h"


/**
 * SECTION: gimpui
 * @title: gimpui
 * @short_description: Common user interface functions. This header includes
 *                     all other GIMP User Interface Library headers.
 * @see_also: gtk_init(), gdk_set_use_xshm(), gtk_widget_set_default_visual().
 *
 * Common user interface functions. This header includes all other
 * GIMP User Interface Library headers.
 **/


/*  local function prototypes  */

static void        gimp_ui_help_func               (const gchar       *help_id,
                                                    gpointer           help_data);
static void        gimp_ui_theme_changed           (GFileMonitor      *monitor,
                                                    GFile             *file,
                                                    GFile             *other_file,
                                                    GFileMonitorEvent  event_type,
                                                    GtkCssProvider    *css_provider);
static void        gimp_ensure_modules             (void);

#ifdef GDK_WINDOWING_QUARTZ
static gboolean    gimp_osx_focus_window           (gpointer);
#endif

#ifndef GDK_WINDOWING_WIN32
static GdkWindow * gimp_ui_get_foreign_window      (gpointer           window);
#endif
static gboolean    gimp_window_transient_on_mapped (GtkWidget         *window,
                                                    GdkEventAny       *event,
                                                    GBytes            *handle);


static gboolean gimp_ui_initialized = FALSE;


/*  public functions  */

/**
 * gimp_ui_init:
 * @prog_name: The name of the plug-in which will be passed as argv[0] to
 *             gtk_init(). It's a convention to use the name of the
 *             executable and _not_ the PDB procedure name.
 *
 * This function initializes GTK+ with gtk_init().
 * It also initializes Gegl and Babl.
 *
 * It also sets up various other things so that the plug-in user looks
 * and behaves like the GIMP core. This includes selecting the GTK+
 * theme and setting up the help system as chosen in the GIMP
 * preferences. Any plug-in that provides a user interface should call
 * this function.
 *
 * It can safely be called more than once.
 * Calls after the first return quickly with no effect.
 **/
void
gimp_ui_init (const gchar *prog_name)
{
  const gchar    *display_name;
  GtkCssProvider *css_provider;
  GFileMonitor   *css_monitor;
  GFile          *file;

  g_return_if_fail (prog_name != NULL);

  if (gimp_ui_initialized)
    return;

  g_set_prgname (prog_name);

  display_name = gimp_display_name ();

  if (display_name)
    {
#if defined (GDK_WINDOWING_X11)
      g_setenv ("DISPLAY", display_name, TRUE);
#else
      g_setenv ("GDK_DISPLAY", display_name, TRUE);
#endif
    }

#ifdef GDK_WINDOWING_QUARTZ
  /* Sets activation policy to prevent plugins from appearing as separate apps
   * in Dock.
   * Makes plugins behave as helper processes of GIMP on macOS.
   */
  [NSApp setActivationPolicy:NSApplicationActivationPolicyAccessory];
#endif

  if (gimp_user_time ())
    {
      /* Construct a fake startup ID as we only want to pass the
       * interaction timestamp, see _gdk_windowing_set_default_display().
       */
      gchar *startup_id = g_strdup_printf ("_TIME%u", gimp_user_time ());

      g_setenv ("DESKTOP_STARTUP_ID", startup_id, TRUE);
      g_free (startup_id);
    }

  gtk_init (NULL, NULL);

  css_provider = gtk_css_provider_new ();
  gtk_style_context_add_provider_for_screen (gdk_screen_get_default (),
                                             GTK_STYLE_PROVIDER (css_provider),
                                             GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

  file = gimp_directory_file ("theme.css", NULL);
  css_monitor = g_file_monitor (file, G_FILE_MONITOR_NONE, NULL, NULL);
  g_object_unref (file);

  gimp_ui_theme_changed (css_monitor, NULL, NULL, G_FILE_MONITOR_EVENT_CHANGED,
                         css_provider);

  g_signal_connect (css_monitor, "changed",
                    G_CALLBACK (gimp_ui_theme_changed),
                    css_provider);

  g_object_unref (css_provider);

  gdk_set_program_class (gimp_wm_class ());

  if (gimp_icon_theme_dir ())
    {
      file = g_file_new_for_path (gimp_icon_theme_dir ());
      gimp_icons_set_icon_theme (file);
      g_object_unref (file);
    }

  gimp_widgets_init (gimp_ui_help_func,
                     gimp_context_get_foreground,
                     gimp_context_get_background,
                     gimp_ensure_modules, NULL);

  gimp_dialogs_show_help_button (gimp_show_help_button ());

#ifdef GDK_WINDOWING_QUARTZ
  g_idle_add (gimp_osx_focus_window, NULL);
#endif

  /* Some widgets use GEGL buffers for thumbnails, previews, etc. */
  gegl_init (NULL, NULL);

  gimp_ui_initialized = TRUE;
}

#ifdef GDK_WINDOWING_QUARTZ
static void
gimp_window_transient_show (GtkWidget *window)
{
  g_signal_handlers_disconnect_by_func (window,
                                        gimp_window_transient_show,
                                        NULL);
  [NSApp arrangeInFront: nil];
}
#endif

/**
 * gimp_window_set_transient_for:
 * @window: the #GtkWindow that should become transient
 * @handle: handle of the window that should become the parent
 *
 * Indicates to the window manager that @window is a transient dialog
 * to the window identified by @handle.
 *
 * Note that @handle is an opaque data, which you should not try to
 * construct yourself or make sense of. It may be different things
 * depending on the OS or even the display server. You should only use
 * a handle returned by [func@Gimp.progress_get_window_handle],
 * [method@Gimp.Display.get_window_handle] or
 * [method@GimpUi.Dialog.get_native_handle].
 *
 * Most of the time you will want to use the convenience function
 * [func@GimpUi.window_set_transient].
 *
 * Since: 3.0
 */
void
gimp_window_set_transient_for (GtkWindow *window,
                               GBytes    *handle)
{
  g_return_if_fail (gimp_ui_initialized);
  g_return_if_fail (GTK_IS_WINDOW (window));
  g_return_if_fail (handle != NULL);

  g_signal_handlers_disconnect_matched (window, G_SIGNAL_MATCH_FUNC,
                                        0, 0, NULL,
                                        gimp_window_transient_on_mapped,
                                        NULL);

  g_signal_connect_data (window, "map-event",
                         G_CALLBACK (gimp_window_transient_on_mapped),
                         g_bytes_ref (handle),
                         (GClosureNotify) g_bytes_unref,
                         G_CONNECT_AFTER);

  if (gtk_widget_get_mapped (GTK_WIDGET (window)))
    gimp_window_transient_on_mapped (GTK_WIDGET (window), NULL, handle);
}

/**
 * gimp_window_set_transient_for_display:
 * @window:  the #GtkWindow that should become transient
 * @display: display of the image window that should become the parent
 *
 * Indicates to the window manager that @window is a transient dialog
 * associated with the GIMP image window that is identified by its
 * display. See [method@Gdk.Window.set_transient_for] for more information.
 *
 * Most of the time you will want to use the convenience function
 * [func@GimpUi.window_set_transient].
 *
 * Since: 2.4
 */
void
gimp_window_set_transient_for_display (GtkWindow   *window,
                                       GimpDisplay *display)
{
  GBytes *handle;

  g_return_if_fail (gimp_ui_initialized);
  g_return_if_fail (GTK_IS_WINDOW (window));
  g_return_if_fail (GIMP_IS_DISPLAY (display));

  g_signal_handlers_disconnect_matched (window, G_SIGNAL_MATCH_FUNC,
                                        0, 0, NULL,
                                        gimp_window_transient_on_mapped,
                                        NULL);

  handle = gimp_display_get_window_handle (display);
  g_signal_connect_data (window, "map-event",
                         G_CALLBACK (gimp_window_transient_on_mapped),
                         handle,
                         (GClosureNotify) g_bytes_unref,
                         G_CONNECT_AFTER);

  if (gtk_widget_get_mapped (GTK_WIDGET (window)))
    gimp_window_transient_on_mapped (GTK_WIDGET (window), NULL, handle);
}

/**
 * gimp_window_set_transient:
 * @window: the #GtkWindow that should become transient
 *
 * Indicates to the window manager that @window is a transient dialog
 * associated with the GIMP window that the plug-in has been
 * started from. See also gimp_window_set_transient_for_display().
 *
 * Since: 2.4
 */
void
gimp_window_set_transient (GtkWindow *window)
{
  GBytes *handle;

  g_return_if_fail (gimp_ui_initialized);
  g_return_if_fail (GTK_IS_WINDOW (window));

  g_signal_handlers_disconnect_matched (window, G_SIGNAL_MATCH_FUNC,
                                        0, 0, NULL,
                                        gimp_window_transient_on_mapped,
                                        NULL);

  handle = gimp_progress_get_window_handle ();
  g_signal_connect_data (window, "map-event",
                         G_CALLBACK (gimp_window_transient_on_mapped),
                         handle,
                         (GClosureNotify) g_bytes_unref,
                         G_CONNECT_AFTER);

  if (gtk_widget_get_mapped (GTK_WIDGET (window)))
    gimp_window_transient_on_mapped (GTK_WIDGET (window), NULL, handle);
}


/*  private functions  */

static void
gimp_ui_help_func (const gchar *help_id,
                   gpointer     help_data)
{
  gimp_help (NULL, help_id);
}

static void
gimp_ui_theme_changed (GFileMonitor      *monitor,
                       GFile             *file,
                       GFile             *other_file,
                       GFileMonitorEvent  event_type,
                       GtkCssProvider    *css_provider)
{
  GError *error = NULL;
  gchar  *contents;

  file = gimp_directory_file ("theme.css", NULL);

  if (g_file_load_contents (file, NULL, &contents, NULL, NULL, &error))
    {
      gboolean prefer_dark_theme;

      prefer_dark_theme = strstr (contents, "/* prefer-dark-theme */") != NULL;

      g_object_set (gtk_settings_get_for_screen (gdk_screen_get_default ()),
                    "gtk-application-prefer-dark-theme", prefer_dark_theme,
                    NULL);

      g_free (contents);
    }
  else
    {
      g_printerr ("%s: error loading %s: %s\n", G_STRFUNC,
                  gimp_file_get_utf8_name (file), error->message);
      g_clear_error (&error);
    }

  if (! gtk_css_provider_load_from_file (css_provider, file, &error))
    {
      g_printerr ("%s: error parsing %s: %s\n", G_STRFUNC,
                  gimp_file_get_utf8_name (file), error->message);
      g_clear_error (&error);
    }

  g_object_unref (file);
}

static void
gimp_ensure_modules (void)
{
  static GimpModuleDB *module_db = NULL;

  if (! module_db)
    {
      gchar *load_inhibit = gimp_get_module_load_inhibit ();
      gchar *module_path  = gimp_gimprc_query ("module-path");

      module_db = gimp_module_db_new (FALSE);

      gimp_module_db_set_load_inhibit (module_db, load_inhibit);
      gimp_module_db_load (module_db, module_path);

      g_free (module_path);
      g_free (load_inhibit);
    }
}

#ifdef GDK_WINDOWING_QUARTZ
static gboolean
gimp_osx_focus_window (gpointer user_data)
{
  [NSApp activateIgnoringOtherApps:YES];
  return FALSE;
}
#endif

/* Currently broken on Win32 so avoiding a "defined but not used"
 * warning when building on Windows.
 */
#ifndef GDK_WINDOWING_WIN32
static GdkWindow *
gimp_ui_get_foreign_window (gpointer window)
{
#ifdef GDK_WINDOWING_X11
  if (GDK_IS_X11_DISPLAY (gdk_display_get_default ()))
    return gdk_x11_window_foreign_new_for_display (gdk_display_get_default (),
                                                   (Window) window);
#endif

#ifdef GDK_WINDOWING_WIN32
  return gdk_win32_window_foreign_new_for_display (gdk_display_get_default (),
                                                   (HWND) window);
#endif

  return NULL;
}
#endif

static gboolean
gimp_window_transient_on_mapped (GtkWidget   *window,
                                 GdkEventAny *event,
                                 GBytes      *handle)
{
  gboolean transient_set = FALSE;

  if (handle == NULL)
    return FALSE;

#ifdef GDK_WINDOWING_WAYLAND
  if (GDK_IS_WAYLAND_DISPLAY (gdk_display_get_default ()))
    {
      char *wayland_handle;

      wayland_handle = (char *) g_bytes_get_data (handle, NULL);
      gdk_wayland_window_set_transient_for_exported (gtk_widget_get_window (window),
                                                     wayland_handle);
      transient_set = TRUE;
    }
#endif

#ifdef GDK_WINDOWING_X11
  if (! transient_set && GDK_IS_X11_DISPLAY (gdk_display_get_default ()))
    {
      GdkWindow *parent;
      Window    *handle_data;
      Window     parent_ID;
      gsize      handle_size;

      handle_data = (Window *) g_bytes_get_data (handle, &handle_size);
      g_return_val_if_fail (handle_size == sizeof (Window), FALSE);
      parent_ID = *handle_data;

      parent = gimp_ui_get_foreign_window ((gpointer) parent_ID);

      if (parent)
        gdk_window_set_transient_for (gtk_widget_get_window (window), parent);

      transient_set = TRUE;
    }
#endif
  /* To know why it is disabled on Win32, see gimp_window_set_transient_cb() in
   * app/widgets/gimpwidgets-utils.c.
   */
#if 0 && defined (GDK_WINDOWING_WIN32)
  if (! transient_set)
    {
      GdkWindow *parent;
      HANDLE    *handle_data;
      HANDLE     parent_ID;
      gsize      handle_size;

      handle_data = (HANDLE *) g_bytes_get_data (handle, &handle_size);
      g_return_val_if_fail (handle_size == sizeof (HANDLE), FALSE);
      parent_ID = *handle_data;

      parent = gimp_ui_get_foreign_window ((gpointer) parent_ID);

      if (parent)
        gdk_window_set_transient_for (gtk_widget_get_window (window), parent);

      transient_set = TRUE;
    }
#endif

  if (! transient_set)
    {
      /*  if setting the window transient failed, at least set
       *  WIN_POS_CENTER, which will center the window on the screen
       *  where the mouse is (see bug #684003).
       */
      gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER);

#ifdef GDK_WINDOWING_QUARTZ
      g_signal_connect (window, "show",
                        G_CALLBACK (gimp_window_transient_show),
                        NULL);
#endif
    }

  return FALSE;
}
