/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpwidgets-private.c
 * Copyright (C) 2003 Sven Neumann <sven@gimp.org>
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

#include <babl/babl.h>
#include <gegl.h>
#include <gtk/gtk.h>

#ifdef _WIN32
#include <dwmapi.h>
#include <gdk/gdkwin32.h>

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif
#endif

#ifdef PLATFORM_OSX
#import <AppKit/AppKit.h>
#endif

#include "libgimpbase/gimpbase.h"

#include "gimpwidgetstypes.h"

#include "gimpicons.h"
#include "gimpwidgets-private.h"

#include "libgimp/libgimp-intl.h"


static gboolean       gimp_widgets_initialized  = FALSE;

GimpHelpFunc          _gimp_standard_help_func  = NULL;
GimpGetColorFunc      _gimp_get_foreground_func = NULL;
GimpGetColorFunc      _gimp_get_background_func = NULL;
GimpEnsureModulesFunc _gimp_ensure_modules_func = NULL;

#if !defined(G_OS_WIN32) && !defined(PLATFORM_OSX)
static void
gimp_widgets_init_platform_settings (void)
{
  GtkSettings *settings = gtk_settings_get_default ();

  if (settings == NULL)
    return;

  /* prefer KDE_FULL_SESSION instead of XDG_CURRENT_DESKTOP and
     KDE_SESSION_VERSION instead of XDG_SESSION_DESKTOP since
     the formers are set by KDE, with no margin for error */
  if (g_getenv ("KDE_FULL_SESSION") != NULL &&
      g_getenv ("KDE_SESSION_VERSION") != NULL)
    g_object_set (settings,
                  "gtk-alternative-button-order", TRUE,
                  "gtk-dialogs-use-header",       FALSE,
                  NULL);
}
#endif

static void
gimp_widgets_init_foreign_enums (void)
{
  static const GimpEnumDesc input_mode_descs[] =
  {
    { GDK_MODE_DISABLED, NC_("input-mode", "Disabled"), NULL },
    { GDK_MODE_SCREEN,   NC_("input-mode", "Screen"),   NULL },
    { GDK_MODE_WINDOW,   NC_("input-mode", "Window"),   NULL },
    { 0, NULL, NULL }
  };

  gimp_type_set_translation_domain (GDK_TYPE_INPUT_MODE,
                                    GETTEXT_PACKAGE "-libgimp");
  gimp_type_set_translation_context (GDK_TYPE_INPUT_MODE, "input-mode");
  gimp_enum_set_value_descriptions (GDK_TYPE_INPUT_MODE, input_mode_descs);
}

void
gimp_widgets_init (GimpHelpFunc           standard_help_func,
                   GimpGetColorFunc       get_foreground_func,
                   GimpGetColorFunc       get_background_func,
                   GimpEnsureModulesFunc  ensure_modules_func,
                   const gchar           *test_base_dir)
{
  GList       *icons   = NULL;
  const gchar *cat_dir;
  gchar       *base_dir;
  gchar       *path;
  GdkPixbuf   *pixbuf;
  GError      *error = NULL;

  g_return_if_fail (standard_help_func != NULL);

  if (gimp_widgets_initialized)
    g_error ("gimp_widgets_init() must only be called once!");

  _gimp_standard_help_func  = standard_help_func;
  _gimp_get_foreground_func = get_foreground_func;
  _gimp_get_background_func = get_background_func;
  _gimp_ensure_modules_func = ensure_modules_func;

  babl_init (); /* color selectors use babl */

  gimp_icons_init ();

  if (test_base_dir)
    {
      cat_dir  = "";
      base_dir = g_build_filename (test_base_dir, "desktop", NULL);
    }
  else
    {
      cat_dir  = "apps";
#ifdef ENABLE_RELOCATABLE_RESOURCES
      base_dir = g_build_filename (gimp_installation_directory (), "share", "icons", "hicolor", NULL);
#else
      base_dir = g_build_filename (DATAROOTDIR, "icons", "hicolor", NULL);
#endif
    }

  /* Loading the application icons. Unfortunately GTK doesn't know how
   * to load any size from a single SVG, so we have to generate common
   * sizes ourselves.
   * To be fair, it could with gtk_window_set_default_icon_name() but
   * then the application icon is dependant to the theme and for now at
   * least, we want the installed icon.
   */
  path   = g_build_filename (base_dir, "16x16", cat_dir, "gimp.png", NULL);
  pixbuf = gdk_pixbuf_new_from_file (path, &error);
  if (pixbuf)
    icons = g_list_prepend (icons, pixbuf);
  else
    g_printerr ("Application icon missing: %s (%s)\n", path, error->message);
  g_clear_error (&error);
  g_free (path);

  path   = g_build_filename (base_dir, "32x32", cat_dir, "gimp.png", NULL);
  pixbuf = gdk_pixbuf_new_from_file (path, &error);
  if (pixbuf)
    icons = g_list_prepend (icons, pixbuf);
  else
    g_printerr ("Application icon missing: %s (%s)\n", path, error->message);
  g_clear_error (&error);
  g_free (path);

  path   = g_build_filename (base_dir, "48x48", cat_dir, "gimp.png", NULL);
  pixbuf = gdk_pixbuf_new_from_file (path, &error);
  if (pixbuf)
    icons = g_list_prepend (icons, pixbuf);
  else
    g_printerr ("Application icon missing: %s (%s)\n", path, error->message);
  g_clear_error (&error);
  g_free (path);

  path   = g_build_filename (base_dir, "64x64", cat_dir, "gimp.png", NULL);
  pixbuf = gdk_pixbuf_new_from_file (path, &error);
  if (pixbuf)
    icons = g_list_prepend (icons, pixbuf);
  else
    g_printerr ("Application icon missing: %s (%s)\n", path, error->message);
  g_clear_error (&error);
  g_free (path);

  path   = g_build_filename (base_dir, "scalable", cat_dir, "gimp.svg", NULL);
  pixbuf = gdk_pixbuf_new_from_file_at_size (path, 128, 128, &error);
  if (pixbuf)
    {
      /* Various common sizes from the same SVG. Why I go into such
       * exhaustive list of sizes is that nowadays desktops/OSes use
       * quite big icon sizes and in some cases, when they don't find
       * the right one, GTK may render quite ugly resized/skewed image.
       */
      icons = g_list_prepend (icons, pixbuf);

      pixbuf = gdk_pixbuf_new_from_file_at_size (path, 144, 144, NULL);
      icons = g_list_prepend (icons, pixbuf);

      pixbuf = gdk_pixbuf_new_from_file_at_size (path, 160, 160, NULL);
      icons = g_list_prepend (icons, pixbuf);

      pixbuf = gdk_pixbuf_new_from_file_at_size (path, 176, 176, NULL);
      icons = g_list_prepend (icons, pixbuf);

      pixbuf = gdk_pixbuf_new_from_file_at_size (path, 192, 192, NULL);
      icons = g_list_prepend (icons, pixbuf);

      pixbuf = gdk_pixbuf_new_from_file_at_size (path, 224, 224, NULL);
      icons = g_list_prepend (icons, pixbuf);
    }
  else
    {
      g_printerr ("Application icon missing: %s (%s)\n", path, error->message);
      g_clear_error (&error);
    }
  g_free (path);

  path   = g_build_filename (base_dir, "256x256", cat_dir, "gimp.png", NULL);
  pixbuf = gdk_pixbuf_new_from_file (path, &error);
  if (pixbuf)
    icons = g_list_prepend (icons, pixbuf);
  else
    g_printerr ("Application icon missing: %s\n", path);
  g_free (path);

  gtk_window_set_default_icon_list (icons);
  g_list_free_full (icons, g_object_unref);

  gimp_widgets_init_foreign_enums ();

#if !defined(G_OS_WIN32) && !defined(PLATFORM_OSX)
  /* align GtkSettings with the UX conventions of the running platform.
     GTK already do the right thing on Windows and macOS. KDE Plasma follows
     the same conventions of Windows but GTK does not detect it. See: #11606 */
  gimp_widgets_init_platform_settings ();
#endif

  gimp_widgets_initialized = TRUE;
  g_free (base_dir);
}

/**
 * gimp_widget_set_identifier:
 * @widget:
 * @identifier:
 *
 * Set an identifier which can be used by the various gimp_blink_*()
 * API. As a default, property widget will use the synced property name
 * as widget identifier. You can always use this function to override a
 * given widget identifier with a more specific name.
 *
 * Note that when a widget is bound to a property, in other words when
 * in one of our propwidgets API, you should rather use
 * gimp_widget_set_bound_property() because it allows more easily to
 * tweak values.
 * gimp_widget_set_identifier() is more destined to random widgets which
 * you just want to be able to blink.
 *
 * It's doesn't need to be in public API because it is only used for
 * internal blinking ability in core GIMP GUI.
 */
void
gimp_widget_set_identifier (GtkWidget   *widget,
                            const gchar *identifier)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  g_object_set_data_full (G_OBJECT (widget),
                          "gimp-widget-identifier",
                          g_strdup (identifier),
                          (GDestroyNotify) g_free);
}

/**
 * gimp_widget_set_bound_property:
 * @widget:
 * @config:
 * @property_name:
 *
 * This is similar to gimp_widget_set_identifier() because the
 * property_name can be used as identifier by our blink API.
 * You can still set explicitly (and additionally)
 * gimp_widget_set_identifier() for rare cases where 2 widgets in a same
 * container would bind the same property (or 2 properties named the
 * same way for 2 different config objects). The identifier will be used
 * in priority to the property name (which can still be used for
 * changing the widget value, so it remains important to also set it).
 *
 * It's doesn't need to be in public API because it is only used for
 * internal blinking ability in core GIMP GUI.
 */
void
gimp_widget_set_bound_property (GtkWidget   *widget,
                                GObject     *config,
                                const gchar *property_name)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  g_object_set_data_full (G_OBJECT (widget),
                          "gimp-widget-property-name",
                          g_strdup (property_name),
                          (GDestroyNotify) g_free);
  g_object_set_data_full (G_OBJECT (widget),
                          "gimp-widget-property-config",
                          g_object_ref (config),
                          (GDestroyNotify) g_object_unref);
}

/**
 * gimp_widget_set_title_bar_theme:
 * @dialog:
 *
 * An implementation of gimp_window_set_title_bar_theme () that can be used
 * in core plug-ins without making it publicly available. It will no-op when
 * called on platforms besides Windows and macOS.
 *
 */
void
gimp_widget_set_title_bar_theme (GtkWidget *dialog)
{
#if defined (G_OS_WIN32) || \
    (defined (PLATFORM_OSX) && MAC_OS_X_VERSION_MIN_REQUIRED >= 101400)
#ifdef G_OS_WIN32
  HWND           hwnd;
#endif
  GdkWindow     *window        = NULL;
  gboolean       use_dark_mode = FALSE;

  window = gtk_widget_get_window (GTK_WIDGET (dialog));
  if (window)
    {
      GtkStyleContext *style;
      GdkRGBA         *color = NULL;

      style = gtk_widget_get_style_context (dialog);
      gtk_style_context_get (style, gtk_style_context_get_state (style),
                             GTK_STYLE_PROPERTY_BACKGROUND_COLOR, &color,
                             NULL);
      if (color)
        {
          if (color->red < 0.5 && color->green < 0.5 && color->blue < 0.5)
            use_dark_mode = TRUE;

          gdk_rgba_free (color);
        }

#ifdef G_OS_WIN32
        hwnd = (HWND) gdk_win32_window_get_handle (window);
        DwmSetWindowAttribute (hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE,
                               &use_dark_mode, sizeof (use_dark_mode));
#elif defined(PLATFORM_OSX)
        if (use_dark_mode)
          [NSApp setAppearance:[NSAppearance appearanceNamed:NSAppearanceNameDarkAqua]];
        else
          [NSApp setAppearance:[NSAppearance appearanceNamed:NSAppearanceNameAqua]];
#endif
    }
#endif
}

/**
 * gimp_widget_set_auto_transient:
 * @dialog:
 *
 * The same auto-transient logic that #GimpDialog applies to itself, exposed
 * for core windows that are not #GimpDialog (e.g. the About dialog and the
 * action search popup). It will no-op on platforms other than macOS.
 *
 */
void
gimp_widget_set_auto_transient (GtkWidget *dialog)
{
#ifdef PLATFORM_OSX
  NSWindow    *parent_window            = nil;
  NSWindow    *dialog_window            = nil;
  const gchar *dialog_window_gtk_title  = gtk_window_get_title (GTK_WINDOW (dialog));
  NSString    *dialog_window_ns_title   = dialog_window_gtk_title ? [NSString stringWithUTF8String:dialog_window_gtk_title] : nil;

  /* we cycle since this is the only reliable way to checking the parent window,
     gdk_quartz_window_get_nswindow would not be enough */
  for (NSWindow *win in [NSApp windows])
    {
      if (! [win isVisible])
        continue;

      /* child dialog window */
      if (! dialog_window && [win canBecomeKeyWindow] && [win parentWindow] == nil)
        {
          if (dialog_window_ns_title && [[win title] isEqualToString:dialog_window_ns_title])
            dialog_window = win;
        }

      /* gimp main window */
      if (! parent_window && [win canBecomeMainWindow] && [win parentWindow] == nil && ! [win isSheet])
        {
          if ((! dialog_window_ns_title || ![[win title] isEqualToString:dialog_window_ns_title]) && win != dialog_window)
            parent_window = win;
        }

      if (dialog_window && parent_window)
        break;
    }
  if (parent_window && dialog_window)
    {
      /* Ideally we should use addChildWindow but it breaks animations
      [parent_window addChildWindow:dialog_window ordered:NSWindowAbove]; */
      __block NSWindow     *focus_saved         = nil;
      __block id            activate_observer   = nil;
      __block id            deactivate_observer = nil;
      __block id            focus_save_observer = nil;
      __block id            focus_set_observer  = nil;
      __block id            close_observer      = nil;
      NSNotificationCenter *center              = [NSNotificationCenter defaultCenter];

      /* gimp main window is active, show dialog on top always */
      [dialog_window setLevel:NSFloatingWindowLevel];
      [dialog_window setHidesOnDeactivate:YES];
      activate_observer = [center
                           addObserverForName:NSWindowDidBecomeKeyNotification
                           object:parent_window
                           queue:[NSOperationQueue mainQueue]
                           usingBlock:^(NSNotification * _Nonnull note) {
        if (! [dialog_window isVisible])
          [dialog_window orderFrontRegardless];
      }];

      /* gimp main window is not active, hide dialog */
      deactivate_observer = [center
                             addObserverForName:NSWindowDidResignKeyNotification
                             object:parent_window
                             queue:[NSOperationQueue mainQueue]
                             usingBlock:^(NSNotification * _Nonnull note) {
        dispatch_async(dispatch_get_main_queue(), ^{
          NSWindow *key_window = [NSApp keyWindow];
          if ([NSApp isActive] && key_window != parent_window && key_window != dialog_window)
            [dialog_window orderOut:nil];
        });
      }];

      /* gimp main window is not active, save last focused window/dialog */
      focus_save_observer = [center
                             addObserverForName:NSApplicationWillResignActiveNotification
                             object:nil
                             queue:[NSOperationQueue mainQueue]
                             usingBlock:^(NSNotification * _Nonnull note) {
        focus_saved = [NSApp keyWindow];
      }];

      /* gimp main window is active, show dialog focused or not */
      focus_set_observer = [center
                            addObserverForName:NSApplicationDidBecomeActiveNotification
                            object:nil
                            queue:[NSOperationQueue mainQueue]
                            usingBlock:^(NSNotification * _Nonnull note) {
        dispatch_async(dispatch_get_main_queue(), ^{
          if ([parent_window isMiniaturized])
            [parent_window deminiaturize:nil];
          [parent_window orderFront:nil];
          if ([dialog_window isVisible])
            [dialog_window orderWindow:NSWindowAbove relativeTo:[parent_window windowNumber]];

          if (focus_saved == parent_window)
            [parent_window makeKeyWindow];
          else
            [dialog_window makeKeyAndOrderFront:nil];
        });
      }];

      /* gimp main window is active but dialog is gone, show main window focused */
      close_observer = [center
                        addObserverForName:NSWindowWillCloseNotification
                        object:dialog_window
                        queue:[NSOperationQueue mainQueue]
                        usingBlock:^(NSNotification * _Nonnull note) {
        [parent_window makeKeyWindow];

        if (activate_observer)   [center removeObserver:activate_observer];
        if (deactivate_observer) [center removeObserver:deactivate_observer];
        if (focus_save_observer) [center removeObserver:focus_save_observer];
        if (focus_set_observer)  [center removeObserver:focus_set_observer];
        if (close_observer)      [center removeObserver:close_observer];
      }];

      {
        /* On the first opening, center the dialog like Linux and Windows. See: #871 */
        NSRect  parent_frame      = [parent_window frame];
        NSRect  dialog_frame      = [dialog_window frame];
        CGFloat dialog_position_x = parent_frame.origin.x + (parent_frame.size.width - dialog_frame.size.width) / 2.0;
        CGFloat dialog_position_y = parent_frame.origin.y + (parent_frame.size.height - dialog_frame.size.height) / 2.0;

        [dialog_window setFrameOrigin:NSMakePoint(dialog_position_x, dialog_position_y)];
      }
    }
#endif
}

/* clean up babl (in particular, so that the fish cache is constructed) if the
 * compiler supports destructors
 */
#ifdef HAVE_FUNC_ATTRIBUTE_DESTRUCTOR

__attribute__ ((destructor))
static void
gimp_widgets_exit (void)
{
  if (gimp_widgets_initialized)
    babl_exit ();
}

#elif defined (__GNUC__)

#warning babl_init() not paired with babl_exit()

#endif
