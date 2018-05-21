/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpicons.c
 * Copyright (C) 2001-2015 Michael Natterer <mitch@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"

#include "gimpicons.h"

#include "icons/Color/gimp-icon-pixbufs.c"

#include "libgimp/libgimp-intl.h"


/**
 * SECTION: gimpicons
 * @title: GimpIcons
 * @short_description: Prebuilt common menu/toolbar items and
 *                     corresponding icons
 *
 * GIMP registers a set of menu/toolbar items and corresponding icons
 * in addition to the standard GTK+ stock items. These can be used
 * just like GTK+ stock items. GIMP also overrides a few of the GTK+
 * icons (namely the ones in dialog size).
 *
 * Stock icons may have a RTL variant which gets used for
 * right-to-left locales.
 **/


#define LIBGIMP_DOMAIN          GETTEXT_PACKAGE "-libgimp"
#define GIMP_TOILET_PAPER       "gimp-toilet-paper"
#define GIMP_DEFAULT_ICON_THEME "Symbolic"


static GFile *icon_theme_path     = NULL;
static GFile *default_search_path = NULL;


static void
gimp_icons_change_icon_theme (GFile *new_search_path)
{
  GFile *old_search_path = g_file_get_parent (icon_theme_path);

  if (! default_search_path)
    default_search_path = gimp_data_directory_file ("icons", NULL);

  if (! g_file_equal (new_search_path, old_search_path))
    {
      GtkIconTheme *icon_theme = gtk_icon_theme_get_default ();

      if (g_file_equal (old_search_path, default_search_path))
        {
          /*  if the old icon theme is in the default search path,
           *  simply prepend the new theme's path
           */
          gchar *path_str = g_file_get_path (new_search_path);

          gtk_icon_theme_prepend_search_path (icon_theme, path_str);
          g_free (path_str);
        }
      else
        {
          /*  if the old theme is not in the default search path,
           *  we need to deal with the search path's first element
           */
          gchar **paths;
          gint    n_paths;

          gtk_icon_theme_get_search_path (icon_theme, &paths, &n_paths);

          if (g_file_equal (new_search_path, default_search_path))
            {
              /*  when switching to a theme in the default path, remove
               *  the first search path element, the default search path
               *  is still in the search path
               */
              gtk_icon_theme_set_search_path (icon_theme,
                                              (const gchar **) paths + 1,
                                              n_paths - 1);
            }
          else
            {
              /*  when switching between two non-default search paths, replace
               *  the first element of the search path with the new
               *  theme's path
               */
              g_free (paths[0]);
              paths[0] = g_file_get_path (new_search_path);

              gtk_icon_theme_set_search_path (icon_theme,
                                              (const gchar **) paths, n_paths);
            }

          g_strfreev (paths);
        }
    }

  g_object_unref (old_search_path);
}

static void
gimp_icons_notify_system_icon_theme (GObject    *settings,
                                     GParamSpec *param,
                                     gpointer    unused)
{
  GdkScreen *screen = gdk_screen_get_default ();
  GValue     value  = G_VALUE_INIT;

  g_value_init (&value, G_TYPE_STRING);

  if (gdk_screen_get_setting (screen, "gtk-icon-theme-name", &value))
    {
      const gchar *new_system_icon_theme = g_value_get_string (&value);
      gchar *cur_system_icon_theme = NULL;

      g_object_get (settings,
                    "gtk-fallback-icon-theme", &cur_system_icon_theme,
                    NULL);
      if (g_strcmp0 (cur_system_icon_theme, new_system_icon_theme))
        {
          g_object_set (settings,
                        "gtk-fallback-icon-theme", new_system_icon_theme,
                        NULL);

          g_object_notify (settings, "gtk-icon-theme-name");
        }

      g_free (cur_system_icon_theme);
    }

  g_value_unset (&value);
}

static gboolean
gimp_icons_sanity_check (GFile       *path,
                         const gchar *theme_name)
{
  gboolean exists = FALSE;
  GFile *child = g_file_get_child (path, theme_name);

  if (g_file_query_exists (child, NULL))
    {
      GFile *index = g_file_get_child (child, "index.theme");

      if (g_file_query_exists (index, NULL))
        exists = TRUE;
      else
        g_printerr ("%s: Icon theme path has no '%s/index.theme': %s\n",
                    G_STRFUNC, theme_name, gimp_file_get_utf8_name (path));

      g_object_unref (index);
    }
  else
    g_printerr ("%s: Icon theme path has no '%s' subdirectory: %s\n",
                G_STRFUNC, theme_name, gimp_file_get_utf8_name (path));

  g_object_unref (child);

  return exists;
}

static void
gimp_prop_size_icon_notify (GObject    *config,
                            GParamSpec *param_spec,
                            GtkWidget  *image)
{
  GtkIconTheme *icon_theme;
  GtkIconInfo  *icon_info;
  gchar        *icon_name;
  gint          icon_size;

  icon_name = g_object_get_data (G_OBJECT (image), "icon-name");
  g_object_get (config,
                param_spec->name, &icon_size,
                NULL);

  icon_theme = gtk_icon_theme_get_for_screen (gtk_widget_get_screen (image));
  icon_info = gtk_icon_theme_lookup_icon (icon_theme, icon_name, icon_size,
                                          GTK_ICON_LOOKUP_GENERIC_FALLBACK);

  if (icon_info)
    {
      /* Only update the image if we found it. */
      GdkPixbuf *pixbuf;

      pixbuf = gtk_icon_info_load_symbolic_for_context (icon_info,
                                                        gtk_widget_get_style_context (image),
                                                        NULL, NULL);
      gtk_image_set_from_pixbuf (GTK_IMAGE (image), pixbuf);
      g_object_unref (icon_info);
      g_object_unref (pixbuf);
    }
}

void
gimp_icons_set_icon_theme (GFile *path)
{
  gchar *icon_theme_name;
  GFile *search_path;

  g_return_if_fail (path == NULL || G_IS_FILE (path));

  if (path)
    path = g_object_ref (path);
  else
    path = gimp_data_directory_file ("icons", GIMP_DEFAULT_ICON_THEME, NULL);

  search_path = g_file_get_parent (path);
  icon_theme_name = g_file_get_basename (path);

  if (gimp_icons_sanity_check (search_path, "hicolor") &&
      gimp_icons_sanity_check (search_path, icon_theme_name))
    {
      if (icon_theme_path)
        {
          /*  this is an icon theme change  */
          gimp_icons_change_icon_theme (search_path);

          if (! g_file_equal (icon_theme_path, path))
            {
              g_object_unref (icon_theme_path);
              icon_theme_path = g_object_ref (path);
            }

          g_object_set (gtk_settings_get_for_screen (gdk_screen_get_default ()),
                        "gtk-icon-theme-name", icon_theme_name,
                        NULL);
        }
      else
        {
          /*  this is the first call upon initialization  */
          icon_theme_path = g_object_ref (path);
        }
    }

  g_free (icon_theme_name);
  g_object_unref (search_path);
  g_object_unref (path);
}

/**
 * gimp_icons_init:
 *
 * Initializes the GIMP stock icon factory.
 *
 * You don't need to call this function as gimp_ui_init() already does
 * this for you.
 */
void
gimp_icons_init (void)
{
  static gboolean initialized = FALSE;

  GtkSettings *settings;
  GdkPixbuf   *pixbuf;
  GError      *error = NULL;
  gchar       *icons_dir;
  gchar       *system_icon_theme;
  gchar       *gimp_icon_theme;

  if (initialized)
    return;

  /*  always prepend the default icon theme, it's never removed from
   *  the path again and acts as fallback for missing icons in other
   *  themes.
   */
  if (! default_search_path)
    default_search_path = gimp_data_directory_file ("icons",
                                                    NULL);

  icons_dir = g_file_get_path (default_search_path);
  gtk_icon_theme_prepend_search_path (gtk_icon_theme_get_default (),
                                      icons_dir);
  g_free (icons_dir);

  /*  if an icon theme was chosen before init(), change to it  */
  if (icon_theme_path)
    {
      GFile *search_path = g_file_get_parent (icon_theme_path);

      if (!g_file_equal (search_path, default_search_path))
        {
          gchar *icon_dir = g_file_get_path (search_path);

          gtk_icon_theme_prepend_search_path (gtk_icon_theme_get_default (),
                                              icon_dir);
          g_free (icon_dir);
        }
      g_object_unref (search_path);

      gimp_icon_theme = g_file_get_basename (icon_theme_path);
    }
  else
    {
      gimp_icon_theme = g_strdup (GIMP_DEFAULT_ICON_THEME);
    }

  settings = gtk_settings_get_for_screen (gdk_screen_get_default ());

  g_object_get (settings, "gtk-icon-theme-name", &system_icon_theme, NULL);

  g_object_set (settings,
                "gtk-fallback-icon-theme", system_icon_theme,
                "gtk-icon-theme-name", gimp_icon_theme,
                NULL);

  g_free (gimp_icon_theme);
  g_free (system_icon_theme);

  g_signal_connect (settings, "notify::gtk-icon-theme-name",
                    G_CALLBACK (gimp_icons_notify_system_icon_theme), NULL);
  pixbuf = gdk_pixbuf_new_from_resource ("/org/gimp/icons/64/gimp-wilber-eek.png",
                                         &error);

  if (pixbuf)
    {
      gtk_icon_theme_add_builtin_icon (GIMP_ICON_WILBER_EEK, 64, pixbuf);
      g_object_unref (pixbuf);
    }
  else
    {
      g_critical ("Failed to create icon image: %s", error->message);
      g_clear_error (&error);
    }

  initialized = TRUE;
}

/**
 * gimp_icons_get_image:
 * @icon_name: the icon name (without "-symbolic" prefix).
 * @parent: a parent from which styling will be derived.
 * @icon_size: initial requested size for the icon.
 * @config: optional object from which size can be synced.
 * @property_name: Name of int property of @config from which size is
 *                 synced.
 *
 * Lookup an icon in the current icon theme, possibly getting the
 * symbolic version and matching the theme background (for instance in
 * case of dark theme).
 * @parent is used to determine the style information, and in particular
 * this function does not add the image to @parent in any way.
 *
 * The returned image is synced to the theme and the displayed icon will
 * be automatically updated if the theme changes.
 *
 * If @config and @property_name is set, the returned image's size will
 * sync to this property (which must therefore be an int property).
 * In such case @icon_size is useless and can be set to any value.
 *
 * Returns: a #GtkImage displaying @icon_name at @icon_size, or NULL if
 *          the lookup failed.
 *          gtk_widget_show() has been called already on the returned
 *          image, allowing to add it directly to a container.
 */
GtkWidget *
gimp_icon_get_image (const gchar *icon_name,
                     GtkWidget   *parent,
                     gint         icon_size,
                     GObject     *config,
                     const gchar *property_name)
{
  GtkWidget     *image      = NULL;
  GParamSpec    *param_spec = NULL;
  GtkIconTheme  *icon_theme;
  gchar         *symbolic_name;
  GtkIconInfo   *icon_info;

  g_return_val_if_fail (icon_name != NULL, NULL);
  g_return_val_if_fail (parent != NULL, NULL);
  g_return_val_if_fail (icon_size >= 10 || (config && property_name), NULL);

  if (config && property_name)
    {
      param_spec = g_object_class_find_property (G_OBJECT_GET_CLASS (config),
                                                 property_name);

      g_return_val_if_fail (param_spec != NULL, NULL);
      g_return_val_if_fail (g_type_is_a (G_TYPE_FROM_INSTANCE (param_spec),
                                         G_TYPE_PARAM_INT), NULL);

      /* Override the icon size parameter with the current property
       * value.
       */
      g_object_get (config,
                    property_name, &icon_size,
                    NULL);
    }

  icon_theme = gtk_icon_theme_get_for_screen (gtk_widget_get_screen (parent));
  symbolic_name  = g_strdup_printf ("%s-symbolic", icon_name);

  icon_info = gtk_icon_theme_lookup_icon (icon_theme, symbolic_name, icon_size,
                                          GTK_ICON_LOOKUP_GENERIC_FALLBACK);

  if (icon_info)
    {
      GdkPixbuf *pixbuf;

      pixbuf = gtk_icon_info_load_symbolic_for_context (icon_info,
                                                        gtk_widget_get_style_context (parent),
                                                        NULL, NULL);
      image = gtk_image_new_from_pixbuf (pixbuf);
      gtk_widget_show (image);

      g_object_set_data_full (G_OBJECT (image),
                              "icon-name", symbolic_name,
                              g_free);
      g_object_unref (icon_info);
      g_object_unref (pixbuf);

      if (param_spec)
        {
          gchar *notify_name = g_strconcat ("notify::", property_name, NULL);

          g_signal_connect_object (config, notify_name,
                                   G_CALLBACK (gimp_prop_size_icon_notify),
                                   image, 0);

          g_free (notify_name);
        }
    }
  else
    g_free (symbolic_name);

  return image;
}
