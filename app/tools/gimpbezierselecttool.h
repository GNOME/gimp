/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#ifndef __GIMP_BEZIER_SELECT_TOOL_H__
#define __GIMP_BEZIER_SELECT_TOOL_H__


#include "gimpselectiontool.h"


#define GIMP_TYPE_BEZIER_SELECT_TOOL            (gimp_bezier_select_tool_get_type ())
#define GIMP_BEZIER_SELECT_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_BEZIER_SELECT_TOOL, GimpBezierSelectTool))
#define GIMP_BEZIER_SELECT_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_BEZIER_SELECT_TOOL, GimpBezierSelectToolClass))
#define GIMP_IS_BEZIER_SELECT_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_BEZIER_SELECT_TOOL))
#define GIMP_IS_BEZIER_SELECT_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_BEZIER_SELECT_TOOL))
#define GIMP_BEZIER_SELECT_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_BEZIER_SELECT_TOOL, GimpBezierSelectToolClass))


#define BEZIER_START     1
#define BEZIER_ADD       2
#define BEZIER_EDIT      4
#define BEZIER_DRAG      8

#define BEZIER_ANCHOR    1
#define BEZIER_CONTROL   2
#define BEZIER_MOVE      3

#define IMAGE_COORDS    1
#define AA_IMAGE_COORDS 2
#define SCREEN_COORDS   3

#define SUBDIVIDE  1000

enum { EXTEND_EDIT, EXTEND_ADD, EXTEND_REMOVE, EXTEND_NEW };


/* typedef struct _GimpBezierSelectTool      GimpBezierSelectTool;
 * typedef struct _GimpBezierSelectPoint     GimpBezierSelectPoint; */
typedef struct _GimpBezierSelectToolClass GimpBezierSelectToolClass;

struct _GimpBezierSelectPoint
{
  gint         type;          /* type of point (anchor/control/move) */
  gdouble      x, y;          /* location of point in image space  */
  gint         sx, sy;        /* location of point in screen space */

  GimpBezierSelectPoint *next;          /* next point on curve               */
  GimpBezierSelectPoint *prev;          /* prev point on curve               */
  GimpBezierSelectPoint *next_curve;    /* Next curve segment                */

  gint         pointflags;    /* Status of point 0 = not selected 
			       * 1 = selected 
			       */ 
};

struct _GimpBezierSelectTool
{
  GimpSelectionTool parent_instance;

  gint          state;        /* start, add, edit or drag          */
  gint          draw_mode;    /* all or part                       */
  gint          closed;       /* is the last curve closed          */

  GimpBezierSelectPoint  *points;       /* the curve                         */
  GimpBezierSelectPoint  *cur_anchor;   /* the current active anchor point   */
  GimpBezierSelectPoint  *cur_control;  /* the current active control point  */
  GimpBezierSelectPoint  *last_point;   /* the last point on the curve       */

  gint          num_points;   /* number of points in the curve     */
  GimpChannel  *mask;         /* null if the curve is open         */
  GSList      **scanlines;    /* used in converting a curve        */
};


struct _GimpBezierSelectToolClass
{
  GimpSelectionToolClass parent_class;
};

typedef void (* GimpBezierSelectPointsFunc) (GimpBezierSelectTool *bezier_sel,
					     GdkPoint     *points,
					     gint          n_points,
					     gpointer      data);

typedef struct
{
  gint count;
} CountCurves;


/* Public functions */

void    gimp_bezier_select_tool_register (GimpToolRegisterCallback  callback,
                                          gpointer                  data);

GType   gimp_bezier_select_tool_get_type (void) G_GNUC_CONST;


gboolean   bezier_tool_selected      (void);

void       gimp_bezier_select_tool_bezier_select (GimpBezierSelectTool *rect_tool,
                                              gint                x,
                                              gint                y,
                                              gint                w,
                                              gint                h);


void       bezier_select                     (GimpImage          *gimage,
                                              gint                x,
                                              gint                y,
                                              gint                w,
                                              gint                h,
                                              SelectOps           op,
                                              gboolean            feather,
                                              gdouble             feather_radius);

gint  bezier_select_load                   (GimpDisplay         *gdisp,
					    GimpBezierSelectPoint      *points,
					    gint              n_points,
					    gint              closed);
void  bezier_draw_curve                    (GimpBezierSelectTool     *bezier_sel,
					    GimpBezierSelectPointsFunc  func,
					    gint              coord,
					    gpointer          data);

void  bezier_select_reset                  (GimpBezierSelectTool     *bezier_sel);
void  bezier_select_free                   (GimpBezierSelectTool     *bezier_sel);
GimpBezierSelectTool* path_to_beziersel    (Path                     *path);

void  bezier_add_point                     (GimpBezierSelectTool     *bezier_sel,
					    gint              type,
					    gdouble           x,
					    gdouble           y);
void  bezier_paste_bezierselect_to_current (GimpDisplay         *gdisp,
					    GimpBezierSelectTool     *bezier_sel);
void  bezier_select_mode                   (gint);
void  bezier_stroke 		           (GimpBezierSelectTool     *bezier_sel,
					    GimpDisplay         *gdisp,
					    gint              subdivisions,
					    gint              open_path);
void  bezier_to_selection                  (GimpBezierSelectTool     *bezier_sel,
					    GimpDisplay         *gdisp);
gint  bezier_distance_along                (GimpBezierSelectTool     *bezier_sel,
					    gint              open_path,
					    gdouble           dist,
					    gint             *x,
					    gint             *y,
					    gdouble          *gradient);
void  bezier_draw                          (GimpDisplay         *gdisp,
					    GimpBezierSelectTool     *bezier_sel);


#endif  /*  __GIMP_BEZIER_SELECT_TOOL_H__  */
