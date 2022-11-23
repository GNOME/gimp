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

#include "paint/ligmaclone.h"
#include "paint/ligmacloneoptions.h"

#include "widgets/ligmahelp-ids.h"

#include "display/ligmadisplay.h"

#include "ligmaclonetool.h"
#include "ligmacloneoptions-gui.h"
#include "ligmatoolcontrol.h"

#include "ligma-intl.h"


static gboolean   ligma_clone_tool_is_alpha_only (LigmaPaintTool *paint_tool,
                                                 LigmaDrawable  *drawable);


G_DEFINE_TYPE (LigmaCloneTool, ligma_clone_tool, LIGMA_TYPE_SOURCE_TOOL)

#define parent_class ligma_clone_tool_parent_class


void
ligma_clone_tool_register (LigmaToolRegisterCallback  callback,
                          gpointer                  data)
{
  (* callback) (LIGMA_TYPE_CLONE_TOOL,
                LIGMA_TYPE_CLONE_OPTIONS,
                ligma_clone_options_gui,
                LIGMA_PAINT_OPTIONS_CONTEXT_MASK |
                LIGMA_CONTEXT_PROP_MASK_PATTERN,
                "ligma-clone-tool",
                _("Clone"),
                _("Clone Tool: Selectively copy from an image or pattern, using a brush"),
                N_("_Clone"), "C",
                NULL, LIGMA_HELP_TOOL_CLONE,
                LIGMA_ICON_TOOL_CLONE,
                data);
}

static void
ligma_clone_tool_class_init (LigmaCloneToolClass *klass)
{
  LigmaPaintToolClass *paint_tool_class = LIGMA_PAINT_TOOL_CLASS (klass);

  paint_tool_class->is_alpha_only = ligma_clone_tool_is_alpha_only;
}

static void
ligma_clone_tool_init (LigmaCloneTool *clone)
{
  LigmaTool       *tool        = LIGMA_TOOL (clone);
  LigmaPaintTool  *paint_tool  = LIGMA_PAINT_TOOL (tool);
  LigmaSourceTool *source_tool = LIGMA_SOURCE_TOOL (tool);

  ligma_tool_control_set_tool_cursor     (tool->control,
                                         LIGMA_TOOL_CURSOR_CLONE);
  ligma_tool_control_set_action_object_2 (tool->control,
                                         "context/context-pattern-select-set");

  paint_tool->status      = _("Click to clone");
  paint_tool->status_ctrl = _("%s to set a new clone source");

  source_tool->status_paint           = _("Click to clone");
  /* Translators: the translation of "Click" must be the first word */
  source_tool->status_set_source      = _("Click to set a new clone source");
  source_tool->status_set_source_ctrl = _("%s to set a new clone source");
}

static gboolean
ligma_clone_tool_is_alpha_only (LigmaPaintTool *paint_tool,
                               LigmaDrawable  *drawable)
{
  LigmaPaintOptions *paint_options = LIGMA_PAINT_TOOL_GET_OPTIONS (paint_tool);
  LigmaContext      *context       = LIGMA_CONTEXT (paint_options);
  LigmaLayerMode     paint_mode    = ligma_context_get_paint_mode (context);

  return ligma_layer_mode_is_alpha_only (paint_mode);
}
