/* GIMP - The GNU Image Manipulation Program
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

#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimptoolinfo.h"

#include "widgets/gimppropwidgets.h"

#include "gimprectangleoptions.h"
#include "gimprectangleselectoptions.h"
#include "gimprectangleselecttool.h"
#include "gimptooloptions-gui.h"

#include "gimp-intl.h"


enum
{
  PROP_ROUND_CORNERS = GIMP_RECTANGLE_OPTIONS_PROP_LAST + 1,
  PROP_CORNER_RADIUS
};


static void   gimp_rect_select_options_set_property (GObject      *object,
                                                     guint         property_id,
                                                     const GValue *value,
                                                     GParamSpec   *pspec);
static void   gimp_rect_select_options_get_property (GObject      *object,
                                                     guint         property_id,
                                                     GValue       *value,
                                                     GParamSpec   *pspec);


G_DEFINE_TYPE_WITH_CODE (GimpRectSelectOptions, gimp_rect_select_options,
                         GIMP_TYPE_SELECTION_OPTIONS,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_RECTANGLE_OPTIONS,
                                                NULL))


static void
gimp_rect_select_options_class_init (GimpRectSelectOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = gimp_rect_select_options_set_property;
  object_class->get_property = gimp_rect_select_options_get_property;

  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_ROUND_CORNERS,
                                    "round-corners", NULL,
                                    FALSE,
                                    GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_CORNER_RADIUS,
                                   "corner-radius", NULL,
                                   0.0, 100.0, 5.0,
                                   GIMP_PARAM_STATIC_STRINGS);

  gimp_rectangle_options_install_properties (object_class);
}

static void
gimp_rect_select_options_init (GimpRectSelectOptions *options)
{
}

static void
gimp_rect_select_options_set_property (GObject      *object,
                                       guint         property_id,
                                       const GValue *value,
                                       GParamSpec   *pspec)
{
  GimpRectSelectOptions *options = GIMP_RECT_SELECT_OPTIONS (object);

  switch (property_id)
    {
    case PROP_ROUND_CORNERS:
      options->round_corners = g_value_get_boolean (value);
      break;

    case PROP_CORNER_RADIUS:
      options->corner_radius = g_value_get_double (value);
      break;

    default:
      gimp_rectangle_options_set_property (object, property_id, value, pspec);
      break;
    }
}

static void
gimp_rect_select_options_get_property (GObject      *object,
                                       guint         property_id,
                                       GValue       *value,
                                       GParamSpec   *pspec)
{
  GimpRectSelectOptions *options = GIMP_RECT_SELECT_OPTIONS (object);

  switch (property_id)
    {
    case PROP_ROUND_CORNERS:
      g_value_set_boolean (value, options->round_corners);
      break;

    case PROP_CORNER_RADIUS:
      g_value_set_double (value, options->corner_radius);
      break;

    default:
      gimp_rectangle_options_get_property (object, property_id, value, pspec);
      break;
    }
}

GtkWidget *
gimp_rect_select_options_gui (GimpToolOptions *tool_options)
{
  GObject   *config = G_OBJECT (tool_options);
  GtkWidget *vbox   = gimp_selection_options_gui (tool_options);

  /*  the round corners frame  */
  if (tool_options->tool_info->tool_type == GIMP_TYPE_RECT_SELECT_TOOL)
    {
      GtkWidget *frame;
      GtkWidget *button;
      GtkWidget *table;

      table = gtk_table_new (1, 3, FALSE);
      gtk_table_set_col_spacings (GTK_TABLE (table), 2);

      frame = gimp_prop_expanding_frame_new (config, "round-corners",
                                             _("Rounded corners"),
                                             table, &button);
      gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
      gtk_widget_show (frame);

      g_object_set_data (G_OBJECT (button), "set_sensitive",
                         GIMP_SELECTION_OPTIONS (tool_options)->antialias_toggle);
      gtk_widget_set_sensitive (GIMP_SELECTION_OPTIONS (tool_options)->antialias_toggle,
                                GIMP_RECT_SELECT_OPTIONS (tool_options)->round_corners);

      gimp_prop_scale_entry_new (config, "corner-radius",
                                 GTK_TABLE (table), 0, 0,
                                 _("Radius:"),
                                 1.0, 10.0, 1,
                                 FALSE, 0.0, 0.0);
  }

  /*  the rectangle options  */
  {
    GtkWidget *vbox_rectangle;

    vbox_rectangle = gimp_rectangle_options_gui (tool_options);
    gtk_box_pack_start (GTK_BOX (vbox), vbox_rectangle, FALSE, FALSE, 0);
    gtk_widget_show (vbox_rectangle);

    g_object_set (GIMP_RECTANGLE_OPTIONS (tool_options),
                  "highlight", FALSE,
                  NULL);
  }

  return vbox;
}
