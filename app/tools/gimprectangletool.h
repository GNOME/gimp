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

#ifndef  __GIMP_RECTANGLE_TOOL_H__
#define  __GIMP_RECTANGLE_TOOL_H__


#ifdef __GNUC__
#warning FIXME #include "gui/gui-types.h"
#endif
#include "gui/gui-types.h"

#include "gimpselectiontool.h"


#define GIMP_TYPE_RECTANGLE_TOOL            (gimp_rectangle_tool_get_type ())
#define GIMP_RECTANGLE_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_RECTANGLE_TOOL, GimpRectangleTool))
#define GIMP_RECTANGLE_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_RECTANGLE_TOOL, GimpRectangleToolClass))
#define GIMP_IS_RECTANGLE_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_RECTANGLE_TOOL))
#define GIMP_RECTANGLE_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_RECTANGLE_TOOL, GimpRectangleToolClass))
#define GIMP_RECTANGLE_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_RECTANGLE_TOOL, GimpRectangleToolClass))


/*  possible functions  */
enum
{
  RECT_CREATING,
  RECT_MOVING,
  RECT_RESIZING_LEFT,
  RECT_RESIZING_RIGHT,
  RECT_EXECUTING
};


typedef struct _GimpRectangleTool      GimpRectangleTool;
typedef struct _GimpRectangleToolClass GimpRectangleToolClass;

struct _GimpRectangleTool
{
  GimpSelectionTool  parent_instance;

  GtkWidget    *controls;
  GtkWidget    *dimensions_entry;

  gint          startx;     /*  starting x coord            */
  gint          starty;     /*  starting y coord            */

  gint          lastx;      /*  previous x coord            */
  gint          lasty;      /*  previous y coord            */

  gint          x1, y1;     /*  upper left hand coordinate  */
  gint          x2, y2;     /*  lower right hand coords     */

  guint         function;   /*  moving or resizing          */

  gint          dx1, dy1;   /*  display coords              */
  gint          dx2, dy2;   /*                              */

  gint          dcw, dch;   /*  width and height of corners */

  gdouble       orig_vals[2];
  gdouble       size_vals[2];
  gdouble	aspect_ratio;
  gboolean	change_aspect_ratio; /* Boolean for the rectangle_info_update function */
  				     /* aspect_ratio should not be changed whith   */
  				     /* rectangle_info_update when it is called from      */
				     /* rectangle_aspect_changed, due to the inaccurate*/
  				     /* decimal precision                         */

};

struct _GimpRectangleToolClass
{
  GimpSelectionToolClass parent_class;

  /*  virtual function  */

  void (* execute) (GimpRectangleTool *rect_tool,
                    gint               x,
                    gint               y,
                    gint               w,
                    gint               h);
};


GType   gimp_rectangle_tool_get_type (void) G_GNUC_CONST;


#endif  /*  __GIMP_RECTANGLE_TOOL_H__  */
