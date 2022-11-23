/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaerrordialog.c
 * Copyright (C) 2004  Sven Neumann <sven@ligma.org>
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

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "ligmaerrordialog.h"
#include "ligmamessagebox.h"

#include "ligma-intl.h"

#define LIGMA_ERROR_DIALOG_MAX_MESSAGES 3


typedef struct
{
  GtkWidget *box;
  gchar     *domain;
  gchar     *message;
} LigmaErrorDialogMessage;

static void   ligma_error_dialog_finalize        (GObject   *object);
static void   ligma_error_dialog_response        (GtkDialog *dialog,
                                                 gint       response_id);

static void   ligma_error_dialog_message_destroy (gpointer   data);

G_DEFINE_TYPE (LigmaErrorDialog, ligma_error_dialog, LIGMA_TYPE_DIALOG)

#define parent_class ligma_error_dialog_parent_class


static void
ligma_error_dialog_class_init (LigmaErrorDialogClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkDialogClass *dialog_class = GTK_DIALOG_CLASS (klass);

  object_class->finalize = ligma_error_dialog_finalize;

  dialog_class->response = ligma_error_dialog_response;
}

static void
ligma_error_dialog_init (LigmaErrorDialog *dialog)
{
  gtk_window_set_role (GTK_WINDOW (dialog), "ligma-message");

  gtk_dialog_add_buttons (GTK_DIALOG (dialog),

                          _("_OK"), GTK_RESPONSE_CLOSE,

                          NULL);

  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_CLOSE);

  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

  dialog->vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      dialog->vbox, TRUE, TRUE, 0);
  gtk_widget_show (dialog->vbox);

  dialog->messages = NULL;
  dialog->overflow = FALSE;
}

static void
ligma_error_dialog_finalize (GObject *object)
{
  LigmaErrorDialog *dialog = LIGMA_ERROR_DIALOG (object);

  g_list_free_full (dialog->messages,
                    ligma_error_dialog_message_destroy);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_error_dialog_response (GtkDialog *dialog,
                            gint       response_id)
{
  gtk_widget_destroy (GTK_WIDGET (dialog));
}

static void
ligma_error_dialog_message_destroy (gpointer data)
{
  LigmaErrorDialogMessage *item = (LigmaErrorDialogMessage *) data;

  g_free (item->domain);
  g_free (item->message);
  g_free (item);
}


/*  public functions  */

GtkWidget *
ligma_error_dialog_new (const gchar *title)
{
  gboolean use_header_bar;

  g_return_val_if_fail (title != NULL, NULL);

  g_object_get (gtk_settings_get_default (),
                "gtk-dialogs-use-header", &use_header_bar,
                NULL);

  return g_object_new (LIGMA_TYPE_ERROR_DIALOG,
                       "title",          title,
                       "use-header-bar", use_header_bar,
                        NULL);
}

void
ligma_error_dialog_add (LigmaErrorDialog *dialog,
                       const gchar     *icon_name,
                       const gchar     *domain,
                       const gchar     *message)
{
  LigmaErrorDialogMessage *item;
  gboolean                overflow = FALSE;

  g_return_if_fail (LIGMA_IS_ERROR_DIALOG (dialog));
  g_return_if_fail (domain != NULL);
  g_return_if_fail (message != NULL);

  if (dialog->messages)
    {
      GList *iter = dialog->messages;

      for (; iter; iter = iter->next)
        {
          item = iter->data;
          if (strcmp (item->domain, domain)   == 0 &&
              strcmp (item->message, message) == 0)
            {
              if (ligma_message_box_repeat (LIGMA_MESSAGE_BOX (item->box)))
                return;
            }
        }
    }

  if (g_list_length (dialog->messages) >= LIGMA_ERROR_DIALOG_MAX_MESSAGES)
    {
      g_printerr ("%s: %s\n\n", domain, message);

      overflow  = TRUE;
      icon_name = LIGMA_ICON_WILBER_EEK;
      domain    = _("Too many error messages!");
      message   = _("Messages are redirected to stderr.");

      if (dialog->overflow)
        {
          /* We were already overflowing. */
          return;
        }
      dialog->overflow = TRUE;
    }

  item = g_new0 (LigmaErrorDialogMessage, 1);
  item->box = g_object_new (LIGMA_TYPE_MESSAGE_BOX,
                            "icon-name", icon_name,
                            NULL);
  item->domain  = g_strdup (domain);
  item->message = g_strdup (message);

  if (overflow)
    ligma_message_box_set_primary_text (LIGMA_MESSAGE_BOX (item->box),
                                       "%s", domain);
  else
    ligma_message_box_set_primary_text (LIGMA_MESSAGE_BOX (item->box),
                                       /* %s is a message domain,
                                        * like "LIGMA Message" or
                                        * "PNG Message"
                                        */
                                       _("%s Message"), domain);

  ligma_message_box_set_text (LIGMA_MESSAGE_BOX (item->box), "%s", message);

  gtk_box_pack_start (GTK_BOX (dialog->vbox), item->box, TRUE, TRUE, 0);
  gtk_widget_show (item->box);

  dialog->messages = g_list_prepend (dialog->messages, item);
}
