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

#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "paint/gimpmybrushoptions.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpcanvasarc.h"

#include "core/gimp.h"

#include "widgets/gimphelp-ids.h"

#include "gimpmybrushoptions-gui.h"
#include "gimpmybrushtool.h"
#include "gimptoolcontrol.h"
#include "core/gimpmybrush.h"

#include "gimp-intl.h"

G_DEFINE_TYPE (GimpMybrushTool, gimp_mybrush_tool, GIMP_TYPE_PAINT_TOOL)

#define parent_class gimp_mybrush_tool_parent_class


static void   gimp_mybrush_tool_options_notify        (GimpTool         *tool,
                                                       GimpToolOptions  *options,
                                                       const GParamSpec *pspec);

static GimpCanvasItem * gimp_mybrush_tool_get_outline (GimpPaintTool *paint_tool,
                                                       GimpDisplay   *display,
                                                       gdouble        x,
                                                       gdouble        y);


void
gimp_mybrush_tool_register (GimpToolRegisterCallback  callback,
                            gpointer                  data)
{
  (* callback) (GIMP_TYPE_MYBRUSH_TOOL,
                GIMP_TYPE_MYBRUSH_OPTIONS,
                gimp_mybrush_options_gui,
                GIMP_CONTEXT_PROP_MASK_FOREGROUND |
                GIMP_CONTEXT_PROP_MASK_BACKGROUND |
                GIMP_CONTEXT_PROP_MASK_OPACITY    |
                GIMP_CONTEXT_PROP_MASK_PAINT_MODE |
                GIMP_CONTEXT_PROP_MASK_MYBRUSH    |
                GIMP_CONTEXT_PROP_MASK_PATTERN    |
                GIMP_CONTEXT_PROP_MASK_EXPAND,
                "gimp-mypaint-brush-tool",
                _("MyPaint Brush"),
                _("MyPaint Brush Tool: Use MyPaint brushes in GIMP"),
                N_("M_yPaint Brush"), "Y",
                NULL, GIMP_HELP_TOOL_MYPAINT_BRUSH,
                GIMP_ICON_TOOL_MYPAINT_BRUSH,
                data);
}

static void
gimp_mybrush_tool_class_init (GimpMybrushToolClass *klass)
{
  GimpToolClass      *tool_class       = GIMP_TOOL_CLASS (klass);
  GimpPaintToolClass *paint_tool_class = GIMP_PAINT_TOOL_CLASS (klass);

  tool_class->options_notify    = gimp_mybrush_tool_options_notify;

  paint_tool_class->get_outline = gimp_mybrush_tool_get_outline;
}

static void
gimp_mybrush_tool_init (GimpMybrushTool *mybrush_tool)
{
  GimpTool *tool = GIMP_TOOL (mybrush_tool);

  gimp_tool_control_set_tool_cursor       (tool->control, GIMP_TOOL_CURSOR_INK);
  gimp_tool_control_set_action_size       (tool->control,
                                           "tools-mypaint-brush-radius-set");
  gimp_tool_control_set_action_pixel_size (tool->control,
                                           "tools-mypaint-brush-pixel-size-set");
  gimp_tool_control_set_action_hardness   (tool->control,
                                           "tools-mypaint-brush-hardness-set");

  gimp_paint_tool_enable_color_picker (GIMP_PAINT_TOOL (mybrush_tool),
                                       GIMP_COLOR_PICK_TARGET_FOREGROUND);
}

static void
gimp_mybrush_tool_options_notify (GimpTool         *tool,
                                  GimpToolOptions  *options,
                                  const GParamSpec *pspec)
{
  GIMP_TOOL_CLASS (parent_class)->options_notify (tool, options, pspec);

  if (! strcmp (pspec->name, "radius"))
    {
      gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));
      gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
    }
}

static GimpCanvasItem *
gimp_mybrush_tool_get_outline (GimpPaintTool *paint_tool,
                               GimpDisplay   *display,
                               gdouble        x,
                               gdouble        y)
{
  GimpMybrushOptions *options = GIMP_MYBRUSH_TOOL_GET_OPTIONS (paint_tool);
  GimpCanvasItem     *item    = NULL;
  GimpDisplayShell   *shell   = gimp_display_get_shell (display);
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

  item = gimp_mybrush_tool_create_cursor (paint_tool, display, x, y, radius);

  if (! item)
    {
      gimp_paint_tool_set_draw_fallback (paint_tool,
                                         TRUE, radius);
    }

  return item;
}

GimpCanvasItem *
gimp_mybrush_tool_create_cursor (GimpPaintTool *paint_tool,
                                 GimpDisplay   *display,
                                 gdouble        x,
                                 gdouble        y,
                                 gdouble        radius)
{

  GimpDisplayShell     *shell;

  g_return_val_if_fail (GIMP_IS_PAINT_TOOL (paint_tool), NULL);
  g_return_val_if_fail (GIMP_IS_DISPLAY (display), NULL);

  shell = gimp_display_get_shell (display);

  /*  don't draw the boundary if it becomes too small  */
  if (SCALEX (shell, radius) > 4 &&
      SCALEY (shell, radius) > 4)
    {
      return gimp_canvas_arc_new(shell, x, y, radius, radius, 0.0, 2 * G_PI, FALSE);
    }

  return NULL;
}
