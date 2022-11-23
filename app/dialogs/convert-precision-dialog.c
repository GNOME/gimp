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
#include "libligmawidgets/ligmawidgets.h"

#include "dialogs-types.h"

#include "gegl/ligma-babl.h"

#include "core/ligmacontext.h"
#include "core/ligmaimage.h"
#include "core/ligma-utils.h"

#include "widgets/ligmahelp-ids.h"
#include "widgets/ligmaviewabledialog.h"
#include "widgets/ligmawidgets-utils.h"

#include "convert-precision-dialog.h"

#include "ligma-intl.h"


typedef struct _ConvertDialog ConvertDialog;

struct _ConvertDialog
{
  LigmaImage                    *image;
  LigmaComponentType             component_type;
  LigmaTRCType                   trc;
  GeglDitherMethod              layer_dither_method;
  GeglDitherMethod              text_layer_dither_method;
  GeglDitherMethod              channel_dither_method;
  LigmaConvertPrecisionCallback  callback;
  gpointer                      user_data;
};


/*  local function prototypes  */

static void   convert_precision_dialog_free     (ConvertDialog    *private);
static void   convert_precision_dialog_response (GtkWidget        *widget,
                                                 gint              response_id,
                                                 ConvertDialog    *private);


/*  public functions  */

GtkWidget *
convert_precision_dialog_new (LigmaImage                    *image,
                              LigmaContext                  *context,
                              GtkWidget                    *parent,
                              LigmaComponentType             component_type,
                              GeglDitherMethod              layer_dither_method,
                              GeglDitherMethod              text_layer_dither_method,
                              GeglDitherMethod              channel_dither_method,
                              LigmaConvertPrecisionCallback  callback,
                              gpointer                      user_data)

{
  ConvertDialog *private;
  GtkWidget     *dialog;
  GtkWidget     *main_vbox;
  GtkWidget     *vbox;
  GtkWidget     *frame;
  GtkWidget     *perceptual_radio;
  const gchar   *enum_desc;
  gchar         *blurb;
  const Babl    *old_format;
  const Babl    *new_format;
  gint           old_bits;
  gint           new_bits;
  gboolean       dither;
  LigmaTRCType    trc;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (GTK_IS_WIDGET (parent), NULL);
  g_return_val_if_fail (callback != NULL, NULL);

  /* random formats with the right precision */
  old_format = ligma_image_get_layer_format (image, FALSE);
  new_format = ligma_babl_format (LIGMA_RGB,
                                 ligma_babl_precision (component_type, FALSE),
                                 FALSE,
                                 babl_format_get_space (old_format));

  old_bits = (babl_format_get_bytes_per_pixel (old_format) * 8 /
              babl_format_get_n_components (old_format));
  new_bits = (babl_format_get_bytes_per_pixel (new_format) * 8 /
              babl_format_get_n_components (new_format));

  /*  don't dither if we are converting to a higher bit depth,
   *  or to more than MAX_DITHER_BITS.
   */
  dither = (new_bits <  old_bits &&
            new_bits <= CONVERT_PRECISION_DIALOG_MAX_DITHER_BITS);

  trc = ligma_babl_format_get_trc (old_format);
  trc = ligma_suggest_trc_for_component_type (component_type, trc);

  private = g_slice_new0 (ConvertDialog);

  private->image                    = image;
  private->component_type           = component_type;
  private->trc                      = trc;
  private->layer_dither_method      = layer_dither_method;
  private->text_layer_dither_method = text_layer_dither_method;
  private->channel_dither_method    = channel_dither_method;
  private->callback                 = callback;
  private->user_data                = user_data;

  ligma_enum_get_value (LIGMA_TYPE_COMPONENT_TYPE, component_type,
                       NULL, NULL, &enum_desc, NULL);

  blurb = g_strdup_printf (_("Convert Image to %s"), enum_desc);

  dialog = ligma_viewable_dialog_new (g_list_prepend (NULL, image), context,
                                     _("Encoding Conversion"),
                                     "ligma-image-convert-precision",
                                     LIGMA_ICON_CONVERT_PRECISION,
                                     blurb,
                                     parent,
                                     ligma_standard_help_func,
                                     LIGMA_HELP_IMAGE_CONVERT_PRECISION,

                                     _("_Cancel"),  GTK_RESPONSE_CANCEL,
                                     _("C_onvert"), GTK_RESPONSE_OK,

                                     NULL);

  g_free (blurb);

  ligma_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

  g_object_weak_ref (G_OBJECT (dialog),
                     (GWeakNotify) convert_precision_dialog_free, private);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (convert_precision_dialog_response),
                    private);

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      main_vbox, TRUE, TRUE, 0);
  gtk_widget_show (main_vbox);


  /*  gamma  */

  frame = ligma_frame_new (_("Gamma"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox = ligma_int_radio_group_new (FALSE, NULL,
                                   G_CALLBACK (ligma_radio_button_update),
                                   &private->trc, NULL,
                                   trc,

                                   _("Linear light"),
                                   LIGMA_TRC_LINEAR, NULL,

                                   _("Non-Linear"),
                                   LIGMA_TRC_NON_LINEAR, NULL,

                                   _("Perceptual (sRGB)"),
                                   LIGMA_TRC_PERCEPTUAL, &perceptual_radio,

                                   NULL);

  if (private->trc != LIGMA_TRC_PERCEPTUAL)
    gtk_widget_hide (perceptual_radio);

  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);


  /*  dithering  */

  if (dither)
    {
      GtkWidget    *hbox;
      GtkWidget    *label;
      GtkWidget    *combo;
      GtkSizeGroup *size_group;

      frame = ligma_frame_new (_("Dithering"));
      gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
      gtk_widget_show (frame);

      vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
      gtk_container_add (GTK_CONTAINER (frame), vbox);
      gtk_widget_show (vbox);

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

      combo = ligma_enum_combo_box_new (GEGL_TYPE_DITHER_METHOD);
      gtk_label_set_mnemonic_widget (GTK_LABEL (label), combo);
      gtk_box_pack_start (GTK_BOX (hbox), combo, TRUE, TRUE, 0);
      gtk_widget_show (combo);

      ligma_int_combo_box_connect (LIGMA_INT_COMBO_BOX (combo),
                                  private->layer_dither_method,
                                  G_CALLBACK (ligma_int_combo_box_get_active),
                                  &private->layer_dither_method, NULL);

      /*  text layers  */

      hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
      gtk_widget_show (hbox);

      label = gtk_label_new_with_mnemonic (_("_Text Layers:"));
      gtk_label_set_xalign (GTK_LABEL (label), 0.0);
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
      gtk_size_group_add_widget (size_group, label);
      gtk_widget_show (label);

      combo = ligma_enum_combo_box_new (GEGL_TYPE_DITHER_METHOD);
      gtk_label_set_mnemonic_widget (GTK_LABEL (label), combo);
      gtk_box_pack_start (GTK_BOX (hbox), combo, TRUE, TRUE, 0);
      gtk_widget_show (combo);

      ligma_int_combo_box_connect (LIGMA_INT_COMBO_BOX (combo),
                                  private->text_layer_dither_method,
                                  G_CALLBACK (ligma_int_combo_box_get_active),
                                  &private->text_layer_dither_method, NULL);

      ligma_help_set_help_data (combo,
                               _("Dithering text layers will make them "
                                 "uneditable"),
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

      combo = ligma_enum_combo_box_new (GEGL_TYPE_DITHER_METHOD);
      gtk_label_set_mnemonic_widget (GTK_LABEL (label), combo);
      gtk_box_pack_start (GTK_BOX (hbox), combo, TRUE, TRUE, 0);
      gtk_widget_show (combo);

      ligma_int_combo_box_connect (LIGMA_INT_COMBO_BOX (combo),
                                  private->channel_dither_method,
                                  G_CALLBACK (ligma_int_combo_box_get_active),
                                  &private->channel_dither_method, NULL);

      g_object_unref (size_group);
    }

  return dialog;
}


/*  private functions  */

static void
convert_precision_dialog_free (ConvertDialog *private)
{
  g_slice_free (ConvertDialog, private);
}

static void
convert_precision_dialog_response (GtkWidget     *dialog,
                                   gint           response_id,
                                   ConvertDialog *private)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      LigmaPrecision precision = ligma_babl_precision (private->component_type,
                                                     private->trc);

      private->callback (dialog,
                         private->image,
                         precision,
                         private->layer_dither_method,
                         private->text_layer_dither_method,
                         private->channel_dither_method,
                         private->user_data);
    }
  else
    {
      gtk_widget_destroy (dialog);
    }
}
