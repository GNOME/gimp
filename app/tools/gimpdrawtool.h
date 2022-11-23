/* LIGMA - The GNU Image Manipulation Program
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

#ifndef __LIGMA_DRAW_TOOL_H__
#define __LIGMA_DRAW_TOOL_H__


#include "ligmatool.h"


#define LIGMA_TOOL_HANDLE_SIZE_CIRCLE    13
#define LIGMA_TOOL_HANDLE_SIZE_CROSS     15
#define LIGMA_TOOL_HANDLE_SIZE_CROSSHAIR 43
#define LIGMA_TOOL_HANDLE_SIZE_LARGE     25
#define LIGMA_TOOL_HANDLE_SIZE_SMALL     7


#define LIGMA_TYPE_DRAW_TOOL            (ligma_draw_tool_get_type ())
#define LIGMA_DRAW_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_DRAW_TOOL, LigmaDrawTool))
#define LIGMA_DRAW_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_DRAW_TOOL, LigmaDrawToolClass))
#define LIGMA_IS_DRAW_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_DRAW_TOOL))
#define LIGMA_IS_DRAW_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_DRAW_TOOL))
#define LIGMA_DRAW_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_DRAW_TOOL, LigmaDrawToolClass))


typedef struct _LigmaDrawToolClass LigmaDrawToolClass;

struct _LigmaDrawTool
{
  LigmaTool        parent_instance;

  LigmaDisplay    *display;        /*  The display we are drawing to (may be
                                   *  a different one than tool->display)
                                   */

  gint            paused_count;   /*  count to keep track of multiple pauses  */
  guint           draw_timeout;   /*  draw delay timeout ID                   */
  guint64         last_draw_time; /*  time of last draw(), monotonically      */

  LigmaToolWidget *widget;
  gchar          *default_status;
  LigmaCanvasItem *preview;
  LigmaCanvasItem *item;
  GList          *group_stack;
};

struct _LigmaDrawToolClass
{
  LigmaToolClass   parent_class;

  /*  virtual function  */

  void (* draw) (LigmaDrawTool *draw_tool);
};


GType            ligma_draw_tool_get_type             (void) G_GNUC_CONST;

void             ligma_draw_tool_start                (LigmaDrawTool     *draw_tool,
                                                      LigmaDisplay      *display);
void             ligma_draw_tool_stop                 (LigmaDrawTool     *draw_tool);

gboolean         ligma_draw_tool_is_active            (LigmaDrawTool     *draw_tool);

void             ligma_draw_tool_pause                (LigmaDrawTool     *draw_tool);
void             ligma_draw_tool_resume               (LigmaDrawTool     *draw_tool);

gdouble          ligma_draw_tool_calc_distance        (LigmaDrawTool     *draw_tool,
                                                      LigmaDisplay      *display,
                                                      gdouble           x1,
                                                      gdouble           y1,
                                                      gdouble           x2,
                                                      gdouble           y2);
gdouble          ligma_draw_tool_calc_distance_square (LigmaDrawTool     *draw_tool,
                                                      LigmaDisplay      *display,
                                                      gdouble           x1,
                                                      gdouble           y1,
                                                      gdouble           x2,
                                                      gdouble           y2);

void             ligma_draw_tool_set_widget           (LigmaDrawTool     *draw_tool,
                                                      LigmaToolWidget   *widget);
void             ligma_draw_tool_set_default_status   (LigmaDrawTool     *draw_tool,
                                                      const gchar      *status);

void             ligma_draw_tool_add_preview          (LigmaDrawTool     *draw_tool,
                                                      LigmaCanvasItem   *item);
void             ligma_draw_tool_remove_preview       (LigmaDrawTool     *draw_tool,
                                                      LigmaCanvasItem   *item);

void             ligma_draw_tool_add_item             (LigmaDrawTool     *draw_tool,
                                                      LigmaCanvasItem   *item);
void             ligma_draw_tool_remove_item          (LigmaDrawTool     *draw_tool,
                                                      LigmaCanvasItem   *item);

LigmaCanvasGroup* ligma_draw_tool_add_stroke_group     (LigmaDrawTool     *draw_tool);
LigmaCanvasGroup* ligma_draw_tool_add_fill_group       (LigmaDrawTool     *draw_tool);

void             ligma_draw_tool_push_group           (LigmaDrawTool     *draw_tool,
                                                      LigmaCanvasGroup  *group);
void             ligma_draw_tool_pop_group            (LigmaDrawTool     *draw_tool);

LigmaCanvasItem * ligma_draw_tool_add_line             (LigmaDrawTool     *draw_tool,
                                                      gdouble           x1,
                                                      gdouble           y1,
                                                      gdouble           x2,
                                                      gdouble           y2);
LigmaCanvasItem * ligma_draw_tool_add_guide            (LigmaDrawTool     *draw_tool,
                                                      LigmaOrientationType  orientation,
                                                      gint              position,
                                                      LigmaGuideStyle    style);
LigmaCanvasItem * ligma_draw_tool_add_crosshair        (LigmaDrawTool     *draw_tool,
                                                      gint              position_x,
                                                      gint              position_y);
LigmaCanvasItem * ligma_draw_tool_add_sample_point     (LigmaDrawTool     *draw_tool,
                                                      gint              x,
                                                      gint              y,
                                                      gint              index);
LigmaCanvasItem * ligma_draw_tool_add_rectangle        (LigmaDrawTool     *draw_tool,
                                                      gboolean          filled,
                                                      gdouble           x,
                                                      gdouble           y,
                                                      gdouble           width,
                                                      gdouble           height);
LigmaCanvasItem * ligma_draw_tool_add_arc              (LigmaDrawTool     *draw_tool,
                                                      gboolean          filled,
                                                      gdouble           x,
                                                      gdouble           y,
                                                      gdouble           width,
                                                      gdouble           height,
                                                      gdouble           start_angle,
                                                      gdouble           slice_angle);
LigmaCanvasItem * ligma_draw_tool_add_transform_preview(LigmaDrawTool     *draw_tool,
                                                      LigmaPickable     *pickable,
                                                      const LigmaMatrix3 *transform,
                                                      gdouble           x1,
                                                      gdouble           y1,
                                                      gdouble           x2,
                                                      gdouble           y2);

LigmaCanvasItem * ligma_draw_tool_add_handle           (LigmaDrawTool     *draw_tool,
                                                      LigmaHandleType    type,
                                                      gdouble           x,
                                                      gdouble           y,
                                                      gint              width,
                                                      gint              height,
                                                      LigmaHandleAnchor  anchor);

LigmaCanvasItem * ligma_draw_tool_add_lines            (LigmaDrawTool     *draw_tool,
                                                      const LigmaVector2 *points,
                                                      gint              n_points,
                                                      LigmaMatrix3      *transform,
                                                      gboolean          filled);

LigmaCanvasItem * ligma_draw_tool_add_strokes          (LigmaDrawTool     *draw_tool,
                                                      const LigmaCoords *points,
                                                      gint              n_points,
                                                      LigmaMatrix3      *transform,
                                                      gboolean          filled);

LigmaCanvasItem * ligma_draw_tool_add_pen              (LigmaDrawTool     *draw_tool,
                                                      const LigmaVector2 *points,
                                                      gint              n_points,
                                                      LigmaContext      *context,
                                                      LigmaActiveColor   color,
                                                      gint              width);

LigmaCanvasItem * ligma_draw_tool_add_boundary         (LigmaDrawTool     *draw_tool,
                                                      const LigmaBoundSeg *bound_segs,
                                                      gint              n_bound_segs,
                                                      LigmaMatrix3      *transform,
                                                      gdouble           offset_x,
                                                      gdouble           offset_y);

LigmaCanvasItem * ligma_draw_tool_add_text_cursor      (LigmaDrawTool     *draw_tool,
                                                      PangoRectangle   *cursor,
                                                      gboolean          overwrite,
                                                      LigmaTextDirection direction);

gboolean         ligma_draw_tool_on_handle            (LigmaDrawTool     *draw_tool,
                                                      LigmaDisplay      *display,
                                                      gdouble           x,
                                                      gdouble           y,
                                                      LigmaHandleType    type,
                                                      gdouble           handle_x,
                                                      gdouble           handle_y,
                                                      gint              width,
                                                      gint              height,
                                                      LigmaHandleAnchor  anchor);


#endif  /*  __LIGMA_DRAW_TOOL_H__  */
