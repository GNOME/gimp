/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimppropgui-diffration-patterns.c
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimpcontext.h"

#include "gimppropgui.h"
#include "gimppropgui-diffration-patterns.h"
#include "gimppropgui-generic.h"

#include "gimp-intl.h"


GtkWidget *
_gimp_prop_gui_new_diffraction_patterns (GObject              *config,
                                         GParamSpec          **param_specs,
                                         guint                 n_param_specs,
                                         GeglRectangle        *area,
                                         GimpContext          *context,
                                         GimpCreatePickerFunc  create_picker_func,
                                         gpointer              picker_creator)
{
  GtkWidget *notebook;
  GtkWidget *vbox;

  g_return_val_if_fail (G_IS_OBJECT (config), NULL);
  g_return_val_if_fail (param_specs != NULL, NULL);
  g_return_val_if_fail (n_param_specs > 0, NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  notebook = gtk_notebook_new ();

  vbox = _gimp_prop_gui_new_generic (config,
                                     param_specs + 0, 3,
                                     area, context,
                                     create_picker_func, picker_creator);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox,
                            gtk_label_new (_("Frequencies")));
  gtk_widget_show (vbox);

  vbox = _gimp_prop_gui_new_generic (config,
                                     param_specs + 3, 3,
                                     area, context,
                                     create_picker_func, picker_creator);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox,
                            gtk_label_new (_("Contours")));
  gtk_widget_show (vbox);

  vbox = _gimp_prop_gui_new_generic (config,
                                     param_specs + 6, 3,
                                     area, context,
                                     create_picker_func, picker_creator);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox,
                            gtk_label_new (_("Sharp Edges")));
  gtk_widget_show (vbox);

  vbox = _gimp_prop_gui_new_generic (config,
                                     param_specs + 9, 3,
                                     area, context,
                                     create_picker_func, picker_creator);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox,
                            gtk_label_new (_("Other Options")));
  gtk_widget_show (vbox);

  return notebook;
}
