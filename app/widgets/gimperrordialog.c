/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimperrordialog.c
 * Copyright (C) 2004  Sven Neumann <sven@gimp.org>
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

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "gimperrordialog.h"
#include "gimpmessagebox.h"

#include "gimp-intl.h"

#define GIMP_ERROR_DIALOG_MAX_MESSAGES 3


static void   gimp_error_dialog_class_init (GimpErrorDialogClass *klass);
static void   gimp_error_dialog_init       (GimpErrorDialog      *dialog);
static void   gimp_error_dialog_finalize   (GObject              *object);
static void   gimp_error_dialog_response   (GtkDialog            *dialog,
                                            gint                  response_id);


static GimpDialogClass *parent_class = NULL;


GType
gimp_error_dialog_get_type (void)
{
  static GType dialog_type = 0;

  if (! dialog_type)
    {
      static const GTypeInfo dialog_info =
      {
        sizeof (GimpErrorDialogClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gimp_error_dialog_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (GimpErrorDialog),
        0,              /* n_preallocs    */
        (GInstanceInitFunc) gimp_error_dialog_init
      };

      dialog_type = g_type_register_static (GIMP_TYPE_DIALOG,
                                            "GimpErrorDialog",
                                            &dialog_info, 0);
    }

  return dialog_type;
}

static void
gimp_error_dialog_class_init (GimpErrorDialogClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkDialogClass *dialog_class = GTK_DIALOG_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = gimp_error_dialog_finalize;

  dialog_class->response = gimp_error_dialog_response;
}

static void
gimp_error_dialog_init (GimpErrorDialog *dialog)
{
  gtk_window_set_role (GTK_WINDOW (dialog), "gimp-message");

  gtk_dialog_add_buttons (GTK_DIALOG (dialog),
                          GTK_STOCK_OK, GTK_RESPONSE_CLOSE,
                          NULL);

  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_CLOSE);

  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

  dialog->vbox = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox),
                      dialog->vbox, TRUE, TRUE, 0);
  gtk_widget_show (dialog->vbox);

  dialog->last_box     = NULL;
  dialog->last_domain  = NULL;
  dialog->last_message = NULL;
  dialog->num_messages = 0;
}

static void
gimp_error_dialog_finalize (GObject *object)
{
  GimpErrorDialog *dialog = GIMP_ERROR_DIALOG (object);

  if (dialog->last_domain)
    {
      g_free (dialog->last_domain);
      dialog->last_domain = NULL;
    }
  if (dialog->last_message)
    {
      g_free (dialog->last_message);
      dialog->last_message = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_error_dialog_response (GtkDialog *dialog,
                            gint       response_id)
{
  gtk_widget_destroy (GTK_WIDGET (dialog));
}


static void
gimp_error_dialog_set_icon (GtkWidget   *dialog,
                            const gchar *stock_id)
{
  GtkIconSet *icon_set;

  gtk_widget_ensure_style (dialog);

  icon_set = gtk_style_lookup_icon_set (dialog->style, stock_id);

  if (icon_set)
    {
      GtkIconSize *sizes;
      GList       *icons = NULL;
      gint         i, n_sizes;

      gtk_icon_set_get_sizes (icon_set, &sizes, &n_sizes);

      for (i = 0; i < n_sizes; i++)
        {
          if (sizes[i] < GTK_ICON_SIZE_DIALOG)  /* skip the large version */
            icons = g_list_prepend (icons,
                                    gtk_widget_render_icon (dialog,
                                                            stock_id, sizes[i],
                                                            NULL));
        }

      g_free (sizes);

      if (icons)
        {
          gtk_window_set_icon_list (GTK_WINDOW (dialog), icons);

          g_list_foreach (icons, (GFunc) g_object_unref, NULL);
          g_list_free (icons);
        }
    }
}


/*  public functions  */

GtkWidget *
gimp_error_dialog_new (const gchar *title,
                       const gchar *stock_id)
{
  GtkWidget *dialog;

  g_return_val_if_fail (title != NULL, NULL);

  dialog = g_object_new (GIMP_TYPE_ERROR_DIALOG,
                         "title", title,
                         NULL);

  if (stock_id)
    gimp_error_dialog_set_icon (dialog, stock_id);

  return dialog;
}

void
gimp_error_dialog_add (GimpErrorDialog *dialog,
                       const gchar     *stock_id,
                       const gchar     *domain,
                       const gchar     *message)
{
  GtkWidget *box;
  gboolean   overflow = FALSE;

  g_return_if_fail (GIMP_IS_ERROR_DIALOG (dialog));
  g_return_if_fail (domain != NULL);
  g_return_if_fail (message != NULL);

  if (dialog->last_box     &&
      dialog->last_domain  && strcmp (dialog->last_domain,  domain)  == 0 &&
      dialog->last_message && strcmp (dialog->last_message, message) == 0)
    {
      if (gimp_message_box_repeat (GIMP_MESSAGE_BOX (dialog->last_box)))
        return;
    }

  if (dialog->num_messages >= GIMP_ERROR_DIALOG_MAX_MESSAGES)
    {
      g_printerr ("%s: %s\n\n", domain, message);

      overflow = TRUE;
      stock_id = GIMP_STOCK_WILBER_EEK;
      domain   = _("Too many error messages!");
      message  = _("Messages are redirected to stderr.");
    }

  /*  yes, these lines are repeated here for a reason  */
  if (dialog->last_box     &&
      dialog->last_domain  && strcmp (dialog->last_domain,  domain)  == 0 &&
      dialog->last_message && strcmp (dialog->last_message, message) == 0)
    {
      if (gimp_message_box_repeat (GIMP_MESSAGE_BOX (dialog->last_box)))
        return;
    }

  box = g_object_new (GIMP_TYPE_MESSAGE_BOX,
                      "stock_id", stock_id,
                      NULL);

  dialog->num_messages++;

  if (overflow)
    gimp_message_box_set_primary_text (GIMP_MESSAGE_BOX (box), domain);
  else
    gimp_message_box_set_primary_text (GIMP_MESSAGE_BOX (box),
                                       _("%s Message"), domain);

  gimp_message_box_set_text (GIMP_MESSAGE_BOX (box), "%s", message);

  gtk_container_add (GTK_CONTAINER (dialog->vbox), box);
  gtk_widget_show (box);

  dialog->last_box = box;

  g_free (dialog->last_domain);
  dialog->last_domain = g_strdup (domain);

  g_free (dialog->last_message);
  dialog->last_message = g_strdup (message);
}
