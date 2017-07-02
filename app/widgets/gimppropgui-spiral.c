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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimpcontext.h"

#include "gimppropgui.h"
#include "gimppropgui-generic.h"
#include "gimppropgui-spiral.h"

#include "gimp-intl.h"


static void
line_callback (GObject       *config,
               GeglRectangle *area,
               gdouble        x1,
               gdouble        y1,
               gdouble        x2,
               gdouble        y2)
{
  gdouble x, y;
  gdouble radius;
  gdouble rotation;

  g_object_set_data_full (G_OBJECT (config), "area",
                          g_memdup (area, sizeof (GeglRectangle)),
                          (GDestroyNotify) g_free);

  x        = x1 / area->width;
  y        = y1 / area->height;
  radius   = sqrt (SQR (x2 - x1) + SQR (y2 - y1));
  rotation = atan2 (-(y2 - y1), x2 - x1) * 180 / G_PI;

  if (rotation < 0)
    rotation += 360.0;

  g_object_set (config,
                "x",        x,
                "y",        y,
                "radius",   radius,
                "rotation", rotation,
                NULL);
}

static void
config_notify (GObject          *config,
               const GParamSpec *pspec,
               gpointer          set_data)
{
  GimpControllerLineCallback  set_func;
  GeglRectangle              *area;
  gdouble                     x, y;
  gdouble                     radius;
  gdouble                     rotation;
  gdouble                     x1, y1, x2, y2;

  set_func = g_object_get_data (G_OBJECT (config), "set-func");
  area     = g_object_get_data (G_OBJECT (config), "area");

  g_object_get (config,
                "x",        &x,
                "y",        &y,
                "radius",   &radius,
                "rotation", &rotation,
                NULL);

  x1 = x * area->width;
  y1 = y * area->height;
  x2 = x1 + cos (rotation * (G_PI / 180.0)) * radius;
  y2 = y1 - sin (rotation * (G_PI / 180.0)) * radius;

  set_func (set_data, area, x1, y1, x2, y2);
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
                                         GIMP_CONTROLLER_TYPE_LINE,
                                         (GCallback) line_callback,
                                         config,
                                         &set_data);

      g_object_set_data (G_OBJECT (config), "set-func", set_func);

      g_object_set_data_full (G_OBJECT (config), "area",
                              g_memdup (area, sizeof (GeglRectangle)),
                              (GDestroyNotify) g_free);

      config_notify (config, NULL, set_data);

      g_signal_connect (config, "notify",
                        G_CALLBACK (config_notify),
                        set_data);
    }

  return vbox;
}
