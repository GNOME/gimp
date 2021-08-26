/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimppropgui-vignette.c
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
#include "gimppropgui-vignette.h"

#include "gimp-intl.h"


#define MAX_GAMMA 1000.0


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
  gdouble proportion;
  gdouble squeeze;
  gdouble scale;

  g_object_set_data_full (G_OBJECT (config), "area",
                          g_memdup2 (area, sizeof (GeglRectangle)),
                          (GDestroyNotify) g_free);

  g_object_get (config,
                "proportion", &proportion,
                NULL);

  if (aspect_ratio >= 0.0)
    scale = 1.0 - aspect_ratio;
  else
    scale = 1.0 / (1.0 + aspect_ratio);

  scale /= 1.0 + proportion * ((gdouble) area->height / area->width - 1.0);

  if (scale <= 1.0)
    squeeze = +2.0 * atan (1.0 / scale - 1.0) / G_PI;
  else
    squeeze = -2.0 * atan (scale - 1.0) / G_PI;

  g_object_set (config,
                "shape",      type,
                "x",          x / area->width,
                "y",          y / area->height,
                "radius",     2.0 * radius / area->width,
                "proportion", proportion,
                "squeeze",    squeeze,
                "rotation",   fmod (fmod (angle * 180.0 / G_PI, 360.0) + 360.0,
                                    360.0),
                "softness",   1.0 - inner_limit,
                "gamma",      midpoint < 1.0 ?
                                MIN (log (0.5) / log (midpoint), MAX_GAMMA) :
                                MAX_GAMMA,
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
  gdouble                      x, y;
  gdouble                      radius;
  gdouble                      proportion;
  gdouble                      squeeze;
  gdouble                      rotation;
  gdouble                      softness;
  gdouble                      gamma;
  gdouble                      aspect_ratio;

  set_func = g_object_get_data (G_OBJECT (config), "set-func");
  area     = g_object_get_data (G_OBJECT (config), "area");

  g_object_get (config,
                "shape",      &shape,
                "x",          &x,
                "y",          &y,
                "radius",     &radius,
                "proportion", &proportion,
                "squeeze",    &squeeze,
                "rotation",   &rotation,
                "softness",   &softness,
                "gamma",      &gamma,
                NULL);

  aspect_ratio = 1.0 + ((gdouble) area->height / area->width - 1.0) *
                       proportion;

  if (squeeze >= 0.0)
    aspect_ratio /= tan (+squeeze * G_PI / 2.0) + 1.0;
  else
    aspect_ratio *= tan (-squeeze * G_PI / 2.0) + 1.0;

  if (aspect_ratio <= 1.0)
    aspect_ratio = 1.0 - aspect_ratio;
  else
    aspect_ratio = 1.0 / aspect_ratio - 1.0;

  set_func (set_data, area,
            shape,
            x * area->width,
            y * area->height,
            radius * area->width / 2.0,
            aspect_ratio,
            rotation / 180.0 * G_PI,
            1.0 - softness,
            pow (0.5, 1.0 / gamma));
}

GtkWidget *
_gimp_prop_gui_new_vignette (GObject                  *config,
                             GParamSpec              **param_specs,
                             guint                     n_param_specs,
                             GeglRectangle            *area,
                             GimpContext              *context,
                             GimpCreatePickerFunc      create_picker_func,
                             GimpCreateControllerFunc  create_controller_func,
                             gpointer                  creator)
{
  GtkWidget *vbox;

  g_return_val_if_fail (G_IS_OBJECT (config), NULL);
  g_return_val_if_fail (param_specs != NULL, NULL);
  g_return_val_if_fail (n_param_specs > 0, NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  vbox = _gimp_prop_gui_new_generic (config,
                                     param_specs, n_param_specs,
                                     area, context,
                                     create_picker_func,
                                     create_controller_func,
                                     creator);


  if (create_controller_func)
    {
      GCallback set_func;
      gpointer  set_data;

      set_func = create_controller_func (creator,
                                         GIMP_CONTROLLER_TYPE_FOCUS,
                                         _("Vignette: "),
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
