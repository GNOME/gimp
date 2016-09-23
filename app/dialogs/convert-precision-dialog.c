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

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "dialogs-types.h"

#include "gegl/gimp-babl.h"
#include "gegl/gimp-gegl-utils.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimpimage-convert-precision.h"
#include "core/gimplist.h"
#include "core/gimpprogress.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpviewablebox.h"
#include "widgets/gimpviewabledialog.h"
#include "widgets/gimpwidgets-utils.h"

#include "convert-precision-dialog.h"

#include "gimp-intl.h"


typedef struct
{
  GtkWidget         *dialog;

  GimpImage         *image;
  GimpProgress      *progress;

  GimpComponentType  component_type;
  gboolean           linear;
  gint               bits;
  gint               layer_dither_type;
  gint               text_layer_dither_type;
  gint               mask_dither_type;
} ConvertDialog;


static void   convert_precision_dialog_response (GtkWidget        *widget,
                                                 gint              response_id,
                                                 ConvertDialog    *dialog);
static void   convert_precision_dialog_free     (ConvertDialog    *dialog);


/*  defaults  */

static gint   saved_layer_dither_type      = 0;
static gint   saved_text_layer_dither_type = 0;
static gint   saved_mask_dither_type       = 0;


/*  public functions  */

GtkWidget *
convert_precision_dialog_new (GimpImage         *image,
                              GimpContext       *context,
                              GtkWidget         *parent,
                              GimpComponentType  component_type,
                              GimpProgress      *progress)
{
  ConvertDialog *dialog;
  GtkWidget     *button;
  GtkWidget     *main_vbox;
  GtkWidget     *vbox;
  GtkWidget     *hbox;
  GtkWidget     *label;
  GtkWidget     *frame;
  GtkWidget     *combo;
  GtkSizeGroup  *size_group;
  const gchar   *enum_desc;
  gchar         *blurb;
  GType          dither_type;
  const Babl    *format;
  gint           bits;
  gboolean       linear;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (GTK_IS_WIDGET (parent), NULL);
  g_return_val_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress), NULL);

  dialog = g_slice_new0 (ConvertDialog);

  /* a random format with precision */
  format = gimp_babl_format (GIMP_RGB, gimp_babl_precision (component_type,
                                                            FALSE), FALSE);
  bits   = (babl_format_get_bytes_per_pixel (format) * 8 /
            babl_format_get_n_components (format));

  linear = gimp_babl_format_get_linear (gimp_image_get_layer_format (image,
                                                                     FALSE));

  dialog->image          = image;
  dialog->progress       = progress;
  dialog->component_type = component_type;
  dialog->linear         = linear;
  dialog->bits           = bits;

  /* gegl:color-reduction only does 16 bits */
  if (bits <= 16)
    {
      dialog->layer_dither_type      = saved_layer_dither_type;
      dialog->text_layer_dither_type = saved_text_layer_dither_type;
      dialog->mask_dither_type       = saved_mask_dither_type;
    }

  gimp_enum_get_value (GIMP_TYPE_COMPONENT_TYPE, component_type,
                       NULL, NULL, &enum_desc, NULL);

  blurb = g_strdup_printf (_("Convert Image to %s"), enum_desc);

  dialog->dialog =
    gimp_viewable_dialog_new (GIMP_VIEWABLE (image), context,
                              _("Precision Conversion"),
                              "gimp-image-convert-precision",
                              GIMP_STOCK_CONVERT_PRECISION,
                              blurb,
                              parent,
                              gimp_standard_help_func,
                              GIMP_HELP_IMAGE_CONVERT_PRECISION,

                              GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,

                              NULL);

  g_free (blurb);

  button = gtk_dialog_add_button (GTK_DIALOG (dialog->dialog),
                                  _("C_onvert"), GTK_RESPONSE_OK);
  gtk_button_set_image (GTK_BUTTON (button),
                        gtk_image_new_from_icon_name (GIMP_STOCK_CONVERT_PRECISION,
                                                      GTK_ICON_SIZE_BUTTON));

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog->dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gtk_window_set_resizable (GTK_WINDOW (dialog->dialog), FALSE);

  g_object_weak_ref (G_OBJECT (dialog->dialog),
                     (GWeakNotify) convert_precision_dialog_free, dialog);

  g_signal_connect (dialog->dialog, "response",
                    G_CALLBACK (convert_precision_dialog_response),
                    dialog);

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog->dialog))),
                      main_vbox, TRUE, TRUE, 0);
  gtk_widget_show (main_vbox);


  /*  dithering  */

  dither_type = gimp_gegl_get_op_enum_type ("gegl:color-reduction",
                                            "dither-strategy");

  frame = gimp_frame_new (_("Dithering"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  /* gegl:color-reduction only does 16 bits */
  gtk_widget_set_sensitive (vbox, bits <= 16);

  size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  /*  layers  */

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new_with_mnemonic (_("_Layers:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_size_group_add_widget (size_group, label);
  gtk_widget_show (label);

  combo = gimp_enum_combo_box_new (dither_type);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), combo);
  gtk_box_pack_start (GTK_BOX (hbox), combo, TRUE, TRUE, 0);
  gtk_widget_show (combo);

  gimp_int_combo_box_connect (GIMP_INT_COMBO_BOX (combo),
                              dialog->layer_dither_type,
                              G_CALLBACK (gimp_int_combo_box_get_active),
                              &dialog->layer_dither_type);

  /*  text layers  */

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new_with_mnemonic (_("_Text Layers:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_size_group_add_widget (size_group, label);
  gtk_widget_show (label);

  combo = gimp_enum_combo_box_new (dither_type);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), combo);
  gtk_box_pack_start (GTK_BOX (hbox), combo, TRUE, TRUE, 0);
  gtk_widget_show (combo);

  gimp_int_combo_box_connect (GIMP_INT_COMBO_BOX (combo),
                              dialog->text_layer_dither_type,
                              G_CALLBACK (gimp_int_combo_box_get_active),
                              &dialog->text_layer_dither_type);

  gimp_help_set_help_data (combo,
                           _("Dithering text layers will make them uneditable"),
                           NULL);

  /*  channels  */

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new_with_mnemonic (_("_Channels and Masks:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_size_group_add_widget (size_group, label);
  gtk_widget_show (label);

  combo = gimp_enum_combo_box_new (dither_type);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), combo);
  gtk_box_pack_start (GTK_BOX (hbox), combo, TRUE, TRUE, 0);
  gtk_widget_show (combo);

  gimp_int_combo_box_connect (GIMP_INT_COMBO_BOX (combo),
                              dialog->mask_dither_type,
                              G_CALLBACK (gimp_int_combo_box_get_active),
                              &dialog->mask_dither_type);

  g_object_unref (size_group);

  /*  gamma  */

  frame = gimp_frame_new (_("Gamma"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  hbox = gimp_int_radio_group_new (FALSE, NULL,
                                   G_CALLBACK (gimp_radio_button_update),
                                   &dialog->linear,
                                   linear,

                                   _("Perceptual gamma (sRGB)"), FALSE, NULL,
                                   _("Linear light"),            TRUE,  NULL,

                                   NULL);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  return dialog->dialog;
}


/*  private functions  */

static void
convert_precision_dialog_response (GtkWidget     *widget,
                                   gint           response_id,
                                   ConvertDialog *dialog)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      GimpProgress  *progress;
      GimpPrecision  precision;
      const gchar   *enum_desc;

      precision = gimp_babl_precision (dialog->component_type,
                                       dialog->linear);

      gimp_enum_get_value (GIMP_TYPE_PRECISION, precision,
                           NULL, NULL, &enum_desc, NULL);

      progress = gimp_progress_start (dialog->progress, FALSE,
                                      _("Converting image to %s"),
                                      enum_desc);

      gimp_image_convert_precision (dialog->image,
                                    precision,
                                    dialog->layer_dither_type,
                                    dialog->text_layer_dither_type,
                                    dialog->mask_dither_type,
                                    progress);

      if (progress)
        gimp_progress_end (progress);

      gimp_image_flush (dialog->image);

       /* gegl:color-reduction only does 16 bits */
      if (dialog->bits <= 16)
        {
          /* Save defaults for next time */
          saved_layer_dither_type      = dialog->layer_dither_type;
          saved_text_layer_dither_type = dialog->text_layer_dither_type;
          saved_mask_dither_type       = dialog->mask_dither_type;
        }
    }

  gtk_widget_destroy (dialog->dialog);
}

static void
convert_precision_dialog_free (ConvertDialog *dialog)
{
  g_slice_free (ConvertDialog, dialog);
}
