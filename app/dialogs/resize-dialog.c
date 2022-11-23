/* LIGMA - The GNU Image Manipulation Program
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

#include "libligmabase/ligmabase.h"
#include "libligmamath/ligmamath.h"
#include "libligmawidgets/ligmawidgets.h"

#include "dialogs-types.h"

#include "core/ligma.h"
#include "core/ligmacontext.h"
#include "core/ligmaimage.h"
#include "core/ligmalayer.h"
#include "core/ligmatemplate.h"

#include "widgets/ligmacontainercombobox.h"
#include "widgets/ligmahelp-ids.h"
#include "widgets/ligmasizebox.h"
#include "widgets/ligmaviewabledialog.h"
#include "widgets/ligmawidgets-constructors.h"

#include "resize-dialog.h"

#include "ligma-intl.h"


#define RESPONSE_RESET 1
#define SB_WIDTH       8


typedef struct _ResizeDialog ResizeDialog;

struct _ResizeDialog
{
  LigmaViewable       *viewable;
  LigmaContext        *context;
  LigmaContext        *parent_context;
  LigmaFillType        fill_type;
  LigmaItemSet         layer_set;
  gboolean            resize_text_layers;
  LigmaResizeCallback  callback;
  gpointer            user_data;

  gdouble             old_xres;
  gdouble             old_yres;
  LigmaUnit            old_res_unit;
  gint                old_width;
  gint                old_height;
  LigmaUnit            old_unit;
  LigmaFillType        old_fill_type;
  LigmaItemSet         old_layer_set;
  gboolean            old_resize_text_layers;

  GtkWidget          *box;
  GtkWidget          *offset;
  GtkWidget          *area;
  GtkWidget          *layer_set_combo;
  GtkWidget          *fill_type_combo;
  GtkWidget          *text_layers_button;

  GtkWidget          *ppi_box;
  GtkWidget          *ppi_image;
  GtkWidget          *ppi_template;
  LigmaTemplate       *template;
};


/*  local function prototypes  */

static void   resize_dialog_free     (ResizeDialog *private);
static void   resize_dialog_response (GtkWidget    *dialog,
                                      gint          response_id,
                                      ResizeDialog *private);
static void   resize_dialog_reset    (ResizeDialog *private);

static void   size_notify            (LigmaSizeBox  *box,
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

static void   template_changed       (LigmaContext  *context,
                                      LigmaTemplate *template,
                                      ResizeDialog *private);

static void   reset_template_clicked (GtkWidget    *button,
                                      ResizeDialog *private);
static void   ppi_select_toggled     (GtkWidget    *radio,
                                      ResizeDialog *private);

/*  public function  */

GtkWidget *
resize_dialog_new (LigmaViewable       *viewable,
                   LigmaContext        *context,
                   const gchar        *title,
                   const gchar        *role,
                   GtkWidget          *parent,
                   LigmaHelpFunc        help_func,
                   const gchar        *help_id,
                   LigmaUnit            unit,
                   LigmaFillType        fill_type,
                   LigmaItemSet         layer_set,
                   gboolean            resize_text_layers,
                   LigmaResizeCallback  callback,
                   gpointer            user_data)
{
  ResizeDialog  *private;
  GtkWidget     *dialog;
  GtkWidget     *main_vbox;
  GtkWidget     *vbox;
  GtkWidget     *center_hbox;
  GtkWidget     *center_left_vbox;
  GtkWidget     *center_right_vbox;
  GtkWidget     *frame;
  GtkWidget     *button;
  GtkWidget     *spinbutton;
  GtkWidget     *entry;
  GtkWidget     *hbox;
  GtkWidget     *combo;
  GtkWidget     *label;
  GtkWidget     *template_selector;
  GtkWidget     *ppi_image;
  GtkWidget     *ppi_template;
  GtkAdjustment *adjustment;
  GdkPixbuf     *pixbuf;
  GtkSizeGroup  *size_group   = NULL;
  LigmaImage     *image        = NULL;
  const gchar   *size_title   = NULL;
  const gchar   *layers_title = NULL;
  gint           width, height;
  gdouble        xres, yres;

  g_return_val_if_fail (LIGMA_IS_VIEWABLE (viewable), NULL);
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (callback != NULL, NULL);

  if (LIGMA_IS_IMAGE (viewable))
    {
      image = LIGMA_IMAGE (viewable);

      width  = ligma_image_get_width (image);
      height = ligma_image_get_height (image);

      size_title   = _("Canvas Size");
      layers_title = _("Layers");
    }
  else if (LIGMA_IS_ITEM (viewable))
    {
      LigmaItem *item = LIGMA_ITEM (viewable);

      image = ligma_item_get_image (item);

      width  = ligma_item_get_width  (item);
      height = ligma_item_get_height (item);

      size_title   = _("Layer Size");
      layers_title = _("Fill With");
    }
  else
    {
      g_return_val_if_reached (NULL);
    }

  private = g_slice_new0 (ResizeDialog);

  private->parent_context = context;
  private->context        = ligma_context_new (context->ligma,
                                              "resize-dialog",
                                              context);

  ligma_image_get_resolution (image, &xres, &yres);

  private->old_xres     = xres;
  private->old_yres     = yres;
  private->old_res_unit = ligma_image_get_unit (image);

  private->viewable           = viewable;
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

  ligma_context_set_template (private->context, NULL);

  dialog = ligma_viewable_dialog_new (g_list_prepend (NULL, viewable), context,
                                     title, role, LIGMA_ICON_OBJECT_RESIZE, title,
                                     parent,
                                     help_func, help_id,

                                     _("Re_set"),   RESPONSE_RESET,
                                     _("_Cancel"),  GTK_RESPONSE_CANCEL,
                                     _("_Resize"),  GTK_RESPONSE_OK,

                                     NULL);

  ligma_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
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

  /* template selector */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new_with_mnemonic (_("_Template:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  template_selector = g_object_new (LIGMA_TYPE_CONTAINER_COMBO_BOX,
                                    "container",         context->ligma->templates,
                                    "context",           private->context,
                                    "view-size",         16,
                                    "view-border-width", 0,
                                    "ellipsize",         PANGO_ELLIPSIZE_NONE,
                                    "focus-on-click",    FALSE,
                                    NULL);

  gtk_box_pack_start (GTK_BOX (hbox), template_selector, TRUE, TRUE, 0);
  gtk_widget_show (template_selector);

  gtk_label_set_mnemonic_widget (GTK_LABEL (label), template_selector);

  g_signal_connect (private->context,
                    "template-changed",
                    G_CALLBACK (template_changed),
                    private);

  /* reset template button */
  button = ligma_icon_button_new (LIGMA_ICON_RESET, NULL);
  gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
  gtk_image_set_from_icon_name (GTK_IMAGE (gtk_bin_get_child (GTK_BIN (button))),
                                LIGMA_ICON_RESET, GTK_ICON_SIZE_MENU);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (button,
                    "clicked",
                    G_CALLBACK (reset_template_clicked),
                    private);

  ligma_help_set_help_data (button,
                           _("Reset the template selection"),
                           NULL);

  /* ppi selector box */
  private->ppi_box = vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_box_pack_start (GTK_BOX (main_vbox), vbox, FALSE, FALSE, 0);

  label = gtk_label_new (_("Template and image print resolution don't match.\n"
                           "Choose how to scale the canvas:"));
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_CENTER);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_box_set_homogeneous (GTK_BOX (hbox), TRUE);
  gtk_widget_show (hbox);

  /* actual label text is set inside template_change fn. */
  ppi_image    = gtk_radio_button_new_with_label (NULL, "");
  ppi_template = gtk_radio_button_new_with_label (NULL, "");

  private->ppi_image    = ppi_image;
  private->ppi_template = ppi_template;

  gtk_radio_button_join_group (GTK_RADIO_BUTTON (ppi_template),
                               GTK_RADIO_BUTTON (ppi_image));

  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (ppi_image), FALSE);
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (ppi_template), FALSE);

  gtk_box_pack_start (GTK_BOX (hbox), ppi_image, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), ppi_template, FALSE, FALSE, 0);

  gtk_widget_show (ppi_image);
  gtk_widget_show (ppi_template);

  g_signal_connect (G_OBJECT (ppi_image),
                    "toggled",
                    G_CALLBACK (ppi_select_toggled),
                    private);

  g_signal_connect (G_OBJECT (ppi_template),
                    "toggled",
                    G_CALLBACK (ppi_select_toggled),
                    private);

  /* For space gain, organize the main widgets in both vertical and
   * horizontal layout.
   * The size and offset fields are on the center left, while the
   * preview and the "Center" button are on center right.
   */
  center_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_box_pack_start (GTK_BOX (main_vbox), center_hbox, FALSE, FALSE, 0);
  gtk_widget_show (center_hbox);

  center_left_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);
  gtk_box_pack_start (GTK_BOX (center_hbox), center_left_vbox, FALSE, FALSE, 0);
  gtk_widget_show (center_left_vbox);

  center_right_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_box_pack_start (GTK_BOX (center_hbox), center_right_vbox, FALSE, FALSE, 0);
  gtk_widget_show (center_right_vbox);

  /* size select frame */
  frame = ligma_frame_new (size_title);
  gtk_box_pack_start (GTK_BOX (center_left_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  /* size box */
  private->box = g_object_new (LIGMA_TYPE_SIZE_BOX,
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

  /* offset frame */
  frame = ligma_frame_new (_("Offset"));
  gtk_box_pack_start (GTK_BOX (center_left_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  /*  the offset sizeentry  */
  adjustment = gtk_adjustment_new (1, 1, 1, 1, 10, 0);
  spinbutton = ligma_spin_button_new (adjustment, 1.0, 2);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_entry_set_width_chars (GTK_ENTRY (spinbutton), SB_WIDTH);

  private->offset = entry = ligma_size_entry_new (1, unit, "%p",
                                                 TRUE, FALSE, FALSE, SB_WIDTH,
                                                 LIGMA_SIZE_ENTRY_UPDATE_SIZE);
  ligma_size_entry_add_field (LIGMA_SIZE_ENTRY (entry),
                             GTK_SPIN_BUTTON (spinbutton), NULL);
  gtk_grid_attach (GTK_GRID (entry), spinbutton, 1, 0, 1, 1);
  gtk_widget_show (spinbutton);

  ligma_size_entry_attach_label (LIGMA_SIZE_ENTRY (entry),
                                _("_X:"), 0, 0, 0.0);
  ligma_size_entry_attach_label (LIGMA_SIZE_ENTRY (entry),_("_Y:"), 1, 0, 0.0);
  gtk_box_pack_start (GTK_BOX (vbox), entry, FALSE, FALSE, 0);
  gtk_widget_show (entry);

  ligma_size_entry_set_resolution (LIGMA_SIZE_ENTRY (entry), 0, xres, FALSE);
  ligma_size_entry_set_resolution (LIGMA_SIZE_ENTRY (entry), 1, yres, FALSE);

  ligma_size_entry_set_refval_boundaries (LIGMA_SIZE_ENTRY (entry), 0, 0, 0);
  ligma_size_entry_set_refval_boundaries (LIGMA_SIZE_ENTRY (entry), 1, 0, 0);

  ligma_size_entry_set_refval (LIGMA_SIZE_ENTRY (entry), 0, 0);
  ligma_size_entry_set_refval (LIGMA_SIZE_ENTRY (entry), 1, 0);

  g_signal_connect (entry, "value-changed",
                    G_CALLBACK (offset_update),
                    private);

  frame = gtk_frame_new (NULL);
  gtk_widget_set_halign (frame, GTK_ALIGN_CENTER);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (center_right_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  private->area = ligma_offset_area_new (width, height);
  gtk_container_add (GTK_CONTAINER (frame), private->area);
  gtk_widget_show (private->area);

  ligma_viewable_get_preview_size (viewable, 200, TRUE, TRUE, &width, &height);
  pixbuf = ligma_viewable_get_pixbuf (viewable, context,
                                     width, height);

  if (pixbuf)
    ligma_offset_area_set_pixbuf (LIGMA_OFFSET_AREA (private->area), pixbuf);

  g_signal_connect (private->area, "offsets-changed",
                    G_CALLBACK (offsets_changed),
                    private);

  g_signal_connect (private->box, "notify",
                    G_CALLBACK (size_notify),
                    private);

  /* Button to center the image on canvas just below the preview. */
  button = gtk_button_new_with_mnemonic (_("C_enter"));
  gtk_box_pack_start (GTK_BOX (center_right_vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (offset_center_clicked),
                    private);

  frame = ligma_frame_new (layers_title);
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  if (LIGMA_IS_IMAGE (viewable))
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
        ligma_enum_combo_box_new (LIGMA_TYPE_ITEM_SET);
      gtk_box_pack_start (GTK_BOX (hbox), combo, TRUE, TRUE, 0);
      gtk_widget_show (combo);

      gtk_label_set_mnemonic_widget (GTK_LABEL (label), combo);

      ligma_int_combo_box_connect (LIGMA_INT_COMBO_BOX (combo),
                                  private->layer_set,
                                  G_CALLBACK (ligma_int_combo_box_get_active),
                                  &private->layer_set, NULL);
    }

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  private->fill_type_combo = combo =
    ligma_enum_combo_box_new (LIGMA_TYPE_FILL_TYPE);
  gtk_box_pack_end (GTK_BOX (hbox), combo, TRUE, TRUE, 0);
  gtk_widget_show (combo);

  ligma_int_combo_box_connect (LIGMA_INT_COMBO_BOX (combo),
                              private->fill_type,
                              G_CALLBACK (ligma_int_combo_box_get_active),
                              &private->fill_type, NULL);

  if (LIGMA_IS_IMAGE (viewable))
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
                        G_CALLBACK (ligma_toggle_button_update),
                        &private->resize_text_layers);

      ligma_help_set_help_data (button,
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
  g_object_unref (private->context);

  g_slice_free (ResizeDialog, private);
}

static void
resize_dialog_response (GtkWidget    *dialog,
                        gint          response_id,
                        ResizeDialog *private)
{
  LigmaSizeEntry *entry = LIGMA_SIZE_ENTRY (private->offset);
  LigmaUnit       unit;
  gint           width;
  gint           height;
  gdouble        xres;
  gdouble        yres;
  LigmaUnit       res_unit;

  switch (response_id)
    {
    case RESPONSE_RESET:
      resize_dialog_reset (private);
      break;

    case GTK_RESPONSE_OK:
      g_object_get (private->box,
                    "width",           &width,
                    "height",          &height,
                    "unit",            &unit,
                    "xresolution",     &xres,
                    "yresolution",     &yres,
                    "resolution-unit", &res_unit,
                    NULL);

      private->callback (dialog,
                         private->viewable,
                         private->parent_context,
                         width,
                         height,
                         unit,
                         ligma_size_entry_get_refval (entry, 0),
                         ligma_size_entry_get_refval (entry, 1),
                         xres,
                         yres,
                         res_unit,
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
                "width",           private->old_width,
                "height",          private->old_height,
                "unit",            private->old_unit,
                "xresolution",     private->old_xres,
                "yresolution",     private->old_yres,
                "resolution-unit", private->old_res_unit,
                NULL);

  if (private->layer_set_combo)
    ligma_int_combo_box_set_active (LIGMA_INT_COMBO_BOX (private->layer_set_combo),
                                   private->old_layer_set);

  ligma_int_combo_box_set_active (LIGMA_INT_COMBO_BOX (private->fill_type_combo),
                                 private->old_fill_type);

  if (private->text_layers_button)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (private->text_layers_button),
                                  private->old_resize_text_layers);

  ligma_context_set_template (private->context, NULL);

  ligma_size_entry_set_unit (LIGMA_SIZE_ENTRY (private->offset),
                            private->old_unit);
}

static void
size_notify (LigmaSizeBox  *box,
             GParamSpec   *pspec,
             ResizeDialog *private)
{
  gint  diff_x = box->width  - private->old_width;
  gint  diff_y = box->height - private->old_height;

  ligma_offset_area_set_size (LIGMA_OFFSET_AREA (private->area),
                             box->width, box->height);

  ligma_size_entry_set_refval_boundaries (LIGMA_SIZE_ENTRY (private->offset), 0,
                                         MIN (0, diff_x), MAX (0, diff_x));
  ligma_size_entry_set_refval_boundaries (LIGMA_SIZE_ENTRY (private->offset), 1,
                                         MIN (0, diff_y), MAX (0, diff_y));
}

static gint
resize_bound_off_x (ResizeDialog *private,
                    gint          offset_x)
{
  LigmaSizeBox *box = LIGMA_SIZE_BOX (private->box);

  if (private->old_width <= box->width)
    return CLAMP (offset_x, 0, (box->width - private->old_width));
  else
    return CLAMP (offset_x, (box->width - private->old_width), 0);
}

static gint
resize_bound_off_y (ResizeDialog *private,
                    gint          off_y)
{
  LigmaSizeBox *box = LIGMA_SIZE_BOX (private->box);

  if (private->old_height <= box->height)
    return CLAMP (off_y, 0, (box->height - private->old_height));
  else
    return CLAMP (off_y, (box->height - private->old_height), 0);
}

static void
offset_update (GtkWidget    *widget,
               ResizeDialog *private)
{
  LigmaSizeEntry *entry = LIGMA_SIZE_ENTRY (private->offset);
  gint           off_x;
  gint           off_y;

  off_x = resize_bound_off_x (private,
                              RINT (ligma_size_entry_get_refval (entry, 0)));
  off_y = resize_bound_off_y (private,
                              RINT (ligma_size_entry_get_refval (entry, 1)));

  ligma_offset_area_set_offsets (LIGMA_OFFSET_AREA (private->area), off_x, off_y);
}

static void
offsets_changed (GtkWidget    *area,
                 gint          off_x,
                 gint          off_y,
                 ResizeDialog *private)
{
  ligma_size_entry_set_refval (LIGMA_SIZE_ENTRY (private->offset), 0, off_x);
  ligma_size_entry_set_refval (LIGMA_SIZE_ENTRY (private->offset), 1, off_y);
}

static void
offset_center_clicked (GtkWidget    *widget,
                       ResizeDialog *private)
{
  LigmaSizeBox *box = LIGMA_SIZE_BOX (private->box);
  gint         off_x;
  gint         off_y;

  off_x = resize_bound_off_x (private, (box->width  - private->old_width)  / 2);
  off_y = resize_bound_off_y (private, (box->height - private->old_height) / 2);

  ligma_size_entry_set_refval (LIGMA_SIZE_ENTRY (private->offset), 0, off_x);
  ligma_size_entry_set_refval (LIGMA_SIZE_ENTRY (private->offset), 1, off_y);

  g_signal_emit_by_name (private->offset, "value-changed", 0);
}

static void
template_changed (LigmaContext  *context,
                  LigmaTemplate *template,
                  ResizeDialog *private)
{
  LigmaUnit unit = private->old_unit;

  private->template = template;

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (private->ppi_image), TRUE);
  gtk_widget_hide (private->ppi_box);

  if (template != NULL)
    {
      gdouble  xres;
      gdouble  yres;
      LigmaUnit res_unit;
      gboolean resolution_mismatch;

      unit     = ligma_template_get_unit            (template);
      xres     = ligma_template_get_resolution_x    (template);
      yres     = ligma_template_get_resolution_y    (template);
      res_unit = ligma_template_get_resolution_unit (template);

      resolution_mismatch = xres     != private->old_xres ||
                            yres     != private->old_yres ||
                            res_unit != private->old_res_unit;

      if (resolution_mismatch &&
          unit != LIGMA_UNIT_PIXEL)
        {
          gchar *text;

          text = g_strdup_printf (_("Scale template to %.2f ppi"),
                                  private->old_xres);
          gtk_button_set_label (GTK_BUTTON (private->ppi_image), text);
          g_free (text);

          text = g_strdup_printf (_("Set image to %.2f ppi"),
                                  xres);
          gtk_button_set_label (GTK_BUTTON (private->ppi_template), text);
          g_free (text);

          gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (private->ppi_image),
                                        TRUE);

          gtk_widget_show (private->ppi_box);
        }
    }

  ppi_select_toggled (NULL, private);

  ligma_size_entry_set_unit (LIGMA_SIZE_ENTRY (private->offset), unit);
}

static void
ppi_select_toggled (GtkWidget    *radio,
                    ResizeDialog *private)
{
  gint             width;
  gint             height;
  LigmaUnit         unit;
  gdouble          xres;
  gdouble          yres;
  LigmaUnit         res_unit;
  GtkToggleButton *image_button;
  gboolean         use_image_ppi;

  width    = private->old_width;
  height   = private->old_height;
  xres     = private->old_xres;
  yres     = private->old_yres;
  res_unit = private->old_res_unit;
  unit     = private->old_unit;

  image_button  = GTK_TOGGLE_BUTTON (private->ppi_image);
  use_image_ppi = gtk_toggle_button_get_active (image_button);

  if (private->template != NULL)
    {
      width    = ligma_template_get_width           (private->template);
      height   = ligma_template_get_height          (private->template);
      unit     = ligma_template_get_unit            (private->template);
      xres     = ligma_template_get_resolution_x    (private->template);
      yres     = ligma_template_get_resolution_y    (private->template);
      res_unit = ligma_template_get_resolution_unit (private->template);
    }

  if (private->template != NULL &&
      unit != LIGMA_UNIT_PIXEL)
    {
      if (use_image_ppi)
        {
          width  = ceil (width  * (private->old_xres / xres));
          height = ceil (height * (private->old_yres / yres));

          xres = private->old_xres;
          yres = private->old_yres;
        }

      g_object_set (private->box,
                    "xresolution",     xres,
                    "yresolution",     yres,
                    "resolution-unit", res_unit,
                    NULL);
    }
  else
    {
      g_object_set (private->box,
                    "xresolution",     private->old_xres,
                    "yresolution",     private->old_yres,
                    "resolution-unit", private->old_res_unit,
                    NULL);
    }

  g_object_set (private->box,
                "width",  width,
                "height", height,
                "unit",   unit,
                NULL);
}

static void
reset_template_clicked (GtkWidget    *button,
                        ResizeDialog *private)
{
  ligma_context_set_template (private->context, NULL);
}
