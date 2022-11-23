/* LIBLIGMA - The LIGMA Library
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

#include "ligma.h"
#include "ligmaui.h"

#include "libligmamodule/ligmamodule.h"

#include "libligmawidgets/ligmawidgets.h"
#include "libligmawidgets/ligmawidgets-private.h"


/**
 * SECTION: ligmaui
 * @title: ligmaui
 * @short_description: Common user interface functions. This header includes
 *                     all other LIGMA User Interface Library headers.
 * @see_also: gtk_init(), gdk_set_use_xshm(), gtk_widget_set_default_visual().
 *
 * Common user interface functions. This header includes all other
 * LIGMA User Interface Library headers.
 **/


/*  local function prototypes  */

static void      ligma_ui_help_func              (const gchar       *help_id,
                                                 gpointer           help_data);
static void      ligma_ui_theme_changed          (GFileMonitor      *monitor,
                                                 GFile             *file,
                                                 GFile             *other_file,
                                                 GFileMonitorEvent  event_type,
                                                 GtkCssProvider    *css_provider);
static void      ligma_ensure_modules            (void);
#ifndef GDK_WINDOWING_WIN32
static void      ligma_window_transient_realized (GtkWidget         *window,
                                                 GdkWindow         *parent);
#endif
static gboolean  ligma_window_set_transient_for  (GtkWindow         *window,
                                                 GdkWindow         *parent);

#ifdef GDK_WINDOWING_QUARTZ
static gboolean  ligma_osx_focus_window          (gpointer);
#endif


static gboolean ligma_ui_initialized = FALSE;


/*  public functions  */

/**
 * ligma_ui_init:
 * @prog_name: The name of the plug-in which will be passed as argv[0] to
 *             gtk_init(). It's a convention to use the name of the
 *             executable and _not_ the PDB procedure name.
 *
 * This function initializes GTK+ with gtk_init().
 *
 * It also sets up various other things so that the plug-in user looks
 * and behaves like the LIGMA core. This includes selecting the GTK+
 * theme and setting up the help system as chosen in the LIGMA
 * preferences. Any plug-in that provides a user interface should call
 * this function.
 **/
void
ligma_ui_init (const gchar *prog_name)
{
  const gchar    *display_name;
  GtkCssProvider *css_provider;
  GFileMonitor   *css_monitor;
  GFile          *file;

  g_return_if_fail (prog_name != NULL);

  if (ligma_ui_initialized)
    return;

  g_set_prgname (prog_name);

  display_name = ligma_display_name ();

  if (display_name)
    {
#if defined (GDK_WINDOWING_X11)
      g_setenv ("DISPLAY", display_name, TRUE);
#else
      g_setenv ("GDK_DISPLAY", display_name, TRUE);
#endif
    }

  if (ligma_user_time ())
    {
      /* Construct a fake startup ID as we only want to pass the
       * interaction timestamp, see _gdk_windowing_set_default_display().
       */
      gchar *startup_id = g_strdup_printf ("_TIME%u", ligma_user_time ());

      g_setenv ("DESKTOP_STARTUP_ID", startup_id, TRUE);
      g_free (startup_id);
    }

  gtk_init (NULL, NULL);

  css_provider = gtk_css_provider_new ();
  gtk_style_context_add_provider_for_screen (gdk_screen_get_default (),
                                             GTK_STYLE_PROVIDER (css_provider),
                                             GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

  file = ligma_directory_file ("theme.css", NULL);
  css_monitor = g_file_monitor (file, G_FILE_MONITOR_NONE, NULL, NULL);
  g_object_unref (file);

  ligma_ui_theme_changed (css_monitor, NULL, NULL, G_FILE_MONITOR_NONE,
                         css_provider);

  g_signal_connect (css_monitor, "changed",
                    G_CALLBACK (ligma_ui_theme_changed),
                    css_provider);

  g_object_unref (css_provider);

  gdk_set_program_class (ligma_wm_class ());

  if (ligma_icon_theme_dir ())
    {
      file = g_file_new_for_path (ligma_icon_theme_dir ());
      ligma_icons_set_icon_theme (file);
      g_object_unref (file);
    }

  ligma_widgets_init (ligma_ui_help_func,
                     ligma_context_get_foreground,
                     ligma_context_get_background,
                     ligma_ensure_modules, NULL);

  ligma_dialogs_show_help_button (ligma_show_help_button ());

#ifdef GDK_WINDOWING_QUARTZ
  g_idle_add (ligma_osx_focus_window, NULL);
#endif

  ligma_ui_initialized = TRUE;
}

static GdkWindow *
ligma_ui_get_foreign_window (guint32 window)
{
#ifdef GDK_WINDOWING_X11
  if (GDK_IS_X11_DISPLAY (gdk_display_get_default ()))
    return gdk_x11_window_foreign_new_for_display (gdk_display_get_default (),
                                                   window);
#endif

#ifdef GDK_WINDOWING_WIN32
  return gdk_win32_window_foreign_new_for_display (gdk_display_get_default (),
                                                   (HWND) (uintptr_t) window);
#endif

  return NULL;
}

/**
 * ligma_ui_get_display_window:
 * @display: a #LigmaDisplay.
 *
 * Returns the #GdkWindow of a display window. The purpose is to allow
 * to make plug-in dialogs transient to the image display as explained
 * with gdk_window_set_transient_for().
 *
 * You shouldn't have to call this function directly. Use
 * ligma_window_set_transient_for_display() instead.
 *
 * Returns: (nullable) (transfer full): A reference to a #GdkWindow or %NULL.
 *               You should unref the window using g_object_unref() as
 *               soon as you don't need it any longer.
 *
 * Since: 2.4
 */
GdkWindow *
ligma_ui_get_display_window (LigmaDisplay *display)
{
  guint32 window;

  g_return_val_if_fail (ligma_ui_initialized, NULL);

  window = ligma_display_get_window_handle (display);
  if (window)
    return ligma_ui_get_foreign_window (window);

  return NULL;
}

/**
 * ligma_ui_get_progress_window:
 *
 * Returns the #GdkWindow of the window this plug-in's progress bar is
 * shown in. Use it to make plug-in dialogs transient to this window
 * as explained with gdk_window_set_transient_for().
 *
 * You shouldn't have to call this function directly. Use
 * ligma_window_set_transient() instead.
 *
 * Returns: (transfer full): A reference to a #GdkWindow or %NULL.
 *               You should unref the window using g_object_unref() as
 *               soon as you don't need it any longer.
 *
 * Since: 2.4
 */
GdkWindow *
ligma_ui_get_progress_window (void)
{
  guint32  window;

  g_return_val_if_fail (ligma_ui_initialized, NULL);

  window = ligma_progress_get_window_handle ();
  if (window)
     return ligma_ui_get_foreign_window (window);

  return NULL;
}

#ifdef GDK_WINDOWING_QUARTZ
static void
ligma_window_transient_show (GtkWidget *window)
{
  g_signal_handlers_disconnect_by_func (window,
                                        ligma_window_transient_show,
                                        NULL);
  [NSApp arrangeInFront: nil];
}
#endif

/**
 * ligma_window_set_transient_for_display:
 * @window:  the #GtkWindow that should become transient
 * @display: display of the image window that should become the parent
 *
 * Indicates to the window manager that @window is a transient dialog
 * associated with the LIGMA image window that is identified by it's
 * display ID. See gdk_window_set_transient_for () for more information.
 *
 * Most of the time you will want to use the convenience function
 * ligma_window_set_transient().
 *
 * Since: 2.4
 */
void
ligma_window_set_transient_for_display (GtkWindow   *window,
                                       LigmaDisplay *display)
{
  g_return_if_fail (ligma_ui_initialized);
  g_return_if_fail (GTK_IS_WINDOW (window));

  if (! ligma_window_set_transient_for (window,
                                       ligma_ui_get_display_window (display)))
    {
      /*  if setting the window transient failed, at least set
       *  WIN_POS_CENTER, which will center the window on the screen
       *  where the mouse is (see bug #684003).
       */
      gtk_window_set_position (window, GTK_WIN_POS_CENTER);

#ifdef GDK_WINDOWING_QUARTZ
      g_signal_connect (window, "show",
                        G_CALLBACK (ligma_window_transient_show),
                        NULL);
#endif
    }
}

/**
 * ligma_window_set_transient:
 * @window: the #GtkWindow that should become transient
 *
 * Indicates to the window manager that @window is a transient dialog
 * associated with the LIGMA window that the plug-in has been
 * started from. See also ligma_window_set_transient_for_display().
 *
 * Since: 2.4
 */
void
ligma_window_set_transient (GtkWindow *window)
{
  g_return_if_fail (ligma_ui_initialized);
  g_return_if_fail (GTK_IS_WINDOW (window));

  if (! ligma_window_set_transient_for (window, ligma_ui_get_progress_window ()))
    {
      /*  see above  */
      gtk_window_set_position (window, GTK_WIN_POS_CENTER);

#ifdef GDK_WINDOWING_QUARTZ
      g_signal_connect (window, "show",
                        G_CALLBACK (ligma_window_transient_show),
                        NULL);
#endif
    }
}


/*  private functions  */

static void
ligma_ui_help_func (const gchar *help_id,
                   gpointer     help_data)
{
  ligma_help (NULL, help_id);
}

static void
ligma_ui_theme_changed (GFileMonitor      *monitor,
                       GFile             *file,
                       GFile             *other_file,
                       GFileMonitorEvent  event_type,
                       GtkCssProvider    *css_provider)
{
  GError *error = NULL;
  gchar  *contents;

  file = ligma_directory_file ("theme.css", NULL);

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
                  ligma_file_get_utf8_name (file), error->message);
      g_clear_error (&error);
    }

  if (! gtk_css_provider_load_from_file (css_provider, file, &error))
    {
      g_printerr ("%s: error parsing %s: %s\n", G_STRFUNC,
                  ligma_file_get_utf8_name (file), error->message);
      g_clear_error (&error);
    }

  g_object_unref (file);
}

static void
ligma_ensure_modules (void)
{
  static LigmaModuleDB *module_db = NULL;

  if (! module_db)
    {
      gchar *load_inhibit = ligma_get_module_load_inhibit ();
      gchar *module_path  = ligma_ligmarc_query ("module-path");

      module_db = ligma_module_db_new (FALSE);

      ligma_module_db_set_load_inhibit (module_db, load_inhibit);
      ligma_module_db_load (module_db, module_path);

      g_free (module_path);
      g_free (load_inhibit);
    }
}

#ifndef GDK_WINDOWING_WIN32
static void
ligma_window_transient_realized (GtkWidget *window,
                                GdkWindow *parent)
{
  if (gtk_widget_get_realized (window))
    gdk_window_set_transient_for (gtk_widget_get_window (window), parent);
}
#endif

static gboolean
ligma_window_set_transient_for (GtkWindow *window,
                               GdkWindow *parent)
{
  gtk_window_set_transient_for (window, NULL);

  /* To know why it is disabled on Win32, see
   * ligma_window_set_transient_for() in app/widgets/ligmawidgets-utils.c.
   */
#ifndef GDK_WINDOWING_WIN32
  g_signal_handlers_disconnect_matched (window, G_SIGNAL_MATCH_FUNC,
                                        0, 0, NULL,
                                        ligma_window_transient_realized,
                                        NULL);

  if (! parent)
    return FALSE;

  if (gtk_widget_get_realized (GTK_WIDGET (window)))
    gdk_window_set_transient_for (gtk_widget_get_window (GTK_WIDGET (window)),
                                  parent);

  g_signal_connect_object (window, "realize",
                           G_CALLBACK (ligma_window_transient_realized),
                           parent, 0);
  g_object_unref (parent);

  return TRUE;
#endif

  return FALSE;
}

#ifdef GDK_WINDOWING_QUARTZ
static gboolean
ligma_osx_focus_window (gpointer user_data)
{
  [NSApp activateIgnoringOtherApps:YES];
  return FALSE;
}
#endif
