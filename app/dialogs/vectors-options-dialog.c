/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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
 */

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "dialogs-types.h"

#include "core/gimpcontext.h"
#include "core/gimpimage.h"

#include "vectors/gimpvectors.h"

#include "widgets/gimpviewabledialog.h"

#include "vectors-options-dialog.h"

#include "gimp-intl.h"


typedef struct _VectorsOptionsDialog VectorsOptionsDialog;

struct _VectorsOptionsDialog
{
  GimpImage                  *image;
  GimpVectors                *vectors;
  GimpVectorsOptionsCallback  callback;
  gpointer                    user_data;

  GtkWidget                  *name_entry;
};


/*  local function prototypes  */

static void  vectors_options_dialog_response (GtkWidget            *dialog,
                                              gint                  response_id,
                                              VectorsOptionsDialog *private);
static void  vectors_options_dialog_free     (VectorsOptionsDialog *private);


/*  public functions  */

GtkWidget *
vectors_options_dialog_new (GimpImage                  *image,
                            GimpVectors                *vectors,
                            GimpContext                *context,
                            GtkWidget                  *parent,
                            const gchar                *title,
                            const gchar                *role,
                            const gchar                *icon_name,
                            const gchar                *desc,
                            const gchar                *help_id,
                            const gchar                *vectors_name,
                            GimpVectorsOptionsCallback  callback,
                            gpointer                    user_data)
{
  VectorsOptionsDialog *private;
  GtkWidget            *dialog;
  GimpViewable         *viewable;
  GtkWidget            *hbox;
  GtkWidget            *vbox;
  GtkWidget            *table;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (vectors == NULL || GIMP_IS_VECTORS (vectors), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (GTK_IS_WIDGET (parent), NULL);
  g_return_val_if_fail (title != NULL, NULL);
  g_return_val_if_fail (role != NULL, NULL);
  g_return_val_if_fail (icon_name != NULL, NULL);
  g_return_val_if_fail (desc != NULL, NULL);
  g_return_val_if_fail (help_id != NULL, NULL);
  g_return_val_if_fail (callback != NULL, NULL);

  private = g_slice_new0 (VectorsOptionsDialog);

  private->image     = image;
  private->vectors   = vectors;
  private->callback  = callback;
  private->user_data = user_data;

  if (vectors)
    viewable = GIMP_VIEWABLE (vectors);
  else
    viewable = GIMP_VIEWABLE (image);

  dialog = gimp_viewable_dialog_new (viewable, context,
                                     title, role, icon_name, desc,
                                     parent,
                                     gimp_standard_help_func,
                                     help_id,

                                     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                     GTK_STOCK_OK,     GTK_RESPONSE_OK,

                                     NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  g_object_weak_ref (G_OBJECT (dialog),
                     (GWeakNotify) vectors_options_dialog_free, private);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (vectors_options_dialog_response),
                    private);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      hbox, TRUE, TRUE, 0);
  gtk_widget_show (hbox);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  table = gtk_table_new (1, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  private->name_entry = gtk_entry_new ();
  gtk_widget_set_size_request (private->name_entry, 150, -1);
  gtk_entry_set_activates_default (GTK_ENTRY (private->name_entry), TRUE);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
                             _("Path name:"), 0.0, 0.5,
                             private->name_entry, 1, FALSE);

  gtk_entry_set_text (GTK_ENTRY (private->name_entry), vectors_name);

  return dialog;
}

static void
vectors_options_dialog_response (GtkWidget            *dialog,
                                 gint                  response_id,
                                 VectorsOptionsDialog *private)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      const gchar *name = gtk_entry_get_text (GTK_ENTRY (private->name_entry));

      private->callback (dialog,
                         private->image,
                         private->vectors,
                         name,
                         private->user_data);
    }
  else
    {
      gtk_widget_destroy (dialog);
    }
}

static void
vectors_options_dialog_free (VectorsOptionsDialog *private)
{
  g_slice_free (VectorsOptionsDialog, private);
}
