/*
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This is a plug-in for the GIMP.
 *
 * Generates images containing vector type drawings.
 *
 * Copyright (C) 1997 Andy Thomas  alt@picnic.demon.co.uk
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
 * 
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "config.h"
#include "libgimp/stdplugins-intl.h"

#include "gfig.h"
#include "gfig_poly.h"

static Dobject  * d_new_ellipse           (gint x, gint y);

static void
d_save_ellipse (Dobject *obj,
		FILE    *to)
{
  DobjPoints *spnt;

  spnt = obj->points;

  if (!spnt)
    return;

  fprintf (to, "<ELLIPSE>\n");

  for (; spnt; spnt = spnt->next)
    fprintf (to, "%d %d\n", spnt->pnt.x, spnt->pnt.y);

  fprintf (to, "</ELLIPSE>\n");
}

Dobject *
d_load_ellipse (FILE *from)
{
  Dobject *new_obj = NULL;
  gint     xpnt;
  gint     ypnt;
  gchar    buf[MAX_LOAD_LINE];

#ifdef DEBUG
  printf ("Load ellipse called\n");
#endif /* DEBUG */

  while (get_line (buf, MAX_LOAD_LINE, from, 0))
    {
      if (sscanf (buf, "%d %d", &xpnt, &ypnt) != 2)
	{
	  /* Must be the end */
	  if (strcmp ("</ELLIPSE>", buf))
	    {
	      g_message ("[%d] Internal load error while loading ellipse",
			 line_no);
	      return NULL;
	    }
	  return new_obj;
	}

      if (!new_obj)
	new_obj = d_new_ellipse (xpnt, ypnt);
      else
	{
	  DobjPoints *edge_pnt;
	  /* Circles only have two points */
	  edge_pnt = g_new0 (DobjPoints, 1);

	  edge_pnt->pnt.x = xpnt;
	  edge_pnt->pnt.y = ypnt;

	  new_obj->points->next = edge_pnt;
	}
    }

  g_message ("[%d] Not enough points for ellipse", line_no);
  return NULL;
}

static void
d_draw_ellipse (Dobject * obj)
{
  DobjPoints *center_pnt;
  DobjPoints *edge_pnt;
  gint bound_wx;
  gint bound_wy;
  gint top_x;
  gint top_y;

  center_pnt = obj->points;

  if (!center_pnt)
    return; /* End-of-line */

  edge_pnt = center_pnt->next;

  if (!edge_pnt)
    {
      g_warning ("Internal error - ellipse no edge pnt");
    }

  draw_sqr (&center_pnt->pnt);
  draw_sqr (&edge_pnt->pnt);

  bound_wx = abs (center_pnt->pnt.x - edge_pnt->pnt.x) * 2;
  bound_wy = abs (center_pnt->pnt.y - edge_pnt->pnt.y) * 2;

  if (edge_pnt->pnt.x > center_pnt->pnt.x)
    top_x = 2 * center_pnt->pnt.x - edge_pnt->pnt.x;
  else
    top_x = edge_pnt->pnt.x;
  
  if (edge_pnt->pnt.y > center_pnt->pnt.y)
    top_y = 2 * center_pnt->pnt.y - edge_pnt->pnt.y;
  else
    top_y = edge_pnt->pnt.y;

  if (drawing_pic)
    {
      gdk_draw_arc (pic_preview->window,
		    pic_preview->style->black_gc,
		    0,
		    adjust_pic_coords (top_x,
				       preview_width),
		    adjust_pic_coords (top_y,
				       preview_height),
		    adjust_pic_coords (bound_wx,
				       preview_width),
		    adjust_pic_coords (bound_wy,
				       preview_height),
		    0,
		    360 * 64);
    }
  else
    {
      gdk_draw_arc (gfig_preview->window,
		    gfig_gc,
		    0,
		    gfig_scale_x (top_x),
		    gfig_scale_y (top_y),
		    gfig_scale_x (bound_wx),
		    gfig_scale_y (bound_wy),
		    0,
		    360 * 64);
    }
}

static void
d_paint_approx_ellipse (Dobject *obj)
{
  /* first point center */
  /* Next point is radius */
  gdouble *line_pnts;
  gint seg_count = 0;
  gint i = 0;
  DobjPoints * center_pnt;
  DobjPoints * radius_pnt;
  gdouble a_axis;
  gdouble b_axis;
  gdouble ang_grid;
  gdouble ang_loop;
  gdouble radius;
  gint loop;
  GdkPoint first_pnt, last_pnt;
  gint first = 1;

  g_assert (obj != NULL);

  /* count - add one to close polygon */
  seg_count = 600;

  center_pnt = obj->points;

  if (!center_pnt || !seg_count)
    return; /* no-line */

  line_pnts = g_new0 (gdouble, 2 * seg_count + 1);

  /* Go around all the points drawing a line from one to the next */

  radius_pnt = center_pnt->next; /* this defines the vetices */

  /* Have center and radius - get lines */
  a_axis = ((gdouble) (radius_pnt->pnt.x - center_pnt->pnt.x));
  b_axis = ((gdouble) (radius_pnt->pnt.y - center_pnt->pnt.y));

  /* Lines */
  ang_grid = 2 * G_PI / (gdouble)  600;

  for (loop = 0; loop <  600; loop++)
    {
      gdouble lx, ly;
      GdkPoint calc_pnt;

      ang_loop = (gdouble)loop * ang_grid;

      radius = (a_axis * b_axis /
		(sqrt (cos (ang_loop) * cos (ang_loop) *
		       (b_axis * b_axis - a_axis * a_axis) + a_axis * a_axis)));

      lx = radius * cos (ang_loop);
      ly = radius * sin (ang_loop);

      calc_pnt.x = RINT (lx + center_pnt->pnt.x);
      calc_pnt.y = RINT (ly + center_pnt->pnt.y);

      /* Miss out duped pnts */
      if (!first)
	{
	  if (calc_pnt.x == last_pnt.x && calc_pnt.y == last_pnt.y)
	    {
	      continue;
	    }
	}

      last_pnt.x = line_pnts[i++] = calc_pnt.x;
      last_pnt.y = line_pnts[i++] = calc_pnt.y;

      if (first)
	{
	  first_pnt = calc_pnt;
	  first = 0;
	}
    }

  line_pnts[i++] = first_pnt.x;
  line_pnts[i++] = first_pnt.y;

  /* Reverse line if approp */
  if (selvals.reverselines)
    reverse_pairs_list (&line_pnts[0], i / 2);

  /* Scale before drawing */
  if (selvals.scaletoimage)
    scale_to_original_xy (&line_pnts[0], i / 2);
  else
    scale_to_xy (&line_pnts[0], i / 2);

  /* One go */
  if (selvals.painttype == PAINT_BRUSH_TYPE)
    {
      gfig_paint (selvals.brshtype,
		  gfig_drawable,
		  i, line_pnts);
    }
  else
    {
      gimp_free_select (gfig_image,
			i, line_pnts,
			selopt.type,
			selopt.antia,
			selopt.feather,
			selopt.feather_radius);
    }

  g_free (line_pnts);
}



static void
d_paint_ellipse (Dobject *obj)
{
  DobjPoints * center_pnt;
  DobjPoints * edge_pnt;
  gint bound_wx;
  gint bound_wy;
  gint top_x;
  gint top_y;
  gdouble dpnts[4];

  /* Drawing ellipse is hard .
   * 1) select circle
   * 2) stroke it
   */

  g_assert (obj != NULL);

  if (selvals.approxcircles)
    {
#ifdef DEBUG
      printf ("Painting ellipse as polygon\n");
#endif /* DEBUG */

      d_paint_approx_ellipse (obj);
      return;
    }      

  center_pnt = obj->points;

  if (!center_pnt)
    return; /* End-of-line */

  edge_pnt = center_pnt->next;

  if (!edge_pnt)
    {
      g_error ("Internal error - ellipse no edge pnt");
    }

  bound_wx = abs (center_pnt->pnt.x - edge_pnt->pnt.x)*2;
  bound_wy = abs (center_pnt->pnt.y - edge_pnt->pnt.y)*2;

  if (edge_pnt->pnt.x > center_pnt->pnt.x)
    top_x = 2*center_pnt->pnt.x - edge_pnt->pnt.x;
  else
    top_x = edge_pnt->pnt.x;
  
  if (edge_pnt->pnt.y > center_pnt->pnt.y)
    top_y = 2*center_pnt->pnt.y - edge_pnt->pnt.y;
  else
    top_y = edge_pnt->pnt.y;

  dpnts[0] = (gdouble)top_x;
  dpnts[1] = (gdouble)top_y;
  dpnts[2] = (gdouble)bound_wx;
  dpnts[3] = (gdouble)bound_wy;

  /* Scale before drawing */
  if (selvals.scaletoimage)
    scale_to_original_xy (&dpnts[0], 2);
  else
    scale_to_xy (&dpnts[0], 2);


  gimp_ellipse_select (gfig_image,
		       dpnts[0], dpnts[1],
		       dpnts[2], dpnts[3],
		       selopt.type,
		       selopt.antia,
		       selopt.feather,
		       selopt.feather_radius);

  /* Is selection all we need ? */
  if (selvals.painttype == PAINT_SELECTION_TYPE)
    return;

  gimp_edit_stroke (gfig_drawable);

  gimp_selection_clear (gfig_image);
}

static Dobject *
d_copy_ellipse (Dobject * obj)
{
  Dobject *nc;

  if (!obj)
    return (NULL);

  g_assert (obj->type == ELLIPSE);

  nc = d_new_ellipse (obj->points->pnt.x, obj->points->pnt.y);

  nc->points->next = d_copy_dobjpoints (obj->points->next);

  return nc;
}

static Dobject *
d_new_ellipse (gint x, gint y)
{
  Dobject *nobj;
  DobjPoints *npnt;
 
  /* Get new object and starting point */

  /* Start point */
  npnt = g_new0 (DobjPoints, 1);

#if DEBUG
  printf ("New ellipse start at (%x,%x)\n", x, y);
#endif /* DEBUG */

  npnt->pnt.x = x;
  npnt->pnt.y = y;

  nobj = g_new0 (Dobject, 1);

  nobj->type = ELLIPSE;
  nobj->points = npnt;
  nobj->drawfunc  = d_draw_ellipse;
  nobj->loadfunc  = d_load_ellipse;
  nobj->savefunc  = d_save_ellipse;
  nobj->paintfunc = d_paint_ellipse;
  nobj->copyfunc  = d_copy_ellipse;

  return (nobj);
}

void
d_update_ellipse (GdkPoint *pnt)
{
  DobjPoints *center_pnt, *edge_pnt;
  gint bound_wx;
  gint bound_wy;
  gint top_x;
  gint top_y;

  /* Undraw last one then draw new one */
  center_pnt = obj_creating->points;
  
  if (!center_pnt)
    return; /* No points */

  
  if ((edge_pnt = center_pnt->next))
    {
      /* Undraw current */
      bound_wx = abs (center_pnt->pnt.x - edge_pnt->pnt.x)*2;
      bound_wy = abs (center_pnt->pnt.y - edge_pnt->pnt.y)*2;
      
      if (edge_pnt->pnt.x > center_pnt->pnt.x)
	top_x = 2*center_pnt->pnt.x - edge_pnt->pnt.x;
      else
	top_x = edge_pnt->pnt.x;
      
      if (edge_pnt->pnt.y > center_pnt->pnt.y)
	top_y = 2*center_pnt->pnt.y - edge_pnt->pnt.y;
      else
	top_y = edge_pnt->pnt.y;

      draw_circle (&edge_pnt->pnt);
      
      gdk_draw_arc (gfig_preview->window,
		    gfig_gc,
		    0,
		    top_x,
		    top_y,
		    bound_wx,
		    bound_wy,
		    0,
		    360*64);
    }

  draw_circle (pnt);

  edge_pnt = g_new0 (DobjPoints, 1);

  edge_pnt->pnt.x = pnt->x;
  edge_pnt->pnt.y = pnt->y;

  bound_wx = abs (center_pnt->pnt.x - edge_pnt->pnt.x)*2;
  bound_wy = abs (center_pnt->pnt.y - edge_pnt->pnt.y)*2;

  if (edge_pnt->pnt.x > center_pnt->pnt.x)
    top_x = 2*center_pnt->pnt.x - edge_pnt->pnt.x;
  else
    top_x = edge_pnt->pnt.x;
  
  if (edge_pnt->pnt.y > center_pnt->pnt.y)
    top_y = 2* center_pnt->pnt.y - edge_pnt->pnt.y;
  else
    top_y = edge_pnt->pnt.y;
  
  gdk_draw_arc (gfig_preview->window,
		gfig_gc,
		0,
		top_x,
		top_y,
		bound_wx,
		bound_wy,
		0,
		360*64);
  
  center_pnt->next = edge_pnt;
}

void
d_ellipse_start (GdkPoint *pnt, gint shift_down)
{
  obj_creating = d_new_ellipse (pnt->x, pnt->y);
}

void
d_ellipse_end (GdkPoint *pnt, gint shift_down)
{
  /* Under contrl point */
  if (!obj_creating->points->next)
    {
      /* No circle created */
      free_one_obj (obj_creating);
    }
  else
    {
      draw_circle (pnt);
      add_to_all_obj (current_obj, obj_creating);
    }

  obj_creating = NULL;
}
