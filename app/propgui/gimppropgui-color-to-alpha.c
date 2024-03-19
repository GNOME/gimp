/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimppropgui-color-to-alpha.c
 * Copyright (C) 2017 Ell
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

#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "propgui-types.h"

#include "core/gimpcontext.h"

#include "gimppropgui.h"
#include "gimppropgui-color-to-alpha.h"
#include "gimppropgui-generic.h"

#include "gimp-intl.h"


static void
threshold_picked (GObject       *config,
                  gpointer       identifier,
                  gdouble        x,
                  gdouble        y,
                  const Babl    *sample_format,
                  GeglColor     *picked_color)
{
  GeglColor *color;
  gdouble    threshold = 0.0;
  gdouble    rgb[3];
  gdouble    picked_rgb[3];

  g_object_get (config,
                "color", &color,
                NULL);

  gegl_color_get_pixel (color, babl_format ("R'G'B' double"), rgb);
  gegl_color_get_pixel (picked_color, babl_format ("R'G'B' double"),
                        picked_rgb);

  threshold = MAX (threshold, fabs (picked_rgb[0] - rgb[0]));
  threshold = MAX (threshold, fabs (picked_rgb[1] - rgb[1]));
  threshold = MAX (threshold, fabs (picked_rgb[2] - rgb[2]));

  g_object_set (config,
                identifier, threshold,
                NULL);

  g_object_unref (color);
}

GtkWidget *
_gimp_prop_gui_new_color_to_alpha (GObject                  *config,
                                   GParamSpec              **param_specs,
                                   guint                     n_param_specs,
                                   GeglRectangle            *area,
                                   GimpContext              *context,
                                   GimpCreatePickerFunc      create_picker_func,
                                   GimpCreateControllerFunc  create_controller_func,
                                   gpointer                  creator)
{
  GtkWidget   *vbox;
  GtkWidget   *button;
  GtkWidget   *hbox;
  GtkWidget   *scale;
  const gchar *label;

  g_return_val_if_fail (G_IS_OBJECT (config), NULL);
  g_return_val_if_fail (param_specs != NULL, NULL);
  g_return_val_if_fail (n_param_specs > 0, NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);

  button = _gimp_prop_gui_new_generic (config, param_specs, 1,
                                       area, context, create_picker_func, NULL,
                                       creator);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  scale = gimp_prop_widget_new (config, "transparency-threshold",
                                area, context, NULL, NULL, NULL, &label);
  gtk_box_pack_start (GTK_BOX (hbox), scale, TRUE, TRUE, 0);

  if (create_picker_func)
    {
      button = create_picker_func (creator,
                                   "transparency-threshold",
                                   GIMP_ICON_COLOR_PICKER_GRAY,
                                   _("Pick farthest full-transparency color"),
                                   /* pick_abyss = */ FALSE,
                                   (GimpPickerCallback) threshold_picked,
                                   config);
      gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
      gtk_widget_show (button);
    }

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  scale = gimp_prop_widget_new (config, "opacity-threshold",
                                area, context, NULL, NULL, NULL, &label);
  gtk_box_pack_start (GTK_BOX (hbox), scale, TRUE, TRUE, 0);

  if (create_picker_func)
    {
      button = create_picker_func (creator,
                                   "opacity-threshold",
                                   GIMP_ICON_COLOR_PICKER_GRAY,
                                   _("Pick nearest full-opacity color"),
                                   /* pick_abyss = */ FALSE,
                                   (GimpPickerCallback) threshold_picked,
                                   config);
      gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
      gtk_widget_show (button);
    }

  return vbox;
}
