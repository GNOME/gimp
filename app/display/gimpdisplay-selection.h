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

typedef struct _selection Selection;

struct _selection
{
  /*  This information is for maintaining the selection's appearance  */
  GdkWindow *   win;             /*  Window to draw to                 */
  void *        gdisp;           /*  GDisplay that owns the selection  */
  GdkGC *       gc_in;           /*  GC for drawing selection outline  */
  GdkGC *       gc_out;          /*  GC for selected regions outside current layer */
  GdkGC *       gc_layer;        /*  GC for current layer outline      */

  /*  This information is for drawing the marching ants around the border  */
  GdkSegment *  segs_in;         /*  gdk segments of area boundary     */
  GdkSegment *  segs_out;        /*  gdk segments of area boundary     */
  GdkSegment *  segs_layer;      /*  gdk segments of area boundary     */
  int           num_segs_in;     /*  number of segments in segs1       */
  int           num_segs_out;    /*  number of segments in segs2       */
  int           num_segs_layer;  /*  number of segments in segs3       */
  int           index_in;        /*  index of current stipple pattern  */
  int           index_out;       /*  index of current stipple pattern  */
  int           index_layer;     /*  index of current stipple pattern  */
  int           state;           /*  internal drawing state            */
  int           paused;          /*  count of pause requests           */
  int           recalc;          /*  flag to recalculate the selection */
  int           speed;           /*  speed of marching ants            */
  int           hidden;          /*  is the selection hidden?          */
  gint          timer;           /*  timer for successive draws        */
  int           cycle;           /*  color cycling turned on           */
  GdkPixmap *   cycle_pix;       /*  cycling pixmap                    */

  /* These are used only if USE_XDRAWPOINTS is defined. */
  GdkPoint *    points_in[8];    /*  points of segs_in for fast ants   */
  int           num_points_in[8]; /* number of points in points_in     */
  GdkGC *       gc_white;        /*  gc for drawing white points       */
  GdkGC *       gc_black;        /*  gc for drawing black points       */
};

/*  Function declarations  */

Selection *  selection_create          (GdkWindow *, gpointer, int, int, int);
void         selection_pause           (Selection *);
void         selection_resume          (Selection *);
void         selection_start           (Selection *, int);
void         selection_invis           (Selection *);
void         selection_layer_invis     (Selection *);
void         selection_hide            (Selection *, void *);
void         selection_free            (Selection *);

#endif  /*  __SELECTION_H__  */
