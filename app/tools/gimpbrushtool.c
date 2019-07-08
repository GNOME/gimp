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

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"

#include "tools-types.h"

#include "config/gimpdisplayconfig.h"

#include "core/gimp.h"
#include "core/gimpbezierdesc.h"
#include "core/gimpbrush.h"
#include "core/gimpimage.h"
#include "core/gimptoolinfo.h"

#include "paint/gimpbrushcore.h"
#include "paint/gimppaintoptions.h"

#include "display/gimpcanvashandle.h"
#include "display/gimpcanvaspath.h"
#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"

#include "gimpbrushtool.h"
#include "gimppainttool-paint.h"
#include "gimptoolcontrol.h"


static void   gimp_brush_tool_constructed     (GObject           *object);

static void   gimp_brush_tool_oper_update     (GimpTool          *tool,
                                               const GimpCoords  *coords,
                                               GdkModifierType    state,
                                               gboolean           proximity,
                                               GimpDisplay       *display);
static void   gimp_brush_tool_cursor_update   (GimpTool          *tool,
                                               const GimpCoords  *coords,
                                               GdkModifierType    state,
                                               GimpDisplay       *display);
static void   gimp_brush_tool_options_notify  (GimpTool          *tool,
                                               GimpToolOptions   *options,
                                               const GParamSpec  *pspec);

static void   gimp_brush_tool_paint_start     (GimpPaintTool     *paint_tool);
static void   gimp_brush_tool_paint_end       (GimpPaintTool     *paint_tool);
static void   gimp_brush_tool_paint_flush     (GimpPaintTool     *paint_tool);
static GimpCanvasItem *
              gimp_brush_tool_get_outline     (GimpPaintTool     *paint_tool,
                                               GimpDisplay       *display,
                                               gdouble            x,
                                               gdouble            y);

static void   gimp_brush_tool_brush_changed   (GimpContext       *context,
                                               GimpBrush         *brush,
                                               GimpBrushTool     *brush_tool);
static void   gimp_brush_tool_set_brush       (GimpBrushCore     *brush_core,
                                               GimpBrush         *brush,
                                               GimpBrushTool     *brush_tool);

static const GimpBezierDesc *
                 gimp_brush_tool_get_boundary (GimpBrushTool     *brush_tool,
                                               gint              *width,
                                               gint              *height);


G_DEFINE_TYPE (GimpBrushTool, gimp_brush_tool, GIMP_TYPE_PAINT_TOOL)

#define parent_class gimp_brush_tool_parent_class


static void
gimp_brush_tool_class_init (GimpBrushToolClass *klass)
{
  GObjectClass       *object_class     = G_OBJECT_CLASS (klass);
  GimpToolClass      *tool_class       = GIMP_TOOL_CLASS (klass);
  GimpPaintToolClass *paint_tool_class = GIMP_PAINT_TOOL_CLASS (klass);

  object_class->constructed     = gimp_brush_tool_constructed;

  tool_class->oper_update       = gimp_brush_tool_oper_update;
  tool_class->cursor_update     = gimp_brush_tool_cursor_update;
  tool_class->options_notify    = gimp_brush_tool_options_notify;

  paint_tool_class->paint_start = gimp_brush_tool_paint_start;
  paint_tool_class->paint_end   = gimp_brush_tool_paint_end;
  paint_tool_class->paint_flush = gimp_brush_tool_paint_flush;
  paint_tool_class->get_outline = gimp_brush_tool_get_outline;
}

static void
gimp_brush_tool_init (GimpBrushTool *brush_tool)
{
  GimpTool *tool = GIMP_TOOL (brush_tool);

  gimp_tool_control_set_action_size     (tool->control,
                                         "tools/tools-paintbrush-size-set");
  gimp_tool_control_set_action_aspect   (tool->control,
                                         "tools/tools-paintbrush-aspect-ratio-set");
  gimp_tool_control_set_action_angle    (tool->control,
                                         "tools/tools-paintbrush-angle-set");
  gimp_tool_control_set_action_spacing  (tool->control,
                                         "tools/tools-paintbrush-spacing-set");
  gimp_tool_control_set_action_hardness (tool->control,
                                         "tools/tools-paintbrush-hardness-set");
  gimp_tool_control_set_action_force    (tool->control,
                                         "tools/tools-paintbrush-force-set");
  gimp_tool_control_set_action_object_1 (tool->control,
                                         "context/context-brush-select-set");
}

static void
gimp_brush_tool_constructed (GObject *object)
{
  GimpTool      *tool       = GIMP_TOOL (object);
  GimpPaintTool *paint_tool = GIMP_PAINT_TOOL (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gimp_assert (GIMP_IS_BRUSH_CORE (paint_tool->core));

  g_signal_connect_object (gimp_tool_get_options (tool), "brush-changed",
                           G_CALLBACK (gimp_brush_tool_brush_changed),
                           paint_tool, 0);

  g_signal_connect_object (paint_tool->core, "set-brush",
                           G_CALLBACK (gimp_brush_tool_set_brush),
                           paint_tool, 0);
}

static void
gimp_brush_tool_oper_update (GimpTool         *tool,
                             const GimpCoords *coords,
                             GdkModifierType   state,
                             gboolean          proximity,
                             GimpDisplay      *display)
{
  GimpPaintOptions *paint_options = GIMP_PAINT_TOOL_GET_OPTIONS (tool);
  GimpDrawable     *drawable;

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  GIMP_TOOL_CLASS (parent_class)->oper_update (tool, coords, state,
                                               proximity, display);

  drawable = gimp_image_get_active_drawable (gimp_display_get_image (display));

  if (! gimp_color_tool_is_enabled (GIMP_COLOR_TOOL (tool)) &&
      drawable && proximity)
    {
      GimpContext   *context    = GIMP_CONTEXT (paint_options);
      GimpPaintTool *paint_tool = GIMP_PAINT_TOOL (tool);
      GimpBrushCore *brush_core = GIMP_BRUSH_CORE (paint_tool->core);

      gimp_brush_core_set_brush (brush_core,
                                 gimp_context_get_brush (context));

      gimp_brush_core_set_dynamics (brush_core,
                                    gimp_context_get_dynamics (context));

      if (GIMP_BRUSH_CORE_GET_CLASS (brush_core)->handles_transforming_brush)
        {
          gimp_brush_core_eval_transform_dynamics (brush_core,
                                                   drawable,
                                                   paint_options,
                                                   coords);
        }
    }

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
}

static void
gimp_brush_tool_cursor_update (GimpTool         *tool,
                               const GimpCoords *coords,
                               GdkModifierType   state,
                               GimpDisplay      *display)
{
  GimpBrushTool *brush_tool = GIMP_BRUSH_TOOL (tool);
  GimpBrushCore *brush_core = GIMP_BRUSH_CORE (GIMP_PAINT_TOOL (brush_tool)->core);

  if (! gimp_color_tool_is_enabled (GIMP_COLOR_TOOL (tool)))
    {
      if (! brush_core->main_brush || ! brush_core->dynamics)
        {
          gimp_tool_set_cursor (tool, display,
                                gimp_tool_control_get_cursor (tool->control),
                                gimp_tool_control_get_tool_cursor (tool->control),
                                GIMP_CURSOR_MODIFIER_BAD);
          return;
        }
    }

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool,  coords, state, display);
}

static void
gimp_brush_tool_options_notify (GimpTool         *tool,
                                GimpToolOptions  *options,
                                const GParamSpec *pspec)
{
  GIMP_TOOL_CLASS (parent_class)->options_notify (tool, options, pspec);

  if (! strcmp (pspec->name, "brush-size")  ||
      ! strcmp (pspec->name, "brush-angle") ||
      ! strcmp (pspec->name, "brush-aspect-ratio"))
    {
      GimpPaintTool *paint_tool = GIMP_PAINT_TOOL (tool);
      GimpBrushCore *brush_core = GIMP_BRUSH_CORE (paint_tool->core);

      g_signal_emit_by_name (brush_core, "set-brush",
                             brush_core->main_brush);
    }
}

static void
gimp_brush_tool_paint_start (GimpPaintTool *paint_tool)
{
  GimpBrushTool        *brush_tool = GIMP_BRUSH_TOOL (paint_tool);
  GimpBrushCore        *brush_core = GIMP_BRUSH_CORE (paint_tool->core);
  const GimpBezierDesc *boundary;

  if (GIMP_PAINT_TOOL_CLASS (parent_class)->paint_start)
    GIMP_PAINT_TOOL_CLASS (parent_class)->paint_start (paint_tool);

  boundary = gimp_brush_tool_get_boundary (brush_tool,
                                           &brush_tool->boundary_width,
                                           &brush_tool->boundary_height);

  if (boundary)
    brush_tool->boundary = gimp_bezier_desc_copy (boundary);

  brush_tool->boundary_scale        = brush_core->scale;
  brush_tool->boundary_aspect_ratio = brush_core->aspect_ratio;
  brush_tool->boundary_angle        = brush_core->angle;
  brush_tool->boundary_reflect      = brush_core->reflect;
  brush_tool->boundary_hardness     = brush_core->hardness;
}

static void
gimp_brush_tool_paint_end (GimpPaintTool *paint_tool)
{
  GimpBrushTool *brush_tool = GIMP_BRUSH_TOOL (paint_tool);

  g_clear_pointer (&brush_tool->boundary, gimp_bezier_desc_free);

  if (GIMP_PAINT_TOOL_CLASS (parent_class)->paint_end)
    GIMP_PAINT_TOOL_CLASS (parent_class)->paint_end (paint_tool);
}

static void
gimp_brush_tool_paint_flush (GimpPaintTool *paint_tool)
{
  GimpBrushTool        *brush_tool = GIMP_BRUSH_TOOL (paint_tool);
  GimpBrushCore        *brush_core = GIMP_BRUSH_CORE (paint_tool->core);
  const GimpBezierDesc *boundary;

  if (GIMP_PAINT_TOOL_CLASS (parent_class)->paint_flush)
    GIMP_PAINT_TOOL_CLASS (parent_class)->paint_flush (paint_tool);

  if (brush_tool->boundary_scale        != brush_core->scale        ||
      brush_tool->boundary_aspect_ratio != brush_core->aspect_ratio ||
      brush_tool->boundary_angle        != brush_core->angle        ||
      brush_tool->boundary_reflect      != brush_core->reflect      ||
      brush_tool->boundary_hardness     != brush_core->hardness)
    {
      g_clear_pointer (&brush_tool->boundary, gimp_bezier_desc_free);

      boundary = gimp_brush_tool_get_boundary (brush_tool,
                                               &brush_tool->boundary_width,
                                               &brush_tool->boundary_height);

      if (boundary)
        brush_tool->boundary = gimp_bezier_desc_copy (boundary);

      brush_tool->boundary_scale        = brush_core->scale;
      brush_tool->boundary_aspect_ratio = brush_core->aspect_ratio;
      brush_tool->boundary_angle        = brush_core->angle;
      brush_tool->boundary_reflect      = brush_core->reflect;
      brush_tool->boundary_hardness     = brush_core->hardness;
    }
}

static GimpCanvasItem *
gimp_brush_tool_get_outline (GimpPaintTool *paint_tool,
                             GimpDisplay   *display,
                             gdouble        x,
                             gdouble        y)
{
  GimpBrushTool  *brush_tool = GIMP_BRUSH_TOOL (paint_tool);
  GimpCanvasItem *item;

  item = gimp_brush_tool_create_outline (brush_tool, display, x, y);

  if (! item)
    {
      GimpBrushCore *brush_core = GIMP_BRUSH_CORE (paint_tool->core);

      if (brush_core->main_brush && brush_core->dynamics)
        {
          /*  if an outline was expected, but got scaled away by
           *  transform/dynamics, draw a circle in the "normal" size.
           */
          GimpPaintOptions *options;

          options = GIMP_PAINT_TOOL_GET_OPTIONS (brush_tool);

          gimp_paint_tool_set_draw_fallback (paint_tool,
                                             TRUE, options->brush_size);
        }
    }

  return item;
}

GimpCanvasItem *
gimp_brush_tool_create_outline (GimpBrushTool *brush_tool,
                                GimpDisplay   *display,
                                gdouble        x,
                                gdouble        y)
{
  GimpTool             *tool;
  GimpDisplayShell     *shell;
  const GimpBezierDesc *boundary = NULL;
  gint                  width    = 0;
  gint                  height   = 0;

  g_return_val_if_fail (GIMP_IS_BRUSH_TOOL (brush_tool), NULL);
  g_return_val_if_fail (GIMP_IS_DISPLAY (display), NULL);

  if (gimp_paint_tool_paint_is_active (GIMP_PAINT_TOOL (brush_tool)))
    {
      boundary = brush_tool->boundary;
      width    = brush_tool->boundary_width;
      height   = brush_tool->boundary_height;
    }
  else
    {
      boundary = gimp_brush_tool_get_boundary (brush_tool, &width, &height);
    }

  if (! boundary)
    return NULL;

  tool  = GIMP_TOOL (brush_tool);
  shell = gimp_display_get_shell (display);

  /*  don't draw the boundary if it becomes too small  */
  if (SCALEX (shell, width)  > 4 &&
      SCALEY (shell, height) > 4)
    {
      x -= width  / 2.0;
      y -= height / 2.0;

      if (gimp_tool_control_get_precision (tool->control) ==
          GIMP_CURSOR_PRECISION_PIXEL_CENTER)
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

      return gimp_canvas_path_new (shell, boundary, x, y, FALSE,
                                   GIMP_PATH_STYLE_OUTLINE);
    }

  return NULL;
}

static void
gimp_brush_tool_brush_changed (GimpContext   *context,
                               GimpBrush     *brush,
                               GimpBrushTool *brush_tool)
{
  GimpPaintTool *paint_tool = GIMP_PAINT_TOOL (brush_tool);
  GimpBrushCore *brush_core = GIMP_BRUSH_CORE (paint_tool->core);

  gimp_brush_core_set_brush (brush_core, brush);

}

static void
gimp_brush_tool_set_brush (GimpBrushCore *brush_core,
                           GimpBrush     *brush,
                           GimpBrushTool *brush_tool)
{
  gimp_draw_tool_pause (GIMP_DRAW_TOOL (brush_tool));

  if (GIMP_BRUSH_CORE_GET_CLASS (brush_core)->handles_transforming_brush)
    {
      GimpPaintCore *paint_core = GIMP_PAINT_CORE (brush_core);

      gimp_brush_core_eval_transform_dynamics (brush_core,
                                               NULL,
                                               GIMP_PAINT_TOOL_GET_OPTIONS (brush_tool),
                                               &paint_core->cur_coords);
    }

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (brush_tool));
}

static const GimpBezierDesc *
gimp_brush_tool_get_boundary (GimpBrushTool *brush_tool,
                              gint          *width,
                              gint          *height)
{
  GimpPaintTool *paint_tool = GIMP_PAINT_TOOL (brush_tool);
  GimpBrushCore *brush_core = GIMP_BRUSH_CORE (paint_tool->core);

  if (paint_tool->draw_brush &&
      brush_core->main_brush &&
      brush_core->dynamics   &&
      brush_core->scale > 0.0)
    {
      return gimp_brush_transform_boundary (brush_core->main_brush,
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
