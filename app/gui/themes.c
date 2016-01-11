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

#include <stdlib.h>

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"

#include "gui-types.h"

#include "config/gimpguiconfig.h"

#include "core/gimp.h"

#include "themes.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   themes_apply_theme         (Gimp                   *gimp,
                                          const gchar            *theme_name);
static void   themes_list_themes_foreach (gpointer                key,
                                          gpointer                value,
                                          gpointer                data);
static gint   themes_name_compare        (const void             *p1,
                                          const void             *p2);
static void   themes_theme_change_notify (GimpGuiConfig          *config,
                                          GParamSpec             *pspec,
                                          Gimp                   *gimp);


/*  private variables  */

static GHashTable *themes_hash = NULL;


/*  public functions  */

void
themes_init (Gimp *gimp)
{
  GimpGuiConfig *config;
  gchar         *themerc;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  config = GIMP_GUI_CONFIG (gimp->config);

  themes_hash = g_hash_table_new_full (g_str_hash,
                                       g_str_equal,
                                       g_free,
                                       g_object_unref);

  if (config->theme_path)
    {
      GList *path;
      GList *list;

      path = gimp_config_path_expand_to_files (config->theme_path, NULL);

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

  themes_apply_theme (gimp, config->theme);

  themerc = gimp_personal_rc_file ("themerc");
  gtk_rc_parse (themerc);
  g_free (themerc);

  g_signal_connect (config, "notify::theme",
                    G_CALLBACK (themes_theme_change_notify),
                    gimp);
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


/*  private functions  */

static void
themes_apply_theme (Gimp        *gimp,
                    const gchar *theme_name)
{
  GFile         *themerc;
  GOutputStream *output;
  GError        *error = NULL;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  themerc = gimp_directory_file ("themerc", NULL);

  if (gimp->be_verbose)
    g_print ("Writing '%s'\n", gimp_file_get_utf8_name (themerc));

  output = G_OUTPUT_STREAM (g_file_replace (themerc,
                                            NULL, FALSE, G_FILE_CREATE_NONE,
                                            NULL, &error));
  if (! output)
    {
      gimp_message_literal (gimp, NULL, GIMP_MESSAGE_ERROR, error->message);
      g_clear_error (&error);
    }
  else
    {
      GFile *theme_dir = themes_get_theme_dir (gimp, theme_name);
      GFile *gtkrc_theme;
      GFile *gtkrc_user;
      gchar *esc_gtkrc_theme;
      gchar *esc_gtkrc_user;
      gchar *tmp;

      if (theme_dir)
        {
          gtkrc_theme = g_file_get_child (theme_dir, "gtkrc");
        }
      else
        {
          /*  get the hardcoded default theme gtkrc  */
          gtkrc_theme = g_file_new_for_path (gimp_gtkrc ());
        }

      gtkrc_user = gimp_directory_file ("gtkrc", NULL);

      tmp = g_file_get_path (gtkrc_theme);
      esc_gtkrc_theme = g_strescape (tmp, NULL);
      g_free (tmp);

      tmp = g_file_get_path (gtkrc_user);
      esc_gtkrc_user = g_strescape (tmp, NULL);
      g_free (tmp);

      if (! g_output_stream_printf
            (output, NULL, NULL, &error,
             "# GIMP themerc\n"
             "#\n"
             "# This file is written on GIMP startup and on every theme change.\n"
             "# It is NOT supposed to be edited manually. Edit your personal\n"
             "# gtkrc file instead (%s).\n"
             "\n"
             "include \"%s\"\n"
             "include \"%s\"\n"
             "\n"
             "# end of themerc\n",
             gimp_file_get_utf8_name (gtkrc_user),
             esc_gtkrc_theme,
             esc_gtkrc_user) ||
          ! g_output_stream_close (output, NULL, &error))
        {
          gimp_message (gimp, NULL, GIMP_MESSAGE_ERROR,
                        _("Error writing '%s': %s"),
                        gimp_file_get_utf8_name (themerc), error->message);
          g_clear_error (&error);
        }

      g_free (esc_gtkrc_theme);
      g_free (esc_gtkrc_user);
      g_object_unref (gtkrc_theme);
      g_object_unref (gtkrc_user);
      g_object_unref (output);
    }

  g_object_unref (themerc);
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
themes_theme_change_notify (GimpGuiConfig *config,
                            GParamSpec    *pspec,
                            Gimp          *gimp)
{
  themes_apply_theme (gimp, config->theme);

  gtk_rc_reparse_all ();
}
