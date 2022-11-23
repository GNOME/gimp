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

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmamath/ligmamath.h"

#include "tools-types.h"

#include "config/ligmadisplayconfig.h"

#include "core/ligma.h"
#include "core/ligmabezierdesc.h"
#include "core/ligmabrush.h"
#include "core/ligmaimage.h"
#include "core/ligmatoolinfo.h"

#include "paint/ligmabrushcore.h"
#include "paint/ligmapaintoptions.h"

#include "display/ligmacanvashandle.h"
#include "display/ligmacanvaspath.h"
#include "display/ligmadisplay.h"
#include "display/ligmadisplayshell.h"

#include "ligmabrushtool.h"
#include "ligmapainttool-paint.h"
#include "ligmatoolcontrol.h"


static void   ligma_brush_tool_constructed     (GObject           *object);

static void   ligma_brush_tool_oper_update     (LigmaTool          *tool,
                                               const LigmaCoords  *coords,
                                               GdkModifierType    state,
                                               gboolean           proximity,
                                               LigmaDisplay       *display);
static void   ligma_brush_tool_cursor_update   (LigmaTool          *tool,
                                               const LigmaCoords  *coords,
                                               GdkModifierType    state,
                                               LigmaDisplay       *display);
static void   ligma_brush_tool_options_notify  (LigmaTool          *tool,
                                               LigmaToolOptions   *options,
                                               const GParamSpec  *pspec);

static void   ligma_brush_tool_paint_start     (LigmaPaintTool     *paint_tool);
static void   ligma_brush_tool_paint_end       (LigmaPaintTool     *paint_tool);
static void   ligma_brush_tool_paint_flush     (LigmaPaintTool     *paint_tool);
static LigmaCanvasItem *
              ligma_brush_tool_get_outline     (LigmaPaintTool     *paint_tool,
                                               LigmaDisplay       *display,
                                               gdouble            x,
                                               gdouble            y);

static void   ligma_brush_tool_brush_changed   (LigmaContext       *context,
                                               LigmaBrush         *brush,
                                               LigmaBrushTool     *brush_tool);
static void   ligma_brush_tool_set_brush       (LigmaBrushCore     *brush_core,
                                               LigmaBrush         *brush,
                                               LigmaBrushTool     *brush_tool);

static const LigmaBezierDesc *
                 ligma_brush_tool_get_boundary (LigmaBrushTool     *brush_tool,
                                               gint              *width,
                                               gint              *height);


G_DEFINE_TYPE (LigmaBrushTool, ligma_brush_tool, LIGMA_TYPE_PAINT_TOOL)

#define parent_class ligma_brush_tool_parent_class


static void
ligma_brush_tool_class_init (LigmaBrushToolClass *klass)
{
  GObjectClass       *object_class     = G_OBJECT_CLASS (klass);
  LigmaToolClass      *tool_class       = LIGMA_TOOL_CLASS (klass);
  LigmaPaintToolClass *paint_tool_class = LIGMA_PAINT_TOOL_CLASS (klass);

  object_class->constructed     = ligma_brush_tool_constructed;

  tool_class->oper_update       = ligma_brush_tool_oper_update;
  tool_class->cursor_update     = ligma_brush_tool_cursor_update;
  tool_class->options_notify    = ligma_brush_tool_options_notify;

  paint_tool_class->paint_start = ligma_brush_tool_paint_start;
  paint_tool_class->paint_end   = ligma_brush_tool_paint_end;
  paint_tool_class->paint_flush = ligma_brush_tool_paint_flush;
  paint_tool_class->get_outline = ligma_brush_tool_get_outline;
}

static void
ligma_brush_tool_init (LigmaBrushTool *brush_tool)
{
  LigmaTool *tool = LIGMA_TOOL (brush_tool);

  ligma_tool_control_set_action_pixel_size (tool->control,
                                           "tools/tools-paintbrush-pixel-size-set");
  ligma_tool_control_set_action_size       (tool->control,
                                           "tools/tools-paintbrush-size-set");
  ligma_tool_control_set_action_aspect     (tool->control,
                                           "tools/tools-paintbrush-aspect-ratio-set");
  ligma_tool_control_set_action_angle      (tool->control,
                                           "tools/tools-paintbrush-angle-set");
  ligma_tool_control_set_action_spacing    (tool->control,
                                           "tools/tools-paintbrush-spacing-set");
  ligma_tool_control_set_action_hardness   (tool->control,
                                           "tools/tools-paintbrush-hardness-set");
  ligma_tool_control_set_action_force      (tool->control,
                                           "tools/tools-paintbrush-force-set");
  ligma_tool_control_set_action_object_1   (tool->control,
                                           "context/context-brush-select-set");
}

static void
ligma_brush_tool_constructed (GObject *object)
{
  LigmaTool      *tool       = LIGMA_TOOL (object);
  LigmaPaintTool *paint_tool = LIGMA_PAINT_TOOL (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  ligma_assert (LIGMA_IS_BRUSH_CORE (paint_tool->core));

  g_signal_connect_object (ligma_tool_get_options (tool), "brush-changed",
                           G_CALLBACK (ligma_brush_tool_brush_changed),
                           paint_tool, 0);

  g_signal_connect_object (paint_tool->core, "set-brush",
                           G_CALLBACK (ligma_brush_tool_set_brush),
                           paint_tool, 0);
}

static void
ligma_brush_tool_oper_update (LigmaTool         *tool,
                             const LigmaCoords *coords,
                             GdkModifierType   state,
                             gboolean          proximity,
                             LigmaDisplay      *display)
{
  LigmaPaintOptions *paint_options = LIGMA_PAINT_TOOL_GET_OPTIONS (tool);
  LigmaImage        *image         = ligma_display_get_image (display);

  ligma_draw_tool_pause (LIGMA_DRAW_TOOL (tool));

  LIGMA_TOOL_CLASS (parent_class)->oper_update (tool, coords, state,
                                               proximity, display);

  if (! ligma_color_tool_is_enabled (LIGMA_COLOR_TOOL (tool)) &&
      image && proximity)
    {
      LigmaContext   *context    = LIGMA_CONTEXT (paint_options);
      LigmaPaintTool *paint_tool = LIGMA_PAINT_TOOL (tool);
      LigmaBrushCore *brush_core = LIGMA_BRUSH_CORE (paint_tool->core);

      ligma_brush_core_set_brush (brush_core,
                                 ligma_context_get_brush (context));

      ligma_brush_core_set_dynamics (brush_core,
                                    ligma_context_get_dynamics (context));

      if (LIGMA_BRUSH_CORE_GET_CLASS (brush_core)->handles_transforming_brush)
        {
          ligma_brush_core_eval_transform_dynamics (brush_core,
                                                   image,
                                                   paint_options,
                                                   coords);
        }
    }

  ligma_draw_tool_resume (LIGMA_DRAW_TOOL (tool));
}

static void
ligma_brush_tool_cursor_update (LigmaTool         *tool,
                               const LigmaCoords *coords,
                               GdkModifierType   state,
                               LigmaDisplay      *display)
{
  LigmaBrushTool *brush_tool = LIGMA_BRUSH_TOOL (tool);
  LigmaBrushCore *brush_core = LIGMA_BRUSH_CORE (LIGMA_PAINT_TOOL (brush_tool)->core);

  if (! ligma_color_tool_is_enabled (LIGMA_COLOR_TOOL (tool)))
    {
      if (! brush_core->main_brush || ! brush_core->dynamics)
        {
          ligma_tool_set_cursor (tool, display,
                                ligma_tool_control_get_cursor (tool->control),
                                ligma_tool_control_get_tool_cursor (tool->control),
                                LIGMA_CURSOR_MODIFIER_BAD);
          return;
        }
    }

  LIGMA_TOOL_CLASS (parent_class)->cursor_update (tool,  coords, state, display);
}

static void
ligma_brush_tool_options_notify (LigmaTool         *tool,
                                LigmaToolOptions  *options,
                                const GParamSpec *pspec)
{
  LIGMA_TOOL_CLASS (parent_class)->options_notify (tool, options, pspec);

  if (! strcmp (pspec->name, "brush-size")  ||
      ! strcmp (pspec->name, "brush-angle") ||
      ! strcmp (pspec->name, "brush-aspect-ratio"))
    {
      LigmaPaintTool *paint_tool = LIGMA_PAINT_TOOL (tool);
      LigmaBrushCore *brush_core = LIGMA_BRUSH_CORE (paint_tool->core);

      g_signal_emit_by_name (brush_core, "set-brush",
                             brush_core->main_brush);
    }
}

static void
ligma_brush_tool_paint_start (LigmaPaintTool *paint_tool)
{
  LigmaBrushTool        *brush_tool = LIGMA_BRUSH_TOOL (paint_tool);
  LigmaBrushCore        *brush_core = LIGMA_BRUSH_CORE (paint_tool->core);
  const LigmaBezierDesc *boundary;

  if (LIGMA_PAINT_TOOL_CLASS (parent_class)->paint_start)
    LIGMA_PAINT_TOOL_CLASS (parent_class)->paint_start (paint_tool);

  boundary = ligma_brush_tool_get_boundary (brush_tool,
                                           &brush_tool->boundary_width,
                                           &brush_tool->boundary_height);

  if (boundary)
    brush_tool->boundary = ligma_bezier_desc_copy (boundary);

  brush_tool->boundary_scale        = brush_core->scale;
  brush_tool->boundary_aspect_ratio = brush_core->aspect_ratio;
  brush_tool->boundary_angle        = brush_core->angle;
  brush_tool->boundary_reflect      = brush_core->reflect;
  brush_tool->boundary_hardness     = brush_core->hardness;
}

static void
ligma_brush_tool_paint_end (LigmaPaintTool *paint_tool)
{
  LigmaBrushTool *brush_tool = LIGMA_BRUSH_TOOL (paint_tool);

  g_clear_pointer (&brush_tool->boundary, ligma_bezier_desc_free);

  if (LIGMA_PAINT_TOOL_CLASS (parent_class)->paint_end)
    LIGMA_PAINT_TOOL_CLASS (parent_class)->paint_end (paint_tool);
}

static void
ligma_brush_tool_paint_flush (LigmaPaintTool *paint_tool)
{
  LigmaBrushTool        *brush_tool = LIGMA_BRUSH_TOOL (paint_tool);
  LigmaBrushCore        *brush_core = LIGMA_BRUSH_CORE (paint_tool->core);
  const LigmaBezierDesc *boundary;

  if (LIGMA_PAINT_TOOL_CLASS (parent_class)->paint_flush)
    LIGMA_PAINT_TOOL_CLASS (parent_class)->paint_flush (paint_tool);

  if (brush_tool->boundary_scale        != brush_core->scale        ||
      brush_tool->boundary_aspect_ratio != brush_core->aspect_ratio ||
      brush_tool->boundary_angle        != brush_core->angle        ||
      brush_tool->boundary_reflect      != brush_core->reflect      ||
      brush_tool->boundary_hardness     != brush_core->hardness)
    {
      g_clear_pointer (&brush_tool->boundary, ligma_bezier_desc_free);

      boundary = ligma_brush_tool_get_boundary (brush_tool,
                                               &brush_tool->boundary_width,
                                               &brush_tool->boundary_height);

      if (boundary)
        brush_tool->boundary = ligma_bezier_desc_copy (boundary);

      brush_tool->boundary_scale        = brush_core->scale;
      brush_tool->boundary_aspect_ratio = brush_core->aspect_ratio;
      brush_tool->boundary_angle        = brush_core->angle;
      brush_tool->boundary_reflect      = brush_core->reflect;
      brush_tool->boundary_hardness     = brush_core->hardness;
    }
}

static LigmaCanvasItem *
ligma_brush_tool_get_outline (LigmaPaintTool *paint_tool,
                             LigmaDisplay   *display,
                             gdouble        x,
                             gdouble        y)
{
  LigmaBrushTool  *brush_tool = LIGMA_BRUSH_TOOL (paint_tool);
  LigmaCanvasItem *item;

  item = ligma_brush_tool_create_outline (brush_tool, display, x, y);

  if (! item)
    {
      LigmaBrushCore *brush_core = LIGMA_BRUSH_CORE (paint_tool->core);

      if (brush_core->main_brush && brush_core->dynamics)
        {
          /*  if an outline was expected, but got scaled away by
           *  transform/dynamics, draw a circle in the "normal" size.
           */
          LigmaPaintOptions *options;

          options = LIGMA_PAINT_TOOL_GET_OPTIONS (brush_tool);

          ligma_paint_tool_set_draw_fallback (paint_tool,
                                             TRUE, options->brush_size);
        }
    }

  return item;
}

LigmaCanvasItem *
ligma_brush_tool_create_outline (LigmaBrushTool *brush_tool,
                                LigmaDisplay   *display,
                                gdouble        x,
                                gdouble        y)
{
  LigmaTool             *tool;
  LigmaDisplayShell     *shell;
  const LigmaBezierDesc *boundary = NULL;
  gint                  width    = 0;
  gint                  height   = 0;

  g_return_val_if_fail (LIGMA_IS_BRUSH_TOOL (brush_tool), NULL);
  g_return_val_if_fail (LIGMA_IS_DISPLAY (display), NULL);

  if (ligma_paint_tool_paint_is_active (LIGMA_PAINT_TOOL (brush_tool)))
    {
      boundary = brush_tool->boundary;
      width    = brush_tool->boundary_width;
      height   = brush_tool->boundary_height;
    }
  else
    {
      boundary = ligma_brush_tool_get_boundary (brush_tool, &width, &height);
    }

  if (! boundary)
    return NULL;

  tool  = LIGMA_TOOL (brush_tool);
  shell = ligma_display_get_shell (display);

  /*  don't draw the boundary if it becomes too small  */
  if (SCALEX (shell, width)  > 4 &&
      SCALEY (shell, height) > 4)
    {
      x -= width  / 2.0;
      y -= height / 2.0;

      if (ligma_tool_control_get_precision (tool->control) ==
          LIGMA_CURSOR_PRECISION_PIXEL_CENTER)
        {
#define EPSILON 0.000001
          /*  Add EPSILON before rounding since e.g.
           *  (5.0 - 0.5) may end up at (4.499999999....)
           *  due to floating point fnords
           */
          x = RINT (x + EPSILON);
          y = RINT (y + EPSILON);
#undef EPSILON
        }

      return ligma_canvas_path_new (shell, boundary, x, y, FALSE,
                                   LIGMA_PATH_STYLE_OUTLINE);
    }

  return NULL;
}

static void
ligma_brush_tool_brush_changed (LigmaContext   *context,
                               LigmaBrush     *brush,
                               LigmaBrushTool *brush_tool)
{
  LigmaPaintTool *paint_tool = LIGMA_PAINT_TOOL (brush_tool);
  LigmaBrushCore *brush_core = LIGMA_BRUSH_CORE (paint_tool->core);

  ligma_brush_core_set_brush (brush_core, brush);

}

static void
ligma_brush_tool_set_brush (LigmaBrushCore *brush_core,
                           LigmaBrush     *brush,
                           LigmaBrushTool *brush_tool)
{
  ligma_draw_tool_pause (LIGMA_DRAW_TOOL (brush_tool));

  if (LIGMA_BRUSH_CORE_GET_CLASS (brush_core)->handles_transforming_brush)
    {
      LigmaPaintCore *paint_core = LIGMA_PAINT_CORE (brush_core);

      ligma_brush_core_eval_transform_dynamics (brush_core,
                                               NULL,
                                               LIGMA_PAINT_TOOL_GET_OPTIONS (brush_tool),
                                               &paint_core->cur_coords);
    }

  ligma_draw_tool_resume (LIGMA_DRAW_TOOL (brush_tool));
}

static const LigmaBezierDesc *
ligma_brush_tool_get_boundary (LigmaBrushTool *brush_tool,
                              gint          *width,
                              gint          *height)
{
  LigmaPaintTool *paint_tool = LIGMA_PAINT_TOOL (brush_tool);
  LigmaBrushCore *brush_core = LIGMA_BRUSH_CORE (paint_tool->core);

  if (paint_tool->draw_brush &&
      brush_core->main_brush &&
      brush_core->dynamics   &&
      brush_core->scale > 0.0)
    {
      return ligma_brush_transform_boundary (brush_core->main_brush,
                                            brush_core->scale,
                                            brush_core->aspect_ratio,
                                            brush_core->angle,
                                            brush_core->reflect,
                                            brush_core->hardness,
                                            width,
                                            height);
    }

  return NULL;
}
