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

static void      gimp_ui_help_func              (const gchar       *help_id,
                                                 gpointer           help_data);
static void      gimp_ui_theme_changed          (GFileMonitor      *monitor,
                                                 GFile             *file,
                                                 GFile             *other_file,
                                                 GFileMonitorEvent  event_type,
                                                 GtkCssProvider    *css_provider);
static void      gimp_ensure_modules            (void);
static void      gimp_window_transient_realized (GtkWidget         *window,
                                                 GdkWindow         *parent);
static gboolean  gimp_window_set_transient_for  (GtkWindow         *window,
                                                 GdkWindow         *parent);

#ifdef GDK_WINDOWING_QUARTZ
static void      gimp_osx_focus_window          (void);
#endif


static gboolean gimp_ui_initialized = FALSE;


/*  public functions  */

/**
 * gimp_ui_init:
 * @prog_name: The name of the plug-in which will be passed as argv[0] to
 *             gtk_init(). It's a convention to use the name of the
 *             executable and _not_ the PDB procedure name.
 * @preview:   This parameter is unused and exists for historical
 *             reasons only.
 *
 * This function initializes GTK+ with gtk_init() and initializes GDK's
 * image rendering subsystem (GdkRGB) to follow the GIMP main program's
 * colormap allocation/installation policy.
 *
 * It also sets up various other things so that the plug-in user looks
 * and behaves like the GIMP core. This includes selecting the GTK+
 * theme and setting up the help system as chosen in the GIMP
 * preferences. Any plug-in that provides a user interface should call
 * this function.
 **/
void
gimp_ui_init (const gchar *prog_name,
              gboolean     preview)
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

  gimp_ui_theme_changed (css_monitor, NULL, NULL, G_FILE_MONITOR_NONE,
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
                     gimp_ensure_modules);

  gimp_dialogs_show_help_button (gimp_show_help_button ());

#ifdef GDK_WINDOWING_QUARTZ
  g_idle_add ((GSourceFunc) gimp_osx_focus_window, NULL);
#endif

  gimp_ui_initialized = TRUE;
}

static GdkWindow *
gimp_ui_get_foreign_window (guint32 window)
{
#ifdef GDK_WINDOWING_X11
  return gdk_x11_window_foreign_new_for_display (gdk_display_get_default (),
                                                 window);
#endif

#ifdef GDK_WINDOWING_WIN32
  return gdk_win32_window_foreign_new_for_display (gdk_display_get_default (),
                                                   window);
#endif

  return NULL;
}

/**
 * gimp_ui_get_display_window:
 * @display: a #GimpDisplay.
 *
 * Returns the #GdkWindow of a display window. The purpose is to allow
 * to make plug-in dialogs transient to the image display as explained
 * with gdk_window_set_transient_for().
 *
 * You shouldn't have to call this function directly. Use
 * gimp_window_set_transient_for_display() instead.
 *
 * Returns: (nullable) (transfer full): A reference to a #GdkWindow or %NULL.
 *               You should unref the window using g_object_unref() as
 *               soon as you don't need it any longer.
 *
 * Since: 2.4
 */
GdkWindow *
gimp_ui_get_display_window (GimpDisplay *display)
{
  guint32 window;

  g_return_val_if_fail (gimp_ui_initialized, NULL);

  window = gimp_display_get_window_handle (display);
  if (window)
    return gimp_ui_get_foreign_window (window);

  return NULL;
}

/**
 * gimp_ui_get_progress_window:
 *
 * Returns the #GdkWindow of the window this plug-in's progress bar is
 * shown in. Use it to make plug-in dialogs transient to this window
 * as explained with gdk_window_set_transient_for().
 *
 * You shouldn't have to call this function directly. Use
 * gimp_window_set_transient() instead.
 *
 * Returns: (transfer full): A reference to a #GdkWindow or %NULL.
 *               You should unref the window using g_object_unref() as
 *               soon as you don't need it any longer.
 *
 * Since: 2.4
 */
GdkWindow *
gimp_ui_get_progress_window (void)
{
  guint32  window;

  g_return_val_if_fail (gimp_ui_initialized, NULL);

  window = gimp_progress_get_window_handle ();
  if (window)
     return gimp_ui_get_foreign_window (window);

  return NULL;
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
 * gimp_window_set_transient_for_display:
 * @window:  the #GtkWindow that should become transient
 * @display: display of the image window that should become the parent
 *
 * Indicates to the window manager that @window is a transient dialog
 * associated with the GIMP image window that is identified by it's
 * display ID. See gdk_window_set_transient_for () for more information.
 *
 * Most of the time you will want to use the convenience function
 * gimp_window_set_transient().
 *
 * Since: 2.4
 */
void
gimp_window_set_transient_for_display (GtkWindow   *window,
                                       GimpDisplay *display)
{
  g_return_if_fail (gimp_ui_initialized);
  g_return_if_fail (GTK_IS_WINDOW (window));

  if (! gimp_window_set_transient_for (window,
                                       gimp_ui_get_display_window (display)))
    {
      /*  if setting the window transient failed, at least set
       *  WIN_POS_CENTER, which will center the window on the screen
       *  where the mouse is (see bug #684003).
       */
      gtk_window_set_position (window, GTK_WIN_POS_CENTER);

#ifdef GDK_WINDOWING_QUARTZ
      g_signal_connect (window, "show",
                        G_CALLBACK (gimp_window_transient_show),
                        NULL);
#endif
    }
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
  g_return_if_fail (gimp_ui_initialized);
  g_return_if_fail (GTK_IS_WINDOW (window));

  if (! gimp_window_set_transient_for (window, gimp_ui_get_progress_window ()))
    {
      /*  see above  */
      gtk_window_set_position (window, GTK_WIN_POS_CENTER);

#ifdef GDK_WINDOWING_QUARTZ
      g_signal_connect (window, "show",
                        G_CALLBACK (gimp_window_transient_show),
                        NULL);
#endif
    }
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

static void
gimp_window_transient_realized (GtkWidget *window,
                                GdkWindow *parent)
{
  if (gtk_widget_get_realized (window))
    gdk_window_set_transient_for (gtk_widget_get_window (window), parent);
}

static gboolean
gimp_window_set_transient_for (GtkWindow *window,
                               GdkWindow *parent)
{
  gtk_window_set_transient_for (window, NULL);

#ifndef GDK_WINDOWING_WIN32
  g_signal_handlers_disconnect_matched (window, G_SIGNAL_MATCH_FUNC,
                                        0, 0, NULL,
                                        gimp_window_transient_realized,
                                        NULL);

  if (! parent)
    return FALSE;

  if (gtk_widget_get_realized (GTK_WIDGET (window)))
    gdk_window_set_transient_for (gtk_widget_get_window (GTK_WIDGET (window)),
                                  parent);

  g_signal_connect_object (window, "realize",
                           G_CALLBACK (gimp_window_transient_realized),
                           parent, 0);
  g_object_unref (parent);

  return TRUE;
#endif

  return FALSE;
}

#ifdef GDK_WINDOWING_QUARTZ
static void
gimp_osx_focus_window (void)
{
  [NSApp activateIgnoringOtherApps:YES];
}
#endif


/* Deprecated API. */


/**
 * gimp_ui_get_display_window_deprecated: (skip)
 * @gdisp_ID: a GimpDisplay ID.
 *
 * Returns the #GdkWindow of a display window. The purpose is to allow
 * to make plug-in dialogs transient to the image display as explained
 * with gdk_window_set_transient_for().
 *
 * You shouldn't have to call this function directly. Use
 * gimp_window_set_transient_for_display() instead.
 *
 * Returns: (nullable) (transfer full): A reference to a #GdkWindow or %NULL.
 *               You should unref the window using g_object_unref() as
 *               soon as you don't need it any longer.
 *
 * Since: 2.4
 */
GdkWindow *
gimp_ui_get_display_window_deprecated (guint32 gdisp_ID)
{
  return gimp_ui_get_display_window (gimp_display_get_by_id (gdisp_ID));
}

/**
 * gimp_window_set_transient_for_display_deprecated: (skip)
 * @window:   the #GtkWindow that should become transient
 * @gdisp_ID: display ID of the image window that should become the parent
 *
 * Indicates to the window manager that @window is a transient dialog
 * associated with the GIMP image window that is identified by it's
 * display ID.  See gdk_window_set_transient_for () for more information.
 *
 * Most of the time you will want to use the convenience function
 * gimp_window_set_transient().
 *
 * Since: 2.4
 */
void
gimp_window_set_transient_for_display_deprecated (GtkWindow *window,
                                                  guint32    gdisp_ID)
{
  gimp_window_set_transient_for_display (window,
                                         gimp_display_get_by_id (gdisp_ID));
}
