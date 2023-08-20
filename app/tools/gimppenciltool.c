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

#include "paint/gimppenciloptions.h"

#include "widgets/gimphelp-ids.h"

#include "gimppenciltool.h"
#include "gimppaintoptions-gui.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


G_DEFINE_TYPE (GimpPencilTool, gimp_pencil_tool, GIMP_TYPE_PAINTBRUSH_TOOL)


void
gimp_pencil_tool_register (GimpToolRegisterCallback  callback,
                           gpointer                  data)
{
  (* callback) (GIMP_TYPE_PENCIL_TOOL,
                GIMP_TYPE_PENCIL_OPTIONS,
                gimp_paint_options_gui,
                GIMP_PAINT_OPTIONS_CONTEXT_MASK |
                GIMP_CONTEXT_PROP_MASK_EXPAND   |
                GIMP_CONTEXT_PROP_MASK_PATTERN  |
                GIMP_CONTEXT_PROP_MASK_GRADIENT,
                "gimp-pencil-tool",
                _("Pencil"),
                _("Pencil Tool: Hard edge painting using a brush"),
                N_("Pe_ncil"), "N",
                NULL, GIMP_HELP_TOOL_PENCIL,
                GIMP_ICON_TOOL_PENCIL,
                data);
}

static void
gimp_pencil_tool_class_init (GimpPencilToolClass *klass)
{
}

static void
gimp_pencil_tool_init (GimpPencilTool *pencil)
{
  GimpTool *tool = GIMP_TOOL (pencil);

  gimp_tool_control_set_tool_cursor (tool->control, GIMP_TOOL_CURSOR_PENCIL);
}
