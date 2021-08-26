/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimppropgui-panorama-projection.c
 * Copyright (C) 2018 Ell
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
#include "gimppropgui-panorama-projection.h"

#include "gimp-intl.h"


static void
gyroscope_callback (GObject       *config,
                    GeglRectangle *area,
                    gdouble        yaw,
                    gdouble        pitch,
                    gdouble        roll,
                    gdouble        zoom,
                    gboolean       invert)
{
  g_object_set_data_full (G_OBJECT (config), "area",
                          g_memdup2 (area, sizeof (GeglRectangle)),
                          (GDestroyNotify) g_free);

  g_object_set (config,
                "pan",     -yaw,
                "tilt",    -pitch,
                "spin",    -roll,
                "zoom",    CLAMP (100.0 * zoom, 0.01, 1000.0),
                "inverse", invert,
                NULL);
}

static void
config_notify (GObject          *config,
               const GParamSpec *pspec,
               gpointer          set_data)
{
  GimpControllerGyroscopeCallback  set_func;
  GeglRectangle                   *area;
  gdouble                          pan;
  gdouble                          tilt;
  gdouble                          spin;
  gdouble                          zoom;
  gboolean                         inverse;

  set_func = g_object_get_data (G_OBJECT (config), "set-func");
  area     = g_object_get_data (G_OBJECT (config), "area");

  g_object_get (config,
                "pan",     &pan,
                "tilt",    &tilt,
                "spin",    &spin,
                "zoom",    &zoom,
                "inverse", &inverse,
                NULL);

  set_func (set_data,
            area,
            -pan, -tilt, -spin,
            zoom / 100.0,
            inverse);
}

GtkWidget *
_gimp_prop_gui_new_panorama_projection (GObject                   *config,
                                        GParamSpec               **param_specs,
                                        guint                      n_param_specs,
                                        GeglRectangle             *area,
                                        GimpContext               *context,
                                        GimpCreatePickerFunc       create_picker_func,
                                        GimpCreateControllerFunc   create_controller_func,
                                        gpointer                   creator)
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
                                         GIMP_CONTROLLER_TYPE_GYROSCOPE,
                                         _("Panorama Projection: "),
                                         (GCallback) gyroscope_callback,
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
