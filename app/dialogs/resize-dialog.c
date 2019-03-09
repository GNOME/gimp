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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "dialogs-types.h"

#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimplayer.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpsizebox.h"
#include "widgets/gimpviewabledialog.h"

#include "resize-dialog.h"

#include "gimp-intl.h"


#define RESPONSE_RESET 1
#define SB_WIDTH       8


typedef struct _ResizeDialog ResizeDialog;

struct _ResizeDialog
{
  GimpViewable       *viewable;
  GimpContext        *context;
  GimpFillType        fill_type;
  GimpItemSet         layer_set;
  gboolean            resize_text_layers;
  GimpResizeCallback  callback;
  gpointer            user_data;

  gint                old_width;
  gint                old_height;
  GimpUnit            old_unit;
  GimpFillType        old_fill_type;
  GimpItemSet         old_layer_set;
  gboolean            old_resize_text_layers;

  GtkWidget          *box;
  GtkWidget          *offset;
  GtkWidget          *area;
  GtkWidget          *layer_set_combo;
  GtkWidget          *fill_type_combo;
  GtkWidget          *text_layers_button;
};


/*  local function prototypes  */

static void   resize_dialog_free     (ResizeDialog *private);
static void   resize_dialog_response (GtkWidget    *dialog,
                                      gint          response_id,
                                      ResizeDialog *private);
static void   resize_dialog_reset    (ResizeDialog *private);

static void   size_notify            (GimpSizeBox  *box,
                                      GParamSpec   *pspec,
                                      ResizeDialog *private);
static void   offset_update          (GtkWidget    *widget,
                                      ResizeDialog *private);
static void   offsets_changed        (GtkWidget    *area,
                                      gint          off_x,
                                      gint          off_y,
                                      ResizeDialog *private);
static void   offset_center_clicked  (GtkWidget    *widget,
                                      ResizeDialog *private);


/*  public function  */

GtkWidget *
resize_dialog_new (GimpViewable       *viewable,
                   GimpContext        *context,
                   const gchar        *title,
                   const gchar        *role,
                   GtkWidget          *parent,
                   GimpHelpFunc        help_func,
                   const gchar        *help_id,
                   GimpUnit            unit,
                   GimpFillType        fill_type,
                   GimpItemSet         layer_set,
                   gboolean            resize_text_layers,
                   GimpResizeCallback  callback,
                   gpointer            user_data)
{
  ResizeDialog  *private;
  GtkWidget     *dialog;
  GtkWidget     *main_vbox;
  GtkWidget     *vbox;
  GtkWidget     *abox;
  GtkWidget     *frame;
  GtkWidget     *button;
  GtkWidget     *spinbutton;
  GtkWidget     *entry;
  GtkWidget     *hbox;
  GtkWidget     *combo;
  GtkAdjustment *adjustment;
  GdkPixbuf     *pixbuf;
  GtkSizeGroup  *size_group   = NULL;
  GimpImage     *image        = NULL;
  const gchar   *size_title   = NULL;
  const gchar   *layers_title = NULL;
  gint           width, height;
  gdouble        xres, yres;

  g_return_val_if_fail (GIMP_IS_VIEWABLE (viewable), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (callback != NULL, NULL);

  if (GIMP_IS_IMAGE (viewable))
    {
      image = GIMP_IMAGE (viewable);

      width  = gimp_image_get_width (image);
      height = gimp_image_get_height (image);

      size_title   = _("Canvas Size");
      layers_title = _("Layers");
    }
  else if (GIMP_IS_ITEM (viewable))
    {
      GimpItem *item = GIMP_ITEM (viewable);

      image = gimp_item_get_image (item);

      width  = gimp_item_get_width  (item);
      height = gimp_item_get_height (item);

      size_title   = _("Layer Size");
      layers_title = _("Fill With");
    }
  else
    {
      g_return_val_if_reached (NULL);
    }

  private = g_slice_new0 (ResizeDialog);

  private->viewable           = viewable;
  private->context            = context;
  private->fill_type          = fill_type;
  private->layer_set          = layer_set;
  private->resize_text_layers = resize_text_layers;
  private->callback           = callback;
  private->user_data          = user_data;

  private->old_width              = width;
  private->old_height             = height;
  private->old_unit               = unit;
  private->old_fill_type          = private->fill_type;
  private->old_layer_set          = private->layer_set;
  private->old_resize_text_layers = private->resize_text_layers;

  dialog = gimp_viewable_dialog_new (viewable, context,
                                     title, role, GIMP_ICON_OBJECT_RESIZE, title,
                                     parent,
                                     help_func, help_id,

                                     _("Re_set"),   RESPONSE_RESET,
                                     _("_Cancel"),  GTK_RESPONSE_CANCEL,
                                     _("_Resize"),  GTK_RESPONSE_OK,

                                     NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           RESPONSE_RESET,
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

  g_object_weak_ref (G_OBJECT (dialog),
                     (GWeakNotify) resize_dialog_free, private);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (resize_dialog_response),
                    private);

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      main_vbox, TRUE, TRUE, 0);
  gtk_widget_show (main_vbox);

  frame = gimp_frame_new (size_title);
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  gimp_image_get_resolution (image, &xres, &yres);

  private->box = g_object_new (GIMP_TYPE_SIZE_BOX,
                               "width",           width,
                               "height",          height,
                               "unit",            unit,
                               "xresolution",     xres,
                               "yresolution",     yres,
                               "keep-aspect",     FALSE,
                               "edit-resolution", FALSE,
                               NULL);
  gtk_container_add (GTK_CONTAINER (frame), private->box);
  gtk_widget_show (private->box);

  frame = gimp_frame_new (_("Offset"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  /*  the offset sizeentry  */
  adjustment = (GtkAdjustment *) gtk_adjustment_new (1, 1, 1, 1, 10, 0);
  spinbutton = gimp_spin_button_new (adjustment, 1.0, 2);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_entry_set_width_chars (GTK_ENTRY (spinbutton), SB_WIDTH);

  private->offset = entry = gimp_size_entry_new (1, unit, "%p",
                                                 TRUE, FALSE, FALSE, SB_WIDTH,
                                                 GIMP_SIZE_ENTRY_UPDATE_SIZE);
  gtk_table_set_col_spacing (GTK_TABLE (entry), 0, 6);
  gtk_table_set_col_spacing (GTK_TABLE (entry), 1, 6);
  gtk_table_set_col_spacing (GTK_TABLE (entry), 3, 12);
  gtk_table_set_row_spacing (GTK_TABLE (entry), 0, 2);

  gimp_size_entry_add_field (GIMP_SIZE_ENTRY (entry),
                             GTK_SPIN_BUTTON (spinbutton), NULL);
  gtk_table_attach_defaults (GTK_TABLE (entry), spinbutton,
                             1, 2, 0, 1);
  gtk_widget_show (spinbutton);

  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (entry),
                                _("_X:"), 0, 0, 0.0);
  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (entry),_("_Y:"), 1, 0, 0.0);
  gtk_box_pack_start (GTK_BOX (vbox), entry, FALSE, FALSE, 0);
  gtk_widget_show (entry);

  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (entry), 0, xres, FALSE);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (entry), 1, yres, FALSE);

  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (entry), 0, 0, 0);
  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (entry), 1, 0, 0);

  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (entry), 0, 0);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (entry), 1, 0);

  g_signal_connect (entry, "value-changed",
                    G_CALLBACK (offset_update),
                    private);

  button = gtk_button_new_with_mnemonic (_("C_enter"));
  gtk_table_attach_defaults (GTK_TABLE (entry), button, 4, 5, 1, 2);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (offset_center_clicked),
                    private);

  abox = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_box_pack_start (GTK_BOX (vbox), abox, FALSE, FALSE, 0);
  gtk_widget_show (abox);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_container_add (GTK_CONTAINER (abox), frame);
  gtk_widget_show (frame);

  private->area = gimp_offset_area_new (width, height);
  gtk_container_add (GTK_CONTAINER (frame), private->area);
  gtk_widget_show (private->area);

  gimp_viewable_get_preview_size (viewable, 200, TRUE, TRUE, &width, &height);
  pixbuf = gimp_viewable_get_pixbuf (viewable, context,
                                     width, height);

  if (pixbuf)
    gimp_offset_area_set_pixbuf (GIMP_OFFSET_AREA (private->area), pixbuf);

  g_signal_connect (private->area, "offsets-changed",
                    G_CALLBACK (offsets_changed),
                    private);

  g_signal_connect (private->box, "notify",
                    G_CALLBACK (size_notify),
                    private);

  frame = gimp_frame_new (layers_title);
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  if (GIMP_IS_IMAGE (viewable))
    {
      GtkWidget *label;

      size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

      hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
      gtk_widget_show (hbox);

      label = gtk_label_new_with_mnemonic (_("Resize _layers:"));
      gtk_label_set_xalign (GTK_LABEL (label), 0.0);
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
      gtk_widget_show (label);

      gtk_size_group_add_widget (size_group, label);

      private->layer_set_combo = combo =
        gimp_enum_combo_box_new (GIMP_TYPE_ITEM_SET);
      gtk_box_pack_start (GTK_BOX (hbox), combo, TRUE, TRUE, 0);
      gtk_widget_show (combo);

      gtk_label_set_mnemonic_widget (GTK_LABEL (label), combo);

      gimp_int_combo_box_connect (GIMP_INT_COMBO_BOX (combo),
                                  private->layer_set,
                                  G_CALLBACK (gimp_int_combo_box_get_active),
                                  &private->layer_set);
    }

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  private->fill_type_combo = combo =
    gimp_enum_combo_box_new (GIMP_TYPE_FILL_TYPE);
  gtk_box_pack_end (GTK_BOX (hbox), combo, TRUE, TRUE, 0);
  gtk_widget_show (combo);

  gimp_int_combo_box_connect (GIMP_INT_COMBO_BOX (combo),
                              private->fill_type,
                              G_CALLBACK (gimp_int_combo_box_get_active),
                              &private->fill_type);

  if (GIMP_IS_IMAGE (viewable))
    {
      GtkWidget *label;

      label = gtk_label_new_with_mnemonic (_("_Fill with:"));
      gtk_label_set_xalign (GTK_LABEL (label), 0.0);
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
      gtk_widget_show (label);

      gtk_label_set_mnemonic_widget (GTK_LABEL (label), combo);

      gtk_size_group_add_widget (size_group, label);

      private->text_layers_button = button =
        gtk_check_button_new_with_mnemonic (_("Resize _text layers"));
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                    private->resize_text_layers);
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
      gtk_widget_show (button);

      g_signal_connect (button, "toggled",
                        G_CALLBACK (gimp_toggle_button_update),
                        &private->resize_text_layers);

      gimp_help_set_help_data (button,
                               _("Resizing text layers will make them uneditable"),
                               NULL);

      g_object_unref (size_group);
    }

  return dialog;
}


/*  private functions  */

static void
resize_dialog_free (ResizeDialog *private)
{
  g_slice_free (ResizeDialog, private);
}

static void
resize_dialog_response (GtkWidget    *dialog,
                        gint          response_id,
                        ResizeDialog *private)
{
  GimpSizeEntry *entry = GIMP_SIZE_ENTRY (private->offset);
  GimpUnit       unit;
  gint           width;
  gint           height;

  switch (response_id)
    {
    case RESPONSE_RESET:
      resize_dialog_reset (private);
      break;

    case GTK_RESPONSE_OK:
      g_object_get (private->box,
                    "width",  &width,
                    "height", &height,
                    "unit",   &unit,
                    NULL);

      private->callback (dialog,
                         private->viewable,
                         private->context,
                         width,
                         height,
                         unit,
                         gimp_size_entry_get_refval (entry, 0),
                         gimp_size_entry_get_refval (entry, 1),
                         private->fill_type,
                         private->layer_set,
                         private->resize_text_layers,
                         private->user_data);
      break;

    default:
      gtk_widget_destroy (dialog);
      break;
    }
}

static void
resize_dialog_reset (ResizeDialog *private)
{
  g_object_set (private->box,
                "keep-aspect", FALSE,
                NULL);

  g_object_set (private->box,
                "width",       private->old_width,
                "height",      private->old_height,
                "unit",        private->old_unit,
                NULL);

  if (private->layer_set_combo)
    gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (private->layer_set_combo),
                                   private->old_layer_set);

  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (private->fill_type_combo),
                                 private->old_fill_type);

  if (private->text_layers_button)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (private->text_layers_button),
                                  private->old_resize_text_layers);
}

static void
size_notify (GimpSizeBox  *box,
             GParamSpec   *pspec,
             ResizeDialog *private)
{
  gint  diff_x = box->width  - private->old_width;
  gint  diff_y = box->height - private->old_height;

  gimp_offset_area_set_size (GIMP_OFFSET_AREA (private->area),
                             box->width, box->height);

  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (private->offset), 0,
                                         MIN (0, diff_x), MAX (0, diff_x));
  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (private->offset), 1,
                                         MIN (0, diff_y), MAX (0, diff_y));
}

static gint
resize_bound_off_x (ResizeDialog *private,
                    gint          offset_x)
{
  GimpSizeBox *box = GIMP_SIZE_BOX (private->box);

  if (private->old_width <= box->width)
    return CLAMP (offset_x, 0, (box->width - private->old_width));
  else
    return CLAMP (offset_x, (box->width - private->old_width), 0);
}

static gint
resize_bound_off_y (ResizeDialog *private,
                    gint          off_y)
{
  GimpSizeBox *box = GIMP_SIZE_BOX (private->box);

  if (private->old_height <= box->height)
    return CLAMP (off_y, 0, (box->height - private->old_height));
  else
    return CLAMP (off_y, (box->height - private->old_height), 0);
}

static void
offset_update (GtkWidget    *widget,
               ResizeDialog *private)
{
  GimpSizeEntry *entry = GIMP_SIZE_ENTRY (private->offset);
  gint           off_x;
  gint           off_y;

  off_x = resize_bound_off_x (private,
                              RINT (gimp_size_entry_get_refval (entry, 0)));
  off_y = resize_bound_off_y (private,
                              RINT (gimp_size_entry_get_refval (entry, 1)));

  gimp_offset_area_set_offsets (GIMP_OFFSET_AREA (private->area), off_x, off_y);
}

static void
offsets_changed (GtkWidget    *area,
                 gint          off_x,
                 gint          off_y,
                 ResizeDialog *private)
{
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (private->offset), 0, off_x);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (private->offset), 1, off_y);
}

static void
offset_center_clicked (GtkWidget    *widget,
                       ResizeDialog *private)
{
  GimpSizeBox *box = GIMP_SIZE_BOX (private->box);
  gint         off_x;
  gint         off_y;

  off_x = resize_bound_off_x (private, (box->width  - private->old_width)  / 2);
  off_y = resize_bound_off_y (private, (box->height - private->old_height) / 2);

  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (private->offset), 0, off_x);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (private->offset), 1, off_y);

  g_signal_emit_by_name (private->offset, "value-changed", 0);
}
