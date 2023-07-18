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

#include <gegl.h>
#include <gtk/gtk.h>
#ifdef GDK_WINDOWING_WAYLAND
#include <gdk/gdkwayland.h>
#endif

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "dialogs-types.h"

#include "core/gimp.h"
#include "core/gimp-utils.h"

#include "widgets/gimpwidgets-utils.h"

#include "welcome-dialog.h"
#include "welcome-dialog-data.h"

#include "gimp-intl.h"


static void   welcome_dialog_release_item_activated (GtkListBox    *listbox,
                                                     GtkListBoxRow *row,
                                                     gpointer       user_data);
static void   welcome_add_link                      (GtkGrid        *grid,
                                                     gint            column,
                                                     gint           *row,
                                                     const gchar    *emoji,
                                                     const gchar    *title,
                                                     const gchar    *link);
static void   welcome_size_allocate                 (GtkWidget      *welcome_dialog,
                                                     GtkAllocation  *allocation,
                                                     gpointer        user_data);


GtkWidget *
welcome_dialog_create (Gimp *gimp)
{
  GtkWidget  *welcome_dialog;
  GList      *windows;

  GtkWidget  *main_vbox;
  GtkWidget  *stack;
  GtkWidget  *grid;
  GtkWidget  *switcher;

  GtkWidget  *scrolled_window;
  GtkWidget  *vbox;
  GtkWidget  *hbox;
  GtkWidget  *image;
  GtkWidget  *listbox;
  GtkWidget  *widget;

  gchar      *release_link;
  gchar      *title;
  gchar      *markup;
  gchar      *tmp;
  gint        row;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  /* Translators: the %s string will be the version, e.g. "3.0". */
  title = g_strdup_printf (_("Welcome to GIMP %s"), GIMP_VERSION);
  windows = gimp_get_image_windows (gimp);
  welcome_dialog = gimp_dialog_new (title,
                                    "gimp-welcome-dialog",
                                    windows ?  windows->data : NULL,
                                    0, NULL, NULL,
                                    NULL);
  g_list_free (windows);
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
                        _("Welcome"));
  gtk_widget_show (vbox);

  image = gtk_image_new_from_icon_name ("gimp-wilber",
                                        GTK_ICON_SIZE_DIALOG);
  gtk_widget_set_valign (image, GTK_ALIGN_CENTER);
  gtk_box_pack_start (GTK_BOX (vbox), image, FALSE, FALSE, 0);
  gtk_widget_show (image);

  g_signal_connect (welcome_dialog,
                    "size-allocate",
                    G_CALLBACK (welcome_size_allocate),
                    image);

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

  if (gimp_welcome_dialog_n_items > 0)
    {
      gint n_demos = 0;

      vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
      gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
      gtk_stack_add_titled (GTK_STACK (stack), vbox, "release-notes",
                            _("Release Notes"));
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

      /* Release note introduction. */

      if (gimp_welcome_dialog_intro_n_paragraphs)
        {
          GString *introduction = NULL;

          for (gint i = 0; i < gimp_welcome_dialog_intro_n_paragraphs; i++)
            {
              if (i == 0)
                introduction = g_string_new (_(gimp_welcome_dialog_intro[i]));
              else
                g_string_append_printf (introduction, "\n%s",
                                        _(gimp_welcome_dialog_intro[i]));
            }
          widget = gtk_label_new (NULL);
          gtk_label_set_markup (GTK_LABEL (widget), introduction->str);
          gtk_label_set_max_width_chars (GTK_LABEL (widget), 70);
          gtk_label_set_selectable (GTK_LABEL (widget), FALSE);
          gtk_label_set_justify (GTK_LABEL (widget), GTK_JUSTIFY_LEFT);
          gtk_label_set_line_wrap (GTK_LABEL (widget), TRUE);
          gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);
          gtk_widget_show (widget);

          g_string_free (introduction, TRUE);
        }

      /* Release note's change items. */

      scrolled_window = gtk_scrolled_window_new (NULL, NULL);
      gtk_box_pack_start (GTK_BOX (vbox), scrolled_window, TRUE, TRUE, 0);
      gtk_widget_show (scrolled_window);

      listbox = gtk_list_box_new ();

      for (gint i = 0; i < gimp_welcome_dialog_n_items; i++)
        {
          GtkWidget *row;
          gchar     *markup;

          /* Add a bold dot for pretty listing. */
          if (i < gimp_welcome_dialog_n_items &&
              gimp_welcome_dialog_demos[i] != NULL)
            {
              markup = g_strdup_printf ("<span weight='ultrabold'>\xe2\x96\xb6</span>  %s",
                                        _((gchar *) gimp_welcome_dialog_items[i]));
              n_demos++;
            }
          else
            {
              markup = g_strdup_printf ("<span weight='ultrabold'>\xe2\x80\xa2</span>  %s",
                                        _((gchar *) gimp_welcome_dialog_items[i]));
            }

          row = gtk_list_box_row_new ();
          widget = gtk_label_new (NULL);
          gtk_label_set_markup (GTK_LABEL (widget), markup);
          gtk_label_set_line_wrap (GTK_LABEL (widget), TRUE);
          gtk_label_set_line_wrap_mode (GTK_LABEL (widget), PANGO_WRAP_WORD);
          gtk_label_set_justify (GTK_LABEL (widget), GTK_JUSTIFY_LEFT);
          gtk_widget_set_halign (widget, GTK_ALIGN_START);
          gtk_label_set_xalign (GTK_LABEL (widget), 0.0);
          gtk_container_add (GTK_CONTAINER (row), widget);

          gtk_list_box_insert (GTK_LIST_BOX (listbox), row, -1);
          gtk_widget_show_all (row);

          g_free (markup);
        }
      gtk_container_add (GTK_CONTAINER (scrolled_window), listbox);
      gtk_list_box_set_selection_mode (GTK_LIST_BOX (listbox),
                                       GTK_SELECTION_NONE);

      g_signal_connect (listbox, "row-activated",
                        G_CALLBACK (welcome_dialog_release_item_activated),
                        gimp);
      gtk_widget_show (listbox);

      if (n_demos > 0)
        {
          /* A small explicative string to help discoverability of the demo
           * ability.
           */
          hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
          gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
          gtk_widget_show (hbox);

          image = gtk_image_new_from_icon_name ("dialog-information",
                                                GTK_ICON_SIZE_MENU);
          gtk_widget_set_valign (image, GTK_ALIGN_CENTER);
          gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
          gtk_widget_show (image);

          widget = gtk_label_new (NULL);
          tmp = g_strdup_printf (_("Click on release items with a %s bullet point to get a tour."),
                                 "<span weight='ultrabold'>\xe2\x96\xb6</span>");
          markup = g_strdup_printf ("<i>%s</i>", tmp);
          g_free (tmp);
          gtk_label_set_markup (GTK_LABEL (widget), markup);
          g_free (markup);
          gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
          gtk_widget_show (widget);

          /* TODO: if a demo changed settings, should we add a "reset"
           * button to get back to previous state?
           */
        }

      /* Link to full release notes on web site at the bottom. */
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
  markup = g_strdup_printf ("<small>%s</small>",
                            _("This welcome dialog is only shown at first launch. "
                              "You can show it again from the \"Help\" menu."));
  gtk_label_set_markup (GTK_LABEL (widget), markup);
  g_free (markup);
  gtk_widget_show (widget);
  gtk_box_pack_start (GTK_BOX (main_vbox), widget, FALSE, FALSE, 0);

  return welcome_dialog;
}

static void
welcome_dialog_release_item_activated (GtkListBox    *listbox,
                                       GtkListBoxRow *row,
                                       gpointer       user_data)
{
  Gimp         *gimp          = user_data;
  GList        *blink_script  = NULL;
  const gchar  *script_string;
  gchar       **script_steps;
  gint          row_index;
  gint          i;

  row_index = gtk_list_box_row_get_index (row);

  g_return_if_fail (row_index < gimp_welcome_dialog_n_items);

  script_string = gimp_welcome_dialog_demos[row_index];

  if (script_string == NULL)
    /* Not an error. Some release items have no demos. */
    return;

  script_steps = g_strsplit (script_string, ",", 0);

  for (i = 0; script_steps[i]; i++)
    {
      gchar **ids;
      gchar  *dockable_id    = NULL;
      gchar  *widget_id      = NULL;
      gchar **settings       = NULL;
      gchar  *settings_value = NULL;

      ids = g_strsplit (script_steps[i], ":", 2);
      /* Even if the string doesn't contain a second part, it is
       * NULL-terminated, hence the widget_id will simply be NULL, which
       * is fine when you just want to blink a dialog.
       */
      dockable_id = ids[0];
      widget_id   = ids[1];

      if (widget_id != NULL)
        {
          settings = g_strsplit (widget_id, "=", 2);

          widget_id = settings[0];
          settings_value = settings[1];
        }

      /* Allowing white spaces so that the demo in XML metadata can be
       * spaced-out or even on multiple lines for clarity.
       */
      dockable_id = g_strstrip (dockable_id);
      if (widget_id != NULL)
        widget_id = g_strstrip (widget_id);

      /* All our dockable IDs start with "gimp-". This allows to write
       * shorter names in the demo script.
       */
      if (! g_str_has_prefix (dockable_id, "gimp-"))
        {
          gchar *tmp = g_strdup_printf ("gimp-%s", dockable_id);

          g_free (ids[0]);
          dockable_id = ids[0] = tmp;
        }

      /* Blink widget. */
      if (g_strcmp0 (dockable_id, "gimp-toolbox") == 0)
        {
          /* All tool button IDs start with "tools-". This allows to
           * write shorter tool names in the demo script.
           */
          if (widget_id != NULL && ! g_str_has_prefix (widget_id, "tools-"))
            {
              gchar *tmp = g_strdup_printf ("tools-%s", widget_id);

              g_free (settings[0]);
              widget_id = settings[0] = tmp;
            }

          gimp_blink_toolbox (gimp, widget_id, &blink_script);
        }
      else
        {
          gimp_blink_dockable (gimp, dockable_id,
                               widget_id, settings_value,
                               &blink_script);
        }

      g_strfreev (ids);
      if (settings)
        g_strfreev (settings);
    }
  if (blink_script != NULL)
    {
      GList *windows = gimp_get_image_windows (gimp);

      /* Losing forcus on the welcome dialog on purpose for the main GUI
       * to be more readable.
       */
      if (windows)
        gtk_window_present (windows->data);

      gimp_blink_play_script (blink_script);

      g_list_free (windows);
    }

  g_strfreev (script_steps);
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

static void
welcome_size_allocate (GtkWidget     *welcome_dialog,
                       GtkAllocation *allocation,
                       gpointer       user_data)
{
  GtkWidget     *image = GTK_WIDGET (user_data);
  GError        *error = NULL;
  GFile         *splash_file;
  GdkPixbuf     *pixbuf;
  GdkMonitor    *monitor;
  GdkRectangle   workarea;
  gint           min_width;
  gint           min_height;
  gint           max_width;
  gint           max_height;
  gint           image_width;
  gint           image_height;

  if (gtk_image_get_storage_type (GTK_IMAGE (image)) == GTK_IMAGE_PIXBUF)
    return;

  monitor = gimp_get_monitor_at_pointer ();
  gdk_monitor_get_workarea (monitor, &workarea);
#ifdef GDK_WINDOWING_WAYLAND
  if (GDK_IS_WAYLAND_DISPLAY (gdk_display_get_default ()))
    {
      /* See the long comment in app/gui/splash.c on why we do this
       * weird stuff for Wayland only.
       * See also #5322.
       */
      min_width  = workarea.width  / 8;
      min_height = workarea.height / 8;
      max_width  = workarea.width  / 4;
      max_height = workarea.height / 4;
    }
  else
#endif
    {
      min_width  = workarea.width  / 4;
      min_height = workarea.height / 4;
      max_width  = workarea.width  / 2;
      max_height = workarea.height / 2;
    }
  image_width = allocation->width + 20;
  image_height = allocation->height + 20;

  /* On big monitors, we get very huge images with a lot of empty space.
   * So let's go with a logic so that we want a max and min size
   * (relatively to desktop area), but we also want to avoid too much
   * empty space. This is why we compute first the dialog size without
   * any image in there.
   */
  image_width = CLAMP (image_width, min_width, max_width);
  image_height = CLAMP (image_height, min_height, max_height);

  splash_file = gimp_data_directory_file ("images", "gimp-splash.png", NULL);
  pixbuf = gdk_pixbuf_new_from_file_at_scale (g_file_peek_path (splash_file),
                                              image_width, image_height,
                                              TRUE, &error);
  if (pixbuf)
    {
      gtk_image_set_from_pixbuf (GTK_IMAGE (image), pixbuf);
      g_object_unref (pixbuf);
    }
  g_object_unref (splash_file);

  gtk_widget_show (image);

  gtk_window_set_resizable (GTK_WINDOW (welcome_dialog), FALSE);
}
