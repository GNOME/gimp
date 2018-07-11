/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimppropgui-hue-saturation.c
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

#include "libgimpwidgets/gimpwidgets.h"

#include "propgui-types.h"

#include "operations/gimphuesaturationconfig.h"
#include "operations/gimpoperationhuesaturation.h"

#include "core/gimpcontext.h"

#include "widgets/gimppropwidgets.h"

#include "gimppropgui.h"
#include "gimppropgui-hue-saturation.h"

#include "gimp-intl.h"


#define COLOR_WIDTH  40
#define COLOR_HEIGHT 20


static void
hue_saturation_config_notify (GObject          *object,
                              const GParamSpec *pspec,
                              GtkWidget        *color_area)
{
  GimpHueSaturationConfig *config = GIMP_HUE_SATURATION_CONFIG (object);
  GimpHueRange             range;
  GimpRGB                  color;

  static const GimpRGB default_colors[7] =
  {
    {   0,   0,   0, },
    { 1.0,   0,   0, },
    { 1.0, 1.0,   0, },
    {   0, 1.0,   0, },
    {   0, 1.0, 1.0, },
    {   0,   0, 1.0, },
    { 1.0,   0, 1.0, }
  };

  range = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (color_area),
                                              "hue-range"));
  color = default_colors[range];

  gimp_operation_hue_saturation_map (config, &color, range, &color);

  gimp_color_area_set_color (GIMP_COLOR_AREA (color_area), &color);
}

static void
hue_saturation_range_callback (GtkWidget *widget,
                               GObject   *config)
{
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
    {
      GimpHueRange range;

      gimp_radio_button_update (widget, &range);
      g_object_set (config,
                    "range", range,
                    NULL);
    }
}

static void
hue_saturation_range_notify (GObject          *object,
                             const GParamSpec *pspec,
                             GtkWidget        *range_radio)
{
  GimpHueSaturationConfig *config = GIMP_HUE_SATURATION_CONFIG (object);

  gimp_int_radio_group_set_active (GTK_RADIO_BUTTON (range_radio),
                                   config->range);
}

GtkWidget *
_gimp_prop_gui_new_hue_saturation (GObject                  *config,
                                   GParamSpec              **param_specs,
                                   guint                     n_param_specs,
                                   GeglRectangle            *area,
                                   GimpContext              *context,
                                   GimpCreatePickerFunc      create_picker_func,
                                   GimpCreateControllerFunc  create_controller_func,
                                   gpointer                  creator)
{
  GtkWidget *main_vbox;
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *abox;
  GtkWidget *table;
  GtkWidget *scale;
  GtkWidget *button;
  GtkWidget *hbox;
  GtkWidget *range_radio;
  GSList    *group = NULL;
  gint       i;

  const struct
  {
    const gchar *label;
    const gchar *tooltip;
    gint         label_col;
    gint         label_row;
    gint         frame_col;
    gint         frame_row;
  }
  hue_range_table[] =
  {
    { N_("M_aster"), N_("Adjust all colors"), 2, 3, 0, 0 },
    { N_("_R"),      N_("Red"),               2, 1, 2, 0 },
    { N_("_Y"),      N_("Yellow"),            1, 2, 0, 2 },
    { N_("_G"),      N_("Green"),             1, 4, 0, 4 },
    { N_("_C"),      N_("Cyan"),              2, 5, 2, 6 },
    { N_("_B"),      N_("Blue"),              3, 4, 4, 4 },
    { N_("_M"),      N_("Magenta"),           3, 2, 4, 2 }
  };

  g_return_val_if_fail (G_IS_OBJECT (config), NULL);
  g_return_val_if_fail (param_specs != NULL, NULL);
  g_return_val_if_fail (n_param_specs > 0, NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);

  frame = gimp_frame_new (_("Select Primary Color to Adjust"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  abox = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_box_pack_start (GTK_BOX (vbox), abox, TRUE, TRUE, 0);
  gtk_widget_show (abox);

  /*  The table containing hue ranges  */
  table = gtk_table_new (7, 5, FALSE);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 4);
  gtk_table_set_col_spacing (GTK_TABLE (table), 3, 4);
  gtk_table_set_row_spacing (GTK_TABLE (table), 0, 2);
  gtk_table_set_row_spacing (GTK_TABLE (table), 5, 2);
  gtk_container_add (GTK_CONTAINER (abox), table);

  /*  the radio buttons for hue ranges  */
  for (i = 0; i < G_N_ELEMENTS (hue_range_table); i++)
    {
      button = gtk_radio_button_new_with_mnemonic (group,
                                                   gettext (hue_range_table[i].label));
      group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));
      g_object_set_data (G_OBJECT (button), "gimp-item-data",
                         GINT_TO_POINTER (i));

      gimp_help_set_help_data (button,
                               gettext (hue_range_table[i].tooltip),
                               NULL);

      if (i == 0)
        {
          gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (button), FALSE);

          range_radio = button;
        }

      gtk_table_attach (GTK_TABLE (table), button,
                        hue_range_table[i].label_col,
                        hue_range_table[i].label_col + 1,
                        hue_range_table[i].label_row,
                        hue_range_table[i].label_row + 1,
                        GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);

      if (i > 0)
        {
          GtkWidget *color_area;
          GimpRGB    color = { 0, };

          frame = gtk_frame_new (NULL);
          gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
          gtk_table_attach (GTK_TABLE (table), frame,
                            hue_range_table[i].frame_col,
                            hue_range_table[i].frame_col + 1,
                            hue_range_table[i].frame_row,
                            hue_range_table[i].frame_row + 1,
                            GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
          gtk_widget_show (frame);

          color_area = gimp_color_area_new (&color, GIMP_COLOR_AREA_FLAT, 0);
          gtk_widget_set_size_request (color_area, COLOR_WIDTH, COLOR_HEIGHT);
          gtk_container_add (GTK_CONTAINER (frame), color_area);
          gtk_widget_show (color_area);

          g_object_set_data (G_OBJECT (color_area), "hue-range",
                             GINT_TO_POINTER (i));
          g_signal_connect_object (config, "notify",
                                   G_CALLBACK (hue_saturation_config_notify),
                                   color_area, 0);
          hue_saturation_config_notify (config, NULL, color_area);
        }

      g_signal_connect (button, "toggled",
                        G_CALLBACK (hue_saturation_range_callback),
                        config);

      gtk_widget_show (button);
    }

  gtk_widget_show (table);

  /* Create the 'Overlap' option slider */
  scale = gimp_prop_spin_scale_new (config, "overlap",
                                    _("_Overlap"), 0.01, 0.1, 0);
  gimp_prop_widget_set_factor (scale, 100.0, 0.0, 0.0, 1);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  frame = gimp_frame_new (_("Adjust Selected Color"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  /*  Create the hue scale widget  */
  scale = gimp_prop_spin_scale_new (config, "hue",
                                    _("_Hue"), 1.0 / 180.0, 15.0 / 180.0, 0);
  gimp_prop_widget_set_factor (scale, 180.0, 0.0, 0.0, 1);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  /*  Create the lightness scale widget  */
  scale = gimp_prop_spin_scale_new (config, "lightness",
                                    _("_Lightness"), 0.01, 0.1, 0);
  gimp_prop_widget_set_factor (scale, 100.0, 0.0, 0.0, 1);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  /*  Create the saturation scale widget  */
  scale = gimp_prop_spin_scale_new (config, "saturation",
                                    _("_Saturation"), 0.01, 0.1, 0);
  gimp_prop_widget_set_factor (scale, 100.0, 0.0, 0.0, 1);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  button = gtk_button_new_with_mnemonic (_("R_eset Color"));
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect_swapped (button, "clicked",
                            G_CALLBACK (gimp_hue_saturation_config_reset_range),
                            config);

  g_signal_connect_object (config, "notify::range",
                           G_CALLBACK (hue_saturation_range_notify),
                           range_radio, 0);
  hue_saturation_range_notify (config, NULL, range_radio);

  return main_vbox;
}
