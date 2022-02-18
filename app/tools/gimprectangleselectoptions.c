/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include "libgimpbase/gimpbase.h"
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


static void   gimp_rectangle_select_options_set_property (GObject      *object,
                                                          guint         property_id,
                                                          const GValue *value,
                                                          GParamSpec   *pspec);
static void   gimp_rectangle_select_options_get_property (GObject      *object,
                                                          guint         property_id,
                                                          GValue       *value,
                                                          GParamSpec   *pspec);


G_DEFINE_TYPE_WITH_CODE (GimpRectangleSelectOptions,
                         gimp_rectangle_select_options,
                         GIMP_TYPE_SELECTION_OPTIONS,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_RECTANGLE_OPTIONS,
                                                NULL))


static void
gimp_rectangle_select_options_class_init (GimpRectangleSelectOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = gimp_rectangle_select_options_set_property;
  object_class->get_property = gimp_rectangle_select_options_get_property;

  /* The 'highlight' property is defined here because we want different
   * default values for the Crop and the Rectangle Select tools.
   */
  GIMP_CONFIG_PROP_BOOLEAN (object_class,
                            GIMP_RECTANGLE_OPTIONS_PROP_HIGHLIGHT,
                            "highlight",
                            _("Highlight"),
                            _("Dim everything outside selection"),
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_DOUBLE (object_class,
                           GIMP_RECTANGLE_OPTIONS_PROP_HIGHLIGHT_OPACITY,
                           "highlight-opacity",
                           _("Highlight opacity"),
                           _("How much to dim everything outside selection"),
                           0.0, 1.0, 0.5,
                           GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_ROUND_CORNERS,
                            "round-corners",
                            _("Rounded corners"),
                            _("Round corners of selection"),
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_DOUBLE (object_class, PROP_CORNER_RADIUS,
                           "corner-radius",
                           _("Radius"),
                           _("Radius of rounding in pixels"),
                           0.0, 10000.0, 10.0,
                           GIMP_PARAM_STATIC_STRINGS);

  gimp_rectangle_options_install_properties (object_class);
}

static void
gimp_rectangle_select_options_init (GimpRectangleSelectOptions *options)
{
}

static void
gimp_rectangle_select_options_set_property (GObject      *object,
                                            guint         property_id,
                                            const GValue *value,
                                            GParamSpec   *pspec)
{
  GimpRectangleSelectOptions *options = GIMP_RECTANGLE_SELECT_OPTIONS (object);

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
gimp_rectangle_select_options_get_property (GObject    *object,
                                            guint       property_id,
                                            GValue     *value,
                                            GParamSpec *pspec)
{
  GimpRectangleSelectOptions *options = GIMP_RECTANGLE_SELECT_OPTIONS (object);

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
gimp_rectangle_select_options_gui (GimpToolOptions *tool_options)
{
  GObject   *config = G_OBJECT (tool_options);
  GtkWidget *vbox   = gimp_selection_options_gui (tool_options);

  /*  the round corners frame  */
  if (tool_options->tool_info->tool_type == GIMP_TYPE_RECTANGLE_SELECT_TOOL)
    {
      GtkWidget *frame;
      GtkWidget *scale;
      GtkWidget *toggle;

      scale = gimp_prop_spin_scale_new (config, "corner-radius",
                                        1.0, 10.0, 1);
      gimp_spin_scale_set_scale_limits (GIMP_SPIN_SCALE (scale),
                                        0.0, 1000.0);
      gimp_spin_scale_set_gamma (GIMP_SPIN_SCALE (scale), 1.7);

      frame = gimp_prop_expanding_frame_new (config, "round-corners", NULL,
                                             scale, NULL);
      gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);

      toggle = GIMP_SELECTION_OPTIONS (tool_options)->antialias_toggle;

      g_object_bind_property (config, "round-corners",
                              toggle, "sensitive",
                              G_BINDING_SYNC_CREATE);
    }

  /*  the rectangle options  */
  {
    GtkWidget *vbox_rectangle;

    vbox_rectangle = gimp_rectangle_options_gui (tool_options);
    gtk_box_pack_start (GTK_BOX (vbox), vbox_rectangle, FALSE, FALSE, 0);
    gtk_widget_show (vbox_rectangle);
  }

  return vbox;
}
