/* The GIMP -- an image manipulation program
 * 
 * This file Copyright (C) 1999 Simon Budig
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

#ifndef __GIMP_PATH_TOOL_H__
#define __GIMP_PATH_TOOL_H__

#include "gimpdrawtool.h"
#include "path_curves.h"


#define GIMP_TYPE_PATH_TOOL            (gimp_path_tool_get_type ())
#define GIMP_PATH_TOOL(obj)            (GTK_CHECK_CAST ((obj), GIMP_TYPE_PATH_TOOL, GimpPathTool))
#define GIMP_IS_PATH_TOOL(obj)         (GTK_CHECK_TYPE ((obj), GIMP_TYPE_PATH_TOOL))
#define GIMP_PATH_TOOL_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GIMP_TYPE_PATH_TOOL, GimpPathToolClass))
#define GIMP_IS_PATH_TOOL_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_PATH_TOOL))


typedef struct _GimpPathToolClass GimpPathToolClass;

struct _GimpPathTool
{
  GimpDrawTool     parent_instance;

  gint         click_type;      /* where did the user click?         */
  gdouble      click_x;         /* X-coordinate of the click         */
  gdouble      click_y;         /* Y-coordinate of the click         */
  gint         click_halfwidth;
  guint        click_modifier;  /* what modifiers were pressed?      */
  NPath       *click_path;      /* On which Path/Curve/Segment       */
  PathCurve   *click_curve;     /* was the click?                    */
  PathSegment *click_segment;
  gdouble      click_position;  /* The position on the segment       */
  gint         click_handle_id; /* The handle ID of the segment      */

  gint         active_count;    /* How many segments are active?     */
  /*
   * WARNING: single_active_segment may contain non NULL Values
   * which point to the nirvana. But they are important!
   * The pointer is garantueed to be valid, when active_count==1
   */
  PathSegment *single_active_segment;  /* The only active segment    */

  gint         state;           /* state of tool                     */
  gint         draw;            /* all or part                       */
  NPath       *cur_path;        /* the current active path           */
  GSList     **scanlines;       /* used in converting a path         */
};

struct _GimpPathToolClass
{
  GimpDrawToolClass parent_class;
};


void     gimp_path_tool_register        (Gimp *gimp);

GtkType  gimp_path_tool_get_type        (void);



#endif  /*  __GIMP_PATH_TOOL_H__  */
