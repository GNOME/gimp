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

#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "paint/gimpinkoptions.h"

#include "widgets/gimphelp-ids.h"

#include "gimpinkoptions-gui.h"
#include "gimpinktool.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


G_DEFINE_TYPE (GimpInkTool, gimp_ink_tool, GIMP_TYPE_PAINT_TOOL)

#define parent_class gimp_ink_tool_parent_class


void
gimp_ink_tool_register (GimpToolRegisterCallback  callback,
                        gpointer                  data)
{
  (* callback) (GIMP_TYPE_INK_TOOL,
                GIMP_TYPE_INK_OPTIONS,
                gimp_ink_options_gui,
                GIMP_CONTEXT_FOREGROUND_MASK |
                GIMP_CONTEXT_BACKGROUND_MASK |
                GIMP_CONTEXT_OPACITY_MASK    |
                GIMP_CONTEXT_PAINT_MODE_MASK,
                "gimp-ink-tool",
                _("Ink"),
                _("Ink Tool: Calligraphy-style painting"),
                N_("In_k"), "K",
                NULL, GIMP_HELP_TOOL_INK,
                GIMP_STOCK_TOOL_INK,
                data);
}

static void
gimp_ink_tool_class_init (GimpInkToolClass *klass)
{
}

static void
gimp_ink_tool_init (GimpInkTool *ink_tool)
{
  GimpTool *tool = GIMP_TOOL (ink_tool);

  gimp_tool_control_set_tool_cursor (tool->control, GIMP_TOOL_CURSOR_INK);
  gimp_tool_control_set_action_value_2 (tool->control,
                                        "tools/tools-ink-blob-size-set");
  gimp_tool_control_set_action_value_3 (tool->control,
                                        "tools/tools-ink-blob-aspect-set");
  gimp_tool_control_set_action_value_4 (tool->control,
                                        "tools/tools-ink-blob-angle-set");

  gimp_paint_tool_enable_color_picker (GIMP_PAINT_TOOL (ink_tool),
                                       GIMP_COLOR_PICK_MODE_FOREGROUND);
}
