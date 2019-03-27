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
#ifdef GDK_DISABLE_DEPRECATED
#undef GDK_DISABLE_DEPRECATED
#endif
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

static void   themes_fix_pixbuf_style    (void);
static void   themes_draw_pixbuf_layout  (GtkStyle               *style,
                                          GdkWindow              *window,
                                          GtkStateType            state_type,
                                          gboolean                use_text,
                                          GdkRectangle           *area,
                                          GtkWidget              *widget,
                                          const gchar            *detail,
                                          gint                    x,
                                          gint                    y,
                                          PangoLayout            *layout);

/*  private variables  */

static GHashTable    *themes_hash        = NULL;
static GtkStyleClass *pixbuf_style_class = NULL;


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

  themes_fix_pixbuf_style ();

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

  g_clear_pointer (&pixbuf_style_class, g_type_class_unref);
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
      GFile  *theme_dir   = themes_get_theme_dir (gimp, theme_name);
      GFile  *gtkrc_user;
      GSList *gtkrc_files = NULL;
      GSList *iter;

      if (theme_dir)
        {
          gtkrc_files = g_slist_prepend (
            gtkrc_files,
            g_file_get_child (theme_dir, "gtkrc"));
        }
      else
        {
          /*  get the hardcoded default theme gtkrc  */
          gtkrc_files = g_slist_prepend (
            gtkrc_files,
            g_file_new_for_path (gimp_gtkrc ()));
        }

      gtkrc_files = g_slist_prepend (
        gtkrc_files,
        gimp_sysconf_directory_file ("gtkrc", NULL));

      gtkrc_user  = gimp_directory_file ("gtkrc", NULL);
      gtkrc_files = g_slist_prepend (
        gtkrc_files,
        gtkrc_user);

      gtkrc_files = g_slist_reverse (gtkrc_files);

      g_output_stream_printf (
        output, NULL, NULL, &error,
        "# GIMP themerc\n"
        "#\n"
        "# This file is written on GIMP startup and on every theme change.\n"
        "# It is NOT supposed to be edited manually. Edit your personal\n"
        "# gtkrc file instead (%s).\n"
        "\n",
        gimp_file_get_utf8_name (gtkrc_user));

      for (iter = gtkrc_files; ! error && iter; iter = g_slist_next (iter))
        {
          GFile *file = iter->data;

          if (g_file_query_exists (file, NULL))
            {
              gchar *path;
              gchar *esc_path;

              path     = g_file_get_path (file);
              esc_path = g_strescape (path, NULL);
              g_free (path);

              g_output_stream_printf (
                output, NULL, NULL, &error,
                "include \"%s\"\n",
                esc_path);

              g_free (esc_path);
            }
        }

      if (! error)
        {
          g_output_stream_printf (
            output, NULL, NULL, &error,
            "\n"
            "# end of themerc\n");
        }

      if (error)
        {
          GCancellable *cancellable = g_cancellable_new ();

          gimp_message (gimp, NULL, GIMP_MESSAGE_ERROR,
                        _("Error writing '%s': %s"),
                        gimp_file_get_utf8_name (themerc), error->message);
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
                        gimp_file_get_utf8_name (themerc), error->message);
          g_clear_error (&error);
        }

      g_slist_free_full (gtkrc_files, g_object_unref);
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

  themes_fix_pixbuf_style ();
}

static void
themes_fix_pixbuf_style (void)
{
  /* This is a "quick'n dirty" trick to get appropriate colors for
   * themes in GTK+2, and in particular dark themes which would display
   * insensitive items with a barely readable layout.
   *
   * This piece of code partly duplicates code from GTK+2 (slightly
   * modified to get readable insensitive items) and will likely have to
   * be removed for GIMP 3.
   *
   * See https://bugzilla.gnome.org/show_bug.cgi?id=770424
   */

  if (! pixbuf_style_class)
    {
      GType type = g_type_from_name ("PixbufStyle");

      if (type)
        {
          pixbuf_style_class = g_type_class_ref (type);

          if (pixbuf_style_class)
            pixbuf_style_class->draw_layout = themes_draw_pixbuf_layout;
        }
    }
}

static void
themes_draw_pixbuf_layout (GtkStyle      *style,
                           GdkWindow     *window,
                           GtkStateType   state_type,
                           gboolean       use_text,
                           GdkRectangle  *area,
                           GtkWidget     *widget,
                           const gchar   *detail,
                           gint           x,
                           gint           y,
                           PangoLayout   *layout)
{
  GdkGC *gc;

  gc = use_text ? style->text_gc[state_type] : style->fg_gc[state_type];

  if (area)
    gdk_gc_set_clip_rectangle (gc, area);

  if (state_type == GTK_STATE_INSENSITIVE)
    {
      GdkGC       *copy = gdk_gc_new (window);
      GdkGCValues  orig;
      GdkColor     fore;
      guint16      r, g, b;

      gdk_gc_copy (copy, gc);
      gdk_gc_get_values (gc, &orig);

      r = 0x40 + (((orig.foreground.pixel >> 16) & 0xff) >> 1);
      g = 0x40 + (((orig.foreground.pixel >>  8) & 0xff) >> 1);
      b = 0x40 + (((orig.foreground.pixel >>  0) & 0xff) >> 1);

      fore.pixel = (r << 16) | (g << 8) | b;
      fore.red   = r * 257;
      fore.green = g * 257;
      fore.blue  = b * 257;

      gdk_gc_set_foreground (copy, &fore);
      gdk_draw_layout (window, copy, x, y, layout);

      g_object_unref (copy);
    }
  else
    {
      gdk_draw_layout (window, gc, x, y, layout);
    }

  if (area)
    gdk_gc_set_clip_rectangle (gc, NULL);
}
