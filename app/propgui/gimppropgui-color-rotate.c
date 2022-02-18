/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimppropgui-color-rotate.c
 * Copyright (C) 2002-2014  Michael Natterer <mitch@gimp.org>
 *                          Sven Neumann <sven@gimp.org>
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

#include "core/gimpcontext.h"

#include "widgets/gimppropwidgets.h"

#include "gimppropgui.h"
#include "gimppropgui-color-rotate.h"
#include "gimppropgui-generic.h"

#include "gimp-intl.h"


static void
invert_segment_clicked (GtkWidget *button,
                        GtkWidget *dial)
{
  gdouble  alpha, beta;
  gboolean clockwise;

  g_object_get (dial,
                "alpha",           &alpha,
                "beta",            &beta,
                "clockwise-delta", &clockwise,
                NULL);

  g_object_set (dial,
                "alpha",           beta,
                "beta",            alpha,
                "clockwise-delta", ! clockwise,
                NULL);
}

static void
select_all_clicked (GtkWidget *button,
                    GtkWidget *dial)
{
  gdouble  alpha, beta;
  gboolean clockwise;

  g_object_get (dial,
                "alpha",           &alpha,
                "clockwise-delta", &clockwise,
                NULL);

  beta = alpha - (clockwise ? -1 : 1) * 0.00001;

  if (beta < 0)
    beta += 2 * G_PI;

  if (beta > 2 * G_PI)
    beta -= 2 * G_PI;

  g_object_set (dial,
                "beta", beta,
                NULL);
}

static GtkWidget *
gimp_prop_angle_range_box_new (GObject     *config,
                               const gchar *alpha_property_name,
                               const gchar *beta_property_name,
                               const gchar *clockwise_property_name)
{
  GtkWidget *main_hbox;
  GtkWidget *vbox;
  GtkWidget *scale;
  GtkWidget *hbox;
  GtkWidget *button;
  GtkWidget *invert_button;
  GtkWidget *all_button;
  GtkWidget *dial;

  main_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);
  gtk_box_pack_start (GTK_BOX (main_hbox), vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  scale = gimp_prop_spin_scale_new (config, alpha_property_name,
                                    1.0, 15.0, 2);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (scale), TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);

  scale = gimp_prop_spin_scale_new (config, beta_property_name,
                                    1.0, 15.0, 2);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (scale), TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_box_set_homogeneous (GTK_BOX (hbox), TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  button = gimp_prop_check_button_new (config, clockwise_property_name,
                                       _("Clockwise"));
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);

  invert_button = gtk_button_new_with_label (_("Invert Range"));
  gtk_box_pack_start (GTK_BOX (hbox), invert_button, TRUE, TRUE, 0);
  gtk_widget_show (invert_button);

  all_button = gtk_button_new_with_label (_("Select All"));
  gtk_box_pack_start (GTK_BOX (hbox), all_button, TRUE, TRUE, 0);
  gtk_widget_show (all_button);

  dial = gimp_prop_angle_range_dial_new (config,
                                         alpha_property_name,
                                         beta_property_name,
                                         clockwise_property_name);
  gtk_box_pack_start (GTK_BOX (main_hbox), dial, FALSE, FALSE, 0);

  g_signal_connect (invert_button, "clicked",
                    G_CALLBACK (invert_segment_clicked),
                    dial);

  g_signal_connect (all_button, "clicked",
                    G_CALLBACK (select_all_clicked),
                    dial);

  gtk_widget_show (main_hbox);

  return main_hbox;
}

static GtkWidget *
gimp_prop_polar_box_new (GObject     *config,
                         const gchar *angle_property_name,
                         const gchar *radius_property_name)
{
  GtkWidget *main_hbox;
  GtkWidget *vbox;
  GtkWidget *scale;
  GtkWidget *polar;

  main_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);
  gtk_box_pack_start (GTK_BOX (main_hbox), vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  scale = gimp_prop_spin_scale_new (config, angle_property_name,
                                    1.0, 15.0, 2);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (scale), TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);

  scale = gimp_prop_spin_scale_new (config, radius_property_name,
                                    1.0, 15.0, 2);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);

  polar = gimp_prop_polar_new (config,
                               angle_property_name,
                               radius_property_name);
  gtk_box_pack_start (GTK_BOX (main_hbox), polar, FALSE, FALSE, 0);

  gtk_widget_show (main_hbox);

  return main_hbox;
}

GtkWidget *
_gimp_prop_gui_new_color_rotate (GObject                  *config,
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
  GtkWidget *box;

  g_return_val_if_fail (G_IS_OBJECT (config), NULL);
  g_return_val_if_fail (param_specs != NULL, NULL);
  g_return_val_if_fail (n_param_specs > 0, NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);

  frame = gimp_frame_new (_("Source Range"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  box = gimp_prop_angle_range_box_new (config,
                                       param_specs[1]->name,
                                       param_specs[2]->name,
                                       param_specs[0]->name);
  gtk_container_add (GTK_CONTAINER (frame), box);

  frame = gimp_frame_new (_("Destination Range"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  box = gimp_prop_angle_range_box_new (config,
                                       param_specs[4]->name,
                                       param_specs[5]->name,
                                       param_specs[3]->name);
  gtk_container_add (GTK_CONTAINER (frame), box);

  frame = gimp_frame_new (_("Gray Handling"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  box = _gimp_prop_gui_new_generic (config,
                                    param_specs + 6, 2,
                                    area, context,
                                    create_picker_func,
                                    create_controller_func,
                                    creator);
  gtk_box_pack_start (GTK_BOX (vbox), box, FALSE, FALSE, 0);

  box = gimp_prop_polar_box_new (config,
                                 param_specs[8]->name,
                                 param_specs[9]->name);
  gtk_box_pack_start (GTK_BOX (vbox), box, FALSE, FALSE, 0);

  return main_vbox;
}
