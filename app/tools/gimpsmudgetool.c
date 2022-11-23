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

#include "paint/ligmasmudgeoptions.h"

#include "widgets/ligmahelp-ids.h"
#include "widgets/ligmapropwidgets.h"

#include "ligmasmudgetool.h"
#include "ligmapaintoptions-gui.h"
#include "ligmatoolcontrol.h"

#include "ligma-intl.h"


static GtkWidget * ligma_smudge_options_gui (LigmaToolOptions *tool_options);


G_DEFINE_TYPE (LigmaSmudgeTool, ligma_smudge_tool, LIGMA_TYPE_BRUSH_TOOL)


void
ligma_smudge_tool_register (LigmaToolRegisterCallback  callback,
                           gpointer                  data)
{
  (* callback) (LIGMA_TYPE_SMUDGE_TOOL,
                LIGMA_TYPE_SMUDGE_OPTIONS,
                ligma_smudge_options_gui,
                LIGMA_PAINT_OPTIONS_CONTEXT_MASK |
                LIGMA_CONTEXT_PROP_MASK_GRADIENT,
                "ligma-smudge-tool",
                _("Smudge"),
                _("Smudge Tool: Smudge selectively using a brush"),
                N_("_Smudge"), "S",
                NULL, LIGMA_HELP_TOOL_SMUDGE,
                LIGMA_ICON_TOOL_SMUDGE,
                data);
}

static void
ligma_smudge_tool_class_init (LigmaSmudgeToolClass *klass)
{
}

static void
ligma_smudge_tool_init (LigmaSmudgeTool *smudge)
{
  LigmaTool      *tool       = LIGMA_TOOL (smudge);
  LigmaPaintTool *paint_tool = LIGMA_PAINT_TOOL (smudge);

  ligma_tool_control_set_tool_cursor (tool->control, LIGMA_TOOL_CURSOR_SMUDGE);

  ligma_paint_tool_enable_color_picker (LIGMA_PAINT_TOOL (smudge),
                                       LIGMA_COLOR_PICK_TARGET_FOREGROUND);

  paint_tool->status      = _("Click to smudge");
  paint_tool->status_line = _("Click to smudge the line");
  paint_tool->status_ctrl = NULL;
}


/*  tool options stuff  */

static GtkWidget *
ligma_smudge_options_gui (LigmaToolOptions *tool_options)
{
  GObject   *config = G_OBJECT (tool_options);
  GtkWidget *vbox   = ligma_paint_options_gui (tool_options);
  GtkWidget *scale;
  GtkWidget *button;

  button = ligma_prop_check_button_new (config, "no-erasing", NULL);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

  button = ligma_prop_check_button_new (config, "sample-merged", NULL);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

  /*  the rate scale  */
  scale = ligma_prop_spin_scale_new (config, "rate",
                                    1.0, 10.0, 1);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);

  scale = ligma_prop_spin_scale_new (config, "flow",
                                    1.0, 10.0, 1);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);

  return vbox;
}
