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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include <webp/encode.h>

#include "file-webp.h"
#include "file-webp-dialog.h"

#include "libgimp/stdplugins-intl.h"


static void
save_dialog_toggle_scale (GObject          *config,
                          const GParamSpec *pspec,
                          GtkAdjustment    *adjustment)
{
  gboolean lossless;

  g_object_get (config,
                "lossless", &lossless,
                NULL);

  gimp_scale_entry_set_sensitive (adjustment, ! lossless);
}

static void
show_maxkeyframe_hints (GObject          *config,
                        const GParamSpec *pspec,
                        GtkLabel         *label)
{
  gint kmax;

  g_object_get (config,
                "keyframe-distance", &kmax,
                NULL);

  if (kmax == 0)
    {
      gtk_label_set_text (label, _("(no keyframes)"));
    }
  else if (kmax == 1)
    {
      gtk_label_set_text (label, _("(all frames are keyframes)"));
    }
  else
    {
      gtk_label_set_text (label, "");
    }
}

gboolean
save_dialog (GimpImage     *image,
             GimpProcedure *procedure,
             GObject       *config)
{
  GtkWidget     *dialog;
  GtkWidget     *vbox;
  GtkWidget     *grid;
  GtkWidget     *expander;
  GtkWidget     *frame;
  GtkWidget     *vbox2;
  GtkWidget     *label;
  GtkWidget     *toggle;
  GtkListStore  *store;
  GtkWidget     *combo;
  GtkAdjustment *quality_scale;
  GtkAdjustment *alpha_quality_scale;
  gint32         nlayers;
  gboolean       animation_supported = FALSE;
  gboolean       run;
  gchar         *text;
  gint           row = 0;

  g_free (gimp_image_get_layers (image, &nlayers));

  animation_supported = nlayers > 1;

  dialog = gimp_procedure_dialog_new (procedure,
                                      GIMP_PROCEDURE_CONFIG (config),
                                      _("Export Image as WebP"));

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_box_pack_start (GTK_BOX (vbox), grid, FALSE, FALSE, 0);
  gtk_widget_show (grid);

  /* Create the lossless checkbox */
  toggle = gimp_prop_check_button_new (config, "lossless",
                                       _("_Lossless"));
  gtk_grid_attach (GTK_GRID (grid), toggle, 0, row, 3, 1);
  row++;

  /* Create the slider for image quality */
  quality_scale = gimp_prop_scale_entry_new (config, "quality",
                                             GTK_GRID (grid), 0, row++,
                                             _("Image _quality:"),
                                             1.0, 10.0, 0,
                                             FALSE, 0, 0);

  /* Create the slider for alpha channel quality */
  alpha_quality_scale = gimp_prop_scale_entry_new (config, "alpha-quality",
                                                   GTK_GRID (grid), 0, row++,
                                                   _("Alpha q_uality:"),
                                                   1.0, 10.0, 0,
                                                   FALSE, 0, 0);

  /* Enable and disable the sliders when the lossless option is selected */
  g_signal_connect (config, "notify::lossless",
                    G_CALLBACK (save_dialog_toggle_scale),
                    quality_scale);
  g_signal_connect (config, "notify::lossless",
                    G_CALLBACK (save_dialog_toggle_scale),
                    alpha_quality_scale);

  save_dialog_toggle_scale (config, NULL, quality_scale);
  save_dialog_toggle_scale (config, NULL, alpha_quality_scale);

  /* Create the combobox containing the presets */
  store = gimp_int_store_new ("Default", WEBP_PRESET_DEFAULT,
                              "Picture", WEBP_PRESET_PICTURE,
                              "Photo",   WEBP_PRESET_PHOTO,
                              "Drawing", WEBP_PRESET_DRAWING,
                              "Icon",    WEBP_PRESET_ICON,
                              "Text",    WEBP_PRESET_TEXT,
                              NULL);
  combo = gimp_prop_int_combo_box_new (config, "preset",
                                       GIMP_INT_STORE (store));
  g_object_unref (store);

  label = gimp_grid_attach_aligned (GTK_GRID (grid), 0, row++,
                                    _("Source _type:"), 0.0, 0.5,
                                    combo, 2);
  gimp_help_set_help_data (label,
                           _("WebP encoder \"preset\""),
                           NULL);

  if (animation_supported)
    {
      GtkWidget      *animation_box;
      GtkWidget      *delay;
      GtkWidget      *hbox;
      GtkWidget      *label_kf;
      GtkWidget      *kf_distance;
      GtkWidget      *hbox_kf;
      PangoAttrList  *attrs;
      PangoAttribute *attr;

      vbox2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);
      gtk_box_pack_start (GTK_BOX (vbox), vbox2, FALSE, FALSE, 0);
      gtk_widget_show (vbox2);

      text = g_strdup_printf ("<b>%s</b>", _("_Advanced Options"));
      expander = gtk_expander_new_with_mnemonic (text);
      gtk_expander_set_use_markup (GTK_EXPANDER (expander), TRUE);
      g_free (text);


      /* Create the top-level animation checkbox expander */
      text = g_strdup_printf ("<b>%s</b>", _("As A_nimation"));
      toggle = gimp_prop_check_button_new (config, "animation",
                                           text);
      g_free (text);

      gtk_label_set_use_markup (GTK_LABEL (gtk_bin_get_child (GTK_BIN (toggle))),
                                TRUE);
      gtk_box_pack_start (GTK_BOX (vbox2), toggle, TRUE, TRUE, 0);

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
      toggle = gimp_prop_check_button_new (config, "animation-loop",
                                           _("Loop _forever"));
      gtk_box_pack_start (GTK_BOX (animation_box), toggle,
                          FALSE, FALSE, 0);

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
      kf_distance = gimp_prop_spin_button_new (config, "keyframe-distance",
                                               1.0, 1.0, 0);
      gtk_box_pack_start (GTK_BOX (hbox_kf), kf_distance, FALSE, FALSE, 0);

      /* Add some hinting text for special values of key-frame distance. */
      label_kf = gtk_label_new (NULL);
      gtk_box_pack_start (GTK_BOX (hbox_kf), label_kf, FALSE, FALSE, 0);
      gtk_widget_show (label_kf);

      attrs = pango_attr_list_new ();
      attr  = pango_attr_style_new (PANGO_STYLE_ITALIC);
      pango_attr_list_insert (attrs, attr);
      gtk_label_set_attributes (GTK_LABEL (label_kf), attrs);
      pango_attr_list_unref (attrs);

      g_signal_connect (config, "notify::keyframe-distance",
                        G_CALLBACK (show_maxkeyframe_hints),
                        label_kf);
      show_maxkeyframe_hints (config, NULL, GTK_LABEL (label_kf));

      /* minimize-size checkbox */
      toggle = gimp_prop_check_button_new (config, "minimize-size",
                                           _("_Minimize output size "
                                             "(slower)"));
      gtk_box_pack_start (GTK_BOX (animation_box), toggle,
                          FALSE, FALSE, 0);

      /* Enable and disable the kf-distance box when the 'minimize size'
       * option is selected
       */
      g_object_bind_property (config,  "minimize-size",
                              hbox_kf, "sensitive",
                              G_BINDING_SYNC_CREATE |
                              G_BINDING_INVERT_BOOLEAN);

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
      delay = gimp_prop_spin_button_new (config, "default-delay",
                                         1, 10, 0);
      gtk_box_pack_start (GTK_BOX (hbox), delay, FALSE, FALSE, 0);

      /* label for 'ms' adjustment */
      label = gtk_label_new (_("milliseconds"));
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
      gtk_widget_show (label);

      /* Create the force-delay checkbox */
      toggle = gimp_prop_check_button_new (config, "force-delay",
                                           _("Use _delay entered above for "
                                             "all frames"));
      gtk_box_pack_start (GTK_BOX (animation_box), toggle, FALSE, FALSE, 0);
  }

  /* Save EXIF data */
  toggle = gimp_prop_check_button_new (config, "save-exif",
                                       _("_Save Exif data"));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);

  /* XMP metadata */
  toggle = gimp_prop_check_button_new (config, "save-xmp",
                                       _("Save _XMP data"));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);

  /* Color profile */
  toggle = gimp_prop_check_button_new (config, "save-color-profile",
                                       _("Save color _profile"));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);

  gtk_widget_show (dialog);

  run = gimp_procedure_dialog_run (GIMP_PROCEDURE_DIALOG (dialog));

  gtk_widget_destroy (dialog);

  return run;
}
