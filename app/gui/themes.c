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

#include <stdlib.h>

#include <gegl.h>
#include <gtk/gtk.h>

#ifdef G_OS_WIN32
#include <windows.h>
#endif
#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#endif

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"

#include "gui-types.h"

#include "config/gimpguiconfig.h"

#include "core/gimp.h"

#include "display/gimpimagewindow.h"

#include "widgets/gimpwidgets-utils.h"

#include "icon-themes.h"
#include "themes.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   themes_apply_theme                   (Gimp                  *gimp,
                                                    GimpGuiConfig         *config);
static void   themes_list_themes_foreach           (gpointer               key,
                                                    gpointer               value,
                                                    gpointer               data);
static gint   themes_name_compare                  (const void            *p1,
                                                    const void            *p2);
static void   themes_theme_paths_notify            (GimpExtensionManager  *manager,
                                                    GParamSpec            *pspec,
                                                    Gimp                  *gimp);
#if defined(G_OS_UNIX) && ! defined(__APPLE__)
static void   themes_theme_settings_portal_changed (GDBusProxy            *proxy,
                                                    const gchar           *sender_name,
                                                    const gchar           *signal_name,
                                                    GVariant              *parameters,
                                                    Gimp                  *gimp);
#endif
#if defined(__APPLE__)
static gboolean themes_macos_is_dark_mode_active   (void);
#endif


/*  private variables  */

static GHashTable       *themes_hash            = NULL;
static GtkStyleProvider *themes_style_provider  = NULL;
#if defined(G_OS_UNIX) && ! defined(__APPLE__)
static GDBusProxy       *themes_settings_portal = NULL;
#endif


/*  public functions  */

void
themes_init (Gimp *gimp)
{
  GimpGuiConfig *config;
#if defined(G_OS_UNIX) && ! defined(__APPLE__)
  GError        *error = NULL;
#endif

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  config = GIMP_GUI_CONFIG (gimp->config);

  /* Check for theme extensions. */
  themes_theme_paths_notify (gimp->extension_manager, NULL, gimp);
  g_signal_connect (gimp->extension_manager, "notify::theme-paths",
                    G_CALLBACK (themes_theme_paths_notify),
                    gimp);

  themes_style_provider = GTK_STYLE_PROVIDER (gtk_css_provider_new ());

  /*  Use GTK_STYLE_PROVIDER_PRIORITY_APPLICATION + 1 so theme files
   *  override default styles provided by widgets themselves.
   */
  gtk_style_context_add_provider_for_screen (gdk_screen_get_default (),
                                             themes_style_provider,
                                             GTK_STYLE_PROVIDER_PRIORITY_APPLICATION + 1);

#if defined(G_OS_UNIX) && ! defined(__APPLE__)
  themes_settings_portal =
    g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                   G_DBUS_PROXY_FLAGS_NONE, NULL,
                                   "org.freedesktop.portal.Desktop",
                                   "/org/freedesktop/portal/desktop",
                                   "org.freedesktop.portal.Settings",
                                   NULL, &error);

  if (error)
    {
      g_printerr ("Could not access portal: %s", error->message);
      g_clear_error (&error);
    }
  else
    {
      g_signal_connect (themes_settings_portal,
                        "g-signal::SettingChanged",
                        G_CALLBACK (themes_theme_settings_portal_changed),
                        gimp);
    }
#endif

  g_signal_connect (config, "notify::theme",
                    G_CALLBACK (themes_theme_change_notify),
                    gimp);
  g_signal_connect_after (config, "notify::icon-theme",
                          G_CALLBACK (themes_theme_change_notify),
                          gimp);
  g_signal_connect (config, "notify::theme-color-scheme",
                    G_CALLBACK (themes_theme_change_notify),
                    gimp);
  g_signal_connect (config, "notify::prefer-symbolic-icons",
                    G_CALLBACK (themes_theme_change_notify),
                    gimp);
  g_signal_connect (config, "notify::override-theme-icon-size",
                    G_CALLBACK (themes_theme_change_notify),
                    gimp);
  g_signal_connect (config, "notify::custom-icon-size",
                    G_CALLBACK (themes_theme_change_notify),
                    gimp);
  g_signal_connect (config, "notify::font-relative-size",
                    G_CALLBACK (themes_theme_change_notify),
                    gimp);

  themes_theme_change_notify (config, NULL, gimp);

#ifdef G_OS_WIN32
  themes_set_title_bar (gimp);
#endif
}

void
themes_exit (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  if (themes_hash)
    {
      g_signal_handlers_disconnect_by_func (gimp->config,
                                            themes_theme_change_notify,
                                            gimp);

      g_hash_table_destroy (themes_hash);
      themes_hash = NULL;
    }

  g_clear_object (&themes_style_provider);

#if defined(G_OS_UNIX) && ! defined(__APPLE__)
  g_clear_object (&themes_settings_portal);
#endif
}

gchar **
themes_list_themes (Gimp *gimp,
                    gint *n_themes)
{

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (n_themes != NULL, NULL);

  *n_themes = g_hash_table_size (themes_hash);

  if (*n_themes > 0)
    {
      gchar **themes;
      gchar **index;

      themes = g_new0 (gchar *, *n_themes + 1);

      index = themes;

      g_hash_table_foreach (themes_hash, themes_list_themes_foreach, &index);

      qsort (themes, *n_themes, sizeof (gchar *), themes_name_compare);

      return themes;
    }

  return NULL;
}

GFile *
themes_get_theme_dir (Gimp        *gimp,
                      const gchar *theme_name)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  if (! theme_name)
    theme_name = GIMP_CONFIG_DEFAULT_THEME;

  return g_hash_table_lookup (themes_hash, theme_name);
}

GFile *
themes_get_theme_file (Gimp        *gimp,
                       const gchar *first_component,
                       ...)
{
  GimpGuiConfig *gui_config;
  GFile         *file;
  const gchar   *component;
  va_list        args;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (first_component != NULL, NULL);

  gui_config = GIMP_GUI_CONFIG (gimp->config);

  file      = g_object_ref (themes_get_theme_dir (gimp, gui_config->theme));
  component = first_component;

  va_start (args, first_component);

  do
    {
      GFile *tmp = g_file_get_child (file, component);
      g_object_unref (file);
      file = tmp;
    }
  while ((component = va_arg (args, gchar *)));

  va_end (args);

  if (! g_file_query_exists (file, NULL))
    {
      g_object_unref (file);

      file      = g_object_ref (themes_get_theme_dir (gimp, NULL));
      component = first_component;

      va_start (args, first_component);

      do
        {
          GFile *tmp = g_file_get_child (file, component);
          g_object_unref (file);
          file = tmp;
        }
      while ((component = va_arg (args, gchar *)));

      va_end (args);
    }

  return file;
}

void
themes_theme_change_notify (GimpGuiConfig *config,
                            GParamSpec    *pspec,
                            Gimp          *gimp)
{
  GFile  *theme_css;
  GError *error = NULL;

  g_object_set (gtk_settings_get_for_screen (gdk_screen_get_default ()),
                "gtk-application-prefer-dark-theme",
                config->theme_scheme != GIMP_THEME_LIGHT,
                NULL);

  themes_apply_theme (gimp, config);

  theme_css = gimp_directory_file ("theme.css", NULL);

  if (gimp->be_verbose)
    g_print ("Parsing '%s'\n",
             gimp_file_get_utf8_name (theme_css));

  if (! gtk_css_provider_load_from_file (GTK_CSS_PROVIDER (themes_style_provider),
                                         theme_css, &error))
    {
      g_printerr ("%s: error parsing %s: %s\n", G_STRFUNC,
                  gimp_file_get_utf8_name (theme_css), error->message);
      g_clear_error (&error);
    }

  g_object_unref (theme_css);

  gtk_style_context_reset_widgets (gdk_screen_get_default ());

#ifdef G_OS_WIN32
  themes_set_title_bar (gimp);
#endif
}


/*  private functions  */

static void
themes_apply_theme (Gimp          *gimp,
                    GimpGuiConfig *config)
{
  GFile           *theme_css;
  GOutputStream   *output;
  GError          *error = NULL;
  gboolean         prefer_dark_theme;
  GimpThemeScheme  color_scheme;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (GIMP_IS_GUI_CONFIG (config));

#if defined(G_OS_UNIX) && ! defined(__APPLE__)
  if (themes_settings_portal && config->theme_scheme == GIMP_THEME_SYSTEM)
    {
      GVariant *tuple_variant;
      GVariant *variant;

      tuple_variant = g_dbus_proxy_call_sync (themes_settings_portal,
                                              "ReadOne",
                                              g_variant_new ("(ss)", "org.freedesktop.appearance", "color-scheme"),
                                              G_DBUS_CALL_FLAGS_NONE,
                                              G_MAXINT,
                                              NULL,
                                              &error);
      if (error)
        {
          g_clear_error (&error);

          tuple_variant = g_dbus_proxy_call_sync (themes_settings_portal,
                                                  "Read",
                                                  g_variant_new ("(ss)", "org.freedesktop.appearance", "color-scheme"),
                                                  G_DBUS_CALL_FLAGS_NONE,
                                                  G_MAXINT,
                                                  NULL,
                                                  &error);
          if (!error)
            {
              /*
               * Since the org.freedesktop.portal.Settings.Read method returns
               * two layers of variant, re-assign the tuple.
               * https://flatpak.github.io/xdg-desktop-portal/docs/doc-org.freedesktop.portal.Settings.html#org-freedesktop-portal-settings-read
               */
              g_variant_get (tuple_variant, "(v)", &tuple_variant);

              g_variant_get (tuple_variant, "v", &variant);
            }
        }
      else
        {
          g_variant_get (tuple_variant, "(v)", &variant);
        }

      if (!error)
        {
          /*
           * 1 means "Prefer dark", see:
           * https://flatpak.github.io/xdg-desktop-portal/docs/doc-org.freedesktop.portal.Settings.html#description
           */
          prefer_dark_theme = (g_variant_get_uint32 (variant) == 1);

          /*
           * Note that normally it's a tri-state flag, with a
           * prefer-light and no-preference case too, except that it
           * looks like both in KDE and GNOME at least, they only set
           * prefer-dark or no-preference and for us, the latter should
           * also mean dark.
           * Therefore for this setting to actually mean something, we
           * are currently breaking the spec by having no-preference
           * mean prefer-light. This should be fixed if/when the main
           * desktops actually implement all 3 options some day.
           */
          color_scheme = prefer_dark_theme ? GIMP_THEME_DARK : GIMP_THEME_LIGHT;
        }
      else
        {
          g_printerr ("%s\n", error->message);
          g_clear_error (&error);

          color_scheme = (config->theme_scheme == GIMP_THEME_SYSTEM) ? GIMP_THEME_DARK : config->theme_scheme;
          prefer_dark_theme = (color_scheme == GIMP_THEME_DARK ||
                               color_scheme == GIMP_THEME_GRAY);
        }
    }
  else
#elif defined(G_OS_WIN32)
  if (config->theme_scheme == GIMP_THEME_SYSTEM)
    {
      prefer_dark_theme = gimp_is_win32_system_theme_dark ();
      color_scheme = prefer_dark_theme ? GIMP_THEME_DARK : GIMP_THEME_LIGHT;
    }
  else
#elif defined(__APPLE__)
  if (config->theme_scheme == GIMP_THEME_SYSTEM)
    {
      prefer_dark_theme = themes_macos_is_dark_mode_active ();
      color_scheme = prefer_dark_theme ? GIMP_THEME_DARK : GIMP_THEME_LIGHT;
    }
  else
#endif
    {
      color_scheme = (config->theme_scheme == GIMP_THEME_SYSTEM) ? GIMP_THEME_DARK : config->theme_scheme;
      prefer_dark_theme = (color_scheme == GIMP_THEME_DARK ||
                           color_scheme == GIMP_THEME_GRAY);
    }

  g_object_set (gtk_settings_get_for_screen (gdk_screen_get_default ()),
                "gtk-application-prefer-dark-theme", prefer_dark_theme,
                NULL);

  theme_css = gimp_directory_file ("theme.css", NULL);

  if (gimp->be_verbose)
    g_print ("Writing '%s'\n", gimp_file_get_utf8_name (theme_css));

  output = G_OUTPUT_STREAM (g_file_replace (theme_css,
                                            NULL, FALSE, G_FILE_CREATE_NONE,
                                            NULL, &error));
  if (! output)
    {
      gimp_message_literal (gimp, NULL, GIMP_MESSAGE_ERROR, error->message);
      g_clear_error (&error);
    }
  else
    {
      GFile  *theme_dir = themes_get_theme_dir (gimp, config->theme);
      GFile  *css_user;
      GSList *css_files = NULL;
      GSList *iter;

      if (theme_dir)
        {
          GFile *file = NULL;
          GFile *fallback;
          GFile *light;
          GFile *gray;
          GFile *dark;

          fallback = g_file_get_child (theme_dir, "gimp.css");
          if (! g_file_query_exists (fallback, NULL))
            g_clear_object (&fallback);

          light = g_file_get_child (theme_dir, "gimp-light.css");
          if (! g_file_query_exists (light, NULL))
            g_clear_object (&light);

          gray  = g_file_get_child (theme_dir, "gimp-gray.css");
          if (! g_file_query_exists (gray, NULL))
            g_clear_object (&gray);

          dark  = g_file_get_child (theme_dir, "gimp-dark.css");
          if (! g_file_query_exists (dark, NULL))
            g_clear_object (&dark);

          switch (color_scheme)
            {
            case GIMP_THEME_LIGHT:
              if (light != NULL)
                file = g_object_ref (light);
              else if (fallback != NULL)
                file = g_object_ref (fallback);
              else if (gray != NULL)
                file = g_object_ref (gray);
              else if (dark != NULL)
                file = g_object_ref (dark);
              break;
            case GIMP_THEME_GRAY:
              if (gray != NULL)
                file = g_object_ref (gray);
              else if (fallback != NULL)
                file = g_object_ref (fallback);
              else if (dark != NULL)
                file = g_object_ref (dark);
              else if (light != NULL)
                file = g_object_ref (light);
              break;
            case GIMP_THEME_DARK:
              if (dark != NULL)
                file = g_object_ref (dark);
              else if (fallback != NULL)
                file = g_object_ref (fallback);
              else if (gray != NULL)
                file = g_object_ref (gray);
              else if (light != NULL)
                file = g_object_ref (light);
              break;
            case GIMP_THEME_SYSTEM:
              g_return_if_reached ();
            }

          if (file != NULL)
            {
              prefer_dark_theme = (file == dark || file == gray);
              css_files = g_slist_prepend (css_files, file);
            }
          else
            {
              gimp_message (gimp, NULL, GIMP_MESSAGE_ERROR,
                            _("Invalid theme: directory '%s' contains neither "
                              "gimp-dark.css, gimp-gray.css, gimp-light.css nor gimp.css."),
                            gimp_file_get_utf8_name (theme_dir));
            }

          g_clear_object (&fallback);
          g_clear_object (&light);
          g_clear_object (&gray);
          g_clear_object (&dark);
        }
      else
        {
          gchar *tmp;

          tmp = g_build_filename (gimp_data_directory (),
                                  "themes", "Default", "gimp.css",
                                  NULL);
          css_files = g_slist_prepend (css_files, g_file_new_for_path (tmp));
          g_free (tmp);

          switch (color_scheme)
            {
            case GIMP_THEME_LIGHT:
              tmp = g_build_filename (gimp_data_directory (),
                                      "themes", "Default", "gimp-light.css",
                                      NULL);
              break;
            case GIMP_THEME_GRAY:
              tmp = g_build_filename (gimp_data_directory (),
                                      "themes", "Default", "gimp-gray.css",
                                      NULL);
              break;
            case GIMP_THEME_DARK:
              tmp = g_build_filename (gimp_data_directory (),
                                      "themes", "Default", "gimp-dark.css",
                                      NULL);
              break;
            case GIMP_THEME_SYSTEM:
              g_return_if_reached ();
            }

          css_files = g_slist_prepend (css_files, g_file_new_for_path (tmp));
          g_free (tmp);
        }

      css_files = g_slist_prepend (
        css_files, gimp_sysconf_directory_file ("gimp.css", NULL));

      css_user  = gimp_directory_file ("gimp.css", NULL);
      css_files = g_slist_prepend (
        css_files, css_user);

      css_files = g_slist_reverse (css_files);

      g_output_stream_printf (
        output, NULL, NULL, &error,
        "/* GIMP theme.css\n"
        " *\n"
        " * This file is written on GIMP startup and on every theme change.\n"
        " * It is NOT supposed to be edited manually. Edit your personal\n"
        " * gimp.css file instead (%s).\n"
        " */\n"
        "\n",
        gimp_file_get_utf8_name (css_user));

      for (iter = css_files; ! error && iter; iter = g_slist_next (iter))
        {
          GFile *file = iter->data;

          if (g_file_query_exists (file, NULL))
            {
              gchar *path;

              path = g_file_get_uri (file);

              g_output_stream_printf (
                output, NULL, NULL, &error,
                "@import url(\"%s\");\n",
                path);

              g_free (path);
            }
        }

      if (! error)
        {
          g_output_stream_printf (
            output, NULL, NULL, &error,
            "\n"
            "* { -gtk-icon-style: %s; }\n"
            "\n"
            "%s",
            icon_themes_current_prefer_symbolic (gimp) ? "symbolic" : "regular",
            prefer_dark_theme ? "/* prefer-dark-theme */\n" : "");

        }

      if (! error && config->override_icon_size)
        {
          const gchar *tool_icon_size   = "large-toolbar";
          const gchar *tab_icon_size    = "small-toolbar";
          const gchar *button_icon_size = "small-toolbar";
          gint         pal_padding      = 4;
          gint         tab_padding      = 1;
          gint         sep_padding      = 1;

          switch (config->custom_icon_size)
            {
            case GIMP_ICON_SIZE_SMALL:
              tool_icon_size   = "small-toolbar";
              tab_icon_size    = "small-toolbar";
              button_icon_size = "small-toolbar";
              pal_padding      = 1;
              tab_padding      = 0;
              sep_padding      = 1;
              break;
            case GIMP_ICON_SIZE_MEDIUM:
              tool_icon_size   = "large-toolbar";
              tab_icon_size    = "small-toolbar";
              button_icon_size = "small-toolbar";
              pal_padding      = 4;
              tab_padding      = 1;
              sep_padding      = 1;
              break;
            case GIMP_ICON_SIZE_LARGE:
              tool_icon_size   = "dnd";
              tab_icon_size    = "large-toolbar";
              button_icon_size = "large-toolbar";
              pal_padding      = 5;
              tab_padding      = 5;
              sep_padding      = 2;
              break;
            case GIMP_ICON_SIZE_HUGE:
              tool_icon_size   = "dialog";
              tab_icon_size    = "dnd";
              button_icon_size = "dnd";
              pal_padding      = 5;
              tab_padding      = 8;
              sep_padding      = 4;
              break;
            }

          g_output_stream_printf (
            output, NULL, NULL, &error,
            "\n"
            "* { -GimpToolPalette-tool-icon-size: %s; }"
            "\n"
            "* { -GimpDock-tool-icon-size: %s; }"
            "\n"
            "* { -GimpDockbook-tab-icon-size: %s; }"
            "\n"
            "* { -GimpColorNotebook-tab-icon-size: %s; }"
            "\n"
            "* { -GimpPrefsBox-tab-icon-size: %s; }"
            "\n"
            "* { -GimpEditor-button-icon-size: %s; }"
            "\n"
            "* { -GimpDisplayShell-button-icon-size: %s; }"
            "\n"
            "* { -GimpFgBgEditor-tool-icon-size: %s; }"
            "\n"
            "toolpalette button { padding: %dpx; }"
            "\n"
            "button, tab { padding: %dpx; }"
            "\n"
            "paned separator { padding: %dpx; }",
            tool_icon_size, tool_icon_size, tab_icon_size, tab_icon_size,
            tab_icon_size, button_icon_size, button_icon_size, tool_icon_size,
            pal_padding, tab_padding, sep_padding);
        }

      if (! error && config->font_relative_size != 1.0)
        {
          /* Intermediate buffer for locale-independent float to string
           * conversion. See issue #11048.
           */
          gchar font_size_string[G_ASCII_DTOSTR_BUF_SIZE];

          g_output_stream_printf (output, NULL, NULL, &error,
                                  "\n"
                                  "* { font-size: %srem; }",
                                  g_ascii_dtostr (font_size_string,
                                                  G_ASCII_DTOSTR_BUF_SIZE,
                                                  config->font_relative_size));
        }

      if (! error)
        {
          g_output_stream_printf (
            output, NULL, NULL, &error,
            "\n\n"
            "/* end of theme.css */\n");
        }

      if (error)
        {
          GCancellable *cancellable = g_cancellable_new ();

          gimp_message (gimp, NULL, GIMP_MESSAGE_ERROR,
                        _("Error writing '%s': %s"),
                        gimp_file_get_utf8_name (theme_css), error->message);
          g_clear_error (&error);

          /* Cancel the overwrite initiated by g_file_replace(). */
          g_cancellable_cancel (cancellable);
          g_output_stream_close (output, cancellable, NULL);
          g_object_unref (cancellable);
        }
      else if (! g_output_stream_close (output, NULL, &error))
        {
          gimp_message (gimp, NULL, GIMP_MESSAGE_ERROR,
                        _("Error closing '%s': %s"),
                        gimp_file_get_utf8_name (theme_css), error->message);
          g_clear_error (&error);
        }

      g_slist_free_full (css_files, g_object_unref);
      g_object_unref (output);
    }

  g_object_unref (theme_css);
}

static void
themes_list_themes_foreach (gpointer key,
                            gpointer value,
                            gpointer data)
{
  gchar ***index = data;

  **index = g_strdup ((gchar *) key);

  (*index)++;
}

static gint
themes_name_compare (const void *p1,
                     const void *p2)
{
  return strcmp (* (char **) p1, * (char **) p2);
}

static void
themes_theme_paths_notify (GimpExtensionManager *manager,
                           GParamSpec           *pspec,
                           Gimp                 *gimp)
{
  GimpGuiConfig *config;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  if (themes_hash)
    g_hash_table_remove_all (themes_hash);
  else
    themes_hash = g_hash_table_new_full (g_str_hash,
                                         g_str_equal,
                                         g_free,
                                         g_object_unref);

  config = GIMP_GUI_CONFIG (gimp->config);
  if (config->theme_path)
    {
      GList *path;
      GList *list;

      g_object_get (gimp->extension_manager,
                    "theme-paths", &path,
                    NULL);
      path = g_list_copy_deep (path, (GCopyFunc) g_object_ref, NULL);
      path = g_list_concat (path, gimp_config_path_expand_to_files (config->theme_path, NULL));

      for (list = path; list; list = g_list_next (list))
        {
          GFile           *dir = list->data;
          GFileEnumerator *enumerator;

          enumerator =
            g_file_enumerate_children (dir,
                                       G_FILE_ATTRIBUTE_STANDARD_NAME ","
                                       G_FILE_ATTRIBUTE_STANDARD_IS_HIDDEN ","
                                       G_FILE_ATTRIBUTE_STANDARD_TYPE,
                                       G_FILE_QUERY_INFO_NONE,
                                       NULL, NULL);

          if (enumerator)
            {
              GFileInfo *info;

              while ((info = g_file_enumerator_next_file (enumerator,
                                                          NULL, NULL)))
                {
                  if (! g_file_info_get_attribute_boolean (info, G_FILE_ATTRIBUTE_STANDARD_IS_HIDDEN) &&
                      g_file_info_get_attribute_uint32 (info, G_FILE_ATTRIBUTE_STANDARD_TYPE) == G_FILE_TYPE_DIRECTORY)
                    {
                      GFile       *file;
                      const gchar *name;
                      gchar       *basename;

                      file = g_file_enumerator_get_child (enumerator, info);
                      name = gimp_file_get_utf8_name (file);

                      basename = g_path_get_basename (name);

                      if (gimp->be_verbose)
                        g_print ("Adding theme '%s' (%s)\n",
                                 basename, name);

                      g_hash_table_insert (themes_hash, basename, file);
                    }

                  g_object_unref (info);
                }

              g_object_unref (enumerator);
            }
        }

      g_list_free_full (path, (GDestroyNotify) g_object_unref);
    }
}

#if defined(G_OS_UNIX) && ! defined(__APPLE__)
static void
themes_theme_settings_portal_changed (GDBusProxy  *proxy,
                                      const gchar *sender_name,
                                      const gchar *signal_name,
                                      GVariant    *parameters,
                                      Gimp        *gimp)
{
  const char *namespace;
  const char *name;
  GVariant   *value = NULL;

  if (g_strcmp0 (signal_name, "SettingChanged"))
    return;

  g_variant_get (parameters, "(&s&sv)", &namespace, &name, &value);

  if (g_strcmp0 (namespace, "org.freedesktop.appearance") == 0 &&
      g_strcmp0 (name, "color-scheme") == 0)
    {
      themes_theme_change_notify (GIMP_GUI_CONFIG (gimp->config), NULL, gimp);
    }

  g_variant_unref (value);
}
#endif

#ifdef __APPLE__
static gboolean
themes_macos_is_dark_mode_active (void)
{
  gboolean    is_dark = FALSE;
  CFStringRef style;

  style = CFPreferencesCopyAppValue (CFSTR ("AppleInterfaceStyle"), kCFPreferencesCurrentUser);
  if (style)
    {
      if (CFStringCompare (style, CFSTR ("Dark"), kCFCompareCaseInsensitive) == kCFCompareEqualTo)
        is_dark = TRUE;
      CFRelease(style);
    }

    return is_dark;
}
#endif /* __APPLE__ */

#ifdef G_OS_WIN32
void
themes_set_title_bar (Gimp *gimp)
{
  GList *windows = gimp_get_image_windows (gimp);
  GList *iter;

  for (iter = windows; iter; iter = g_list_next (iter))
    {
      GtkWidget *window = GTK_WIDGET (windows->data);

      gimp_window_set_title_bar_theme (gimp, window);
    }

  if (windows)
    g_list_free (windows);
}
#endif
