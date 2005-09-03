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
#include "gimpnewrectselectoptions.h"
#include "gimptooloptions-gui.h"

#include "gimp-intl.h"


static void   gimp_new_rect_select_options_class_init                   (GimpNewRectSelectOptionsClass *klass);
static void   gimp_new_rect_select_options_rectangle_options_iface_init (GimpRectangleOptionsInterface *rectangle_iface);


static GimpSelectionOptionsClass *parent_class = NULL;


GType
gimp_new_rect_select_options_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      static const GTypeInfo info =
      {
        sizeof (GimpNewRectSelectOptionsClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gimp_new_rect_select_options_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (GimpNewRectSelectOptions),
        0,              /* n_preallocs    */
        (GInstanceInitFunc) NULL
      };
      static const GInterfaceInfo rectangle_info =
      {
        (GInterfaceInitFunc) gimp_new_rect_select_options_rectangle_options_iface_init,           /* interface_init */
        NULL,           /* interface_finalize */
        NULL            /* interface_data */
      };

      type = g_type_register_static (GIMP_TYPE_SELECTION_OPTIONS,
                                     "GimpNewRectSelectOptions",
                                     &info, 0);
      g_type_add_interface_static (type,
                                   GIMP_TYPE_RECTANGLE_OPTIONS,
                                   &rectangle_info);
     }

  return type;
}

static void
gimp_new_rect_select_options_class_init (GimpNewRectSelectOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->set_property = gimp_rectangle_options_set_property;
  object_class->get_property = gimp_rectangle_options_get_property;

  gimp_rectangle_options_install_properties (object_class);
}

static void
gimp_new_rect_select_options_rectangle_options_iface_init (GimpRectangleOptionsInterface *rectangle_iface)
{
}

GtkWidget *
gimp_new_rect_select_options_gui (GimpToolOptions *tool_options)
{
  GtkWidget *vbox;
  GtkWidget *vbox_rectangle;

  vbox = gimp_selection_options_gui (tool_options);

  /*  rectangle options  */
  vbox_rectangle = gimp_rectangle_options_gui (tool_options);
  gtk_box_pack_start (GTK_BOX (vbox), vbox_rectangle, FALSE, FALSE, 0);
  gtk_widget_show (vbox_rectangle);

  return vbox;
}
