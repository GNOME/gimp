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

#ifndef __SELECTION_H__
#define __SELECTION_H__


struct _Selection
{
  /*  This information is for maintaining the selection's appearance  */
  GdkWindow   *win;              /*  Window to draw to                 */
  GDisplay    *gdisp;            /*  GDisplay that owns the selection  */
  GdkGC       *gc_in;            /*  GC for drawing selection outline  */
  GdkGC       *gc_out;           /*  GC for selected regions outside
				  *  current layer */
  GdkGC       *gc_layer;         /*  GC for current layer outline      */

  /*  This information is for drawing the marching ants around the border  */
  GdkSegment  *segs_in;          /*  gdk segments of area boundary     */
  GdkSegment  *segs_out;         /*  gdk segments of area boundary     */
  GdkSegment  *segs_layer;       /*  gdk segments of area boundary     */
  gint         num_segs_in;      /*  number of segments in segs1       */
  gint         num_segs_out;     /*  number of segments in segs2       */
  gint         num_segs_layer;   /*  number of segments in segs3       */
  gint         index_in;         /*  index of current stipple pattern  */
  gint         index_out;        /*  index of current stipple pattern  */
  gint         index_layer;      /*  index of current stipple pattern  */
  gint         state;            /*  internal drawing state            */
  gint         paused;           /*  count of pause requests           */
  gint         recalc;           /*  flag to recalculate the selection */
  gint         speed;            /*  speed of marching ants            */
  gint         hidden;           /*  is the selection hidden?          */
  gint         timer;            /*  timer for successive draws        */
  gint         cycle;            /*  color cycling turned on           */
  GdkPixmap   *cycle_pix;        /*  cycling pixmap                    */

  /* These are used only if USE_XDRAWPOINTS is defined. */
  GdkPoint    *points_in[8];     /*  points of segs_in for fast ants   */
  gint         num_points_in[8]; /*  number of points in points_in     */
  GdkGC       *gc_white;         /*  gc for drawing white points       */
  GdkGC       *gc_black;         /*  gc for drawing black points       */
};


/*  Function declarations  */

Selection *  selection_create          (GdkWindow *win,
					GDisplay  *disp,
					gint       size,
					gint       width,
					gint       speed);
void         selection_pause           (Selection *select);
void         selection_resume          (Selection *select);
void         selection_start           (Selection *select,
					gint       recalc);
void         selection_invis           (Selection *select);
void         selection_layer_invis     (Selection *select);
void         selection_hide            (Selection *select,
					GDisplay  *gdisp);
void         selection_free            (Selection *select);


#endif  /*  __SELECTION_H__  */
