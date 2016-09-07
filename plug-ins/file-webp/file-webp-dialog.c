/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * file-webp - WebP file format plug-in for the GIMP
 * Copyright (C) 2015  Nathan Osman
 * Copyright (C) 2016  Ben Touchette
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
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

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include <webp/encode.h>

#include "file-webp.h"
#include "file-webp-dialog.h"

#include "libgimp/stdplugins-intl.h"


static void           save_dialog_toggle_scale   (GtkWidget  *widget,
                                                  gpointer    data);

static void           save_dialog_toggle_minsize (GtkWidget  *widget,
                                                  gpointer    data);


static void
save_dialog_toggle_scale (GtkWidget *widget,
                          gpointer   data)
{
  gimp_scale_entry_set_sensitive (data,
                                  ! gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)));
}

static void
save_dialog_toggle_minsize (GtkWidget *widget,
                            gpointer   data)
{
  gtk_widget_set_sensitive (GTK_WIDGET (data),
                            ! gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)));
}

gboolean
save_dialog (WebPSaveParams *params,
             gint32          image_ID,
             gint32          n_layers)
{
  GtkWidget     *dialog;
  GtkWidget     *vbox;
  GtkWidget     *table;
  GtkWidget     *expander;
  GtkWidget     *frame;
  GtkWidget     *vbox2;
  GtkWidget     *label;
  GtkWidget     *toggle;
  GtkWidget     *toggle_minsize;
  GtkWidget     *combo;
  GtkAdjustment *quality_scale;
  GtkAdjustment *alpha_quality_scale;
  gboolean       animation_supported = FALSE;
  gboolean       run;
  gchar         *text;
  gint           row = 0;

  animation_supported = n_layers > 1;

  /* Create the dialog */
  dialog = gimp_export_dialog_new (_("WebP"), PLUG_IN_BINARY, SAVE_PROC);

  /* Create the vbox */
  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_box_pack_start (GTK_BOX (gimp_export_dialog_get_content_area (dialog)),
                      vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  /* Create the table */
  table = gtk_table_new (4, 3, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  /* Create the lossless checkbox */
  toggle = gtk_check_button_new_with_label (_("Lossless"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
                                params->lossless);
  gtk_table_attach (GTK_TABLE (table), toggle,
                    0, 3, row, row + 1,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (toggle);
  row++;

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &params->lossless);

  /* Create the slider for image quality */
  quality_scale = gimp_scale_entry_new (GTK_TABLE (table),
                                        0, row++,
                                        _("Image quality:"),
                                        125,
                                        0,
                                        params->quality,
                                        0.0, 100.0,
                                        1.0, 10.0,
                                        0, TRUE,
                                        0.0, 0.0,
                                        _("Image quality"),
                                        NULL);
  gimp_scale_entry_set_sensitive (quality_scale, ! params->lossless);

  g_signal_connect (quality_scale, "value-changed",
                    G_CALLBACK (gimp_float_adjustment_update),
                    &params->quality);

  /* Create the slider for alpha channel quality */
  alpha_quality_scale = gimp_scale_entry_new (GTK_TABLE (table),
                                              0, row++,
                                              _("Alpha quality:"),
                                              125,
                                              0,
                                              params->alpha_quality,
                                              0.0, 100.0,
                                              1.0, 10.0,
                                              0, TRUE,
                                              0.0, 0.0,
                                              _("Alpha channel quality"),
                                              NULL);
  gimp_scale_entry_set_sensitive (alpha_quality_scale, ! params->lossless);

  g_signal_connect (alpha_quality_scale, "value-changed",
                    G_CALLBACK (gimp_float_adjustment_update),
                    &params->alpha_quality);

  /* Enable and disable the sliders when the lossless option is selected */
  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (save_dialog_toggle_scale),
                    quality_scale);
  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (save_dialog_toggle_scale),
                    alpha_quality_scale);

  /* Create the combobox containing the presets */
  combo = gimp_int_combo_box_new ("Default", WEBP_PRESET_DEFAULT,
                                  "Picture", WEBP_PRESET_PICTURE,
                                  "Photo",   WEBP_PRESET_PHOTO,
                                  "Drawing", WEBP_PRESET_DRAWING,
                                  "Icon",    WEBP_PRESET_ICON,
                                  "Text",    WEBP_PRESET_TEXT,
                                  NULL);
  label = gimp_table_attach_aligned (GTK_TABLE (table), 0, row++,
                                     _("Source type:"), 0.0, 0.5,
                                     combo, 2, FALSE);
  gimp_help_set_help_data (label,
                           _("WebP encoder \"preset\""),
                           NULL);

  gimp_int_combo_box_connect (GIMP_INT_COMBO_BOX (combo),
                              params->preset,
                              G_CALLBACK (gimp_int_combo_box_get_active),
                              &params->preset);

  if (animation_supported)
    {
      GtkWidget     *animation_box;
      GtkAdjustment *adj;
      GtkWidget     *delay;
      GtkWidget     *hbox;
      GtkWidget     *label_kf;
      GtkAdjustment *adj_kf;
      GtkWidget     *kf_distance;
      GtkWidget     *hbox_kf;

      vbox2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);
      gtk_box_pack_start (GTK_BOX (vbox), vbox2, FALSE, FALSE, 0);
      gtk_widget_show (vbox2);

      text = g_strdup_printf ("<b>%s</b>", _("_Advanced Options"));
      expander = gtk_expander_new_with_mnemonic (text);
      gtk_expander_set_use_markup (GTK_EXPANDER (expander), TRUE);
      g_free (text);


      /* Create the top-level animation checkbox expander */
      text = g_strdup_printf ("<b>%s</b>", _("As A_nimation"));
      toggle = gtk_check_button_new_with_mnemonic (text);
      g_free (text);

      gtk_label_set_use_markup (GTK_LABEL (gtk_bin_get_child (GTK_BIN (toggle))),
                                TRUE);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
                                    params->animation);
      gtk_box_pack_start (GTK_BOX (vbox2), toggle, TRUE, TRUE, 0);
      gtk_widget_show (toggle);

      g_signal_connect (toggle, "toggled",
                        G_CALLBACK (gimp_toggle_button_update),
                        &params->animation);

      frame = gimp_frame_new ("<expander>");
      gtk_box_pack_start (GTK_BOX (vbox2), frame, TRUE, TRUE, 0);
      gtk_widget_show (frame);

      g_object_bind_property (toggle, "active",
                              frame,  "visible",
                              G_BINDING_SYNC_CREATE);

      /* animation options box */
      animation_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
      gtk_container_add (GTK_CONTAINER (frame), animation_box);
      gtk_widget_show (animation_box);

      /* loop animation checkbox */
      toggle = gtk_check_button_new_with_label (_("Loop forever"));
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), params->loop);
      gtk_box_pack_start (GTK_BOX (animation_box), toggle,
                          FALSE, FALSE, 0);
      gtk_widget_show (toggle);

      g_signal_connect (toggle, "toggled",
                        G_CALLBACK (gimp_toggle_button_update),
                        &params->loop);

      /* create a hbox for 'max key-frame distance */
      hbox_kf = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
      gtk_box_pack_start (GTK_BOX (animation_box), hbox_kf, FALSE, FALSE, 0);
      gtk_widget_set_sensitive (hbox_kf, TRUE);
      gtk_widget_show (hbox_kf);

      /* label for 'max key-frame distance' adjustment */
      label_kf = gtk_label_new (_("Max distance between key-frames:"));
      gtk_label_set_xalign (GTK_LABEL (label_kf), 0.2);
      gtk_box_pack_start (GTK_BOX (hbox_kf), label_kf, FALSE, FALSE, 0);
      gtk_widget_show (label_kf);

      /* key-frame distance entry */
      adj_kf = (GtkAdjustment *) gtk_adjustment_new (params->kf_distance,
                                                     1, 10000, 1, 10, 0);
      kf_distance = gtk_spin_button_new (adj_kf, 1, 0);
      gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (kf_distance), TRUE);
      gtk_box_pack_start (GTK_BOX (hbox_kf), kf_distance, FALSE, FALSE, 0);
      gtk_widget_show (kf_distance);

      g_signal_connect (adj_kf, "value-changed",
                        G_CALLBACK (gimp_int_adjustment_update),
                        &params->kf_distance);

      /* minimize-size checkbox */
      toggle_minsize = gtk_check_button_new_with_label (_("Minimize output size (slower)"));

      gtk_box_pack_start (GTK_BOX (animation_box), toggle_minsize,
                          FALSE, FALSE, 0);
      gtk_widget_show (toggle_minsize);

      g_signal_connect (toggle_minsize, "toggled",
                        G_CALLBACK (gimp_toggle_button_update),
                        &params->minimize_size);


      /* Enable and disable the kf-distance box when the 'minimize size' option is selected */
      g_signal_connect (toggle_minsize, "toggled",
                        G_CALLBACK (save_dialog_toggle_minsize),
                        hbox_kf);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle_minsize), params->minimize_size);

      /* create a hbox for delay */
      hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
      gtk_box_pack_start (GTK_BOX (animation_box), hbox, FALSE, FALSE, 0);
      gtk_widget_show (hbox);

      /* label for 'delay' adjustment */
      label = gtk_label_new (_("Delay between frames where unspecified:"));
      gtk_label_set_xalign (GTK_LABEL (label), 0.0);
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
      gtk_widget_show (label);

      /* default delay */
      adj = gtk_adjustment_new (params->delay, 1, 10000, 1, 10, 0);
      delay = gtk_spin_button_new (adj, 1, 0);
      gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (delay), TRUE);
      gtk_box_pack_start (GTK_BOX (hbox), delay, FALSE, FALSE, 0);
      gtk_widget_show (delay);

      g_signal_connect (adj, "value-changed",
                        G_CALLBACK (gimp_int_adjustment_update),
                        &params->delay);

      /* label for 'ms' adjustment */
      label = gtk_label_new (_("milliseconds"));
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
      gtk_widget_show (label);

      /* Create the force-delay checkbox */
      toggle = gtk_check_button_new_with_label (_("Use delay entered above for all frames"));
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
                                    params->force_delay);
      gtk_box_pack_start (GTK_BOX (animation_box), toggle, FALSE, FALSE, 0);
      gtk_widget_show (toggle);

      g_signal_connect (toggle, "toggled",
                        G_CALLBACK (gimp_toggle_button_update),
                        &params->force_delay);
  }

  /* Advanced options */
  text = g_strdup_printf ("<b>%s</b>", _("_Advanced Options"));
  expander = gtk_expander_new_with_mnemonic (text);
  gtk_expander_set_use_markup (GTK_EXPANDER (expander), TRUE);
  g_free (text);

  gtk_box_pack_start (GTK_BOX (vbox), expander, TRUE, TRUE, 0);
  gtk_widget_show (expander);

  frame = gimp_frame_new ("<expander>");
  gtk_container_add (GTK_CONTAINER (expander), frame);
  gtk_widget_show (frame);

  vbox2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox2);
  gtk_widget_show (vbox2);

  /* Save EXIF data */
  toggle = gtk_check_button_new_with_mnemonic (_("Save _Exif data"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), params->exif);
  gtk_box_pack_start (GTK_BOX (vbox2), toggle, FALSE, FALSE, 0);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &params->exif);

  /* XMP metadata */
  toggle = gtk_check_button_new_with_mnemonic (_("Save _XMP data"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), params->xmp);
  gtk_box_pack_start (GTK_BOX (vbox2), toggle, FALSE, FALSE, 0);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &params->xmp);


  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  return run;
}
