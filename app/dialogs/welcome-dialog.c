/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * welcome-dialog.c
 * Copyright (C) 2022 Jehan
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

#include <appstream-glib.h>
#include <gegl.h>
#include <gtk/gtk.h>
#ifdef GDK_WINDOWING_WAYLAND
#include <gdk/gdkwayland.h>
#endif

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "dialogs-types.h"

#include "config/gimpguiconfig.h"

#include "core/gimp.h"
#include "core/gimp-utils.h"

#include "widgets/gimphelp-ids.h"

#include "welcome-dialog.h"

#include "gimp-intl.h"


static void     welcome_add_link        (GtkGrid        *grid,
                                         gint            column,
                                         gint           *row,
                                         const gchar    *emoji,
                                         const gchar    *title,
                                         const gchar    *link);


GtkWidget *
welcome_dialog_create (Gimp *gimp)
{
  GtkWidget     *welcome_dialog;
  AsApp         *app           = NULL;
  const gchar   *release_notes = NULL;
  GError        *error         = NULL;
  GFile         *splash_file;
  GdkPixbuf     *pixbuf;
  GdkMonitor    *monitor;
  GdkRectangle   workarea;

  GList         *windows;

  GtkWidget     *main_vbox;
  GtkWidget     *stack;
  GtkWidget     *grid;
  GtkWidget     *switcher;

  GtkWidget     *scrolled_window;
  GtkWidget     *vbox;
  GtkWidget     *hbox;
  GtkWidget     *image;
  GtkWidget     *widget;

  GtkTextBuffer *buffer;
  GtkTextIter    iter;

  gchar         *release_link;
  gchar         *appdata_path;
  gchar         *title;
  gchar         *markup;
  gchar         *tmp;

  gint           row;
  gint           max_width;
  gint           max_height;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  appdata_path = g_build_filename (DESKTOP_DATADIR, "metainfo",
                                   "org.gimp.GIMP.appdata.xml",
                                   NULL);
  if (g_file_test (appdata_path, G_FILE_TEST_IS_REGULAR))
    {
      AsRelease *release;

      app = as_app_new ();
      if (as_app_parse_file (app, appdata_path,
                             AS_APP_PARSE_FLAG_USE_HEURISTICS,
                             &error))
        {
          if ((release = as_app_get_release (app, GIMP_VERSION)) != NULL)
            release_notes = as_release_get_description (release, g_getenv ("LANGUAGE")) ?
              as_release_get_description (release, g_getenv ("LANGUAGE")) :
              as_release_get_description (release, NULL);
          else if (GIMP_MICRO_VERSION % 2 == 0)
            g_printerr ("%s: no <release> tag for version %s in '%s'\n",
                        G_STRFUNC, GIMP_VERSION, appdata_path);
        }
      else if (error)
        {
          g_printerr ("%s: %s\n", G_STRFUNC, error->message);
          g_clear_error (&error);
        }
      else
        {
          g_printerr ("%s: failed to load AppStream file '%s'\n", G_STRFUNC, appdata_path);
        }
    }
  else
    {
      /* Note that none of the errors here and above should happen.
       * Each of our releases (even micro value) should have a <release>
       * tag. But I am just printing to stderr and half-ignoring the
       * miss because it is not serious enough to break normal GIMP
       * usage.
       */
      g_printerr ("%s: AppStream file '%s' is not a regular file.\n", G_STRFUNC, appdata_path);
    }
  g_free (appdata_path);

  monitor = gimp_get_monitor_at_pointer ();
  gdk_monitor_get_workarea (monitor, &workarea);
#ifdef GDK_WINDOWING_WAYLAND
  if (GDK_IS_WAYLAND_DISPLAY (gdk_display_get_default ()))
    {
      /* See the long comment in app/gui/splash.c on why we do this
       * weird stuff for Wayland only.
       * See also #5322.
       */
      max_width  = workarea.width  / 4;
      max_height = workarea.height / 4;
    }
  else
#endif
    {
      max_width  = workarea.width  / 2;
      max_height = workarea.height / 2;
    }

  /* Translators: the %s string will be the version, e.g. "3.0". */
  title = g_strdup_printf (_("Welcome to GIMP %s"), GIMP_VERSION);
  windows = gimp_get_image_windows (gimp);
  welcome_dialog = gimp_dialog_new (title,
                                    "gimp-welcome-dialog",
                                    windows ?  windows->data : NULL,
                                    0, NULL, NULL,
                                    NULL);
  g_list_free (windows);
  gtk_window_set_resizable (GTK_WINDOW (welcome_dialog), FALSE);
  gtk_window_set_position (GTK_WINDOW (welcome_dialog), GTK_WIN_POS_CENTER_ON_PARENT);
  g_free (title);

  g_signal_connect (welcome_dialog,
                    "response",
                    G_CALLBACK (gtk_widget_destroy),
                    NULL);

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (welcome_dialog))),
                      main_vbox, TRUE, TRUE, 0);
  gtk_widget_show (main_vbox);

  stack = gtk_stack_new ();
  gtk_box_pack_start (GTK_BOX (main_vbox), stack, TRUE, TRUE, 0);
  gtk_widget_show (stack);

  /****************/
  /* Welcome page */
  /****************/

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_stack_add_titled (GTK_STACK (stack), vbox, "welcome",
                        "Welcome");
  gtk_widget_show (vbox);

  splash_file = gimp_data_directory_file ("images", "gimp-splash.png", NULL);
  pixbuf = gdk_pixbuf_new_from_file_at_scale (g_file_peek_path (splash_file),
                                              max_width, max_height,
                                              TRUE, &error);
  if (pixbuf)
    {
      image = gtk_image_new_from_pixbuf (pixbuf);
      g_object_unref (pixbuf);
    }
  else
    {
      g_printerr ("%s: Error loading '%s': %s\n", G_STRFUNC,
                  g_file_peek_path (splash_file),
                  error->message);
      g_clear_error (&error);

      image = gtk_image_new_from_icon_name ("gimp-wilber",
                                            GTK_ICON_SIZE_DIALOG);
    }
  g_object_unref (splash_file);

  gtk_widget_set_valign (image, GTK_ALIGN_CENTER);
  gtk_box_pack_start (GTK_BOX (vbox), image, FALSE, FALSE, 0);
  gtk_widget_show (image);

  /* Welcome title. */

  /* Translators: the %s string will be the version, e.g. "3.0". */
  tmp = g_strdup_printf (_("You installed GIMP %s!"), GIMP_VERSION);
  markup = g_strdup_printf ("<big>%s</big>", tmp);
  g_free (tmp);
  widget = gtk_label_new (NULL);
  gtk_label_set_markup (GTK_LABEL (widget), markup);
  g_free (markup);
  gtk_label_set_selectable (GTK_LABEL (widget), TRUE);
  gtk_label_set_justify (GTK_LABEL (widget), GTK_JUSTIFY_CENTER);
  gtk_label_set_line_wrap (GTK_LABEL (widget), FALSE);
  gtk_box_pack_start (GTK_BOX (vbox), widget, TRUE, TRUE, 0);
  gtk_widget_show (widget);

  grid = gtk_grid_new ();
  gtk_grid_set_column_homogeneous (GTK_GRID (grid), TRUE);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 0);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 4);
  gtk_box_pack_start (GTK_BOX (vbox), grid, TRUE, TRUE, 0);
  gtk_widget_show (grid);

  /* Welcome message: left */

  markup = _("GIMP is a Free Software for image authoring and manipulation.\n"
             "Want to know more?");

  widget = gtk_label_new (NULL);
  gtk_label_set_max_width_chars (GTK_LABEL (widget), 30);
  /*gtk_widget_set_size_request (widget, max_width / 2, -1);*/
  gtk_label_set_line_wrap (GTK_LABEL (widget), TRUE);
  gtk_widget_set_vexpand (widget, FALSE);
  gtk_widget_set_hexpand (widget, FALSE);

  /* Making sure the labels are well top aligned to avoid some ugly
   * misalignment if left and right labels have different sizes,
   * but also left-aligned so that the messages are slightly to the left
   * of the emoji/link list below.
   * Following design decisions by Aryeom.
   */
  gtk_label_set_xalign (GTK_LABEL (widget), 0.0);
  gtk_label_set_yalign (GTK_LABEL (widget), 0.0);
  gtk_widget_set_margin_bottom (widget, 10);
  gtk_label_set_markup (GTK_LABEL (widget), markup);

  gtk_grid_attach (GTK_GRID (grid), widget, 0, 0, 1, 1);

  gtk_widget_show (widget);

  row = 1;
  welcome_add_link (GTK_GRID (grid), 0, &row,
                    /* "globe with meridians" emoticone in UTF-8. */
                    "\xf0\x9f\x8c\x90",
                    _("GIMP website"), "https://www.gimp.org/");
  welcome_add_link (GTK_GRID (grid), 0, &row,
                    /* "graduation cap" emoticone in UTF-8. */
                    "\xf0\x9f\x8e\x93",
                    _("Tutorials"),
                    "https://www.gimp.org/tutorials/");
  welcome_add_link (GTK_GRID (grid), 0, &row,
                    /* "open book" emoticone in UTF-8. */
                    "\xf0\x9f\x93\x96",
                    _("Documentation"),
                    "https://docs.gimp.org/");

  /* XXX: should we add API docs for plug-in developers once it's
   * properly set up? */

  /* Welcome message: right */

  markup = _("GIMP is a Community Software under the GNU general public license v3.\n"
             "Want to contribute?");

  widget = gtk_label_new (NULL);
  gtk_label_set_line_wrap (GTK_LABEL (widget), TRUE);
  gtk_label_set_max_width_chars (GTK_LABEL (widget), 30);
  /*gtk_widget_set_size_request (widget, max_width / 2, -1);*/

  /* Again the alignments are important. */
  gtk_label_set_xalign (GTK_LABEL (widget), 0.0);
  gtk_widget_set_vexpand (widget, FALSE);
  gtk_widget_set_hexpand (widget, FALSE);
  gtk_label_set_xalign (GTK_LABEL (widget), 0.0);
  gtk_label_set_yalign (GTK_LABEL (widget), 0.0);
  gtk_widget_set_margin_bottom (widget, 10);
  gtk_label_set_markup (GTK_LABEL (widget), markup);

  gtk_grid_attach (GTK_GRID (grid), widget, 1, 0, 1, 1);

  gtk_widget_show (widget);

  row = 1;
  welcome_add_link (GTK_GRID (grid), 1, &row,
                    /* "keyboard" emoticone in UTF-8. */
                    "\xe2\x8c\xa8",
                    _("Contributing"),
                    "https://www.gimp.org/develop/");
  welcome_add_link (GTK_GRID (grid), 1, &row,
                    /* "love letter" emoticone in UTF-8. */
                    "\xf0\x9f\x92\x8c",
                    _("Donating"),
                    "https://www.gimp.org/donating/");

  /*****************/
  /* Release Notes */
  /*****************/

  if (release_notes)
    {
      vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
      gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
      gtk_stack_add_titled (GTK_STACK (stack), vbox, "release-notes",
                            "Release Notes");
      gtk_widget_show (vbox);

      /* Release note title. */

      hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
      gtk_container_set_border_width (GTK_CONTAINER (hbox), 6);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
      gtk_widget_show (hbox);

      /* Translators: the %s string will be the version, e.g. "3.0". */
      tmp = g_strdup_printf (_("GIMP %s Release Notes"), GIMP_VERSION);
      markup = g_strdup_printf ("<b><big>%s</big></b>", tmp);
      g_free (tmp);
      widget = gtk_label_new (NULL);
      gtk_label_set_markup (GTK_LABEL (widget), markup);
      g_free (markup);
      gtk_label_set_selectable (GTK_LABEL (widget), FALSE);
      gtk_label_set_justify (GTK_LABEL (widget), GTK_JUSTIFY_CENTER);
      gtk_label_set_line_wrap (GTK_LABEL (widget), FALSE);
      gtk_box_pack_start (GTK_BOX (hbox), widget, TRUE, TRUE, 0);
      gtk_widget_show (widget);

      image = gtk_image_new_from_icon_name ("gimp-user-manual",
                                            GTK_ICON_SIZE_DIALOG);
      gtk_widget_set_valign (image, GTK_ALIGN_START);
      gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
      gtk_widget_show (image);

      /* Release note contents. */

      scrolled_window = gtk_scrolled_window_new (NULL, NULL);
      gtk_box_pack_start (GTK_BOX (vbox), scrolled_window, TRUE, TRUE, 0);
      gtk_widget_show (scrolled_window);

      widget = gtk_text_view_new ();
      gtk_widget_set_vexpand (widget, TRUE);
      gtk_widget_set_hexpand (widget, TRUE);
      gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (widget), GTK_WRAP_WORD_CHAR);
      gtk_text_view_set_editable (GTK_TEXT_VIEW (widget), FALSE);
      gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (widget), FALSE);
      gtk_text_view_set_justification (GTK_TEXT_VIEW (widget), GTK_JUSTIFY_LEFT);
      buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (widget));
      gtk_text_buffer_get_start_iter (buffer, &iter);

      markup = gimp_appstream_to_pango_markup (release_notes);
      gtk_text_buffer_insert_markup (buffer, &iter, markup, -1);
      g_free (markup);

      gtk_container_add (GTK_CONTAINER (scrolled_window), widget);
      gtk_widget_show (widget);

      hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
      gtk_widget_show (hbox);

      if (GIMP_MINOR_VERSION % 2 == 0)
        release_link = g_strdup_printf ("https://www.gimp.org/release-notes/gimp-%d.%d.html",
                                        GIMP_MAJOR_VERSION, GIMP_MINOR_VERSION);
      else
        release_link = g_strdup ("https://www.gimp.org/");

      widget = gtk_link_button_new_with_label (release_link, _("Learn more"));
      gtk_widget_show (widget);
      gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
      g_free (release_link);

      /*****************/
      /* Task switcher */
      /*****************/

      switcher = gtk_stack_switcher_new ();
      gtk_stack_switcher_set_stack (GTK_STACK_SWITCHER (switcher),
                                    GTK_STACK (stack));
      gtk_box_pack_start (GTK_BOX (main_vbox), switcher, FALSE, FALSE, 0);
      gtk_widget_set_halign (switcher, GTK_ALIGN_CENTER);
      gtk_widget_show (switcher);
    }

  /**************/
  /* Info label */
  /**************/

  widget = gtk_label_new (NULL);
  tmp = g_strdup (_("This welcome dialog is only shown at first launch. "
                    "You can call if again from the \"Help\" menu."));
  markup = g_strdup_printf ("<small>%s</small>", tmp);
  g_free (tmp);
  widget = gtk_label_new (NULL);
  gtk_label_set_markup (GTK_LABEL (widget), markup);
  gtk_widget_show (widget);
  gtk_box_pack_start (GTK_BOX (main_vbox), widget, FALSE, FALSE, 0);

  g_clear_object (&app);

  return welcome_dialog;
}

static void
welcome_add_link (GtkGrid     *grid,
                  gint         column,
                  gint        *row,
                  const gchar *emoji,
                  const gchar *title,
                  const gchar *link)
{
  GtkWidget *hbox;
  GtkWidget *button;
  GtkWidget *icon;

  /* TODO: Aryeom doesn't like the spacing here. There is too much
   * spacing between the link lines and between emojis and links. But we
   * didn't manage to find how to close a bit these 2 spacings in GTK.
   * :-/
   */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_grid_attach (grid, hbox, column, *row, 1, 1);
  /* These margin are by design to emphasize a bit the link list by
   * moving them a tiny bit to the right instead of being exactly
   * aligned with the top text.
   */
  gtk_widget_set_margin_start (hbox, 10);
  gtk_widget_show (hbox);

  ++(*row);

  icon = gtk_label_new (emoji);
  gtk_box_pack_start (GTK_BOX (hbox), icon, FALSE, FALSE, 0);
  gtk_widget_show (icon);

  button = gtk_link_button_new_with_label (link, title);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);
}
