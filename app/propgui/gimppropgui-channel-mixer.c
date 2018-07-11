/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimppropgui-channel-mixer.c
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

#include "gimppropgui.h"
#include "gimppropgui-channel-mixer.h"

#include "gimp-intl.h"


GtkWidget *
_gimp_prop_gui_new_channel_mixer (GObject                  *config,
                                  GParamSpec              **param_specs,
                                  guint                     n_param_specs,
                                  GeglRectangle            *area,
                                  GimpContext              *context,
                                  GimpCreatePickerFunc      create_picker_func,
                                  GimpCreateControllerFunc  create_controller_func,
                                  gpointer                  creator)
{
  GtkWidget   *main_vbox;
  GtkWidget   *frame;
  GtkWidget   *vbox;
  GtkWidget   *checkbox;
  GtkWidget   *scale;
  const gchar *label;

  g_return_val_if_fail (G_IS_OBJECT (config), NULL);
  g_return_val_if_fail (param_specs != NULL, NULL);
  g_return_val_if_fail (n_param_specs > 0, NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);

  frame = gimp_frame_new (_("Red channel"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  scale = gimp_prop_widget_new (config, "rr-gain",
                                area, context, NULL, NULL, NULL, &label);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  scale = gimp_prop_widget_new (config, "rg-gain",
                                area, context, NULL, NULL, NULL, &label);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  scale = gimp_prop_widget_new (config, "rb-gain",
                                area, context, NULL, NULL, NULL, &label);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);


  frame = gimp_frame_new (_("Green channel"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  scale = gimp_prop_widget_new (config, "gr-gain",
                                area, context, NULL, NULL, NULL, &label);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  scale = gimp_prop_widget_new (config, "gg-gain",
                                area, context, NULL, NULL, NULL, &label);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  scale = gimp_prop_widget_new (config, "gb-gain",
                                area, context, NULL, NULL, NULL, &label);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);


  frame = gimp_frame_new (_("Blue channel"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  scale = gimp_prop_widget_new (config, "br-gain",
                                area, context, NULL, NULL, NULL, &label);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  scale = gimp_prop_widget_new (config, "bg-gain",
                                area, context, NULL, NULL, NULL, &label);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  scale = gimp_prop_widget_new (config, "bb-gain",
                                area, context, NULL, NULL, NULL, &label);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);


  checkbox = gimp_prop_widget_new (config, "preserve-luminosity",
                                   area, context, NULL, NULL, NULL, &label);
  gtk_box_pack_start (GTK_BOX (main_vbox), checkbox, FALSE, FALSE, 0);
  gtk_widget_show (checkbox);

  return main_vbox;
}
