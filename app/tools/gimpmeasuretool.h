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

#ifndef __MEASURE_H__
#define __MEASURE_H__


#include "tool.h"


/*  possible measure functions  */
typedef enum 
{
  CREATING,
  ADDING,
  MOVING,
  MOVING_ALL,
  GUIDING,
  FINISHED
} MeasureFunction;


#define GIMP_TYPE_MEASURE_TOOL            (gimp_measure_tool_get_type ())
#define GIMP_MEASURE_TOOL(obj)            (GTK_CHECK_CAST ((obj), GIMP_TYPE_MEASURE_TOOL, GimpMeasureTool))
#define GIMP_IS_MEASURE_TOOL(obj)         (GTK_CHECK_TYPE ((obj), GIMP_TYPE_MEASURE_TOOL))
#define GIMP_MEASURE_TOOL_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GIMP_TYPE_MEASURE_TOOL, GimpMeasureToolClass))
#define GIMP_IS_MEASURE_TOOL_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_MEASURE_TOOL))

typedef struct _GimpMeasureTool      GimpMeasureTool;
typedef struct _GimpMeasureToolClass GimpMeasureToolClass;

struct _GimpMeasureTool
{
  GimpTool  parent_instance;

  DrawCore *core;       /*  Core select object  */

  MeasureFunction  function;    /*  function we're performing  */
  gint             last_x;      /*  last x coordinate          */
  gint             last_y;      /*  last y coordinate          */
  gint             point;       /*  what are we manipulating?  */
  gint             num_points;  /*  how many points?           */
  gint             x[3];        /*  three x coordinates        */
  gint             y[3];        /*  three y coordinates        */
  gdouble          angle1;      /*  first angle                */
  gdouble          angle2;      /*  second angle               */
  guint            context_id;  /*  for the statusbar          */
};

struct _GimpMeasureToolClass
{
  GimpToolClass parent_class;
};


GimpTool * tools_new_measure_tool     (void);
GtkType    gimp_measure_tool_get_type (void);

void       gimp_measure_tool_register (void);


#endif  /*  __MEASURE_H__  */
