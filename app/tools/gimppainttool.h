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

#ifndef __GIMP_PAINT_TOOL_H__
#define __GIMP_PAINT_TOOL_H__


#include "gimpcolortool.h"


#define GIMP_PAINT_TOOL_LINE_MASK (gimp_get_extend_selection_mask ())


#define GIMP_TYPE_PAINT_TOOL            (gimp_paint_tool_get_type ())
#define GIMP_PAINT_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_PAINT_TOOL, GimpPaintTool))
#define GIMP_PAINT_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_PAINT_TOOL, GimpPaintToolClass))
#define GIMP_IS_PAINT_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_PAINT_TOOL))
#define GIMP_IS_PAINT_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_PAINT_TOOL))
#define GIMP_PAINT_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_PAINT_TOOL, GimpPaintToolClass))

#define GIMP_PAINT_TOOL_GET_OPTIONS(t)  (GIMP_PAINT_OPTIONS (gimp_tool_get_options (GIMP_TOOL (t))))


typedef struct _GimpPaintToolClass GimpPaintToolClass;

struct _GimpPaintTool
{
  GimpColorTool  parent_instance;

  gboolean       active;
  gboolean       pick_colors;      /*  pick color if ctrl is pressed           */
  gboolean       can_multi_paint;  /*  if paint works with multiple drawables  */
  gboolean       draw_line;

  gboolean       show_cursor;
  gboolean       draw_brush;
  gboolean       snap_brush;
  gboolean       draw_fallback;
  gint           fallback_size;
  gboolean       draw_circle;
  gint           circle_size;

  const gchar   *status;       /* status message */
  const gchar   *status_line;  /* status message when drawing a line */
  const gchar   *status_ctrl;  /* additional message for the ctrl modifier */

  GimpPaintCore *core;

  GimpDisplay   *display;
  GList         *drawables;

  gdouble        cursor_x;
  gdouble        cursor_y;

  gdouble        paint_x;
  gdouble        paint_y;
};

struct _GimpPaintToolClass
{
  GimpColorToolClass  parent_class;

  void             (* paint_prepare) (GimpPaintTool *paint_tool,
                                      GimpDisplay   *display);
  void             (* paint_start)   (GimpPaintTool *paint_tool);
  void             (* paint_end)     (GimpPaintTool *paint_tool);
  void             (* paint_flush)   (GimpPaintTool *paint_tool);

  GimpCanvasItem * (* get_outline)   (GimpPaintTool *paint_tool,
                                      GimpDisplay   *display,
                                      gdouble        x,
                                      gdouble        y);

  gboolean         (* is_alpha_only) (GimpPaintTool *paint_tool,
                                      GimpDrawable  *drawable);
};


GType   gimp_paint_tool_get_type            (void) G_GNUC_CONST;

void    gimp_paint_tool_set_active          (GimpPaintTool       *tool,
                                             gboolean             active);

void    gimp_paint_tool_enable_color_picker (GimpPaintTool       *tool,
                                             GimpColorPickTarget  target);

void    gimp_paint_tool_enable_multi_paint  (GimpPaintTool       *tool);

void    gimp_paint_tool_set_draw_fallback   (GimpPaintTool       *tool,
                                             gboolean             draw_fallback,
                                             gint                 fallback_size);

void    gimp_paint_tool_set_draw_circle     (GimpPaintTool       *tool,
                                             gboolean             draw_circle,
                                             gint                 circle_size);

void    gimp_paint_tool_force_draw          (GimpPaintTool       *tool,
                                             gboolean             force);


#endif  /*  __GIMP_PAINT_TOOL_H__  */
