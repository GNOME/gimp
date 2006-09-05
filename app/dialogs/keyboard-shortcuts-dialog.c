/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include "dialogs-types.h"

#include "core/gimp.h"

#include "widgets/gimpactionview.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpuimanager.h"

#include "keyboard-shortcuts-dialog.h"

#include "gimp-intl.h"


GtkWidget *
keyboard_shortcuts_dialog_new (Gimp *gimp)
{
  GtkWidget *dialog;
  GtkWidget *vbox;
  GtkWidget *scrolled_window;
  GtkWidget *view;
  GtkWidget *box;
  GtkWidget *button;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  dialog = gimp_dialog_new (_("Configure Keyboard Shortcuts"),
                            "gimp-keyboard-shortcuts-dialog",
                            NULL, 0,
                            gimp_standard_help_func,
                            GIMP_HELP_KEYBOARD_SHORTCUTS,

                            GTK_STOCK_CLOSE, GTK_RESPONSE_OK,

                            NULL);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (gtk_widget_destroy),
                    NULL);

  vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), vbox);
  gtk_widget_show (vbox);

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window),
                                       GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (vbox), scrolled_window, TRUE, TRUE, 0);
  gtk_widget_show (scrolled_window);

  view = gimp_action_view_new (gimp_ui_managers_from_name ("<Image>")->data,
                               NULL, TRUE);
  gtk_widget_set_size_request (view, 300, 400);
  gtk_container_add (GTK_CONTAINER (scrolled_window), view);
  gtk_widget_show (view);

  box = gimp_hint_box_new (_("To edit a shortcut key, click on the "
                             "corresponding row and type a new "
                             "accelerator, or press backspace to "
                             "clear."));
  gtk_box_pack_start (GTK_BOX (vbox), box, FALSE, FALSE, 0);
  gtk_widget_show (box);

  button = gimp_prop_check_button_new (G_OBJECT (gimp->config), "save-accels",
                                       _("_Save keyboard shortcuts on exit"));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  return dialog;
}
