/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-2001 Spencer Kimball, Peter Mattis, and others.
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

#pragma once

#include "gimptool.h"


#define GIMP_TOOL_HANDLE_SIZE_CIRCLE    13
#define GIMP_TOOL_HANDLE_SIZE_CROSS     15
#define GIMP_TOOL_HANDLE_SIZE_CROSSHAIR 43
#define GIMP_TOOL_HANDLE_SIZE_LARGE     25
#define GIMP_TOOL_HANDLE_SIZE_SMALL     7


#define GIMP_TYPE_DRAW_TOOL            (gimp_draw_tool_get_type ())
#define GIMP_DRAW_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_DRAW_TOOL, GimpDrawTool))
#define GIMP_DRAW_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_DRAW_TOOL, GimpDrawToolClass))
#define GIMP_IS_DRAW_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_DRAW_TOOL))
#define GIMP_IS_DRAW_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_DRAW_TOOL))
#define GIMP_DRAW_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_DRAW_TOOL, GimpDrawToolClass))


typedef struct _GimpDrawToolClass GimpDrawToolClass;

struct _GimpDrawTool
{
  GimpTool        parent_instance;

  GimpDisplay    *display;        /*  The display we are drawing to (may be
                                   *  a different one than tool->display)
                                   */

  gint            paused_count;   /*  count to keep track of multiple pauses  */
  guint           draw_timeout;   /*  draw delay timeout ID                   */
  guint64         last_draw_time; /*  time of last draw(), monotonically      */

  GimpToolWidget *widget;
  gchar          *default_status;
  GimpCanvasItem *preview;
  GimpCanvasItem *item;
  GList          *group_stack;
};

struct _GimpDrawToolClass
{
  GimpToolClass   parent_class;

  /*  virtual function  */

  void (* draw) (GimpDrawTool *draw_tool);
};


GType            gimp_draw_tool_get_type             (void) G_GNUC_CONST;

void             gimp_draw_tool_start                (GimpDrawTool     *draw_tool,
                                                      GimpDisplay      *display);
void             gimp_draw_tool_stop                 (GimpDrawTool     *draw_tool);

gboolean         gimp_draw_tool_is_active            (GimpDrawTool     *draw_tool);

void             gimp_draw_tool_pause                (GimpDrawTool     *draw_tool);
void             gimp_draw_tool_resume               (GimpDrawTool     *draw_tool);

gdouble          gimp_draw_tool_calc_distance        (GimpDrawTool     *draw_tool,
                                                      GimpDisplay      *display,
                                                      gdouble           x1,
                                                      gdouble           y1,
                                                      gdouble           x2,
                                                      gdouble           y2);
gdouble          gimp_draw_tool_calc_distance_square (GimpDrawTool     *draw_tool,
                                                      GimpDisplay      *display,
                                                      gdouble           x1,
                                                      gdouble           y1,
                                                      gdouble           x2,
                                                      gdouble           y2);

void             gimp_draw_tool_set_widget           (GimpDrawTool     *draw_tool,
                                                      GimpToolWidget   *widget);
void             gimp_draw_tool_set_default_status   (GimpDrawTool     *draw_tool,
                                                      const gchar      *status);

void             gimp_draw_tool_add_preview          (GimpDrawTool     *draw_tool,
                                                      GimpCanvasItem   *item);
void             gimp_draw_tool_remove_preview       (GimpDrawTool     *draw_tool,
                                                      GimpCanvasItem   *item);

void             gimp_draw_tool_add_item             (GimpDrawTool     *draw_tool,
                                                      GimpCanvasItem   *item);
void             gimp_draw_tool_remove_item          (GimpDrawTool     *draw_tool,
                                                      GimpCanvasItem   *item);

GimpCanvasGroup* gimp_draw_tool_add_stroke_group     (GimpDrawTool     *draw_tool);
GimpCanvasGroup* gimp_draw_tool_add_fill_group       (GimpDrawTool     *draw_tool);

void             gimp_draw_tool_push_group           (GimpDrawTool     *draw_tool,
                                                      GimpCanvasGroup  *group);
void             gimp_draw_tool_pop_group            (GimpDrawTool     *draw_tool);

GimpCanvasItem * gimp_draw_tool_add_line             (GimpDrawTool     *draw_tool,
                                                      gdouble           x1,
                                                      gdouble           y1,
                                                      gdouble           x2,
                                                      gdouble           y2);
GimpCanvasItem * gimp_draw_tool_add_guide            (GimpDrawTool     *draw_tool,
                                                      GimpOrientationType  orientation,
                                                      gint              position,
                                                      GimpGuideStyle    style);
GimpCanvasItem * gimp_draw_tool_add_crosshair        (GimpDrawTool     *draw_tool,
                                                      gint              position_x,
                                                      gint              position_y);
GimpCanvasItem * gimp_draw_tool_add_sample_point     (GimpDrawTool     *draw_tool,
                                                      gint              x,
                                                      gint              y,
                                                      gint              index);
GimpCanvasItem * gimp_draw_tool_add_rectangle        (GimpDrawTool     *draw_tool,
                                                      gboolean          filled,
                                                      gdouble           x,
                                                      gdouble           y,
                                                      gdouble           width,
                                                      gdouble           height);
GimpCanvasItem * gimp_draw_tool_add_arc              (GimpDrawTool     *draw_tool,
                                                      gboolean          filled,
                                                      gdouble           x,
                                                      gdouble           y,
                                                      gdouble           width,
                                                      gdouble           height,
                                                      gdouble           start_angle,
                                                      gdouble           slice_angle);
GimpCanvasItem * gimp_draw_tool_add_transform_preview(GimpDrawTool     *draw_tool,
                                                      GimpPickable     *pickable,
                                                      const GimpMatrix3 *transform,
                                                      gdouble           x1,
                                                      gdouble           y1,
                                                      gdouble           x2,
                                                      gdouble           y2);

GimpCanvasItem * gimp_draw_tool_add_handle           (GimpDrawTool     *draw_tool,
                                                      GimpHandleType    type,
                                                      gdouble           x,
                                                      gdouble           y,
                                                      gint              width,
                                                      gint              height,
                                                      GimpHandleAnchor  anchor);

GimpCanvasItem * gimp_draw_tool_add_lines            (GimpDrawTool     *draw_tool,
                                                      const GimpVector2 *points,
                                                      gint              n_points,
                                                      GimpMatrix3      *transform,
                                                      gboolean          filled);

GimpCanvasItem * gimp_draw_tool_add_strokes          (GimpDrawTool     *draw_tool,
                                                      const GimpCoords *points,
                                                      gint              n_points,
                                                      GimpMatrix3      *transform,
                                                      gboolean          filled);

GimpCanvasItem * gimp_draw_tool_add_pen              (GimpDrawTool     *draw_tool,
                                                      const GimpVector2 *points,
                                                      gint              n_points,
                                                      GimpContext      *context,
                                                      GimpActiveColor   color,
                                                      gint              width);

GimpCanvasItem * gimp_draw_tool_add_boundary         (GimpDrawTool     *draw_tool,
                                                      const GimpBoundSeg *bound_segs,
                                                      gint              n_bound_segs,
                                                      GimpMatrix3      *transform,
                                                      gdouble           offset_x,
                                                      gdouble           offset_y);

GimpCanvasItem * gimp_draw_tool_add_text_cursor      (GimpDrawTool     *draw_tool,
                                                      PangoRectangle   *cursor,
                                                      gboolean          overwrite,
                                                      GimpTextDirection direction);

GimpCanvasItem * gimp_draw_tool_add_text             (GimpDrawTool     *draw_tool,
                                                      gdouble           x,
                                                      gdouble           y,
                                                      gdouble           font_size,
                                                      gchar            *text);

gboolean         gimp_draw_tool_on_handle            (GimpDrawTool     *draw_tool,
                                                      GimpDisplay      *display,
                                                      gdouble           x,
                                                      gdouble           y,
                                                      GimpHandleType    type,
                                                      gdouble           handle_x,
                                                      gdouble           handle_y,
                                                      gint              width,
                                                      gint              height,
                                                      GimpHandleAnchor  anchor);
