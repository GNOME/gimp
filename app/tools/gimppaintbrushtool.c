/* The GIMP -- an image manipulation program
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

#include "paint/gimppaintoptions.h"

#include "widgets/gimphelp-ids.h"

#include "gimppaintbrushtool.h"
#include "gimppaintoptions-gui.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


G_DEFINE_TYPE (GimpPaintbrushTool, gimp_paintbrush_tool, GIMP_TYPE_BRUSH_TOOL)


void
gimp_paintbrush_tool_register (GimpToolRegisterCallback  callback,
                               gpointer                  data)
{
  (* callback) (GIMP_TYPE_PAINTBRUSH_TOOL,
                GIMP_TYPE_PAINT_OPTIONS,
                gimp_paint_options_gui,
                GIMP_PAINT_OPTIONS_CONTEXT_MASK |
                GIMP_CONTEXT_GRADIENT_MASK,
                "gimp-paintbrush-tool",
                _("Paintbrush"),
                _("Paint fuzzy brush strokes"),
                N_("_Paintbrush"), "P",
                NULL, GIMP_HELP_TOOL_PAINTBRUSH,
                GIMP_STOCK_TOOL_PAINTBRUSH,
                data);
}

static void
gimp_paintbrush_tool_class_init (GimpPaintbrushToolClass *klass)
{
}

static void
gimp_paintbrush_tool_init (GimpPaintbrushTool *paintbrush)
{
  GimpTool *tool = GIMP_TOOL (paintbrush);

  gimp_tool_control_set_tool_cursor (tool->control,
                                     GIMP_TOOL_CURSOR_PAINTBRUSH);

  gimp_paint_tool_enable_color_picker (GIMP_PAINT_TOOL (paintbrush),
                                       GIMP_COLOR_PICK_MODE_FOREGROUND);
}
