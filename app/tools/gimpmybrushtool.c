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

#include "libligmamath/ligmamath.h"
#include "libligmawidgets/ligmawidgets.h"

#include "tools-types.h"

#include "paint/ligmamybrushoptions.h"

#include "display/ligmadisplay.h"
#include "display/ligmadisplayshell.h"
#include "display/ligmacanvasarc.h"

#include "core/ligma.h"

#include "widgets/ligmahelp-ids.h"

#include "ligmamybrushoptions-gui.h"
#include "ligmamybrushtool.h"
#include "ligmatoolcontrol.h"
#include "core/ligmamybrush.h"

#include "ligma-intl.h"

G_DEFINE_TYPE (LigmaMybrushTool, ligma_mybrush_tool, LIGMA_TYPE_PAINT_TOOL)

#define parent_class ligma_mybrush_tool_parent_class


static void   ligma_mybrush_tool_options_notify        (LigmaTool         *tool,
                                                       LigmaToolOptions  *options,
                                                       const GParamSpec *pspec);

static LigmaCanvasItem * ligma_mybrush_tool_get_outline (LigmaPaintTool *paint_tool,
                                                       LigmaDisplay   *display,
                                                       gdouble        x,
                                                       gdouble        y);


void
ligma_mybrush_tool_register (LigmaToolRegisterCallback  callback,
                            gpointer                  data)
{
  (* callback) (LIGMA_TYPE_MYBRUSH_TOOL,
                LIGMA_TYPE_MYBRUSH_OPTIONS,
                ligma_mybrush_options_gui,
                LIGMA_CONTEXT_PROP_MASK_FOREGROUND |
                LIGMA_CONTEXT_PROP_MASK_BACKGROUND |
                LIGMA_CONTEXT_PROP_MASK_OPACITY    |
                LIGMA_CONTEXT_PROP_MASK_PAINT_MODE |
                LIGMA_CONTEXT_PROP_MASK_MYBRUSH,
                "ligma-mypaint-brush-tool",
                _("MyPaint Brush"),
                _("MyPaint Brush Tool: Use MyPaint brushes in LIGMA"),
                N_("M_yPaint Brush"), "Y",
                NULL, LIGMA_HELP_TOOL_MYPAINT_BRUSH,
                LIGMA_ICON_TOOL_MYPAINT_BRUSH,
                data);
}

static void
ligma_mybrush_tool_class_init (LigmaMybrushToolClass *klass)
{
  LigmaToolClass      *tool_class       = LIGMA_TOOL_CLASS (klass);
  LigmaPaintToolClass *paint_tool_class = LIGMA_PAINT_TOOL_CLASS (klass);

  tool_class->options_notify    = ligma_mybrush_tool_options_notify;

  paint_tool_class->get_outline = ligma_mybrush_tool_get_outline;
}

static void
ligma_mybrush_tool_init (LigmaMybrushTool *mybrush_tool)
{
  LigmaTool *tool = LIGMA_TOOL (mybrush_tool);

  ligma_tool_control_set_tool_cursor       (tool->control, LIGMA_TOOL_CURSOR_INK);
  ligma_tool_control_set_action_size       (tool->control,
                                           "tools/tools-mypaint-brush-radius-set");
  ligma_tool_control_set_action_pixel_size (tool->control,
                                           "tools/tools-mypaint-brush-pixel-size-set");
  ligma_tool_control_set_action_hardness   (tool->control,
                                           "tools/tools-mypaint-brush-hardness-set");

  ligma_paint_tool_enable_color_picker (LIGMA_PAINT_TOOL (mybrush_tool),
                                       LIGMA_COLOR_PICK_TARGET_FOREGROUND);
}

static void
ligma_mybrush_tool_options_notify (LigmaTool         *tool,
                                  LigmaToolOptions  *options,
                                  const GParamSpec *pspec)
{
  LIGMA_TOOL_CLASS (parent_class)->options_notify (tool, options, pspec);

  if (! strcmp (pspec->name, "radius"))
    {
      ligma_draw_tool_pause (LIGMA_DRAW_TOOL (tool));
      ligma_draw_tool_resume (LIGMA_DRAW_TOOL (tool));
    }
}

static LigmaCanvasItem *
ligma_mybrush_tool_get_outline (LigmaPaintTool *paint_tool,
                               LigmaDisplay   *display,
                               gdouble        x,
                               gdouble        y)
{
  LigmaMybrushOptions *options = LIGMA_MYBRUSH_TOOL_GET_OPTIONS (paint_tool);
  LigmaCanvasItem     *item    = NULL;
  LigmaDisplayShell   *shell   = ligma_display_get_shell (display);
  gdouble             radius;

  /* Radius size as used by libmypaint is logarithmic.
   * The drawn outline in the MyBrush tool is more of a general idea of
   * the brush size. With each brush having its own logic, actual drawn
   * dabs may be quite different, smaller but also bigger. And there are
   * some random elements in there too.
   * See also mypaint-brush.c code in libmypaint.
   */
  radius = exp (options->radius);
  radius = MAX (MAX (4 / shell->scale_x, 4 / shell->scale_y), radius);

  item = ligma_mybrush_tool_create_cursor (paint_tool, display, x, y, radius);

  if (! item)
    {
      ligma_paint_tool_set_draw_fallback (paint_tool,
                                         TRUE, radius);
    }

  return item;
}

LigmaCanvasItem *
ligma_mybrush_tool_create_cursor (LigmaPaintTool *paint_tool,
                                 LigmaDisplay   *display,
                                 gdouble        x,
                                 gdouble        y,
                                 gdouble        radius)
{

  LigmaDisplayShell     *shell;

  g_return_val_if_fail (LIGMA_IS_PAINT_TOOL (paint_tool), NULL);
  g_return_val_if_fail (LIGMA_IS_DISPLAY (display), NULL);

  shell = ligma_display_get_shell (display);

  /*  don't draw the boundary if it becomes too small  */
  if (SCALEX (shell, radius) > 4 &&
      SCALEY (shell, radius) > 4)
    {
      return ligma_canvas_arc_new(shell, x, y, radius, radius, 0.0, 2 * G_PI, FALSE);
    }

  return NULL;
}
