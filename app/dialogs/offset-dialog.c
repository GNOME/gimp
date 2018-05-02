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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "dialogs-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpdrawable.h"
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


typedef struct _OffsetDialog OffsetDialog;

struct _OffsetDialog
{
  GimpDrawable       *drawable;
  GimpContext        *context;
  GimpOffsetType      fill_type;
  GimpOffsetCallback  callback;
  gpointer            user_data;

  GtkWidget          *off_se;
};


/*  local function prototypes  */

static void  offset_dialog_free             (OffsetDialog *private);
static void  offset_dialog_response         (GtkWidget    *dialog,
                                             gint          response_id,
                                             OffsetDialog *private);
static void  offset_dialog_half_xy_callback (GtkWidget    *widget,
                                             OffsetDialog *private);
static void  offset_dialog_half_x_callback  (GtkWidget    *widget,
                                             OffsetDialog *private);
static void  offset_dialog_half_y_callback  (GtkWidget    *widget,
                                             OffsetDialog *private);


/*  public functions  */

GtkWidget *
offset_dialog_new (GimpDrawable       *drawable,
                   GimpContext        *context,
                   GtkWidget          *parent,
                   GimpOffsetCallback  callback,
                   gpointer            user_data)
{
  OffsetDialog  *private;
  GimpImage     *image;
  GimpItem      *item;
  GtkWidget     *dialog;
  GtkWidget     *main_vbox;
  GtkWidget     *vbox;
  GtkWidget     *hbox;
  GtkWidget     *button;
  GtkWidget     *spinbutton;
  GtkWidget     *frame;
  GtkWidget     *radio_button;
  GtkAdjustment *adjustment;
  gdouble        xres;
  gdouble        yres;
  const gchar   *title = NULL;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (GTK_IS_WIDGET (parent), NULL);
  g_return_val_if_fail (callback != NULL, NULL);

  item  = GIMP_ITEM (drawable);
  image = gimp_item_get_image (item);

  private = g_slice_new0 (OffsetDialog);

  private->drawable  = drawable;
  private->context   = context;
  private->fill_type = gimp_drawable_has_alpha (drawable) | WRAP_AROUND;
  private->callback  = callback;
  private->user_data = user_data;

  gimp_image_get_resolution (image, &xres, &yres);

  if (GIMP_IS_LAYER (drawable))
    title = _("Offset Layer");
  else if (GIMP_IS_LAYER_MASK (drawable))
    title = _("Offset Layer Mask");
  else if (GIMP_IS_CHANNEL (drawable))
    title = _("Offset Channel");
  else
    g_warning ("%s: unexpected drawable type", G_STRFUNC);

  dialog = gimp_viewable_dialog_new (GIMP_VIEWABLE (drawable), context,
                                     _("Offset"), "gimp-drawable-offset",
                                     GIMP_ICON_TOOL_MOVE,
                                     title,
                                     parent,
                                     gimp_standard_help_func,
                                     GIMP_HELP_LAYER_OFFSET,

                                     _("_Cancel"), GTK_RESPONSE_CANCEL,
                                     /*  offset, used as a verb  */
                                     _("_Offset"), GTK_RESPONSE_OK,

                                     NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

  g_object_weak_ref (G_OBJECT (dialog),
                     (GWeakNotify) offset_dialog_free, private);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (offset_dialog_response),
                    private);

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      main_vbox, TRUE, TRUE, 0);
  gtk_widget_show (main_vbox);

  /*  The offset frame  */
  frame = gimp_frame_new (_("Offset"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  adjustment = (GtkAdjustment *)
    gtk_adjustment_new (1, 1, 1, 1, 10, 0);
  spinbutton = gtk_spin_button_new (adjustment, 1.0, 2);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_entry_set_width_chars (GTK_ENTRY (spinbutton), 10);

  private->off_se = gimp_size_entry_new (1, GIMP_UNIT_PIXEL, "%a",
                                        TRUE, TRUE, FALSE, 10,
                                        GIMP_SIZE_ENTRY_UPDATE_SIZE);

  gimp_size_entry_add_field (GIMP_SIZE_ENTRY (private->off_se),
                             GTK_SPIN_BUTTON (spinbutton), NULL);
  gtk_grid_attach (GTK_GRID (private->off_se), spinbutton, 1, 0, 1, 1);
  gtk_widget_show (spinbutton);

  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (private->off_se),
                                _("_X:"), 0, 0, 0.0);
  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (private->off_se),
                                _("_Y:"), 1, 0, 0.0);

  gtk_box_pack_start (GTK_BOX (vbox), private->off_se, FALSE, FALSE, 0);
  gtk_widget_show (private->off_se);

  gimp_size_entry_set_unit (GIMP_SIZE_ENTRY (private->off_se), GIMP_UNIT_PIXEL);

  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (private->off_se), 0,
                                  xres, FALSE);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (private->off_se), 1,
                                  yres, FALSE);

  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (private->off_se), 0,
                                         - gimp_item_get_width (item),
                                         gimp_item_get_width (item));
  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (private->off_se), 1,
                                         - gimp_item_get_height (item),
                                         gimp_item_get_height (item));

  gimp_size_entry_set_size (GIMP_SIZE_ENTRY (private->off_se), 0,
                            0, gimp_item_get_width (item));
  gimp_size_entry_set_size (GIMP_SIZE_ENTRY (private->off_se), 1,
                            0, gimp_item_get_height (item));

  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (private->off_se), 0, 0);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (private->off_se), 1, 0);

  button = gtk_button_new_with_mnemonic (_("By width/_2, height/2"));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (offset_dialog_half_xy_callback),
                    private);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  button = gtk_button_new_with_mnemonic ("By _width/2");
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (offset_dialog_half_x_callback),
                    private);

  button = gtk_button_new_with_mnemonic ("By _height/2");
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (offset_dialog_half_y_callback),
                    private);

  /*  The edge behavior frame  */
  frame = gimp_int_radio_group_new (TRUE, _("Edge Behavior"),
                                    G_CALLBACK (gimp_radio_button_update),
                                    &private->fill_type, private->fill_type,

                                    _("W_rap around"),
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

  return dialog;
}


/*  private functions  */

static void
offset_dialog_free (OffsetDialog *private)
{
  g_slice_free (OffsetDialog, private);
}

static void
offset_dialog_response (GtkWidget    *dialog,
                        gint          response_id,
                        OffsetDialog *private)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      gint offset_x;
      gint offset_y;

      offset_x =
        RINT (gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (private->off_se),
                                          0));
      offset_y =
        RINT (gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (private->off_se),
                                          1));

      private->callback (dialog,
                         private->drawable,
                         private->context,
                         private->fill_type & WRAP_AROUND ? TRUE : FALSE,
                         private->fill_type & FILL_MASK,
                         offset_x,
                         offset_y);
    }
  else
    {
      gtk_widget_destroy (dialog);
    }
}

static void
offset_dialog_half_xy_callback (GtkWidget    *widget,
                                OffsetDialog *private)
{
  GimpItem *item = GIMP_ITEM (private->drawable);

  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (private->off_se),
                              0, gimp_item_get_width (item) / 2);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (private->off_se),
                              1, gimp_item_get_height (item) / 2);
}

static void
offset_dialog_half_x_callback (GtkWidget    *widget,
                               OffsetDialog *private)
{
  GimpItem *item = GIMP_ITEM (private->drawable);

  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (private->off_se),
                              0, gimp_item_get_width (item) / 2);
}

static void
offset_dialog_half_y_callback (GtkWidget    *widget,
                               OffsetDialog *private)
{
  GimpItem *item = GIMP_ITEM (private->drawable);

  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (private->off_se),
                              1, gimp_item_get_height (item) / 2);
}
