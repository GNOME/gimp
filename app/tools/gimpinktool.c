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

#include "paint/ligmainkoptions.h"

#include "widgets/ligmahelp-ids.h"

#include "ligmainkoptions-gui.h"
#include "ligmainktool.h"
#include "ligmatoolcontrol.h"

#include "ligma-intl.h"


static LigmaCanvasItem * ligma_ink_tool_get_outline    (LigmaPaintTool    *paint_tool,
                                                      LigmaDisplay      *display,
                                                      gdouble           x,
                                                      gdouble           y);
static gboolean         ligma_ink_tool_is_alpha_only  (LigmaPaintTool    *paint_tool,
                                                      LigmaDrawable     *drawable);

static void             ligma_ink_tool_options_notify (LigmaTool         *tool,
                                                      LigmaToolOptions  *options,
                                                      const GParamSpec *pspec);


G_DEFINE_TYPE (LigmaInkTool, ligma_ink_tool, LIGMA_TYPE_PAINT_TOOL)

#define parent_class ligma_ink_tool_parent_class


void
ligma_ink_tool_register (LigmaToolRegisterCallback  callback,
                        gpointer                  data)
{
  (* callback) (LIGMA_TYPE_INK_TOOL,
                LIGMA_TYPE_INK_OPTIONS,
                ligma_ink_options_gui,
                LIGMA_CONTEXT_PROP_MASK_FOREGROUND |
                LIGMA_CONTEXT_PROP_MASK_BACKGROUND |
                LIGMA_CONTEXT_PROP_MASK_OPACITY    |
                LIGMA_CONTEXT_PROP_MASK_PAINT_MODE,
                "ligma-ink-tool",
                _("Ink"),
                _("Ink Tool: Calligraphy-style painting"),
                N_("In_k"), "K",
                NULL, LIGMA_HELP_TOOL_INK,
                LIGMA_ICON_TOOL_INK,
                data);
}

static void
ligma_ink_tool_class_init (LigmaInkToolClass *klass)
{
  LigmaToolClass      *tool_class       = LIGMA_TOOL_CLASS (klass);
  LigmaPaintToolClass *paint_tool_class = LIGMA_PAINT_TOOL_CLASS (klass);

  tool_class->options_notify    = ligma_ink_tool_options_notify;

  paint_tool_class->get_outline   = ligma_ink_tool_get_outline;
  paint_tool_class->is_alpha_only = ligma_ink_tool_is_alpha_only;
}

static void
ligma_ink_tool_init (LigmaInkTool *ink_tool)
{
  LigmaTool *tool = LIGMA_TOOL (ink_tool);

  ligma_tool_control_set_tool_cursor   (tool->control, LIGMA_TOOL_CURSOR_INK);
  ligma_tool_control_set_action_pixel_size (tool->control,
                                           "tools/tools-ink-blob-pixel-size-set");
  ligma_tool_control_set_action_size       (tool->control,
                                           "tools/tools-ink-blob-size-set");
  ligma_tool_control_set_action_aspect     (tool->control,
                                           "tools/tools-ink-blob-aspect-set");
  ligma_tool_control_set_action_angle      (tool->control,
                                           "tools/tools-ink-blob-angle-set");

  ligma_paint_tool_enable_color_picker (LIGMA_PAINT_TOOL (ink_tool),
                                       LIGMA_COLOR_PICK_TARGET_FOREGROUND);
}

static void
ligma_ink_tool_options_notify (LigmaTool         *tool,
                              LigmaToolOptions  *options,
                              const GParamSpec *pspec)
{
  LIGMA_TOOL_CLASS (parent_class)->options_notify (tool, options, pspec);

  if (g_strcmp0 (pspec->name, "size") == 0 &&
      LIGMA_PAINT_TOOL (tool)->draw_brush)
    {
      /* This triggers a redraw of the tool pointer, especially useful
       * here when we change the pen size with on-canvas interaction.
       */
      ligma_draw_tool_pause (LIGMA_DRAW_TOOL (tool));
      ligma_draw_tool_resume (LIGMA_DRAW_TOOL (tool));
    }
}

static LigmaCanvasItem *
ligma_ink_tool_get_outline (LigmaPaintTool *paint_tool,
                           LigmaDisplay   *display,
                           gdouble        x,
                           gdouble        y)
{
  LigmaInkOptions *options = LIGMA_INK_TOOL_GET_OPTIONS (paint_tool);

  ligma_paint_tool_set_draw_circle (paint_tool, TRUE,
                                   options->size);

  return NULL;
}

static gboolean
ligma_ink_tool_is_alpha_only (LigmaPaintTool *paint_tool,
                             LigmaDrawable  *drawable)
{
  LigmaPaintOptions *paint_options = LIGMA_PAINT_TOOL_GET_OPTIONS (paint_tool);
  LigmaContext      *context       = LIGMA_CONTEXT (paint_options);
  LigmaLayerMode     paint_mode    = ligma_context_get_paint_mode (context);

  return ligma_layer_mode_is_alpha_only (paint_mode);
}
