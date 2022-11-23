/* LIGMA - The GNU Image Manipulation Program
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

#include "libligmawidgets/ligmawidgets.h"

#include "tools-types.h"

#include "paint/ligmaairbrush.h"
#include "paint/ligmaairbrushoptions.h"

#include "widgets/ligmahelp-ids.h"
#include "widgets/ligmapropwidgets.h"

#include "ligmaairbrushtool.h"
#include "ligmapaintoptions-gui.h"
#include "ligmapainttool-paint.h"
#include "ligmatoolcontrol.h"

#include "ligma-intl.h"


static void        ligma_airbrush_tool_constructed    (GObject          *object);

static void        ligma_airbrush_tool_airbrush_stamp (LigmaAirbrush     *airbrush,
                                                      LigmaAirbrushTool *airbrush_tool);

static void        ligma_airbrush_tool_stamp          (LigmaAirbrushTool *airbrush_tool,
                                                      gpointer          data);

static GtkWidget * ligma_airbrush_options_gui         (LigmaToolOptions  *tool_options);


G_DEFINE_TYPE (LigmaAirbrushTool, ligma_airbrush_tool, LIGMA_TYPE_PAINTBRUSH_TOOL)

#define parent_class ligma_airbrush_tool_parent_class


void
ligma_airbrush_tool_register (LigmaToolRegisterCallback  callback,
                             gpointer                  data)
{
  (* callback) (LIGMA_TYPE_AIRBRUSH_TOOL,
                LIGMA_TYPE_AIRBRUSH_OPTIONS,
                ligma_airbrush_options_gui,
                LIGMA_PAINT_OPTIONS_CONTEXT_MASK |
                LIGMA_CONTEXT_PROP_MASK_GRADIENT,
                "ligma-airbrush-tool",
                _("Airbrush"),
                _("Airbrush Tool: Paint using a brush, with variable pressure"),
                N_("_Airbrush"), "A",
                NULL, LIGMA_HELP_TOOL_AIRBRUSH,
                LIGMA_ICON_TOOL_AIRBRUSH,
                data);
}

static void
ligma_airbrush_tool_class_init (LigmaAirbrushToolClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = ligma_airbrush_tool_constructed;
}

static void
ligma_airbrush_tool_init (LigmaAirbrushTool *airbrush)
{
  LigmaTool *tool = LIGMA_TOOL (airbrush);

  ligma_tool_control_set_tool_cursor (tool->control, LIGMA_TOOL_CURSOR_AIRBRUSH);
}

static void
ligma_airbrush_tool_constructed (GObject *object)
{
  LigmaPaintTool *paint_tool = LIGMA_PAINT_TOOL (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  g_signal_connect_object (paint_tool->core, "stamp",
                           G_CALLBACK (ligma_airbrush_tool_airbrush_stamp),
                           object, 0);
}

static void
ligma_airbrush_tool_airbrush_stamp (LigmaAirbrush     *airbrush,
                                   LigmaAirbrushTool *airbrush_tool)
{
  LigmaPaintTool *paint_tool = LIGMA_PAINT_TOOL (airbrush_tool);

  ligma_paint_tool_paint_push (
    paint_tool,
    (LigmaPaintToolPaintFunc) ligma_airbrush_tool_stamp,
    NULL);
}

static void
ligma_airbrush_tool_stamp (LigmaAirbrushTool *airbrush_tool,
                          gpointer          data)
{
  LigmaPaintTool *paint_tool = LIGMA_PAINT_TOOL (airbrush_tool);

  ligma_airbrush_stamp (LIGMA_AIRBRUSH (paint_tool->core));
}


/*  tool options stuff  */

static GtkWidget *
ligma_airbrush_options_gui (LigmaToolOptions *tool_options)
{
  GObject   *config = G_OBJECT (tool_options);
  GtkWidget *vbox   = ligma_paint_options_gui (tool_options);
  GtkWidget *button;
  GtkWidget *scale;

  button = ligma_prop_check_button_new (config, "motion-only", NULL);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

  scale = ligma_prop_spin_scale_new (config, "rate",
                                    1.0, 10.0, 1);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);

  scale = ligma_prop_spin_scale_new (config, "flow",
                                    1.0, 10.0, 1);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);

  return vbox;
}
