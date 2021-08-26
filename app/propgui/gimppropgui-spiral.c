/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimppropgui-spiral.c
 * Copyright (C) 2017  Michael Natterer <mitch@gimp.org>
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
#include "gimppropgui-spiral.h"

#include "gimp-intl.h"


typedef enum
{
  GEGL_SPIRAL_TYPE_LINEAR,
  GEGL_SPIRAL_TYPE_LOGARITHMIC
} GeglSpiralType;


static void
slider_line_callback (GObject                    *config,
                      GeglRectangle              *area,
                      gdouble                     x1,
                      gdouble                     y1,
                      gdouble                     x2,
                      gdouble                     y2,
                      const GimpControllerSlider *sliders,
                      gint                        n_sliders)
{
  GeglSpiralType type;
  gdouble        x, y;
  gdouble        radius;
  gdouble        rotation;
  gdouble        base;
  gdouble        balance;

  g_object_set_data_full (G_OBJECT (config), "area",
                          g_memdup2 (area, sizeof (GeglRectangle)),
                          (GDestroyNotify) g_free);

  g_object_get (config,
                "type",    &type,
                "base",    &base,
                "balance", &balance,
                NULL);

  x        = x1 / area->width;
  y        = y1 / area->height;
  radius   = sqrt (SQR (x2 - x1) + SQR (y2 - y1));
  rotation = atan2 (-(y2 - y1), x2 - x1) * 180 / G_PI;

  if (rotation < 0)
    rotation += 360.0;

  switch (type)
    {
    case GEGL_SPIRAL_TYPE_LINEAR:
      balance = 3.0 - 4.0 * sliders[0].value;

      break;

    case GEGL_SPIRAL_TYPE_LOGARITHMIC:
      {
        gdouble old_base = base;

        base = 1.0 / sliders[1].value;
        base = MIN (base, 1000000.0);

        /* keep "balance" fixed when changing "base", or when "base" is 1, in
         * which case there's no inverse mapping for the slider value, and we
         * can get NaN.
         */
        if (base == old_base && base > 1.0)
          {
            balance = -4.0 * log (sliders[0].value) / log (base) - 1.0;
            balance = CLAMP (balance, -1.0, 1.0);
          }
      }
      break;
    }

  g_object_set (config,
                "x",        x,
                "y",        y,
                "radius",   radius,
                "base",     base,
                "rotation", rotation,
                "balance",  balance,
                NULL);
}

static void
config_notify (GObject          *config,
               const GParamSpec *pspec,
               gpointer          set_data)
{
  GimpControllerSliderLineCallback  set_func;
  GeglRectangle                    *area;
  GeglSpiralType                    type;
  gdouble                           x, y;
  gdouble                           radius;
  gdouble                           rotation;
  gdouble                           base;
  gdouble                           balance;
  gdouble                           x1, y1, x2, y2;
  GimpControllerSlider              sliders[2];
  gint                              n_sliders = 0;

  set_func = g_object_get_data (G_OBJECT (config), "set-func");
  area     = g_object_get_data (G_OBJECT (config), "area");

  g_object_get (config,
                "type",     &type,
                "x",        &x,
                "y",        &y,
                "radius",   &radius,
                "rotation", &rotation,
                "base",     &base,
                "balance",  &balance,
                NULL);

  x1 = x * area->width;
  y1 = y * area->height;
  x2 = x1 + cos (rotation * (G_PI / 180.0)) * radius;
  y2 = y1 - sin (rotation * (G_PI / 180.0)) * radius;

  switch (type)
  {
  case GEGL_SPIRAL_TYPE_LINEAR:
    n_sliders = 1;

    /* balance */
    sliders[0]       = GIMP_CONTROLLER_SLIDER_DEFAULT;
    sliders[0].min   = 0.5;
    sliders[0].max   = 1.0;
    sliders[0].value = 0.5 + (1.0 - balance) / 4.0;

    break;

  case GEGL_SPIRAL_TYPE_LOGARITHMIC:
    n_sliders = 2;

    /* balance */
    sliders[0]       = GIMP_CONTROLLER_SLIDER_DEFAULT;
    sliders[0].min   = 1.0 / sqrt (base);
    sliders[0].max   = 1.0;
    sliders[0].value = pow (base, -(balance + 1.0) / 4.0);

    /* base */
    sliders[1]       = GIMP_CONTROLLER_SLIDER_DEFAULT;
    sliders[1].min   = 0.0;
    sliders[1].max   = 1.0;
    sliders[1].value = 1.0 / base;

    break;
  }

  set_func (set_data, area, x1, y1, x2, y2, sliders, n_sliders);
}

GtkWidget *
_gimp_prop_gui_new_spiral (GObject                  *config,
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
                                         GIMP_CONTROLLER_TYPE_SLIDER_LINE,
                                         _("Spiral: "),
                                         (GCallback) slider_line_callback,
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
