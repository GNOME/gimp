/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpmessagedialog.c
 * Copyright (C) 2004 Sven Neumann <sven@gimp.org>
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

#include "gimpmessagebox.h"
#include "gimpmessagedialog.h"


GType
gimp_message_dialog_get_type (void)
{
  static GType dialog_type = 0;

  if (! dialog_type)
    {
      static const GTypeInfo dialog_info =
      {
        sizeof (GimpMessageDialogClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        NULL,           /* class_init     */
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (GimpMessageDialog),
        0,              /* n_preallocs    */
        NULL            /* instance_init  */
      };

      dialog_type = g_type_register_static (GIMP_TYPE_DIALOG,
                                            "GimpMessageDialog",
                                            &dialog_info, 0);
    }

  return dialog_type;
}


/*  public functions  */

GtkWidget *
gimp_message_dialog_new (const gchar    *title,
                         const gchar    *stock_id,
                         GtkWidget      *parent,
                         GtkDialogFlags  flags,
                         GimpHelpFunc    help_func,
                         const gchar    *help_id,
                         ...)
{
  GimpMessageDialog *dialog;
  va_list            args;

  g_return_val_if_fail (title != NULL, NULL);
  g_return_val_if_fail (parent == NULL || GTK_IS_WIDGET (parent), NULL);

  dialog = g_object_new (GIMP_TYPE_MESSAGE_DIALOG,
                         "title",     title,
                         "role",      "gimp-message-dialog",
                         "modal",     (flags & GTK_DIALOG_MODAL),
                         "help-func", help_func,
                         "help-id",   help_id,
                         NULL);

  if (parent)
    {
      if (! GTK_IS_WINDOW (parent))
        parent = gtk_widget_get_toplevel (parent);

      if (GTK_IS_WINDOW (parent))
        {
          gtk_window_set_transient_for (GTK_WINDOW (dialog),
                                        GTK_WINDOW (parent));

          if (flags & GTK_DIALOG_DESTROY_WITH_PARENT)
            gtk_window_set_destroy_with_parent (GTK_WINDOW (dialog), TRUE);
        }
      else
        {
          gtk_window_set_screen (GTK_WINDOW (dialog),
                                 gtk_widget_get_screen (parent));
        }
    }

  va_start (args, help_id);

  gimp_dialog_add_buttons_valist (GIMP_DIALOG (dialog), args);

  va_end (args);

  dialog->box = g_object_new (GIMP_TYPE_MESSAGE_BOX,
                              "stock_id",  stock_id,
                              NULL);

  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox),
                      GTK_WIDGET (dialog->box), FALSE, FALSE, 0);
  gtk_widget_show (GTK_WIDGET (dialog->box));

  return GTK_WIDGET (dialog);
}
