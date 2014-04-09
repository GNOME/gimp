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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
#include "gimptoolcontrol.h"


static void   gimp_brush_tool_constructed     (GObject           *object);

static void   gimp_brush_tool_motion          (GimpTool          *tool,
                                               const GimpCoords  *coords,
                                               guint32            time,
                                               GdkModifierType    state,
                                               GimpDisplay       *display);
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

static void   gimp_brush_tool_draw            (GimpDrawTool      *draw_tool);

static void   gimp_brush_tool_brush_changed   (GimpContext       *context,
                                               GimpBrush         *brush,
                                               GimpBrushTool     *brush_tool);
static void   gimp_brush_tool_set_brush       (GimpBrushCore     *brush_core,
                                               GimpBrush         *brush,
                                               GimpBrushTool     *brush_tool);
static void   gimp_brush_tool_notify_brush    (GimpDisplayConfig *config,
                                               GParamSpec        *pspec,
                                               GimpBrushTool     *brush_tool);


G_DEFINE_TYPE (GimpBrushTool, gimp_brush_tool, GIMP_TYPE_PAINT_TOOL)

#define parent_class gimp_brush_tool_parent_class


static void
gimp_brush_tool_class_init (GimpBrushToolClass *klass)
{
  GObjectClass      *object_class    = G_OBJECT_CLASS (klass);
  GimpToolClass     *tool_class      = GIMP_TOOL_CLASS (klass);
  GimpDrawToolClass *draw_tool_class = GIMP_DRAW_TOOL_CLASS (klass);

  object_class->constructed  = gimp_brush_tool_constructed;

  tool_class->motion         = gimp_brush_tool_motion;
  tool_class->oper_update    = gimp_brush_tool_oper_update;
  tool_class->cursor_update  = gimp_brush_tool_cursor_update;
  tool_class->options_notify = gimp_brush_tool_options_notify;

  draw_tool_class->draw      = gimp_brush_tool_draw;
}

static void
gimp_brush_tool_init (GimpBrushTool *brush_tool)
{
  GimpTool *tool = GIMP_TOOL (brush_tool);

  gimp_tool_control_set_action_value_2 (tool->control,
                                        "tools/tools-paint-brush-size-set");
  gimp_tool_control_set_action_value_3  (tool->control,
                                         "context/context-brush-aspect-set");
  gimp_tool_control_set_action_value_4  (tool->control,
                                         "context/context-brush-angle-set");
  gimp_tool_control_set_action_object_1 (tool->control,
                                         "context/context-brush-select-set");

  brush_tool->show_cursor = TRUE;
  brush_tool->draw_brush  = TRUE;
  brush_tool->brush_x     = 0.0;
  brush_tool->brush_y     = 0.0;
}

static void
gimp_brush_tool_constructed (GObject *object)
{
  GimpTool          *tool       = GIMP_TOOL (object);
  GimpPaintTool     *paint_tool = GIMP_PAINT_TOOL (object);
  GimpBrushTool     *brush_tool = GIMP_BRUSH_TOOL (object);
  GimpDisplayConfig *display_config;

  if (G_OBJECT_CLASS (parent_class)->constructed)
    G_OBJECT_CLASS (parent_class)->constructed (object);

  g_assert (GIMP_IS_BRUSH_CORE (paint_tool->core));

  display_config = GIMP_DISPLAY_CONFIG (tool->tool_info->gimp->config);

  brush_tool->show_cursor = display_config->show_paint_tool_cursor;
  brush_tool->draw_brush  = display_config->show_brush_outline;

  g_signal_connect_object (display_config, "notify::show-paint-tool-cursor",
                           G_CALLBACK (gimp_brush_tool_notify_brush),
                           brush_tool, 0);
  g_signal_connect_object (display_config, "notify::show-brush-outline",
                           G_CALLBACK (gimp_brush_tool_notify_brush),
                           brush_tool, 0);

  g_signal_connect_object (gimp_tool_get_options (tool), "brush-changed",
                           G_CALLBACK (gimp_brush_tool_brush_changed),
                           brush_tool, 0);

  g_signal_connect_object (paint_tool->core, "set-brush",
                           G_CALLBACK (gimp_brush_tool_set_brush),
                           brush_tool, 0);
}

static void
gimp_brush_tool_motion (GimpTool         *tool,
                        const GimpCoords *coords,
                        guint32           time,
                        GdkModifierType   state,
                        GimpDisplay      *display)
{
  GimpBrushTool *brush_tool = GIMP_BRUSH_TOOL (tool);

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  GIMP_TOOL_CLASS (parent_class)->motion (tool, coords, time, state, display);

  if (! gimp_color_tool_is_enabled (GIMP_COLOR_TOOL (tool)))
    {
      brush_tool->brush_x = coords->x;
      brush_tool->brush_y = coords->y;
    }

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
}

static void
gimp_brush_tool_oper_update (GimpTool         *tool,
                             const GimpCoords *coords,
                             GdkModifierType   state,
                             gboolean          proximity,
                             GimpDisplay      *display)
{
  GimpBrushTool    *brush_tool    = GIMP_BRUSH_TOOL (tool);
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

      brush_tool->brush_x = coords->x;
      brush_tool->brush_y = coords->y;

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
      else if (! brush_tool->show_cursor &&
               gimp_tool_control_get_cursor_modifier (tool->control) !=
               GIMP_CURSOR_MODIFIER_BAD)
        {
          gimp_tool_set_cursor (tool, display,
                                GIMP_CURSOR_NONE,
                                GIMP_TOOL_CURSOR_NONE,
                                GIMP_CURSOR_MODIFIER_NONE);
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
gimp_brush_tool_draw (GimpDrawTool *draw_tool)
{
  GimpBrushTool  *brush_tool = GIMP_BRUSH_TOOL (draw_tool);
  GimpCanvasItem *item;

  GIMP_DRAW_TOOL_CLASS (parent_class)->draw (draw_tool);

  if (gimp_color_tool_is_enabled (GIMP_COLOR_TOOL (draw_tool)))
    return;

  item = gimp_brush_tool_create_outline (brush_tool,
                                         draw_tool->display,
                                         brush_tool->brush_x,
                                         brush_tool->brush_y);

  if (! item && ! brush_tool->show_cursor)
    {
      item = gimp_canvas_handle_new (gimp_display_get_shell (draw_tool->display),
                                     GIMP_HANDLE_CROSS,
                                     GIMP_HANDLE_ANCHOR_CENTER,
                                     brush_tool->brush_x,
                                     brush_tool->brush_y,
                                     GIMP_TOOL_HANDLE_SIZE_SMALL,
                                     GIMP_TOOL_HANDLE_SIZE_SMALL);
    }

  if (item)
    {
      gimp_draw_tool_add_item (draw_tool, item);
      g_object_unref (item);
    }
}

GimpCanvasItem *
gimp_brush_tool_create_outline (GimpBrushTool *brush_tool,
                                GimpDisplay   *display,
                                gdouble        x,
                                gdouble        y)
{
  GimpBrushCore        *brush_core;
  GimpPaintOptions     *options;
  GimpDisplayShell     *shell;
  const GimpBezierDesc *boundary = NULL;
  gint                  width    = 0;
  gint                  height   = 0;

  g_return_val_if_fail (GIMP_IS_BRUSH_TOOL (brush_tool), NULL);
  g_return_val_if_fail (GIMP_IS_DISPLAY (display), NULL);

  if (! brush_tool->draw_brush)
    return NULL;

  brush_core = GIMP_BRUSH_CORE (GIMP_PAINT_TOOL (brush_tool)->core);
  options    = GIMP_PAINT_TOOL_GET_OPTIONS (brush_tool);
  shell      = gimp_display_get_shell (display);

  if (! brush_core->main_brush || ! brush_core->dynamics)
    return NULL;

  if (brush_core->scale > 0.0)
    boundary = gimp_brush_transform_boundary (brush_core->main_brush,
                                              brush_core->scale,
                                              brush_core->aspect_ratio,
                                              brush_core->angle,
                                              brush_core->hardness,
                                              &width,
                                              &height);

  /*  don't draw the boundary if it becomes too small  */
  if (boundary                   &&
      SCALEX (shell, width)  > 4 &&
      SCALEY (shell, height) > 4)
    {
      x -= width  / 2.0;
      y -= height / 2.0;

      if (gimp_paint_options_get_brush_mode (options) == GIMP_BRUSH_HARD)
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

static void
gimp_brush_tool_notify_brush (GimpDisplayConfig *config,
                              GParamSpec        *pspec,
                              GimpBrushTool     *brush_tool)
{
  gimp_draw_tool_pause (GIMP_DRAW_TOOL (brush_tool));

  brush_tool->show_cursor = config->show_paint_tool_cursor;
  brush_tool->draw_brush  = config->show_brush_outline;

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (brush_tool));
}
