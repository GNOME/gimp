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

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "dialogs-types.h"

#include "core/gimpimage.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpviewabledialog.h"

#include "print-size-dialog.h"

#include "gimp-intl.h"


#define RESPONSE_RESET 1
#define SB_WIDTH       8


typedef struct _PrintSizeDialog PrintSizeDialog;

struct _PrintSizeDialog
{
  GimpImage              *image;
  GtkWidget              *size_entry;
  GtkWidget              *resolution_entry;
  GimpResolutionCallback  callback;
  gpointer                user_data;
};


static void   print_size_dialog_response (GtkWidget       *dialog,
                                          gint             response_id,
                                          PrintSizeDialog *private);
static void   print_size_dialog_reset    (PrintSizeDialog *private);


GtkWidget *
print_size_dialog_new (GimpImage              *image,
                       const gchar            *title,
                       const gchar            *role,
                       GtkWidget              *parent,
                       GimpHelpFunc            help_func,
                       const gchar            *help_id,
                       GimpResolutionCallback  callback,
                       gpointer                user_data)
{
  GtkWidget       *dialog;
  GtkWidget       *frame;
  GtkWidget       *table;
  GtkWidget       *entry;
  GtkWidget       *label;
  GtkWidget       *width;
  GtkWidget       *height;
  GtkWidget       *hbox;
  GtkWidget       *chain;
  GtkObject       *adj;
  PrintSizeDialog *private;
  gdouble          xres, yres;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (callback != NULL, NULL);

  dialog = gimp_viewable_dialog_new (GIMP_VIEWABLE (image),
                                     title, role,
                                     GIMP_STOCK_PRINT_RESOLUTION, title,
                                     parent,
                                     help_func, help_id,

                                     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                     GIMP_STOCK_RESET, RESPONSE_RESET,
                                     GTK_STOCK_OK,     GTK_RESPONSE_OK,

                                     NULL);

  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

  private = g_new0 (PrintSizeDialog, 1);

  g_signal_connect_swapped (dialog, "destroy",
                            G_CALLBACK (g_free),
                            private);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (print_size_dialog_response),
                    private);

  private->image     = image;
  private->callback  = callback;
  private->user_data = user_data;

  gimp_image_get_resolution (image, &xres, &yres);

  frame = gimp_frame_new (_("Print Size"));
  gtk_container_set_border_width (GTK_CONTAINER (frame), 12);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox),
                      frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (4, 3, FALSE);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 12);
  gtk_table_set_row_spacing (GTK_TABLE (table), 0, 2);
  gtk_table_set_row_spacing (GTK_TABLE (table), 2, 2);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  /*  the print size entry  */

  width = gimp_spin_button_new (&adj, 1, 1, 1, 1, 10, 0, 1, 2);
  gtk_entry_set_width_chars (GTK_ENTRY (width), SB_WIDTH);

  height = gimp_spin_button_new (&adj, 1, 1, 1, 1, 10, 0, 1, 2);
  gtk_entry_set_width_chars (GTK_ENTRY (height), SB_WIDTH);

  entry = gimp_size_entry_new (0, gimp_image_get_unit (image), "%p",
                               FALSE, FALSE, FALSE, SB_WIDTH,
                               GIMP_SIZE_ENTRY_UPDATE_SIZE);

  label = gtk_label_new_with_mnemonic (_("_Width:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), width);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  label = gtk_label_new_with_mnemonic (_("H_eight:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), height);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_table_attach_defaults (GTK_TABLE (table), hbox, 1, 2, 0, 2);
  gtk_widget_show (hbox);

  gtk_table_set_row_spacing (GTK_TABLE (entry), 0, 2);
  gtk_table_set_col_spacing (GTK_TABLE (entry), 1, 6);

  gtk_box_pack_start (GTK_BOX (hbox), entry, FALSE, FALSE, 0);
  gtk_widget_show (entry);

  gimp_size_entry_add_field (GIMP_SIZE_ENTRY (entry),
			     GTK_SPIN_BUTTON (height), NULL);
  gtk_table_attach_defaults (GTK_TABLE (entry), height, 0, 1, 1, 2);
  gtk_widget_show (height);

  gimp_size_entry_add_field (GIMP_SIZE_ENTRY (entry),
			     GTK_SPIN_BUTTON (width), NULL);
  gtk_table_attach_defaults (GTK_TABLE (entry), width, 0, 1, 0, 1);
  gtk_widget_show (width);

  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (entry), 0, xres, FALSE);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (entry), 1, yres, FALSE);

  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (entry), 0, image->width);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (entry), 1, image->height);

  /*  the resolution entry  */

  width = gimp_spin_button_new (&adj, 1, 1, 1, 1, 10, 0, 1, 2);
  gtk_entry_set_width_chars (GTK_ENTRY (width), SB_WIDTH);

  height = gimp_spin_button_new (&adj, 1, 1, 1, 1, 10, 0, 1, 2);
  gtk_entry_set_width_chars (GTK_ENTRY (height), SB_WIDTH);

  label = gtk_label_new_with_mnemonic (_("_X resolution:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), width);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3,
                    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  label = gtk_label_new_with_mnemonic (_("_Y resolution:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), height);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 3, 4,
                    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_table_attach_defaults (GTK_TABLE (table), hbox, 1, 2, 2, 4);
  gtk_widget_show (hbox);

  entry = gimp_size_entry_new (0, gimp_image_get_unit (image), _("pixels/%a"),
                               FALSE, FALSE, FALSE, SB_WIDTH,
                               GIMP_SIZE_ENTRY_UPDATE_RESOLUTION);

  gtk_table_set_row_spacing (GTK_TABLE (entry), 0, 2);
  gtk_table_set_col_spacing (GTK_TABLE (entry), 1, 2);
  gtk_table_set_col_spacing (GTK_TABLE (entry), 2, 2);

  gtk_box_pack_start (GTK_BOX (hbox), entry, FALSE, FALSE, 0);
  gtk_widget_show (entry);

  gimp_size_entry_add_field (GIMP_SIZE_ENTRY (entry),
                             GTK_SPIN_BUTTON (height), NULL);
  gtk_table_attach_defaults (GTK_TABLE (entry), height, 0, 1, 1, 2);
  gtk_widget_show (height);

  gimp_size_entry_add_field (GIMP_SIZE_ENTRY (entry),
                             GTK_SPIN_BUTTON (width), NULL);
  gtk_table_attach_defaults (GTK_TABLE (entry), width, 0, 1, 0, 1);
  gtk_widget_show (width);

  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (entry), 0,
                                         GIMP_MIN_RESOLUTION,
                                         GIMP_MAX_RESOLUTION);
  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (entry), 1,
                                         GIMP_MIN_RESOLUTION,
                                         GIMP_MAX_RESOLUTION);

  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (entry), 0, xres);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (entry), 1, yres);

  chain = gimp_chain_button_new (GIMP_CHAIN_RIGHT);
  gimp_chain_button_set_active (GIMP_CHAIN_BUTTON (chain), TRUE);
  gtk_table_attach_defaults (GTK_TABLE (entry), chain, 1, 2, 0, 2);
  gtk_widget_show (chain);

  return dialog;
}

static void
print_size_dialog_response (GtkWidget       *dialog,
                            gint             response_id,
                            PrintSizeDialog *private)
{
  GimpUnit  unit;
  gdouble   xres, yres;

  switch (response_id)
    {
    case RESPONSE_RESET:
      print_size_dialog_reset (private);
      break;

    case GTK_RESPONSE_OK:
      unit = gimp_image_get_unit (private->image);
      gimp_image_get_resolution (private->image, &xres, &yres);

      private->callback (dialog,
                         private->image,
                         xres, yres, unit,
                         private->user_data);
      break;

    default:
      gtk_widget_destroy (dialog);
      break;
    }
}

static void
print_size_dialog_reset (PrintSizeDialog *private)
{
  GimpUnit unit;
  gdouble  xres, yres;

  gimp_image_get_resolution (private->image, &xres, &yres);
  unit = gimp_image_get_unit (private->image);
}
