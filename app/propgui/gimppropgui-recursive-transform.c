/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimppropgui-recursive-transform.c
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
#include "gimppropgui-generic.h"
#include "gimppropgui-recursive-transform.h"

#include "gimp-intl.h"


static void
transform_grid_callback (GObject                    *config,
                         GeglRectangle              *area,
                         const GimpMatrix3          *transform)
{
  gchar *transform_str;

  g_object_set_data_full (G_OBJECT (config), "area",
                          g_memdup (area, sizeof (GeglRectangle)),
                          (GDestroyNotify) g_free);

  transform_str = gegl_matrix3_to_string ((GeglMatrix3 *) transform);

  g_object_set (config,
                "transform", transform_str,
                NULL);

  g_free (transform_str);
}

static void
config_notify (GObject          *config,
               const GParamSpec *pspec,
               gpointer          set_data)
{
  GimpControllerTransformGridCallback  set_func;
  GeglRectangle                       *area;
  gchar                               *transform_str;
  GimpMatrix3                          transform;

  set_func = g_object_get_data (G_OBJECT (config), "set-func");
  area     = g_object_get_data (G_OBJECT (config), "area");

  g_object_get (config,
                "transform", &transform_str,
                NULL);

  gegl_matrix3_parse_string ((GeglMatrix3 *) &transform, transform_str);

  set_func (set_data, area, &transform);

  g_free (transform_str);
}

GtkWidget *
_gimp_prop_gui_new_recursive_transform (GObject                  *config,
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

  /* skip the "transform" property, which is controlled by a transform-grid
   * controller.
   */
  if (create_controller_func)
    {
      param_specs++;
      n_param_specs--;
    }

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
                                         GIMP_CONTROLLER_TYPE_TRANSFORM_GRID,
                                         _("Recursive Transform: "),
                                         (GCallback) transform_grid_callback,
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
