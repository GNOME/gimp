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
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"

#include "gimpicons.h"

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
#define GIMP_DEFAULT_ICON_THEME "Default"


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

gboolean
gimp_icons_set_icon_theme (GFile *path)
{
  gchar    *icon_theme_name;
  GFile    *search_path;
  gboolean  success = FALSE;

  g_return_val_if_fail (path == NULL || G_IS_FILE (path), FALSE);

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
        }
      else
        {
          /*  this is the first call upon initialization  */
          icon_theme_path = g_object_ref (path);
        }

      g_object_set (gtk_settings_get_for_screen (gdk_screen_get_default ()),
                    "gtk-icon-theme-name", icon_theme_name,
                    NULL);

      success = TRUE;
    }

  g_free (icon_theme_name);
  g_object_unref (search_path);
  g_object_unref (path);

  return success;
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

  g_object_set (settings,
                "gtk-icon-theme-name", gimp_icon_theme,
                NULL);

  g_free (gimp_icon_theme);

  pixbuf = gdk_pixbuf_new_from_resource ("/org/gimp/icons/64/gimp-wilber-eek.png",
                                         &error);

  if (pixbuf)
    {
      gtk_icon_theme_add_resource_path (gtk_icon_theme_get_default (),
                                        "/org/gimp/icons/64/gimp-wilber-eek.png");
      g_object_unref (pixbuf);
    }
  else
    {
      g_critical ("Failed to create icon image: %s", error->message);
      g_clear_error (&error);
    }

  initialized = TRUE;
}
