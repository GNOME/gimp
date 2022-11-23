/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * ligmapropgui-hue-saturation.c
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

#include "libligmawidgets/ligmawidgets.h"

#include "propgui-types.h"

#include "operations/ligmahuesaturationconfig.h"
#include "operations/ligmaoperationhuesaturation.h"

#include "core/ligmacontext.h"

#include "widgets/ligmapropwidgets.h"

#include "ligmapropgui.h"
#include "ligmapropgui-hue-saturation.h"

#include "ligma-intl.h"


#define COLOR_WIDTH  40
#define COLOR_HEIGHT 20


static void
hue_saturation_config_notify (GObject          *object,
                              const GParamSpec *pspec,
                              GtkWidget        *color_area)
{
  LigmaHueSaturationConfig *config = LIGMA_HUE_SATURATION_CONFIG (object);
  LigmaHueRange             range;
  LigmaRGB                  color;

  static const LigmaRGB default_colors[7] =
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

  ligma_operation_hue_saturation_map (config, &color, range, &color);

  ligma_color_area_set_color (LIGMA_COLOR_AREA (color_area), &color);
}

static void
hue_saturation_range_callback (GtkWidget *widget,
                               GObject   *config)
{
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
    {
      LigmaHueRange range;

      ligma_radio_button_update (widget, &range);
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
  LigmaHueSaturationConfig *config = LIGMA_HUE_SATURATION_CONFIG (object);

  ligma_int_radio_group_set_active (GTK_RADIO_BUTTON (range_radio),
                                   config->range);
}

GtkWidget *
_ligma_prop_gui_new_hue_saturation (GObject                  *config,
                                   GParamSpec              **param_specs,
                                   guint                     n_param_specs,
                                   GeglRectangle            *area,
                                   LigmaContext              *context,
                                   LigmaCreatePickerFunc      create_picker_func,
                                   LigmaCreateControllerFunc  create_controller_func,
                                   gpointer                  creator)
{
  GtkWidget *main_vbox;
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *grid;
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
  hue_range_grid[] =
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
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), NULL);

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);

  frame = ligma_frame_new (_("Select Primary Color to Adjust"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  /*  The grid containing hue ranges  */
  grid = gtk_grid_new ();
  gtk_widget_set_halign (grid, GTK_ALIGN_CENTER);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 4);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 2);
  gtk_box_pack_start (GTK_BOX (vbox), grid, FALSE, FALSE, 0);

  /*  the radio buttons for hue ranges  */
  for (i = 0; i < G_N_ELEMENTS (hue_range_grid); i++)
    {
      button = gtk_radio_button_new_with_mnemonic (group,
                                                   gettext (hue_range_grid[i].label));
      group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));
      g_object_set_data (G_OBJECT (button), "ligma-item-data",
                         GINT_TO_POINTER (i));

      ligma_help_set_help_data (button,
                               gettext (hue_range_grid[i].tooltip),
                               NULL);

      if (i == 0)
        {
          gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (button), FALSE);

          range_radio = button;
        }

      gtk_grid_attach (GTK_GRID (grid), button,
                       hue_range_grid[i].label_col,
                       hue_range_grid[i].label_row,
                       1, 1);

      if (i > 0)
        {
          GtkWidget *color_area;
          LigmaRGB    color = { 0, };

          frame = gtk_frame_new (NULL);
          gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
          gtk_grid_attach (GTK_GRID (grid), frame,
                           hue_range_grid[i].frame_col,
                           hue_range_grid[i].frame_row,
                           1, 1);
          gtk_widget_show (frame);

          color_area = ligma_color_area_new (&color, LIGMA_COLOR_AREA_FLAT, 0);
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

  gtk_widget_show (grid);

  /* Create the 'Overlap' option slider */
  scale = ligma_prop_spin_scale_new (config, "overlap", 0.01, 0.1, 0);
  ligma_prop_widget_set_factor (scale, 100.0, 1.0, 10.0, 1);
  ligma_spin_scale_set_label (LIGMA_SPIN_SCALE (scale), _("_Overlap"));
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);

  frame = ligma_frame_new (_("Adjust Selected Color"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  /*  Create the hue scale widget  */
  scale = ligma_prop_spin_scale_new (config, "hue",
                                    1.0 / 180.0, 15.0 / 180.0, 0);
  ligma_prop_widget_set_factor (scale, 180.0, 1.0, 15.0, 1);
  ligma_spin_scale_set_label (LIGMA_SPIN_SCALE (scale), _("_Hue"));
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);

  /*  Create the lightness scale widget  */
  scale = ligma_prop_spin_scale_new (config, "lightness", 0.01, 0.1, 0);
  ligma_prop_widget_set_factor (scale, 100.0, 1.0, 10.0, 1);
  ligma_spin_scale_set_label (LIGMA_SPIN_SCALE (scale), _("_Lightness"));
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);

  /*  Create the saturation scale widget  */
  scale = ligma_prop_spin_scale_new (config, "saturation", 0.01, 0.1, 0);
  ligma_prop_widget_set_factor (scale, 100.0, 1.0, 10.0, 1);
  ligma_spin_scale_set_label (LIGMA_SPIN_SCALE (scale), _("_Saturation"));
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  button = gtk_button_new_with_mnemonic (_("R_eset Color"));
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect_swapped (button, "clicked",
                            G_CALLBACK (ligma_hue_saturation_config_reset_range),
                            config);

  g_signal_connect_object (config, "notify::range",
                           G_CALLBACK (hue_saturation_range_notify),
                           range_radio, 0);
  hue_saturation_range_notify (config, NULL, range_radio);

  return main_vbox;
}
