/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "widgets/gimpwidgets-utils.h"

#include "gimprectangleoptions.h"
#include "gimprectangleselectoptions.h"
#include "gimptooloptions-gui.h"

#include "gimp-intl.h"


static void   gimp_rect_select_options_rectangle_options_iface_init (GimpRectangleOptionsInterface *iface);


G_DEFINE_TYPE_WITH_CODE (GimpRectSelectOptions, gimp_rect_select_options,
                         GIMP_TYPE_SELECTION_OPTIONS,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_RECTANGLE_OPTIONS,
                                                gimp_rect_select_options_rectangle_options_iface_init))


static void
gimp_rect_select_options_class_init (GimpRectSelectOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = gimp_rectangle_options_set_property;
  object_class->get_property = gimp_rectangle_options_get_property;

  gimp_rectangle_options_install_properties (object_class);
}

static void
gimp_rect_select_options_init (GimpRectSelectOptions *options)
{
}

static void
gimp_rect_select_options_rectangle_options_iface_init (GimpRectangleOptionsInterface *iface)
{
}

GtkWidget *
gimp_rect_select_options_gui (GimpToolOptions *tool_options)
{
  GtkWidget *vbox = gimp_selection_options_gui (tool_options);
  GtkWidget *vbox_rectangle;

  /*  rectangle options  */
  vbox_rectangle = gimp_rectangle_options_gui (tool_options);
  gtk_box_pack_start (GTK_BOX (vbox), vbox_rectangle, FALSE, FALSE, 0);
  gtk_widget_show (vbox_rectangle);

  gimp_rectangle_options_set_highlight (GIMP_RECTANGLE_OPTIONS (tool_options),
                                        FALSE);

  return vbox;
}
