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

#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "paint/gimpairbrush.h"
#include "paint/gimpairbrushoptions.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimppropwidgets.h"

#include "gimpairbrushtool.h"
#include "gimppaintoptions-gui.h"
#include "gimppainttool-paint.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


static void        gimp_airbrush_tool_constructed    (GObject          *object);

static void        gimp_airbrush_tool_airbrush_stamp (GimpAirbrush     *airbrush,
                                                      GimpAirbrushTool *airbrush_tool);

static void        gimp_airbrush_tool_stamp          (GimpAirbrushTool *airbrush_tool,
                                                      gpointer          data);

static GtkWidget * gimp_airbrush_options_gui         (GimpToolOptions  *tool_options);


G_DEFINE_TYPE (GimpAirbrushTool, gimp_airbrush_tool, GIMP_TYPE_PAINTBRUSH_TOOL)

#define parent_class gimp_airbrush_tool_parent_class


void
gimp_airbrush_tool_register (GimpToolRegisterCallback  callback,
                             gpointer                  data)
{
  (* callback) (GIMP_TYPE_AIRBRUSH_TOOL,
                GIMP_TYPE_AIRBRUSH_OPTIONS,
                gimp_airbrush_options_gui,
                GIMP_PAINT_OPTIONS_CONTEXT_MASK |
                GIMP_CONTEXT_PROP_MASK_GRADIENT,
                "gimp-airbrush-tool",
                _("Airbrush"),
                _("Airbrush Tool: Paint using a brush, with variable pressure"),
                N_("_Airbrush"), "A",
                NULL, GIMP_HELP_TOOL_AIRBRUSH,
                GIMP_ICON_TOOL_AIRBRUSH,
                data);
}

static void
gimp_airbrush_tool_class_init (GimpAirbrushToolClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = gimp_airbrush_tool_constructed;
}

static void
gimp_airbrush_tool_init (GimpAirbrushTool *airbrush)
{
  GimpTool *tool = GIMP_TOOL (airbrush);

  gimp_tool_control_set_tool_cursor (tool->control, GIMP_TOOL_CURSOR_AIRBRUSH);
}

static void
gimp_airbrush_tool_constructed (GObject *object)
{
  GimpPaintTool *paint_tool = GIMP_PAINT_TOOL (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  g_signal_connect_object (paint_tool->core, "stamp",
                           G_CALLBACK (gimp_airbrush_tool_airbrush_stamp),
                           object, 0);
}

static void
gimp_airbrush_tool_airbrush_stamp (GimpAirbrush     *airbrush,
                                   GimpAirbrushTool *airbrush_tool)
{
  GimpPaintTool *paint_tool = GIMP_PAINT_TOOL (airbrush_tool);

  gimp_paint_tool_paint_push (
    paint_tool,
    (GimpPaintToolPaintFunc) gimp_airbrush_tool_stamp,
    NULL);
}

static void
gimp_airbrush_tool_stamp (GimpAirbrushTool *airbrush_tool,
                          gpointer          data)
{
  GimpPaintTool *paint_tool = GIMP_PAINT_TOOL (airbrush_tool);

  gimp_airbrush_stamp (GIMP_AIRBRUSH (paint_tool->core));
}


/*  tool options stuff  */

static GtkWidget *
gimp_airbrush_options_gui (GimpToolOptions *tool_options)
{
  GObject   *config = G_OBJECT (tool_options);
  GtkWidget *vbox   = gimp_paint_options_gui (tool_options);
  GtkWidget *button;
  GtkWidget *scale;

  button = gimp_prop_check_button_new (config, "motion-only", NULL);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  scale = gimp_prop_spin_scale_new (config, "rate", NULL,
                                    1.0, 10.0, 1);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  scale = gimp_prop_spin_scale_new (config, "flow", NULL,
                                    1.0, 10.0, 1);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  return vbox;
}
