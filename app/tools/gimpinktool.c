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

#include "paint/gimpinkoptions.h"

#include "widgets/gimphelp-ids.h"

#include "gimpinkoptions-gui.h"
#include "gimpinktool.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


static GimpCanvasItem * gimp_ink_tool_get_outline    (GimpPaintTool    *paint_tool,
                                                      GimpDisplay      *display,
                                                      gdouble           x,
                                                      gdouble           y);
static gboolean         gimp_ink_tool_is_alpha_only  (GimpPaintTool    *paint_tool,
                                                      GimpDrawable     *drawable);

static void             gimp_ink_tool_options_notify (GimpTool         *tool,
                                                      GimpToolOptions  *options,
                                                      const GParamSpec *pspec);


G_DEFINE_TYPE (GimpInkTool, gimp_ink_tool, GIMP_TYPE_PAINT_TOOL)

#define parent_class gimp_ink_tool_parent_class


void
gimp_ink_tool_register (GimpToolRegisterCallback  callback,
                        gpointer                  data)
{
  (* callback) (GIMP_TYPE_INK_TOOL,
                GIMP_TYPE_INK_OPTIONS,
                gimp_ink_options_gui,
                GIMP_CONTEXT_PROP_MASK_FOREGROUND |
                GIMP_CONTEXT_PROP_MASK_BACKGROUND |
                GIMP_CONTEXT_PROP_MASK_OPACITY    |
                GIMP_CONTEXT_PROP_MASK_PAINT_MODE |
                GIMP_CONTEXT_PROP_MASK_PATTERN    |
                GIMP_CONTEXT_PROP_MASK_EXPAND,
                "gimp-ink-tool",
                _("Ink"),
                _("Ink Tool: Calligraphy-style painting"),
                N_("In_k"), "K",
                NULL, GIMP_HELP_TOOL_INK,
                GIMP_ICON_TOOL_INK,
                data);
}

static void
gimp_ink_tool_class_init (GimpInkToolClass *klass)
{
  GimpToolClass      *tool_class       = GIMP_TOOL_CLASS (klass);
  GimpPaintToolClass *paint_tool_class = GIMP_PAINT_TOOL_CLASS (klass);

  tool_class->options_notify    = gimp_ink_tool_options_notify;

  paint_tool_class->get_outline   = gimp_ink_tool_get_outline;
  paint_tool_class->is_alpha_only = gimp_ink_tool_is_alpha_only;
}

static void
gimp_ink_tool_init (GimpInkTool *ink_tool)
{
  GimpTool *tool = GIMP_TOOL (ink_tool);

  gimp_tool_control_set_tool_cursor   (tool->control, GIMP_TOOL_CURSOR_INK);
  gimp_tool_control_set_action_pixel_size (tool->control,
                                           "tools-ink-blob-pixel-size-set");
  gimp_tool_control_set_action_size       (tool->control,
                                           "tools-ink-blob-size-set");
  gimp_tool_control_set_action_aspect     (tool->control,
                                           "tools-ink-blob-aspect-set");
  gimp_tool_control_set_action_angle      (tool->control,
                                           "tools-ink-blob-angle-set");

  gimp_paint_tool_enable_color_picker (GIMP_PAINT_TOOL (ink_tool),
                                       GIMP_COLOR_PICK_TARGET_FOREGROUND);
}

static void
gimp_ink_tool_options_notify (GimpTool         *tool,
                              GimpToolOptions  *options,
                              const GParamSpec *pspec)
{
  GIMP_TOOL_CLASS (parent_class)->options_notify (tool, options, pspec);

  if (g_strcmp0 (pspec->name, "size") == 0 &&
      GIMP_PAINT_TOOL (tool)->draw_brush)
    {
      /* This triggers a redraw of the tool pointer, especially useful
       * here when we change the pen size with on-canvas interaction.
       */
      gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));
      gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
    }
}

static GimpCanvasItem *
gimp_ink_tool_get_outline (GimpPaintTool *paint_tool,
                           GimpDisplay   *display,
                           gdouble        x,
                           gdouble        y)
{
  GimpInkOptions *options = GIMP_INK_TOOL_GET_OPTIONS (paint_tool);

  gimp_paint_tool_set_draw_circle (paint_tool, TRUE,
                                   options->size);

  return NULL;
}

static gboolean
gimp_ink_tool_is_alpha_only (GimpPaintTool *paint_tool,
                             GimpDrawable  *drawable)
{
  GimpPaintOptions *paint_options = GIMP_PAINT_TOOL_GET_OPTIONS (paint_tool);
  GimpContext      *context       = GIMP_CONTEXT (paint_options);
  GimpLayerMode     paint_mode    = gimp_context_get_paint_mode (context);

  return gimp_layer_mode_is_alpha_only (paint_mode);
}
