/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * icon-themes.c
 * Copyright (C) 2015 Benoit Touchette
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

#include <stdlib.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "gui-types.h"

#include "config/gimpguiconfig.h"

#include "core/gimp.h"

#include "icon-themes.h"

#include "gimp-intl.h"


static void   icons_list_icons_foreach  (gpointer       key,
                                         gpointer       value,
                                         gpointer       data);
static gint   icons_name_compare        (const void    *p1,
                                         const void    *p2);
static void   icons_theme_change_notify (GimpGuiConfig *config,
                                         GParamSpec    *pspec,
                                         Gimp          *gimp);
void          icons_set_icon_theme      (GtkIconTheme  *new_icon_theme);
static void   icons_apply_theme         (Gimp          *gimp,
                                         const gchar   *icon_theme_name);


static GHashTable *icon_themes_hash = NULL;


void
icon_themes_init (Gimp *gimp)
{
  GimpGuiConfig *config;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  config = GIMP_GUI_CONFIG (gimp->config);

  icon_themes_hash = g_hash_table_new_full (g_str_hash,
                                            g_str_equal,
                                            g_free,
                                            g_object_unref);

  if (config->icon_theme_path)
    {
      GList *path;
      GList *list;

      path = gimp_config_path_expand_to_files (config->icon_theme_path, NULL);

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
                  if (! g_file_info_get_is_hidden (info) &&
                      g_file_info_get_file_type (info) == G_FILE_TYPE_DIRECTORY)
                    {
                      GFile *file;
                      GFile *index_theme;

                      file = g_file_enumerator_get_child (enumerator, info);

                      /* make sure there is a index.theme file */
                      index_theme = g_file_get_child (file, "index.theme");

                      if (index_theme)
                        {
                          const gchar *name;
                          gchar       *basename;

                          g_object_unref (index_theme);

                          name     = gimp_file_get_utf8_name (file);
                          basename = g_path_get_basename (name);

                          if (gimp->be_verbose)
                            g_print ("Adding icon theme '%s' (%s)\n",
                                     basename, name);

                          g_hash_table_insert (icon_themes_hash, basename, file);
                        }
                    }

                  g_object_unref (info);
                }

              g_object_unref (enumerator);
            }
        }

      g_list_free_full (path, (GDestroyNotify) g_object_unref);
    }

  g_signal_connect (config, "notify::icon-theme",
                    G_CALLBACK (icons_theme_change_notify),
                    gimp);

  icons_theme_change_notify (config, NULL, gimp);
}

void
icon_themes_exit (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  if (icon_themes_hash)
    {
      g_signal_handlers_disconnect_by_func (gimp->config,
                                            icons_theme_change_notify,
                                            gimp);

      g_hash_table_destroy (icon_themes_hash);
      icon_themes_hash = NULL;
    }
}

gchar **
icon_themes_list_themes (Gimp *gimp,
                         gint *n_icon_themes)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (n_icon_themes != NULL, NULL);

  *n_icon_themes = g_hash_table_size (icon_themes_hash);

  if (*n_icon_themes > 0)
    {
      gchar **icon_themes;
      gchar **index;

      icon_themes = g_new0 (gchar *, *n_icon_themes + 1);

      index = icon_themes;

      g_hash_table_foreach (icon_themes_hash, icons_list_icons_foreach, &index);

      qsort (icon_themes, *n_icon_themes, sizeof (gchar *), icons_name_compare);

      return icon_themes;
    }

  return NULL;
}

GFile *
icon_themes_get_theme_dir (Gimp        *gimp,
                           const gchar *icon_theme_name)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  if (! icon_theme_name)
    icon_theme_name = "Default";

  return g_hash_table_lookup (icon_themes_hash, icon_theme_name);
}

static void
icons_apply_theme (Gimp        *gimp,
                   const gchar *icon_theme_name)
{
  GtkIconTheme  *icon_theme;
  GFile         *icon_theme_dir;
  gchar         *icon_theme_path;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  if (! icon_theme_name)
    icon_theme_name = "Default";

  if (gimp->be_verbose)
    g_print ("Loading icon theme '%s'\n", icon_theme_name);

  icon_theme = gtk_icon_theme_get_default ();

  icon_theme_dir = icon_themes_get_theme_dir (gimp, icon_theme_name);
  icon_theme_path = g_file_get_path(icon_theme_dir);

  gtk_icon_theme_prepend_search_path (icon_theme, icon_theme_path);
  g_free (icon_theme_path);
}

static void
icons_list_icons_foreach (gpointer key,
                          gpointer value,
                          gpointer data)
{
  gchar ***index = data;

  **index = g_strdup ((gchar *) key);

  (*index)++;
}

static gint
icons_name_compare (const void *p1,
                     const void *p2)
{
  return strcmp (* (char **) p1, * (char **) p2);
}

static void
icons_theme_change_notify (GimpGuiConfig *config,
                           GParamSpec    *pspec,
                           Gimp          *gimp)
{
  icons_apply_theme (gimp, config->icon_theme);
}
