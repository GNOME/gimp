/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-2003 Spencer Kimball, Peter Mattis, and others.
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

#ifndef __GIMP_TOOL_GUI_H__
#define __GIMP_TOOL_GUI_H__


#include "libgimptool/gimptool.h"


typedef enum
{
  GIMP_TOOL_GUI_STATE_INVISIBLE,
  GIMP_TOOL_GUI_STATE_VISIBLE
} GimpToolGuiState;

typedef enum
{
  GIMP_HANDLE_SQUARE,
  GIMP_HANDLE_FILLED_SQUARE,
  GIMP_HANDLE_CIRCLE,
  GIMP_HANDLE_FILLED_CIRCLE,
  GIMP_HANDLE_CROSS
} GimpHandleType;


#define GIMP_TYPE_TOOL_GUI            (gimp_tool_gui_get_type ())
#define GIMP_TOOL_GUI(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_TOOL_GUI, GimpToolGui))
#define GIMP_TOOL_GUI_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_TOOL_GUI, GimpToolGuiClass))
#define GIMP_IS_TOOL_GUI(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_TOOL_GUI))
#define GIMP_IS_TOOL_GUI_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_TOOL_GUI))
#define GIMP_TOOL_GUI_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_TOOL_GUI, GimpToolGuiClass))


typedef struct _GimpToolGuiClass GimpToolGuiClass;

struct _GimpToolGui
{
  GimpObject         parent_instance;

  GimpDisplay       *gdisp;        /*  The display we are drawing on (may be
                                    *  a different one than tool->gdisp)
                                    */
  GdkWindow         *win;          /*  Window to draw draw operation to       */
  GdkGC             *gc;           /*  Graphics context for draw functions    */

  GimpToolGuiState   draw_state;   /*  Current state in the draw process      */
  gint               paused_count; /*  count to keep track of multiple pauses */

  gint               line_width;   /*  line attributes                        */
  gint               line_style;   /**/
  gint               cap_style;    /**/
  gint               join_style;   /**/
};

struct _GimpToolGuiClass
{
  GimpObjectClass   parent_class;

  /*  virtual functions  */

  void (* draw)   (GimpToolGui *tool_gui);
  
  void (* start)  (GimpToolGui *tool, 
                   GimpDisplay *gdisp);
  void (* stop)   (GimpToolGui *tool); 
  void (* pause)  (GimpToolGui *tool); 
  void (* resume) (GimpToolGui *tool); 
};


GType      gimp_tool_gui_get_type                 (void) G_GNUC_CONST;

void       gimp_tool_gui_draw                     (GimpToolGui   *tool_gui);

void       gimp_tool_gui_start                    (GimpToolGui   *tool_gui,
                                                   GimpDisplay   *gdisp);
void       gimp_tool_gui_stop                     (GimpToolGui   *tool_gui);
void       gimp_tool_gui_pause                    (GimpToolGui   *tool_gui);
void       gimp_tool_gui_resume                   (GimpToolGui   *tool_gui);

gdouble    gimp_tool_gui_calc_distance            (GimpToolGui   *tool_gui,
                                                   GimpDisplay   *gdisp,
                                                   gdouble        x1,
                                                   gdouble        y1,
                                                   gdouble        x2,
                                                   gdouble        y2);
gboolean   gimp_tool_gui_in_radius                (GimpToolGui   *tool_gui,
                                                   GimpDisplay   *gdisp,
                                                   gdouble        x1,
                                                   gdouble        y1,
                                                   gdouble        x2,
                                                   gdouble        y2,
                                                   gint           radius);

void       gimp_tool_gui_draw_line                (GimpToolGui   *tool_gui,
                                                   gdouble        x1,
                                                   gdouble        y1,
                                                   gdouble        x2,
                                                   gdouble        y2,
                                                   gboolean       use_offsets);
void       gimp_tool_gui_draw_rectangle           (GimpToolGui   *tool_gui,
                                                   gboolean       filled,
                                                   gdouble        x,
                                                   gdouble        y,
                                                   gdouble        width,
                                                   gdouble        height,
                                                   gboolean       use_offsets);
void       gimp_tool_gui_draw_arc                 (GimpToolGui   *tool_gui,
                                                   gboolean       filled,
                                                   gdouble        x,
                                                   gdouble        y,
                                                   gdouble        width,
                                                   gdouble        height,
                                                   gint           angle1,
                                                   gint           angle2,
                                                   gboolean       use_offsets);

void       gimp_tool_gui_draw_rectangle_by_anchor (GimpToolGui   *tool_gui,
                                                   gboolean       filled,
                                                   gdouble        x,
                                                   gdouble        y,
                                                   gint           width,
                                                   gint           height,
                                                   GtkAnchorType  anchor,
                                                   gboolean       use_offsets);
void       gimp_tool_gui_draw_arc_by_anchor       (GimpToolGui   *tool_gui,
                                                   gboolean       filled,
                                                   gdouble        x,
                                                   gdouble        y,
                                                   gint           radius_x,
                                                   gint           radius_y,
                                                   gint           angle1,
                                                   gint           angle2,
                                                   GtkAnchorType  anchor,
                                                   gboolean       use_offsets);
void       gimp_tool_gui_draw_cross_by_anchor     (GimpToolGui   *tool_gui,
                                                   gdouble        x,
                                                   gdouble        y,
                                                   gint           width,
                                                   gint           height,
                                                   GtkAnchorType  anchor,
                                                   gboolean       use_offsets);

void       gimp_tool_gui_draw_handle              (GimpToolGui   *tool_gui, 
                                                   GimpHandleType type,
                                                   gdouble        x,
                                                   gdouble        y,
                                                   gint           width,
                                                   gint           height,
                                                   GtkAnchorType  anchor,
                                                   gboolean       use_offsets);
gboolean   gimp_tool_gui_on_handle                (GimpToolGui   *tool_gui,
                                                   GimpDisplay   *gdisp,
                                                   gdouble        x,
                                                   gdouble        y,
                                                   GimpHandleType type,
                                                   gdouble        handle_x,
                                                   gdouble        handle_y,
                                                   gint           width,
                                                   gint           height,
                                                   GtkAnchorType  anchor,
                                                   gboolean       use_offsets);

void       gimp_tool_gui_draw_lines               (GimpToolGui   *tool_gui, 
                                                   gdouble       *points,
                                                   gint           n_points,
                                                   gboolean       filled,
                                                   gboolean       use_offsets);

void       gimp_tool_gui_draw_strokes             (GimpToolGui   *tool_gui, 
                                                   GimpCoords    *points,
                                                   gint           n_points,
                                                   gboolean       filled,
                                                   gboolean       use_offsets);


#endif  /*  __GIMP_TOOL_GUI_H__  */
