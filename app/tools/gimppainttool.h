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

#ifndef __LIGMA_PAINT_TOOL_H__
#define __LIGMA_PAINT_TOOL_H__


#include "ligmacolortool.h"


#define LIGMA_PAINT_TOOL_LINE_MASK (ligma_get_extend_selection_mask ())


#define LIGMA_TYPE_PAINT_TOOL            (ligma_paint_tool_get_type ())
#define LIGMA_PAINT_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_PAINT_TOOL, LigmaPaintTool))
#define LIGMA_PAINT_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_PAINT_TOOL, LigmaPaintToolClass))
#define LIGMA_IS_PAINT_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_PAINT_TOOL))
#define LIGMA_IS_PAINT_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_PAINT_TOOL))
#define LIGMA_PAINT_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_PAINT_TOOL, LigmaPaintToolClass))

#define LIGMA_PAINT_TOOL_GET_OPTIONS(t)  (LIGMA_PAINT_OPTIONS (ligma_tool_get_options (LIGMA_TOOL (t))))


typedef struct _LigmaPaintToolClass LigmaPaintToolClass;

struct _LigmaPaintTool
{
  LigmaColorTool  parent_instance;

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

  LigmaPaintCore *core;

  LigmaDisplay   *display;
  GList         *drawables;

  gdouble        cursor_x;
  gdouble        cursor_y;

  gdouble        paint_x;
  gdouble        paint_y;
};

struct _LigmaPaintToolClass
{
  LigmaColorToolClass  parent_class;

  void             (* paint_prepare) (LigmaPaintTool *paint_tool,
                                      LigmaDisplay   *display);
  void             (* paint_start)   (LigmaPaintTool *paint_tool);
  void             (* paint_end)     (LigmaPaintTool *paint_tool);
  void             (* paint_flush)   (LigmaPaintTool *paint_tool);

  LigmaCanvasItem * (* get_outline)   (LigmaPaintTool *paint_tool,
                                      LigmaDisplay   *display,
                                      gdouble        x,
                                      gdouble        y);

  gboolean         (* is_alpha_only) (LigmaPaintTool *paint_tool,
                                      LigmaDrawable  *drawable);
};


GType   ligma_paint_tool_get_type            (void) G_GNUC_CONST;

void    ligma_paint_tool_set_active          (LigmaPaintTool       *tool,
                                             gboolean             active);

void    ligma_paint_tool_enable_color_picker (LigmaPaintTool       *tool,
                                             LigmaColorPickTarget  target);

void    ligma_paint_tool_enable_multi_paint  (LigmaPaintTool       *tool);

void    ligma_paint_tool_set_draw_fallback   (LigmaPaintTool       *tool,
                                             gboolean             draw_fallback,
                                             gint                 fallback_size);

void    ligma_paint_tool_set_draw_circle     (LigmaPaintTool       *tool,
                                             gboolean             draw_circle,
                                             gint                 circle_size);

void    ligma_paint_tool_force_draw          (LigmaPaintTool       *tool,
                                             gboolean             force);


#endif  /*  __LIGMA_PAINT_TOOL_H__  */
