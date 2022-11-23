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

#include "paint/ligmapenciloptions.h"

#include "widgets/ligmahelp-ids.h"

#include "ligmapenciltool.h"
#include "ligmapaintoptions-gui.h"
#include "ligmatoolcontrol.h"

#include "ligma-intl.h"


G_DEFINE_TYPE (LigmaPencilTool, ligma_pencil_tool, LIGMA_TYPE_PAINTBRUSH_TOOL)


void
ligma_pencil_tool_register (LigmaToolRegisterCallback  callback,
                           gpointer                  data)
{
  (* callback) (LIGMA_TYPE_PENCIL_TOOL,
                LIGMA_TYPE_PENCIL_OPTIONS,
                ligma_paint_options_gui,
                LIGMA_PAINT_OPTIONS_CONTEXT_MASK |
                LIGMA_CONTEXT_PROP_MASK_GRADIENT,
                "ligma-pencil-tool",
                _("Pencil"),
                _("Pencil Tool: Hard edge painting using a brush"),
                N_("Pe_ncil"), "N",
                NULL, LIGMA_HELP_TOOL_PENCIL,
                LIGMA_ICON_TOOL_PENCIL,
                data);
}

static void
ligma_pencil_tool_class_init (LigmaPencilToolClass *klass)
{
}

static void
ligma_pencil_tool_init (LigmaPencilTool *pencil)
{
  LigmaTool *tool = LIGMA_TOOL (pencil);

  ligma_tool_control_set_tool_cursor (tool->control, LIGMA_TOOL_CURSOR_PENCIL);
}
