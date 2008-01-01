/* GIMP - The GNU Image Manipulation Program
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
#include "core/gimpdrawable.h"
#include "core/gimpimage-arrange.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpviewabledialog.h"

#include "align-dialog.h"

#include "gimp-intl.h"


typedef struct
{
  GtkWidget              *dialog;
  GimpImage              *image;
  GtkWidget              *button[6];
  GimpAlignReferenceType  align_reference_type;
} AlignDialog;


static void        align_dialog_response (GtkWidget         *dialog,
                                          gint               response_id,
                                          AlignDialog       *private);

static void        align_dialog_free     (AlignDialog       *private);

static void        do_align              (GtkWidget         *widget,
                                          gpointer           data);

static GtkWidget * button_with_stock     (GimpAlignmentType  action,
                                          AlignDialog       *private);

/*  public functions  */

GtkWidget *
align_dialog_new (GimpImage   *image,
                  GimpContext *context,
                  GtkWidget   *parent)
{
  AlignDialog  *private;

  GtkWidget    *vbox;
  GtkWidget    *frame;
  GtkWidget    *hbox;
  GtkWidget    *combo;
  GtkWidget    *button;
  GtkWidget    *dialog;
  GtkWidget    *main_vbox;
  gint          n          = 0;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GTK_IS_WIDGET (parent), NULL);

  private = g_slice_new0 (AlignDialog);

  private->image                = image;
  private->align_reference_type = GIMP_ALIGN_REFERENCE_ACTIVE_LAYER;

  dialog = gimp_viewable_dialog_new (GIMP_VIEWABLE (image),
                                     context,
                                     _("Align"), "gimp-align",
                                     GIMP_STOCK_CENTER, _("Align Linked Items"),
                                     parent,
                                     gimp_standard_help_func,
                                     GIMP_HELP_IMAGE_ALIGN_ITEMS,

                                     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,

                                     NULL);

  private->dialog = dialog;

  g_object_weak_ref (G_OBJECT (dialog),
                     (GWeakNotify) align_dialog_free, private);

  g_object_weak_ref (G_OBJECT (image),
                     (GWeakNotify) gtk_widget_destroy, dialog);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (align_dialog_response),
                    private);

  main_vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), main_vbox);
  gtk_widget_show (main_vbox);

  vbox = gtk_vbox_new (FALSE, 6);
  gtk_container_add (GTK_CONTAINER (main_vbox), vbox);
  gtk_widget_show (vbox);

  frame = gimp_frame_new (_("Relative to:"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  combo = gimp_enum_combo_box_new (GIMP_TYPE_ALIGN_REFERENCE);
  gtk_container_add (GTK_CONTAINER (frame), combo);
  gimp_int_combo_box_connect (GIMP_INT_COMBO_BOX (combo),
                              GIMP_ALIGN_REFERENCE_ACTIVE_LAYER,
                              G_CALLBACK (gimp_int_combo_box_get_active),
                              &private->align_reference_type);
  gtk_widget_show (combo);

  hbox = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  button = button_with_stock (GIMP_ALIGN_LEFT, private);
  private->button[n++] = button;
  gimp_help_set_help_data (button, _("Align left edges"), NULL);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  button = button_with_stock (GIMP_ALIGN_HCENTER, private);
  private->button[n++] = button;
  gimp_help_set_help_data (button, _("Align horizontal centers"), NULL);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  button = button_with_stock (GIMP_ALIGN_RIGHT, private);
  private->button[n++] = button;
  gimp_help_set_help_data (button, _("Align right edges"), NULL);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  hbox = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  button = button_with_stock (GIMP_ALIGN_TOP, private);
  private->button[n++] = button;
  gimp_help_set_help_data (button, _("Align top edges"), NULL);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  button = button_with_stock (GIMP_ALIGN_VCENTER, private);
  private->button[n++] = button;
  gimp_help_set_help_data (button, _("Align vertical centers"), NULL);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  button = button_with_stock (GIMP_ALIGN_BOTTOM, private);
  private->button[n++] = button;
  gimp_help_set_help_data (button, _("Align bottom edges"), NULL);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  return dialog;
}


/*  private functions  */

static void
align_dialog_response (GtkWidget  *dialog,
                      gint        response_id,
                      AlignDialog *private)
{
  gtk_widget_destroy (dialog);
}

static void
align_dialog_free (AlignDialog *private)
{
  g_object_weak_unref (G_OBJECT (private->image),
                       (GWeakNotify) gtk_widget_destroy,
                       private->dialog);

  g_slice_free (AlignDialog, private);
}

static void
do_align (GtkWidget *widget,
          gpointer   data)
{
  AlignDialog       *private = data;
  GimpAlignmentType  action;
  GimpImage         *image;
  GObject           *reference_object = NULL;

  image  = private->image;
  action = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget), "action"));

  switch (private->align_reference_type)
    {
    case GIMP_ALIGN_REFERENCE_IMAGE:
      reference_object = G_OBJECT (image);
      break;

    case GIMP_ALIGN_REFERENCE_SELECTION:
      reference_object = G_OBJECT (gimp_image_get_mask (image));
      break;

    case GIMP_ALIGN_REFERENCE_ACTIVE_LAYER:
      reference_object = G_OBJECT (gimp_image_get_active_layer (image));
      break;

    case GIMP_ALIGN_REFERENCE_ACTIVE_CHANNEL:
      reference_object = G_OBJECT (gimp_image_get_active_channel (image));
      break;

    case GIMP_ALIGN_REFERENCE_ACTIVE_PATH:
      reference_object = G_OBJECT (gimp_image_get_active_vectors (image));
      break;
    }

  if (! reference_object)
    return;

  gimp_image_align_linked_items (image, action, reference_object);

  gimp_image_flush (image);
}

static GtkWidget *
button_with_stock (GimpAlignmentType  action,
                   AlignDialog       *private)
{
  GtkWidget   *button;
  GtkWidget   *image;
  const gchar *stock_id = NULL;

  switch (action)
    {
    case GIMP_ALIGN_LEFT:
      stock_id = GIMP_STOCK_GRAVITY_WEST;
      break;
    case GIMP_ALIGN_HCENTER:
      stock_id = GIMP_STOCK_HCENTER;
      break;
    case GIMP_ALIGN_RIGHT:
      stock_id = GIMP_STOCK_GRAVITY_EAST;
      break;
    case GIMP_ALIGN_TOP:
      stock_id = GIMP_STOCK_GRAVITY_NORTH;
      break;
    case GIMP_ALIGN_VCENTER:
      stock_id = GIMP_STOCK_VCENTER;
      break;
    case GIMP_ALIGN_BOTTOM:
      stock_id = GIMP_STOCK_GRAVITY_SOUTH;
      break;
    default:
      g_return_val_if_reached (NULL);
      break;
    }

  button = gtk_button_new ();
  g_object_set_data (G_OBJECT (button), "action", GINT_TO_POINTER (action));

  image = gtk_image_new_from_stock (stock_id, GTK_ICON_SIZE_BUTTON);
  gtk_misc_set_padding (GTK_MISC (image), 2, 2);
  gtk_container_add (GTK_CONTAINER (button), image);
  gtk_widget_show (image);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (do_align),
                    private);

  gtk_widget_set_sensitive (button, TRUE);
  gtk_widget_show (button);

  return button;
}

