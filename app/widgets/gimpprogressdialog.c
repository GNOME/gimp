/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpprogressdialog.c
 * Copyright (C) 2004 Michael Natterer <mitch@gimp.org>
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

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimpprogress.h"

#include "gimpprogressdialog.h"

#include "gimp-intl.h"


static void    gimp_progress_dialog_class_init (GimpProgressDialogClass *klass);
static void    gimp_progress_dialog_init       (GimpProgressDialog      *dialog);
static void    gimp_progress_dialog_progress_iface_init (GimpProgressInterface *progress_iface);

static void    gimp_progress_dialog_response           (GtkDialog    *dialog,
                                                        gint          response_id);

static GimpProgress *
               gimp_progress_dialog_progress_start     (GimpProgress *progress,
                                                        const gchar  *message,
                                                        gboolean      cancelable);
static void    gimp_progress_dialog_progress_end       (GimpProgress *progress);
static void    gimp_progress_dialog_progress_set_text  (GimpProgress *progress,
                                                        const gchar  *message);
static void    gimp_progress_dialog_progress_set_value (GimpProgress *progress,
                                                        gdouble       percentage);
static gdouble gimp_progress_dialog_progress_get_value (GimpProgress *progress);


static GimpDialogClass *parent_class = NULL;


GType
gimp_progress_dialog_get_type (void)
{
  static GType dialog_type = 0;

  if (! dialog_type)
    {
      static const GTypeInfo dialog_info =
      {
        sizeof (GimpProgressDialogClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gimp_progress_dialog_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (GimpProgressDialog),
        0,              /* n_preallocs    */
        (GInstanceInitFunc) gimp_progress_dialog_init,
      };

      static const GInterfaceInfo progress_iface_info =
      {
        (GInterfaceInitFunc) gimp_progress_dialog_progress_iface_init,
        NULL,           /* iface_finalize */
        NULL            /* iface_data     */
      };

      dialog_type = g_type_register_static (GIMP_TYPE_DIALOG,
					    "GimpProgressDialog",
					    &dialog_info, 0);

      g_type_add_interface_static (dialog_type, GIMP_TYPE_PROGRESS,
                                   &progress_iface_info);
    }

  return dialog_type;
}

static void
gimp_progress_dialog_class_init (GimpProgressDialogClass *klass)
{
  GtkDialogClass *dialog_class = GTK_DIALOG_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  dialog_class->response = gimp_progress_dialog_response;
}

static void
gimp_progress_dialog_init (GimpProgressDialog *dialog)
{
}

static void
gimp_progress_dialog_progress_iface_init (GimpProgressInterface *progress_iface)
{
  progress_iface->start     = gimp_progress_dialog_progress_start;
  progress_iface->end       = gimp_progress_dialog_progress_end;
  progress_iface->set_text  = gimp_progress_dialog_progress_set_text;
  progress_iface->set_value = gimp_progress_dialog_progress_set_value;
  progress_iface->get_value = gimp_progress_dialog_progress_get_value;
}

static void
gimp_progress_dialog_response (GtkDialog *dialog,
                               gint       response_id)
{
  if (response_id == GTK_RESPONSE_CANCEL)
    gimp_progress_cancel (GIMP_PROGRESS (dialog));
}

static GimpProgress *
gimp_progress_dialog_progress_start (GimpProgress *progress,
                                     const gchar  *message,
                                     gboolean      cancelable)
{
  GimpProgressDialog *dialog = GIMP_PROGRESS_DIALOG (progress);

  if (! dialog->progress_active)
    {
      GtkProgressBar *bar = GTK_PROGRESS_BAR (dialog->progressbar);

      gtk_label_set_text (GTK_LABEL (dialog->label), message);
      gtk_progress_bar_set_fraction (bar, 0.0);
      gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog),
                                         GTK_RESPONSE_CANCEL, cancelable);

      gtk_window_present (GTK_WINDOW (dialog));

      dialog->progress_active = TRUE;

      return progress;
    }

  return NULL;
}

static void
gimp_progress_dialog_progress_end (GimpProgress *progress)
{
  GimpProgressDialog *dialog = GIMP_PROGRESS_DIALOG (progress);

  if (dialog->progress_active)
    {
      GtkProgressBar *bar = GTK_PROGRESS_BAR (dialog->progressbar);

      gtk_label_set_text (GTK_LABEL (dialog->label), "");
      gtk_progress_bar_set_fraction (bar, 0.0);
      gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog),
                                         GTK_RESPONSE_CANCEL, FALSE);

      gtk_widget_hide (GTK_WIDGET (dialog));

      dialog->progress_active = FALSE;
    }
}

static void
gimp_progress_dialog_progress_set_text (GimpProgress *progress,
                                        const gchar  *message)
{
  GimpProgressDialog *dialog = GIMP_PROGRESS_DIALOG (progress);

  if (dialog->progress_active)
    {
      gtk_label_set_text (GTK_LABEL (dialog->label), message);
    }
}

static void
gimp_progress_dialog_progress_set_value (GimpProgress *progress,
                                         gdouble       percentage)
{
  GimpProgressDialog *dialog = GIMP_PROGRESS_DIALOG (progress);

  if (dialog->progress_active)
    {
      GtkProgressBar *bar = GTK_PROGRESS_BAR (dialog->progressbar);

      gtk_progress_bar_set_fraction (bar, percentage);

      if (GTK_WIDGET_DRAWABLE (dialog->progressbar))
        gdk_window_process_updates (dialog->progressbar->window, TRUE);
    }
}

static gdouble
gimp_progress_dialog_progress_get_value (GimpProgress *progress)
{
  GimpProgressDialog *dialog = GIMP_PROGRESS_DIALOG (progress);

  if (dialog->progress_active)
    {
      GtkProgressBar *bar = GTK_PROGRESS_BAR (dialog->progressbar);

      return gtk_progress_bar_get_fraction (bar);
    }

  return 0.0;
}

GtkWidget *
gimp_progress_dialog_new (void)
{
  GimpProgressDialog *dialog;
  GtkWidget          *vbox;

  dialog = g_object_new (GIMP_TYPE_PROGRESS_DIALOG,
                         "title",     _("Progress"),
                         "role",      "progress",
                         "resizable", FALSE,
                         NULL);

  gtk_dialog_add_button (GTK_DIALOG (dialog),
                         GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);

  vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), vbox);
  gtk_widget_show (vbox);

  dialog->label = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (dialog->label), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (vbox), dialog->label, FALSE, FALSE, 0);
  gtk_widget_show (dialog->label);

  dialog->progressbar = gtk_progress_bar_new ();
  gtk_widget_set_size_request (dialog->progressbar, 150, 20);
  gtk_box_pack_start (GTK_BOX (vbox), dialog->progressbar, FALSE, FALSE, 0);
  gtk_widget_show (dialog->progressbar);

  return GTK_WIDGET (dialog);
}
