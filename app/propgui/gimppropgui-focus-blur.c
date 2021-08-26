/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimppropgui-focus-blur.c
 * Copyright (C) 2020 Ell
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
#include "gimppropgui-generic.h"
#include "gimppropgui-focus-blur.h"

#include "gimp-intl.h"


static gint
find_param (GParamSpec  **param_specs,
            guint         n_param_specs,
            const gchar  *name)
{
  gint i;

  for (i = 0; i < n_param_specs; i++)
    {
      if (! strcmp (param_specs[i]->name, name))
        break;
    }

  return i;
}

static void
focus_callback (GObject       *config,
                GeglRectangle *area,
                GimpLimitType  type,
                gdouble        x,
                gdouble        y,
                gdouble        radius,
                gdouble        aspect_ratio,
                gdouble        angle,
                gdouble        inner_limit,
                gdouble        midpoint)
{
  g_object_set_data_full (G_OBJECT (config), "area",
                          g_memdup2 (area, sizeof (GeglRectangle)),
                          (GDestroyNotify) g_free);

  g_object_set (config,
                "shape",        type,
                "x",            x / area->width,
                "y",            y / area->height,
                "radius",       2.0 * radius / area->width,
                "focus",        inner_limit,
                "midpoint",     midpoint,
                "aspect-ratio", aspect_ratio,
                "rotation",     fmod (
                                  fmod (angle * 180.0 / G_PI + 180.0, 360.0) +
                                  360.0,
                                  360.0) - 180.0,
                NULL);
}

static void
config_notify (GObject          *config,
               const GParamSpec *pspec,
               gpointer          set_data)
{
  GimpControllerFocusCallback  set_func;
  GeglRectangle               *area;
  GimpLimitType                shape;
  gdouble                      radius;
  gdouble                      focus;
  gdouble                      midpoint;
  gdouble                      x, y;
  gdouble                      aspect_ratio;
  gdouble                      rotation;

  set_func = g_object_get_data (G_OBJECT (config), "set-func");
  area     = g_object_get_data (G_OBJECT (config), "area");

  g_object_get (config,
                "shape",        &shape,
                "radius",       &radius,
                "focus",        &focus,
                "midpoint",     &midpoint,
                "x",            &x,
                "y",            &y,
                "aspect-ratio", &aspect_ratio,
                "rotation",     &rotation,
                NULL);

  set_func (set_data, area,
            shape,
            x * area->width,
            y * area->height,
            radius * area->width / 2.0,
            aspect_ratio,
            rotation / 180.0 * G_PI,
            focus,
            midpoint);
}

GtkWidget *
_gimp_prop_gui_new_focus_blur (GObject                  *config,
                               GParamSpec              **param_specs,
                               guint                     n_param_specs,
                               GeglRectangle            *area,
                               GimpContext              *context,
                               GimpCreatePickerFunc      create_picker_func,
                               GimpCreateControllerFunc  create_controller_func,
                               gpointer                  creator)
{
  GtkWidget *vbox;
  gint       first_geometry_param;
  gint       last_geometry_param;

  g_return_val_if_fail (G_IS_OBJECT (config), NULL);
  g_return_val_if_fail (param_specs != NULL, NULL);
  g_return_val_if_fail (n_param_specs > 0, NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  first_geometry_param = find_param (param_specs, n_param_specs,
                                     "shape") + 1;
  last_geometry_param  = find_param (param_specs, n_param_specs,
                                     "high-quality");

  if (last_geometry_param <= first_geometry_param)
    {
      vbox = _gimp_prop_gui_new_generic (config,
                                         param_specs, n_param_specs,
                                         area, context,
                                         create_picker_func,
                                         create_controller_func,
                                         creator);
    }
  else
    {
      GtkWidget   *widget;
      GtkWidget   *expander;
      GtkWidget   *frame;
      const gchar *label;

      vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);

      widget = gimp_prop_widget_new (config,
                                     "shape",
                                     area, context,
                                     create_picker_func,
                                     create_controller_func,
                                     creator,
                                     &label);
      gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);
      gtk_widget_show (widget);

      widget = _gimp_prop_gui_new_generic (config,
                                           param_specs,
                                           first_geometry_param - 1,
                                           area, context,
                                           create_picker_func,
                                           create_controller_func,
                                           creator);
      gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);
      gtk_widget_show (widget);

      widget = _gimp_prop_gui_new_generic (config,
                                           param_specs + last_geometry_param,
                                           n_param_specs - last_geometry_param,
                                           area, context,
                                           create_picker_func,
                                           create_controller_func,
                                           creator);
      gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);
      gtk_widget_show (widget);

      expander = gtk_expander_new (_("Geometry Options"));
      gtk_box_pack_start (GTK_BOX (vbox), expander, FALSE, FALSE, 0);
      gtk_widget_show (expander);

      frame = gimp_frame_new (NULL);
      gtk_container_add (GTK_CONTAINER (expander), frame);
      gtk_widget_show (frame);

      widget = _gimp_prop_gui_new_generic (config,
                                           param_specs + first_geometry_param,
                                           last_geometry_param -
                                           first_geometry_param,
                                           area, context,
                                           create_picker_func,
                                           create_controller_func,
                                           creator);
      gtk_container_add (GTK_CONTAINER (frame), widget);
      gtk_widget_show (widget);
    }

  if (create_controller_func)
    {
      GCallback set_func;
      gpointer  set_data;

      set_func = create_controller_func (creator,
                                         GIMP_CONTROLLER_TYPE_FOCUS,
                                         _("Focus Blur: "),
                                         (GCallback) focus_callback,
                                         config,
                                         &set_data);

      g_object_set_data (G_OBJECT (config), "set-func", set_func);

      g_object_set_data_full (G_OBJECT (config), "area",
                              g_memdup2 (area, sizeof (GeglRectangle)),
                              (GDestroyNotify) g_free);

      config_notify (config, NULL, set_data);

      g_signal_connect (config, "notify",
                        G_CALLBACK (config_notify),
                        set_data);
    }

  return vbox;
}

