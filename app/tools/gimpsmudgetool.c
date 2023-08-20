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

#include "paint/gimpsmudgeoptions.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimppropwidgets.h"

#include "gimpsmudgetool.h"
#include "gimppaintoptions-gui.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


static GtkWidget * gimp_smudge_options_gui (GimpToolOptions *tool_options);


G_DEFINE_TYPE (GimpSmudgeTool, gimp_smudge_tool, GIMP_TYPE_BRUSH_TOOL)


void
gimp_smudge_tool_register (GimpToolRegisterCallback  callback,
                           gpointer                  data)
{
  (* callback) (GIMP_TYPE_SMUDGE_TOOL,
                GIMP_TYPE_SMUDGE_OPTIONS,
                gimp_smudge_options_gui,
                GIMP_PAINT_OPTIONS_CONTEXT_MASK |
                GIMP_CONTEXT_PROP_MASK_EXPAND   |
                GIMP_CONTEXT_PROP_MASK_PATTERN  |
                GIMP_CONTEXT_PROP_MASK_GRADIENT,
                "gimp-smudge-tool",
                _("Smudge"),
                _("Smudge Tool: Smudge selectively using a brush"),
                N_("_Smudge"), "S",
                NULL, GIMP_HELP_TOOL_SMUDGE,
                GIMP_ICON_TOOL_SMUDGE,
                data);
}

static void
gimp_smudge_tool_class_init (GimpSmudgeToolClass *klass)
{
}

static void
gimp_smudge_tool_init (GimpSmudgeTool *smudge)
{
  GimpTool      *tool       = GIMP_TOOL (smudge);
  GimpPaintTool *paint_tool = GIMP_PAINT_TOOL (smudge);

  gimp_tool_control_set_tool_cursor (tool->control, GIMP_TOOL_CURSOR_SMUDGE);

  gimp_paint_tool_enable_color_picker (GIMP_PAINT_TOOL (smudge),
                                       GIMP_COLOR_PICK_TARGET_FOREGROUND);

  paint_tool->status      = _("Click to smudge");
  paint_tool->status_line = _("Click to smudge the line");
  paint_tool->status_ctrl = NULL;
}


/*  tool options stuff  */

static GtkWidget *
gimp_smudge_options_gui (GimpToolOptions *tool_options)
{
  GObject   *config = G_OBJECT (tool_options);
  GtkWidget *vbox   = gimp_paint_options_gui (tool_options);
  GtkWidget *scale;
  GtkWidget *button;

  button = gimp_prop_check_button_new (config, "no-erasing", NULL);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

  button = gimp_prop_check_button_new (config, "sample-merged", NULL);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

  /*  the rate scale  */
  scale = gimp_prop_spin_scale_new (config, "rate",
                                    1.0, 10.0, 1);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);

  scale = gimp_prop_spin_scale_new (config, "flow",
                                    1.0, 10.0, 1);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);

  return vbox;
}
