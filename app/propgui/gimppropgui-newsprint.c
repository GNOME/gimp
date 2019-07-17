/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimppropgui-newsprint.c
 * Copyright (C) 2019  Michael Natterer <mitch@gimp.org>
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
#include "gimppropgui-generic.h"
#include "gimppropgui-newsprint.h"

#include "gimp-intl.h"


GtkWidget *
_gimp_prop_gui_new_newsprint (GObject                  *config,
                              GParamSpec              **param_specs,
                              guint                     n_param_specs,
                              GeglRectangle            *area,
                              GimpContext              *context,
                              GimpCreatePickerFunc      create_picker_func,
                              GimpCreateControllerFunc  create_controller_func,
                              gpointer                  creator)
{
  g_return_val_if_fail (G_IS_OBJECT (config), NULL);
  g_return_val_if_fail (param_specs != NULL, NULL);
  g_return_val_if_fail (n_param_specs > 0, NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  return _gimp_prop_gui_new_generic (config,
                                     param_specs,
                                     n_param_specs,
                                     area,
                                     context,
                                     create_picker_func,
                                     create_controller_func,
                                     creator);
}
