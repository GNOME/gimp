/*
 * This is a plug-in for GIMP.
 *
 * Generates clickable image maps.
 *
 * Copyright (C) 1998-2002 Maurits Rijk  lpeek.mrijk@consunet.nl
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
 *
 */

#include "config.h"

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "imap_default_dialog.h"
#include "imap_main.h"

#include "libgimp/stdplugins-intl.h"


static void
dialog_response (GtkWidget       *widget,
                 gint             response_id,
                 DefaultDialog_t *dialog)
{
  switch (response_id)
    {
    case GTK_RESPONSE_APPLY:
      if (dialog->apply_cb)
        dialog->apply_cb (dialog->apply_cb_data);
      else if (dialog->ok_cb)
        dialog->ok_cb (dialog->ok_cb_data);
      break;

    case GTK_RESPONSE_OK:
      gtk_widget_hide (dialog->dialog);
      if (dialog->ok_cb)
        dialog->ok_cb (dialog->ok_cb_data);
      break;

    default:
      gtk_widget_hide (dialog->dialog);
      if (dialog->cancel_cb)
        dialog->cancel_cb (dialog->cancel_cb_data);
      break;
    }
}

void
default_dialog_set_ok_cb(DefaultDialog_t *dialog, void (*ok_cb)(gpointer),
                         gpointer ok_cb_data)
{
   dialog->ok_cb = ok_cb;
   dialog->ok_cb_data = ok_cb_data;
}

void
default_dialog_set_apply_cb(DefaultDialog_t *dialog,
                            void (*apply_cb)(gpointer),
                            gpointer apply_cb_data)
{
   dialog->apply_cb = apply_cb;
   dialog->apply_cb_data = apply_cb_data;
}

void
default_dialog_set_cancel_cb(DefaultDialog_t *dialog,
                             void (*cancel_cb)(gpointer),
                             gpointer cancel_cb_data)
{
   dialog->cancel_cb = cancel_cb;
   dialog->cancel_cb_data = cancel_cb_data;
}

DefaultDialog_t *
make_default_dialog (const gchar *title)
{
  DefaultDialog_t *data = g_new0 (DefaultDialog_t, 1);

  data->ok_cb = NULL;
  data->apply_cb = NULL;
  data->cancel_cb = NULL;

  data->dialog = gimp_dialog_new (title, PLUG_IN_ROLE,
                                  get_dialog(), 0,
                                  gimp_standard_help_func, PLUG_IN_PROC,
                                  NULL);

  data->apply = gtk_dialog_add_button (GTK_DIALOG (data->dialog),
                                       _("_Apply"), GTK_RESPONSE_APPLY);

  data->cancel = gtk_dialog_add_button (GTK_DIALOG (data->dialog),
                                        _("_Cancel"), GTK_RESPONSE_CANCEL);

  data->ok = gtk_dialog_add_button (GTK_DIALOG (data->dialog),
                                    _("_OK"), GTK_RESPONSE_OK);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (data->dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_APPLY,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  g_signal_connect (data->dialog, "response",
                    G_CALLBACK (dialog_response),
                    data);
  g_signal_connect (data->dialog, "destroy",
                    G_CALLBACK (gtk_widget_destroyed),
                    &data->dialog);

  data->vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (data->vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (data->dialog))),
                      data->vbox, TRUE, TRUE, 0);
  gtk_widget_show (data->vbox);

  return data;
}

void
default_dialog_show(DefaultDialog_t *dialog)
{
   gtk_widget_show(dialog->dialog);
}

void
default_dialog_hide_cancel_button(DefaultDialog_t *dialog)
{
  gtk_widget_hide(dialog->cancel);
}

void
default_dialog_hide_apply_button(DefaultDialog_t *dialog)
{
  gtk_widget_hide(dialog->apply);
}

void
default_dialog_hide_help_button(DefaultDialog_t *dialog)
{
  /* gtk_widget_hide(dialog->help); */
}

void
default_dialog_set_title(DefaultDialog_t *dialog, const gchar *title)
{
   gtk_window_set_title(GTK_WINDOW(dialog->dialog), title);
}

void
default_dialog_set_label(DefaultDialog_t *dialog, const gchar *text)
{
   GtkWidget *label = gtk_label_new(text);

   gtk_box_pack_start (GTK_BOX (dialog->vbox), label, TRUE, TRUE, 0);
   gtk_widget_show(label);
}

GtkWidget*
default_dialog_add_grid (DefaultDialog_t *dialog)
{
  GtkWidget *grid = gtk_grid_new ();

  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);

  gtk_box_pack_start (GTK_BOX (dialog->vbox), grid, TRUE, TRUE, 0);
  gtk_widget_show (grid);

  return grid;
}
