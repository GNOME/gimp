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

#ifndef  __GIMP_INK_TOOL_H__
#define  __GIMP_INK_TOOL_H__


#include "tool.h"
#include "blob.h"  /* only used by ink */


#define DIST_SMOOTHER_BUFFER 10
#define TIME_SMOOTHER_BUFFER 10


#define GIMP_TYPE_INK_TOOL            (gimp_ink_tool_get_type ())
#define GIMP_INK_TOOL(obj)            (GTK_CHECK_CAST ((obj), GIMP_TYPE_INK_TOOL, GimpInkTool))
#define GIMP_IS_INK_TOOL(obj)         (GTK_CHECK_TYPE ((obj), GIMP_TYPE_INK_TOOL))
#define GIMP_INK_TOOL_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GIMP_TYPE_INK_TOOL, GimpInkToolClass))
#define GIMP_IS_INK_TOOL_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_INK_TOOL))


typedef struct _GimpInkTool      GimpInkTool;
typedef struct _GimpInkToolClass GimpInkToolClass;

struct _GimpInkTool
{
  GimpTool  parent_instance;
  
  Blob     *last_blob;	   /*  blob for last cursor position  */

  gint      x1, y1;        /*  image space coordinate         */
  gint      x2, y2;        /*  image space coords             */

  /* circular distance history buffer */
  gdouble   dt_buffer[DIST_SMOOTHER_BUFFER];
  gint      dt_index;

  /* circular timing history buffer */
  guint32   ts_buffer[TIME_SMOOTHER_BUFFER];
  gint      ts_index;

  gdouble   last_time;     /*  previous time of a motion event      */
  gdouble   lastx, lasty;  /*  previous position of a motion event  */

  gboolean  init_velocity;
};

struct _GimpInkToolClass
{
  GimpToolClass  parent_class;
};


void      gimp_ink_tool_register (void);

GtkType   gimp_ink_tool_get_type (void);


#endif  /*  __GIMP_INK_TOOL_H__  */
