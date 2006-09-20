/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-2001 Spencer Kimball, Peter Mattis, and others.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __GIMP_DRAW_TOOL_H__
#define __GIMP_DRAW_TOOL_H__


#include "gimptool.h"


typedef enum
{
  GIMP_HANDLE_SQUARE,
  GIMP_HANDLE_FILLED_SQUARE,
  GIMP_HANDLE_CIRCLE,
  GIMP_HANDLE_FILLED_CIRCLE,
  GIMP_HANDLE_CROSS
} GimpHandleType;


#define GIMP_TYPE_DRAW_TOOL            (gimp_draw_tool_get_type ())
#define GIMP_DRAW_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_DRAW_TOOL, GimpDrawTool))
#define GIMP_DRAW_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_DRAW_TOOL, GimpDrawToolClass))
#define GIMP_IS_DRAW_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_DRAW_TOOL))
#define GIMP_IS_DRAW_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_DRAW_TOOL))
#define GIMP_DRAW_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_DRAW_TOOL, GimpDrawToolClass))


typedef struct _GimpDrawToolClass GimpDrawToolClass;

struct _GimpDrawTool
{
  GimpTool      parent_instance;

  GimpDisplay  *display;      /*  The display we are drawing to (may be
                               *  a different one than tool->display)
                               */

  gint          paused_count; /*  count to keep track of multiple pauses */

  GList        *vectors;      /*  GimpVectors to render                  */
  GimpMatrix3  *transform;    /*  Transformation matrix fof the vectors  */
};

struct _GimpDrawToolClass
{
  GimpToolClass   parent_class;

  /*  virtual function  */

  void (* draw) (GimpDrawTool *draw_tool);
};


GType      gimp_draw_tool_get_type                 (void) G_GNUC_CONST;

void       gimp_draw_tool_start                    (GimpDrawTool     *draw_tool,
                                                    GimpDisplay      *display);
void       gimp_draw_tool_stop                     (GimpDrawTool     *draw_tool);

gboolean   gimp_draw_tool_is_active                (GimpDrawTool     *draw_tool);

void       gimp_draw_tool_pause                    (GimpDrawTool     *draw_tool);
void       gimp_draw_tool_resume                   (GimpDrawTool     *draw_tool);

void       gimp_draw_tool_set_vectors              (GimpDrawTool     *draw_tool,
                                                    GList            *vectors);
void       gimp_draw_tool_set_transform            (GimpDrawTool     *draw_tool,
                                                    GimpMatrix3      *transform);

gdouble    gimp_draw_tool_calc_distance            (GimpDrawTool     *draw_tool,
                                                    GimpDisplay      *display,
                                                    gdouble           x1,
                                                    gdouble           y1,
                                                    gdouble           x2,
                                                    gdouble           y2);
gboolean   gimp_draw_tool_in_radius                (GimpDrawTool     *draw_tool,
                                                    GimpDisplay      *display,
                                                    gdouble           x1,
                                                    gdouble           y1,
                                                    gdouble           x2,
                                                    gdouble           y2,
                                                    gint              radius);

void       gimp_draw_tool_draw_line                (GimpDrawTool     *draw_tool,
                                                    gdouble           x1,
                                                    gdouble           y1,
                                                    gdouble           x2,
                                                    gdouble           y2,
                                                    gboolean          use_offsets);
void       gimp_draw_tool_draw_dashed_line         (GimpDrawTool     *draw_tool,
                                                    gdouble           x1,
                                                    gdouble           y1,
                                                    gdouble           x2,
                                                    gdouble           y2,
                                                    gboolean          use_offsets);
void       gimp_draw_tool_draw_rectangle           (GimpDrawTool     *draw_tool,
                                                    gboolean          filled,
                                                    gdouble           x,
                                                    gdouble           y,
                                                    gdouble           width,
                                                    gdouble           height,
                                                    gboolean          use_offsets);
void       gimp_draw_tool_draw_arc                 (GimpDrawTool     *draw_tool,
                                                    gboolean          filled,
                                                    gdouble           x,
                                                    gdouble           y,
                                                    gdouble           width,
                                                    gdouble           height,
                                                    gint              angle1,
                                                    gint              angle2,
                                                    gboolean          use_offsets);

void       gimp_draw_tool_draw_rectangle_by_anchor (GimpDrawTool     *draw_tool,
                                                    gboolean          filled,
                                                    gdouble           x,
                                                    gdouble           y,
                                                    gint              width,
                                                    gint              height,
                                                    GtkAnchorType     anchor,
                                                    gboolean          use_offsets);
void       gimp_draw_tool_draw_arc_by_anchor       (GimpDrawTool     *draw_tool,
                                                    gboolean          filled,
                                                    gdouble           x,
                                                    gdouble           y,
                                                    gint              radius_x,
                                                    gint              radius_y,
                                                    gint              angle1,
                                                    gint              angle2,
                                                    GtkAnchorType     anchor,
                                                    gboolean          use_offsets);
void       gimp_draw_tool_draw_cross_by_anchor     (GimpDrawTool     *draw_tool,
                                                    gdouble           x,
                                                    gdouble           y,
                                                    gint              width,
                                                    gint              height,
                                                    GtkAnchorType     anchor,
                                                    gboolean          use_offsets);

void       gimp_draw_tool_draw_handle              (GimpDrawTool     *draw_tool,
                                                    GimpHandleType    type,
                                                    gdouble           x,
                                                    gdouble           y,
                                                    gint              width,
                                                    gint              height,
                                                    GtkAnchorType     anchor,
                                                    gboolean          use_offsets);
gboolean   gimp_draw_tool_on_handle                (GimpDrawTool     *draw_tool,
                                                    GimpDisplay      *display,
                                                    gdouble           x,
                                                    gdouble           y,
                                                    GimpHandleType    type,
                                                    gdouble           handle_x,
                                                    gdouble           handle_y,
                                                    gint              width,
                                                    gint              height,
                                                    GtkAnchorType     anchor,
                                                    gboolean          use_offsets);
gboolean   gimp_draw_tool_on_vectors_handle        (GimpDrawTool     *draw_tool,
                                                    GimpDisplay      *display,
                                                    GimpVectors      *vectors,
                                                    const GimpCoords *coord,
                                                    gint              width,
                                                    gint              height,
                                                    GimpAnchorType    preferred,
                                                    gboolean          exclusive,
                                                    GimpAnchor      **ret_anchor,
                                                    GimpStroke      **ret_stroke);
gboolean   gimp_draw_tool_on_vectors_curve         (GimpDrawTool     *draw_tool,
                                                    GimpDisplay      *display,
                                                    GimpVectors      *vectors,
                                                    const GimpCoords *coord,
                                                    gint              width,
                                                    gint              height,
                                                    GimpCoords       *ret_coords,
                                                    gdouble          *ret_pos,
                                                    GimpAnchor      **ret_segment_start,
                                                    GimpAnchor      **ret_segment_end,
                                                    GimpStroke      **ret_stroke);

gboolean   gimp_draw_tool_on_vectors               (GimpDrawTool     *draw_tool,
                                                    GimpDisplay      *display,
                                                    const GimpCoords *coord,
                                                    gint              width,
                                                    gint              height,
                                                    GimpCoords       *ret_coords,
                                                    gdouble          *ret_pos,
                                                    GimpAnchor      **ret_segment_start,
                                                    GimpAnchor      **ret_segment_end,
                                                    GimpStroke      **ret_stroke,
                                                    GimpVectors     **ret_vectors);

void       gimp_draw_tool_draw_lines               (GimpDrawTool     *draw_tool,
                                                    const gdouble    *points,
                                                    gint              n_points,
                                                    gboolean          filled,
                                                    gboolean          use_offsets);

void       gimp_draw_tool_draw_strokes             (GimpDrawTool     *draw_tool,
                                                    const GimpCoords *points,
                                                    gint              n_points,
                                                    gboolean          filled,
                                                    gboolean          use_offsets);

void       gimp_draw_tool_draw_boundary            (GimpDrawTool     *draw_tool,
                                                    const BoundSeg   *bound_segs,
                                                    gint              n_bound_segs,
                                                    gdouble           offset_x,
                                                    gdouble           offset_y,
                                                    gboolean          use_offsets);


#endif  /*  __GIMP_DRAW_TOOL_H__  */
