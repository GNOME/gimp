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

#include "operations/layer-modes/ligma-layer-modes.h"

#include "paint/ligmapaintoptions.h"

#include "widgets/ligmahelp-ids.h"

#include "ligmapaintbrushtool.h"
#include "ligmapaintoptions-gui.h"
#include "ligmatoolcontrol.h"

#include "ligma-intl.h"


static gboolean   ligma_paintbrush_tool_is_alpha_only (LigmaPaintTool *paint_tool,
                                                      LigmaDrawable  *drawable);


G_DEFINE_TYPE (LigmaPaintbrushTool, ligma_paintbrush_tool, LIGMA_TYPE_BRUSH_TOOL)


void
ligma_paintbrush_tool_register (LigmaToolRegisterCallback  callback,
                               gpointer                  data)
{
  (* callback) (LIGMA_TYPE_PAINTBRUSH_TOOL,
                LIGMA_TYPE_PAINT_OPTIONS,
                ligma_paint_options_gui,
                LIGMA_PAINT_OPTIONS_CONTEXT_MASK |
                LIGMA_CONTEXT_PROP_MASK_GRADIENT,
                "ligma-paintbrush-tool",
                _("Paintbrush"),
                _("Paintbrush Tool: Paint smooth strokes using a brush"),
                N_("_Paintbrush"), "P",
                NULL, LIGMA_HELP_TOOL_PAINTBRUSH,
                LIGMA_ICON_TOOL_PAINTBRUSH,
                data);
}

static void
ligma_paintbrush_tool_class_init (LigmaPaintbrushToolClass *klass)
{
  LigmaPaintToolClass *paint_tool_class = LIGMA_PAINT_TOOL_CLASS (klass);

  paint_tool_class->is_alpha_only = ligma_paintbrush_tool_is_alpha_only;
}

static void
ligma_paintbrush_tool_init (LigmaPaintbrushTool *paintbrush)
{
  LigmaTool *tool = LIGMA_TOOL (paintbrush);

  ligma_tool_control_set_tool_cursor (tool->control,
                                     LIGMA_TOOL_CURSOR_PAINTBRUSH);

  ligma_paint_tool_enable_color_picker (LIGMA_PAINT_TOOL (paintbrush),
                                       LIGMA_COLOR_PICK_TARGET_FOREGROUND);
}

static gboolean
ligma_paintbrush_tool_is_alpha_only (LigmaPaintTool *paint_tool,
                                    LigmaDrawable  *drawable)
{
  LigmaPaintOptions *paint_options = LIGMA_PAINT_TOOL_GET_OPTIONS (paint_tool);
  LigmaContext      *context       = LIGMA_CONTEXT (paint_options);
  LigmaLayerMode     paint_mode    = ligma_context_get_paint_mode (context);

  return ligma_layer_mode_is_alpha_only (paint_mode);
}
