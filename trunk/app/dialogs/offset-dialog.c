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

#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "dialogs-types.h"

#include "core/gimp.h"
#include "core/gimpchannel.h"
#include "core/gimpcontext.h"
#include "core/gimpdrawable.h"
#include "core/gimpdrawable-offset.h"
#include "core/gimpitem.h"
#include "core/gimplayer.h"
#include "core/gimplayermask.h"
#include "core/gimpimage.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpviewabledialog.h"

#include "offset-dialog.h"

#include "gimp-intl.h"


#define WRAP_AROUND  (1 << 3)
#define FILL_MASK    (GIMP_OFFSET_BACKGROUND | GIMP_OFFSET_TRANSPARENT)


typedef struct
{
  GimpContext    *context;

  GtkWidget      *dialog;
  GtkWidget      *off_se;

  GimpOffsetType  fill_type;

  GimpImage      *image;
} OffsetDialog;


/*  local function prototypes  */

static void  offset_response            (GtkWidget    *widget,
                                         gint          response_id,
                                         OffsetDialog *dialog);
static void  offset_halfheight_callback (GtkWidget    *widget,
                                         OffsetDialog *dialog);
static void  offset_dialog_free         (OffsetDialog *dialog);


/*  public functions  */

GtkWidget *
offset_dialog_new (GimpDrawable *drawable,
                   GimpContext  *context,
                   GtkWidget    *parent)
{
  GimpItem     *item;
  OffsetDialog *dialog;
  GtkWidget    *main_vbox;
  GtkWidget    *vbox;
  GtkWidget    *hbox;
  GtkWidget    *button;
  GtkWidget    *spinbutton;
  GtkWidget    *frame;
  GtkWidget    *radio_button;
  GtkObject    *adjustment;
  const gchar  *title = NULL;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (GTK_IS_WIDGET (parent), NULL);

  dialog = g_slice_new0 (OffsetDialog);

  dialog->context   = context;
  dialog->fill_type = gimp_drawable_has_alpha (drawable) | WRAP_AROUND;
  item = GIMP_ITEM (drawable);
  dialog->image     = gimp_item_get_image (item);

  if (GIMP_IS_LAYER (drawable))
    title = _("Offset Layer");
  else if (GIMP_IS_LAYER_MASK (drawable))
    title = _("Offset Layer Mask");
  else if (GIMP_IS_CHANNEL (drawable))
    title = _("Offset Channel");
  else
    g_warning ("%s: unexpected drawable type", G_STRFUNC);

  dialog->dialog =
    gimp_viewable_dialog_new (GIMP_VIEWABLE (drawable), context,
                              _("Offset"), "gimp-drawable-offset",
                              GIMP_STOCK_TOOL_MOVE,
                              title,
                              parent,
                              gimp_standard_help_func,
                              GIMP_HELP_LAYER_OFFSET,

                              GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                              /*  offset, used as a verb  */
                              _("_Offset"),     GTK_RESPONSE_OK,

                              NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog->dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gtk_window_set_resizable (GTK_WINDOW (dialog->dialog), FALSE);

  g_object_weak_ref (G_OBJECT (dialog->dialog),
                     (GWeakNotify) offset_dialog_free, dialog);

  g_signal_connect (dialog->dialog, "response",
                    G_CALLBACK (offset_response),
                    dialog);

  main_vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog->dialog)->vbox),
                     main_vbox);
  gtk_widget_show (main_vbox);

  /*  The offset frame  */
  frame = gimp_frame_new (_("Offset"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  hbox = gtk_hbox_new (0, FALSE);
  gtk_container_add (GTK_CONTAINER (frame), hbox);
  gtk_widget_show (hbox);

  vbox = gtk_vbox_new (6, FALSE);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  spinbutton = gimp_spin_button_new (&adjustment,
                                     1, 1, 1, 1, 10, 1,
                                     1, 2);
  gtk_entry_set_width_chars (GTK_ENTRY (spinbutton), 10);

  dialog->off_se = gimp_size_entry_new (1, GIMP_UNIT_PIXEL, "%a",
                                        TRUE, TRUE, FALSE, 10,
                                        GIMP_SIZE_ENTRY_UPDATE_SIZE);

  gtk_table_set_col_spacing (GTK_TABLE (dialog->off_se), 0, 4);
  gtk_table_set_col_spacing (GTK_TABLE (dialog->off_se), 1, 4);
  gtk_table_set_row_spacing (GTK_TABLE (dialog->off_se), 0, 2);

  gimp_size_entry_add_field (GIMP_SIZE_ENTRY (dialog->off_se),
                             GTK_SPIN_BUTTON (spinbutton), NULL);
  gtk_table_attach_defaults (GTK_TABLE (dialog->off_se), spinbutton,
                             1, 2, 0, 1);
  gtk_widget_show (spinbutton);

  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (dialog->off_se),
                                _("_X:"), 0, 0, 0.0);
  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (dialog->off_se),
                                _("_Y:"), 1, 0, 0.0);

  gtk_box_pack_start (GTK_BOX (vbox), dialog->off_se, FALSE, FALSE, 0);
  gtk_widget_show (dialog->off_se);

  gimp_size_entry_set_unit (GIMP_SIZE_ENTRY (dialog->off_se), GIMP_UNIT_PIXEL);

  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (dialog->off_se), 0,
                                  dialog->image->xresolution, FALSE);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (dialog->off_se), 1,
                                  dialog->image->yresolution, FALSE);

  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (dialog->off_se), 0,
                                         -item->width,
                                         item->width);
  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (dialog->off_se), 1,
                                         -item->height,
                                         item->height);

  gimp_size_entry_set_size (GIMP_SIZE_ENTRY (dialog->off_se), 0,
                            0, item->width);
  gimp_size_entry_set_size (GIMP_SIZE_ENTRY (dialog->off_se), 1,
                            0, item->width);

  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (dialog->off_se), 0, 0);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (dialog->off_se), 1, 0);

  button = gtk_button_new_with_mnemonic (_("Offset by  x/_2, y/2"));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (offset_halfheight_callback),
                    dialog);

  /*  The edge behavior frame  */
  frame = gimp_int_radio_group_new (TRUE, _("Edge Behavior"),
                                    G_CALLBACK (gimp_radio_button_update),
                                    &dialog->fill_type, dialog->fill_type,

                                    _("_Wrap around"),
                                    WRAP_AROUND, NULL,

                                    _("Fill with _background color"),
                                    GIMP_OFFSET_BACKGROUND, NULL,

                                    _("Make _transparent"),
                                    GIMP_OFFSET_TRANSPARENT, &radio_button,
                                    NULL);

  if (! gimp_drawable_has_alpha (drawable))
    gtk_widget_set_sensitive (radio_button, FALSE);

  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  return dialog->dialog;
}


/*  private functions  */

static void
offset_response (GtkWidget    *widget,
                 gint          response_id,
                 OffsetDialog *dialog)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      GimpImage *image = dialog->image;

      if (image)
        {
          GimpDrawable *drawable = gimp_image_get_active_drawable (image);
          gint          offset_x;
          gint          offset_y;

          offset_x =
            RINT (gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (dialog->off_se),
                                              0));
          offset_y =
            RINT (gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (dialog->off_se),
                                              1));

          gimp_drawable_offset (drawable,
                                dialog->context,
                                dialog->fill_type & WRAP_AROUND ? TRUE : FALSE,
                                dialog->fill_type & FILL_MASK,
                                offset_x, offset_y);
          gimp_image_flush (image);
        }
    }

  gtk_widget_destroy (dialog->dialog);
}

static void
offset_halfheight_callback (GtkWidget    *widget,
                            OffsetDialog *dialog)
{
  GimpImage *image = dialog->image;

  if (image)
    {
      GimpItem *item = GIMP_ITEM (gimp_image_get_active_drawable (image));

      gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (dialog->off_se),
                                  0, item->width / 2);
      gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (dialog->off_se),
                                  1, item->height / 2);
   }
}

static void
offset_dialog_free (OffsetDialog *dialog)
{
  g_slice_free (OffsetDialog, dialog);
}
