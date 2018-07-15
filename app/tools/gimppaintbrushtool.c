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

#include "operations/layer-modes/gimp-layer-modes.h"

#include "paint/gimppaintoptions.h"

#include "widgets/gimphelp-ids.h"

#include "gimppaintbrushtool.h"
#include "gimppaintoptions-gui.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


static gboolean   gimp_paintbrush_tool_is_alpha_only (GimpPaintTool *paint_tool,
                                                      GimpDrawable  *drawable);


G_DEFINE_TYPE (GimpPaintbrushTool, gimp_paintbrush_tool, GIMP_TYPE_BRUSH_TOOL)


void
gimp_paintbrush_tool_register (GimpToolRegisterCallback  callback,
                               gpointer                  data)
{
  (* callback) (GIMP_TYPE_PAINTBRUSH_TOOL,
                GIMP_TYPE_PAINT_OPTIONS,
                gimp_paint_options_gui,
                GIMP_PAINT_OPTIONS_CONTEXT_MASK |
                GIMP_CONTEXT_PROP_MASK_GRADIENT,
                "gimp-paintbrush-tool",
                _("Paintbrush"),
                _("Paintbrush Tool: Paint smooth strokes using a brush"),
                N_("_Paintbrush"), "P",
                NULL, GIMP_HELP_TOOL_PAINTBRUSH,
                GIMP_ICON_TOOL_PAINTBRUSH,
                data);
}

static void
gimp_paintbrush_tool_class_init (GimpPaintbrushToolClass *klass)
{
  GimpPaintToolClass *paint_tool_class = GIMP_PAINT_TOOL_CLASS (klass);

  paint_tool_class->is_alpha_only = gimp_paintbrush_tool_is_alpha_only;
}

static void
gimp_paintbrush_tool_init (GimpPaintbrushTool *paintbrush)
{
  GimpTool *tool = GIMP_TOOL (paintbrush);

  gimp_tool_control_set_tool_cursor (tool->control,
                                     GIMP_TOOL_CURSOR_PAINTBRUSH);

  gimp_paint_tool_enable_color_picker (GIMP_PAINT_TOOL (paintbrush),
                                       GIMP_COLOR_PICK_TARGET_FOREGROUND);
}

static gboolean
gimp_paintbrush_tool_is_alpha_only (GimpPaintTool *paint_tool,
                                    GimpDrawable  *drawable)
{
  GimpPaintOptions *paint_options = GIMP_PAINT_TOOL_GET_OPTIONS (paint_tool);
  GimpContext      *context       = GIMP_CONTEXT (paint_options);
  GimpLayerMode     paint_mode    = gimp_context_get_paint_mode (context);

  return gimp_layer_mode_is_alpha_only (paint_mode);
}
