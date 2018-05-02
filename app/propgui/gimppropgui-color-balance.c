/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimppropgui-color-balance.c
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

#include "libgimpwidgets/gimpwidgets.h"

#include "propgui-types.h"

#include "operations/gimpcolorbalanceconfig.h"

#include "core/gimpcontext.h"

#include "widgets/gimppropwidgets.h"
#include "widgets/gimpspinscale.h"

#include "gimppropgui.h"
#include "gimppropgui-color-balance.h"

#include "gimp-intl.h"


static void
create_levels_scale (GObject     *config,
                     const gchar *property_name,
                     const gchar *left,
                     const gchar *right,
                     GtkWidget   *grid,
                     gint         col)
{
  GtkWidget *label;
  GtkWidget *scale;

  label = gtk_label_new (left);
  gtk_label_set_xalign (GTK_LABEL (label), 1.0);
  gtk_grid_attach (GTK_GRID (grid), label, 0, col, 1, 1);
  gtk_widget_show (label);

  scale = gimp_prop_spin_scale_new (config, property_name,
                                    NULL, 0.01, 0.1, 0);
  gimp_spin_scale_set_label (GIMP_SPIN_SCALE (scale), NULL);
  gimp_prop_widget_set_factor (scale, 100.0, 0.0, 0.0, 1);
  gtk_widget_set_hexpand (scale, TRUE);
  gtk_grid_attach (GTK_GRID (grid), scale, 1, col, 1, 1);
  gtk_widget_show (scale);

  label = gtk_label_new (right);
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_grid_attach (GTK_GRID (grid), label, 2, col, 1, 1);
  gtk_widget_show (label);
}

GtkWidget *
_gimp_prop_gui_new_color_balance (GObject                  *config,
                                  GParamSpec              **param_specs,
                                  guint                     n_param_specs,
                                  GeglRectangle            *area,
                                  GimpContext              *context,
                                  GimpCreatePickerFunc      create_picker_func,
                                  GimpCreateControllerFunc  create_controller_func,
                                  gpointer                  creator)
{
  GtkWidget *main_vbox;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *grid;
  GtkWidget *button;
  GtkWidget *frame;

  g_return_val_if_fail (G_IS_OBJECT (config), NULL);
  g_return_val_if_fail (param_specs != NULL, NULL);
  g_return_val_if_fail (n_param_specs > 0, NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);

  frame = gimp_prop_enum_radio_frame_new (config, "range",
                                          _("Select Range to Adjust"),
                                          0, 0);
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  frame = gimp_frame_new (_("Adjust Color Levels"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  /*  The grid containing sliders  */
  grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (grid), 4);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 2);
  gtk_box_pack_start (GTK_BOX (vbox), grid, FALSE, FALSE, 0);
  gtk_widget_show (grid);

  create_levels_scale (config, "cyan-red",
                       _("Cyan"), _("Red"),
                       grid, 0);

  create_levels_scale (config, "magenta-green",
                       _("Magenta"), _("Green"),
                       grid, 1);

  create_levels_scale (config, "yellow-blue",
                       _("Yellow"), _("Blue"),
                       grid, 2);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  button = gtk_button_new_with_mnemonic (_("R_eset Range"));
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect_swapped (button, "clicked",
                            G_CALLBACK (gimp_color_balance_config_reset_range),
                            config);

  button = gimp_prop_check_button_new (config,
                                       "preserve-luminosity",
                                       _("Preserve _luminosity"));
  gtk_box_pack_end (GTK_BOX (main_vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  return main_vbox;
}
