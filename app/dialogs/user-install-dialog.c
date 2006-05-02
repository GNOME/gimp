/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * user-install-dialog.c
 * Copyright (C) 2000-2006 Michael Natterer and Sven Neumann
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <stdlib.h>

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "dialogs-types.h"

#include "core/gimp-user-install.h"

#include "user-install-dialog.h"

#include "gimp-intl.h"


enum
{
  WELCOME_PAGE,
  INSTALLATION_PAGE
};


static gboolean  migrate;


static void
user_install_dialog_set_title (GtkWidget   *dialog,
                               const gchar *title)
{
  GtkLabel *label = g_object_get_data (G_OBJECT (dialog), "title-label");

  gtk_label_set_text (label, title);
}

static GtkWidget *
user_install_dialog_set_page (GtkWidget *dialog,
                              gint       index)
{
  GtkNotebook *notebook = g_object_get_data (G_OBJECT (dialog), "notebook");
  GtkWidget   *page     = gtk_notebook_get_nth_page (notebook, index);

  user_install_dialog_set_title (dialog,
                                 gtk_notebook_get_menu_label_text (notebook,
                                                                   page));

  gtk_notebook_set_current_page (notebook, index);

  return page;
}

static void
user_install_dialog_response (GtkWidget       *dialog,
                              gint             response_id,
                              GimpUserInstall *install)
{
  GtkWidget *notebook = g_object_get_data (G_OBJECT (dialog), "notebook");
  gint       index;

  if (response_id != GTK_RESPONSE_OK)
    exit (EXIT_SUCCESS);

  index = gtk_notebook_get_current_page (GTK_NOTEBOOK (notebook));

  switch (index)
    {
    case WELCOME_PAGE:
      {
        user_install_dialog_set_page (dialog, ++index);

        /*  Creating the directories can take some time on NFS, so inform
         *  the user and set the buttons insensitive
         */
        gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_CANCEL, FALSE);
        gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK, TRUE);

        if (gimp_user_install_run (install, migrate))
          {
            user_install_dialog_set_title (dialog,
                                           _("Installation successful!"));

            gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog),
                                               GTK_RESPONSE_OK, TRUE);
          }
        else
          {
            user_install_dialog_set_title (dialog,
                                           _("Installation failed!"));
          }

        gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_CANCEL, TRUE);
      }
      break;

    case INSTALLATION_PAGE:
      gtk_widget_destroy (dialog);
      break;

    default:
      g_assert_not_reached ();
      break;
    }
}

static GtkWidget *
user_install_dialog_append_page (GtkWidget   *dialog,
                                 const gchar *title)
{
  GtkWidget *notebook = g_object_get_data (G_OBJECT (dialog), "notebook");
  GtkWidget *page     = gtk_vbox_new (FALSE, 12);

  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page, NULL);
  gtk_widget_show (page);

  gtk_notebook_set_menu_label_text (GTK_NOTEBOOK (notebook), page, title);

  return page;
}

static GtkWidget *
user_install_dialog_add_log (GtkWidget *dialog)
{
  GtkWidget     *scrolled_window;
  GtkTextBuffer *log_buffer;
  GtkWidget     *log_view;

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);

  log_buffer = gtk_text_buffer_new (NULL);

  gtk_text_buffer_create_tag (log_buffer, "bold",
                              "weight", PANGO_WEIGHT_BOLD,
                              NULL);

  log_view = gtk_text_view_new_with_buffer (log_buffer);
  g_object_unref (log_buffer);

  gtk_text_view_set_editable (GTK_TEXT_VIEW (log_view), FALSE);

  gtk_container_add (GTK_CONTAINER (scrolled_window), log_view);
  gtk_widget_show (log_view);

  g_object_set_data (G_OBJECT (dialog), "log-view", log_view);
  g_object_set_data (G_OBJECT (dialog), "log-buffer", log_buffer);

  return scrolled_window;
}

static GtkWidget *
user_install_dialog_add_welcome_page (GtkWidget       *dialog,
                                      GimpUserInstall *install)
{
  GtkWidget *page;
  GtkWidget *widget;
  gchar     *version;

  page = user_install_dialog_append_page (dialog,
                                          _("Welcome to the GNU Image "
                                            "Manipulation Program"));

  if (gimp_user_install_is_migration (install, &version))
    {
      gchar *title;

      title = g_strdup_printf (_("It seems you have used GIMP %s before."),
                               version);
      g_free (version);

      migrate = TRUE;

      widget = gimp_int_radio_group_new (TRUE, title,
                                         G_CALLBACK (gimp_radio_button_update),
                                         &migrate, migrate,

                                         _("_Use my old settings"),
                                         TRUE,  NULL,

                                         _("_Do a fresh installation"),
                                         FALSE, NULL,

                                         NULL);
      g_free (title);
    }
  else
    {
      gchar *text;

      text = g_strdup_printf (_("It appears that you are using GIMP for the "
                                "first time.  GIMP will now create a folder "
                                "named '<b>%s</b>' and copy some files to it."),
                                gimp_filename_to_utf8 (gimp_directory ()));

      widget = g_object_new (GTK_TYPE_LABEL,
                             "use-markup", TRUE,
                             "label",      text,
                             "wrap",       TRUE,
                             "xalign",     0.0,
                             NULL);
      g_free (text);
    }

  gtk_box_pack_start (GTK_BOX (page), widget, FALSE, FALSE, 0);
  gtk_widget_show (widget);

  return page;
}

static GtkWidget *
user_install_dialog_add_install_page (GtkWidget *dialog)
{
  GtkWidget *page;
  GtkWidget *expander;
  GtkWidget *log;

  page = user_install_dialog_append_page (dialog, _("Installing..."));

  expander = gtk_expander_new (_("Installation Log"));
  gtk_box_pack_start (GTK_BOX (page), expander, TRUE, TRUE, 0);
  gtk_widget_show (expander);

  log = user_install_dialog_add_log (dialog);
  gtk_widget_set_size_request (log, -1, 300);
  gtk_container_add (GTK_CONTAINER (expander), log);
  gtk_widget_show (log);

  return page;
}

static void
user_install_dialog_log (const gchar *message,
                         gboolean     error,
                         gpointer     data)
{
  GtkWidget     *dialog = GTK_WIDGET (data);
  GtkWidget     *view   = g_object_get_data (G_OBJECT (dialog), "log-view");
  GtkTextBuffer *buffer = g_object_get_data (G_OBJECT (dialog), "log-buffer");
  GdkPixbuf     *pixbuf;
  GtkTextIter    cursor;

  g_return_if_fail (GTK_IS_WIDGET (view));
  g_return_if_fail (GTK_IS_TEXT_BUFFER (buffer));

  gtk_text_buffer_insert_at_cursor (buffer, error ? "\n" : " ", -1);

  gtk_text_buffer_get_end_iter (buffer, &cursor);

  pixbuf =
    gtk_widget_render_icon (view,
                            error ? GIMP_STOCK_ERROR     : GTK_STOCK_APPLY,
                            error ? GTK_ICON_SIZE_DIALOG : GTK_ICON_SIZE_MENU,
                            NULL);

  gtk_text_buffer_insert_pixbuf (buffer, &cursor, pixbuf);
  g_object_unref (pixbuf);

  if (error)
    {
      gtk_text_buffer_insert (buffer, &cursor, "\n", -1);
      gtk_text_buffer_insert_with_tags_by_name (buffer, &cursor,
                                                message, -1,
                                                "bold",
                                                NULL);
    }
  else
    {
      gtk_text_buffer_insert (buffer, &cursor, message, -1);
    }

  gtk_text_buffer_insert (buffer, &cursor, "\n", -1);

  while (gtk_events_pending ())
    gtk_main_iteration ();
}

void
user_install_dialog_run (GimpUserInstall *install)
{
  GtkWidget *dialog;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *notebook;
  GtkWidget *label;
  GdkPixbuf *wilber;
  gchar     *filename;

  g_return_if_fail (install != NULL);

  dialog = gimp_dialog_new (_("GIMP User Installation"),
                            "gimp-user-installation",
                            NULL, 0,
                            NULL, NULL,

                            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                            _("C_ontinue"),    GTK_RESPONSE_OK,

                            NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  g_signal_connect (dialog, "destroy",
                    G_CALLBACK (gtk_main_quit),
                    NULL);

  gimp_user_install_set_log_handler (install, user_install_dialog_log, dialog);

  vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), vbox);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_widget_show (vbox);

  hbox = gtk_hbox_new (FALSE, 12);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  filename = g_build_filename (gimp_data_directory(),
                               "images", "wilber-wizard.png", NULL);
  wilber = gdk_pixbuf_new_from_file (filename, NULL);
  g_free (filename);

  if (wilber)
    {
      GtkWidget *image = gtk_image_new_from_pixbuf (wilber);

      g_object_unref (wilber);

      gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
      gtk_widget_show (image);
    }

  label = gtk_label_new (NULL);
  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gimp_label_set_attributes (GTK_LABEL (label),
                             PANGO_ATTR_SCALE, PANGO_SCALE_X_LARGE,
                             -1);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  g_object_set_data (G_OBJECT (dialog), "title-label", label);

  notebook = gtk_notebook_new ();
  gtk_notebook_set_show_tabs (GTK_NOTEBOOK (notebook), FALSE);
  gtk_notebook_set_show_border (GTK_NOTEBOOK (notebook), FALSE);
  gtk_box_pack_start (GTK_BOX (vbox), notebook, TRUE, TRUE, 0);
  gtk_widget_show (notebook);

  g_object_set_data (G_OBJECT (dialog), "notebook", notebook);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (user_install_dialog_response),
                    install);

  user_install_dialog_add_welcome_page (dialog, install);
  user_install_dialog_add_install_page (dialog);

  user_install_dialog_set_page (dialog, WELCOME_PAGE);

  gtk_widget_show (dialog);

  gtk_main ();
}

