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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimp.h"
#include "core/gimptoolinfo.h"
#include "config/gimpcoreconfig.h"

#include "widgets/gimppropwidgets.h"
#include "widgets/gimpwidgets-utils.h"

#include "gimprectangleoptions.h"
#include "gimprectangleselectoptions.h"
#include "gimprectangleselecttool.h"
#include "gimptooloptions-gui.h"
#include "gimpselectbyshapetool.h"

#include "gimp-intl.h"


enum
{
  PROP_ROUND_CORNERS = GIMP_RECTANGLE_OPTIONS_PROP_LAST + 1,
  PROP_CORNER_RADIUS,
  PROP_SHAPE_TYPE,
  PROP_HORIZONTAL,
  PROP_N_SIDES
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
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class,
                                    GIMP_RECTANGLE_OPTIONS_PROP_HIGHLIGHT,
                                    "highlight",
                                    N_("Dim everything outside selection"),
                                    FALSE,
                                    GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_ROUND_CORNERS,
                                    "round-corners",
                                    N_("Round corners of selection"),
                                    FALSE,
                                    GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_CORNER_RADIUS,
                                   "corner-radius",
                                   N_("Radius of rounding in pixels"),
                                   0.0, 100.0, 5.0,
                                   GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_INSTALL_PROP_ENUM (object_class, PROP_SHAPE_TYPE,
                                 "shape-type",
                                 N_("Select shape of the selection"),
                                 GIMP_TYPE_SHAPE_TYPE,
                                 GIMP_SHAPE_RECTANGLE,
                                 GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_HORIZONTAL,
                                    "horizontal",
                                    N_("Choice between horizontal and vertical"),
                                    FALSE,
                                    GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_INSTALL_PROP_INT (object_class, PROP_N_SIDES,
                                   "n-sides",
                                   N_("Number of sides, in the polygon"),
                                   0, 10, 1,
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

    case PROP_SHAPE_TYPE:
      options->shape_type = g_value_get_enum (value);
      break; 

    case PROP_HORIZONTAL:
      options->horizontal = g_value_get_boolean (value);
      break;   
    
    case PROP_N_SIDES:
      options->n_sides = g_value_get_int (value);
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

    case PROP_SHAPE_TYPE:
      g_value_set_enum (value, options->shape_type);
      break;

    case PROP_HORIZONTAL:
      g_value_set_enum (value, options->horizontal);
      break;  
    
    case PROP_N_SIDES:
      g_value_set_int (value, options->n_sides);
      break;

    default:
      gimp_rectangle_options_get_property (object, property_id, value, pspec);
      break;
    }
}

static gboolean
gimp_select_by_shape_options_shape_type(GBinding     *binding,
                                            const GValue *source_value,
                                            GValue       *target_value,
                                            gpointer      user_data)
{
  gint type = g_value_get_enum (source_value);

  g_value_set_boolean (target_value,
                       type == GPOINTER_TO_INT (user_data));

  return TRUE;
}

GtkWidget *
gimp_rectangle_select_options_gui (GimpToolOptions *tool_options)
{
  GObject   *config = G_OBJECT (tool_options);
  GtkWidget *vbox   = gimp_selection_options_gui (tool_options);
  GtkWidget *combo;
  GtkWidget *button;
  GtkWidget *frame;
  GtkWidget *scale;
  GtkWidget *inner_vbox;

  /*  the round corners frame  */
  if (tool_options->tool_info->tool_type == GIMP_TYPE_RECTANGLE_SELECT_TOOL)
    {
      GtkWidget *toggle;
      

      scale = gimp_prop_spin_scale_new (config, "corner-radius",
                                        _("Radius"),
                                        1.0, 10.0, 1);

      frame = gimp_prop_expanding_frame_new (config, "round-corners",
                                             _("Rounded corners"),
                                             scale, NULL);
      gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
      gtk_widget_show (frame);

      toggle = GIMP_SELECTION_OPTIONS (tool_options)->antialias_toggle;

      g_object_bind_property (config, "round-corners",
                              toggle, "sensitive",
                              G_BINDING_SYNC_CREATE);
    }

  /* for select by shape tool */
  if (tool_options->tool_info->tool_type == GIMP_TYPE_SELECT_BY_SHAPE_TOOL)  
   {  
      combo = gimp_prop_enum_combo_box_new (config, "shape-type", 0, 0);
      gimp_int_combo_box_set_label (GIMP_INT_COMBO_BOX (combo), _("Shape"));
      g_object_set (combo, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
      gtk_box_pack_start (GTK_BOX (vbox), combo, TRUE, TRUE, 0);
      gtk_widget_show (combo);

      frame = gimp_frame_new (NULL);
      gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
      gtk_widget_show (frame);

      inner_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
      gtk_container_add (GTK_CONTAINER (frame), inner_vbox);
      gtk_widget_show (inner_vbox);
  
      scale = gimp_prop_spin_scale_new (config, "corner-radius",
                                        _("Radius"),
                                        1.0, 10.0, 1);
      gtk_box_pack_start (GTK_BOX (inner_vbox), scale, FALSE, FALSE, 0);

      g_object_bind_property_full (config, "shape-type",
                               scale,  "visible",
                               G_BINDING_SYNC_CREATE,
                               gimp_select_by_shape_options_shape_type,
                               NULL,
                               GINT_TO_POINTER (GIMP_SHAPE_ROUNDED_RECT),
                               NULL);
      
      scale = gimp_prop_spin_scale_new (config, "n-sides",
                                       _("Number of Sides in Polygon ?"),
                                      1.0, 10.0, 1);
      gtk_box_pack_start (GTK_BOX (inner_vbox), scale, FALSE, FALSE, 0);

      g_object_bind_property_full (config, "shape-type",
                               scale,  "visible",
                               G_BINDING_SYNC_CREATE,
                               gimp_select_by_shape_options_shape_type,
                               NULL,
                               GINT_TO_POINTER (GIMP_SHAPE_N_POLYGON),
                               NULL);
      
      button = gimp_prop_check_button_new (config, "horizontal",
                                           _("Horizontal Line"));
      gtk_box_pack_start (GTK_BOX (inner_vbox), button, FALSE, FALSE, 0);

      g_object_bind_property_full (config, "shape-type",
                               button,  "visible",
                               G_BINDING_SYNC_CREATE,
                               gimp_select_by_shape_options_shape_type,
                               NULL,
                               GINT_TO_POINTER (GIMP_SHAPE_SINGLE_ROW),
                               NULL);
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
