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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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


static gdouble
get_max_color_extent (GObject *config)
{
  GimpRGB *color;
  gdouble  max_extent = 0.0;

  g_object_get (config,
                "color", &color,
                NULL);

  max_extent = MAX (max_extent, MAX (color->r, 1.0 - color->r));
  max_extent = MAX (max_extent, MAX (color->g, 1.0 - color->g));
  max_extent = MAX (max_extent, MAX (color->b, 1.0 - color->b));

  g_free (color);

  return max_extent;
}

static gdouble
compress_threshold (GObject *config,
                    gdouble  threshold)
{
  return MIN (threshold / get_max_color_extent (config), 1.0);
}

static gdouble
uncompress_threshold (GObject *config,
                      gdouble  threshold)
{
  return threshold * get_max_color_extent (config);
}

static void
threshold_picked (GObject       *config,
                  gpointer       identifier,
                  gdouble        x,
                  gdouble        y,
                  const Babl    *sample_format,
                  const GimpRGB *picked_color)
{
  GimpRGB  *color;
  gboolean  compress_threshold_range;
  gdouble   threshold = 0.0;

  g_object_get (config,
                "color",                    &color,
                "compress-threshold-range", &compress_threshold_range,
                NULL);

  threshold = MAX (threshold, fabs (picked_color->r - color->r));
  threshold = MAX (threshold, fabs (picked_color->g - color->g));
  threshold = MAX (threshold, fabs (picked_color->b - color->b));

  if (compress_threshold_range)
    threshold = compress_threshold (config, threshold);

  g_object_set (config,
                identifier, threshold,
                NULL);

  g_free (color);
}

static void
compress_threshold_range_clicked (GtkWidget *toggle,
                                  GObject   *config)
{
  gdouble  transparency_threshold;
  gdouble  opacity_threshold;
  gboolean compress_threshold_range;

  g_object_get (config,
                "transparency-threshold",   &transparency_threshold,
                "opacity-threshold",        &opacity_threshold,
                "compress-threshold-range", &compress_threshold_range,
                NULL);

  if (compress_threshold_range)
    {
      transparency_threshold = compress_threshold (config, transparency_threshold);
      opacity_threshold      = compress_threshold (config, opacity_threshold);
    }
  else
    {
      transparency_threshold = uncompress_threshold (config, transparency_threshold);
      opacity_threshold      = uncompress_threshold (config, opacity_threshold);
    }

  g_object_set (config,
                "transparency-threshold", transparency_threshold,
                "opacity-threshold",      opacity_threshold,
                NULL);
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
  GtkWidget   *toggle;
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
  gtk_widget_show (button);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  scale = gimp_prop_widget_new (config, "transparency-threshold",
                                area, context, NULL, NULL, NULL, &label);
  gtk_box_pack_start (GTK_BOX (hbox), scale, TRUE, TRUE, 0);
  gtk_widget_show (scale);

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
  gtk_widget_show (scale);

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

  toggle = gimp_prop_widget_new (config, "compress-threshold-range",
                                 area, context, NULL, NULL, NULL, &label);
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "clicked",
                    G_CALLBACK (compress_threshold_range_clicked),
                    config);

  return vbox;
}
