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
 */

#include <stdio.h>
#include <stdlib.h>

#include <gtk/gtk.h>

#include "config.h"
#include "libgimp/stdplugins-intl.h"

#include "gfig.h"
#include "gfig_arc.h"
#include "gfig_bezier.h"
#include "gfig_circle.h"
#include "gfig_dobject.h"
#include "gfig_ellipse.h"
#include "gfig_line.h"
#include "gfig_poly.h"
#include "gfig_spiral.h"
#include "gfig_star.h"

static Dobject  *operation_obj;
static GdkPoint *move_all_pnt; /* Point moving all from */

static void     draw_one_obj    (Dobject * obj);
static void	do_move_obj (Dobject  *obj,
			     GdkPoint *to_pnt);
static void	do_move_all_obj (GdkPoint *to_pnt);
static void	do_move_obj_pnt (Dobject  *obj,
				 GdkPoint *to_pnt);
static void	remove_obj_from_list (GFigObj *obj,
				      Dobject *del_obj);
static gint	scan_obj_points (DobjPoints *opnt,
				 GdkPoint   *pnt);

/* Delete a list of points */
void
d_delete_dobjpoints (DobjPoints * pnts)
{
  DobjPoints *next;
  DobjPoints *pnt2del = pnts;

  while (pnt2del)
    {
      next = pnt2del->next;
      g_free (pnt2del);
      pnt2del = next;
    }
}

DobjPoints*
new_dobjpoint (gint x, gint y)
{
  DobjPoints *npnt = g_new0 (DobjPoints, 1);

  npnt->pnt.x = x;
  npnt->pnt.y = y;
  
  return npnt;
}

DobjPoints *
d_copy_dobjpoints (DobjPoints * pnts)
{
  DobjPoints *ret = NULL;
  DobjPoints *head = NULL;
  DobjPoints *newpnt;
  DobjPoints *pnt2copy = pnts;

  while (pnt2copy)
    {
      newpnt = g_new0 (DobjPoints, 1);
      newpnt->pnt.x = pnt2copy->pnt.x;
      newpnt->pnt.y = pnt2copy->pnt.y;

      if (!ret)
	head = ret = newpnt;
      else
	{
	  head->next = newpnt;
	  head = newpnt;
	}
      pnt2copy = pnt2copy->next;
    }

  return ret;
}

static DobjPoints *
get_diffs (Dobject  *obj,
	   gint16   *xdiff,
	   gint16   *ydiff,
	   GdkPoint *to_pnt)
{
  DobjPoints *spnt;

  g_assert (obj != NULL);

  for (spnt = obj->points; spnt; spnt = spnt->next)
    {
      if (spnt->found_me)
	{
	  *xdiff = spnt->pnt.x - to_pnt->x;
	  *ydiff = spnt->pnt.y - to_pnt->y;
	  return spnt;
	}
    }
  return NULL;
}

static gboolean
inside_sqr (GdkPoint *cpnt,
	    GdkPoint *testpnt)
{
  /* Return TRUE if testpnt is near cpnt */
  gint16 x = cpnt->x;
  gint16 y = cpnt->y;
  gint16 tx = testpnt->x;
  gint16 ty = testpnt->y;

  return (abs (x - tx) <= SQ_SIZE && abs (y - ty) < SQ_SIZE);
}

static gint
scan_obj_points (DobjPoints *opnt,
		 GdkPoint   *pnt)
{
  while (opnt)
    {
      if (inside_sqr (&opnt->pnt, pnt))
	{
	  opnt->found_me = TRUE;
	  return TRUE;
	}
      opnt->found_me = FALSE;
      opnt = opnt->next;
    }
  return FALSE;
}

static Dobject *
get_nearest_objs (GFigObj  *obj,
		  GdkPoint *pnt)
{
  /* Nearest object to given point or NULL */
  DAllObjs *all;
  Dobject  *test_obj;
  gint count = 0;

  if (!obj)
    return NULL;

  all = obj->obj_list;

  while (all)
    {
      test_obj = all->obj;

      if (count == obj_show_single || obj_show_single == -1)
	if (scan_obj_points (test_obj->points, pnt))
	  {
	    return test_obj;
	  }
      all = all->next;
      count++;
    }
  return NULL;
}

void
object_operation_start (GdkPoint *pnt,
			gint      shift_down)
{
  Dobject *new_obj;

  /* Find point in given object list */
  operation_obj = get_nearest_objs (current_obj, pnt);

  /* Special case if shift down && move obj then moving all objs */

  if (shift_down && selvals.otype == MOVE_OBJ)
    {
      move_all_pnt = g_malloc0 (sizeof (*move_all_pnt));
      *move_all_pnt = *pnt; /* Structure copy */
      setup_undo ();
      return;
    }

  if (!operation_obj)
    return;/* None to work on */

  setup_undo ();

  switch (selvals.otype)
    {
    case MOVE_OBJ:
      if (operation_obj->type == BEZIER)
	{
	  d_draw_bezier (operation_obj);
	  tmp_bezier = operation_obj;
	  d_draw_bezier (operation_obj);
	}
      break;
    case MOVE_POINT:
      if (operation_obj->type == BEZIER)
	{
	  d_draw_bezier (operation_obj);
	  tmp_bezier = operation_obj;
	  d_draw_bezier (operation_obj);
	}
      /* If shift is down the break into sep lines */
      if ((operation_obj->type == POLY  
	  || operation_obj->type == STAR)
	 && shift_down)
	{
	  switch (operation_obj->type)
	    {
	    case POLY:
	      d_poly2lines (operation_obj);
	      break;
	    case STAR:
	      d_star2lines (operation_obj);
	      break;
	    default:
	      break;
	    }
	  /* Re calc which object point we are lookin at */
	  scan_obj_points (operation_obj->points, pnt);
	}
      break;
    case COPY_OBJ:
      /* Copy the "operation object" */
      /* Then bung us into "copy/move" mode */

      new_obj = (Dobject *) operation_obj->copyfunc (operation_obj);
      if (new_obj)
	{
	  scan_obj_points (new_obj->points, pnt);
	  add_to_all_obj (current_obj, new_obj);
	  operation_obj = new_obj;
	  selvals.otype = MOVE_COPY_OBJ;
	  new_obj->drawfunc (new_obj);
	}
      break;
    case DEL_OBJ:
      remove_obj_from_list (current_obj, operation_obj);
      break;
    case MOVE_COPY_OBJ: /* Never when button down */
    default:
      g_warning ("Internal error selvals.otype object operation start");
      break;
    }
}

void
object_operation_end (GdkPoint *pnt,
		      gint      shift_down)
{
  if (selvals.otype != DEL_OBJ && operation_obj && 
      operation_obj->type == BEZIER)
    {
      d_draw_bezier (operation_obj);
      tmp_bezier = NULL; /* use as switch */
      d_draw_bezier (operation_obj);
    }

  operation_obj = NULL;

  if (move_all_pnt)
    {
      g_free (move_all_pnt);
      move_all_pnt = 0;
    }

  /* Special case - if copying mode MUST be copy when button up received */
  if (selvals.otype == MOVE_COPY_OBJ)
    selvals.otype = COPY_OBJ;
}

/* Move object around */
void
object_operation (GdkPoint *to_pnt,
		  gint      shift_down)
{
  /* Must do diffent things depending on object type */
  /* but must have object to operate on! */

  /* Special case - if shift own and move_obj then move ALL objects */
  if (move_all_pnt && shift_down && selvals.otype == MOVE_OBJ)
    {
      do_move_all_obj (to_pnt);
      return;
    }

  if (!operation_obj)
    return;

  switch (selvals.otype)
    {
    case MOVE_OBJ:
    case MOVE_COPY_OBJ:
      switch (operation_obj->type)
	{
	case LINE:
	case CIRCLE:
	case ELLIPSE:
	case POLY:
	case ARC:
	case STAR:
	case SPIRAL:
	case BEZIER:
	  do_move_obj (operation_obj, to_pnt);
	  break;
	default:
	  /* Internal error */
	  g_warning ("Internal error in operation_obj->type");
	  break;
	}
      break;
    case MOVE_POINT:
      switch (operation_obj->type)
	{
	case LINE:
	case CIRCLE:
	case ELLIPSE:
	case POLY:
	case ARC:
	case STAR:
	case SPIRAL:
	case BEZIER:
	  do_move_obj_pnt (operation_obj, to_pnt);
	  break;
	default:
	  /* Internal error */
	  g_warning ("Internal error in operation_obj->type");
	  break;
	}
      break;
    case DEL_OBJ:
      break;
    case COPY_OBJ: /* Should have been changed to MOVE_COPY_OBJ */
    default:
      g_warning ("Internal error selvals.otype");
      break;
    }
}

static void
update_pnts (Dobject *obj,
	     gint16   xdiff,
	     gint16   ydiff)
{
  DobjPoints *spnt;

  g_assert (obj != NULL);

  /* Update all pnts */
  for (spnt = obj->points; spnt; spnt = spnt->next)
    {
      spnt->pnt.x -= xdiff;
      spnt->pnt.y -= ydiff;
    }
}

static void
remove_obj_from_list (GFigObj *obj,
		      Dobject *del_obj)
{
  /* Nearest object to given point or NULL */
  DAllObjs *all;
  DAllObjs *prev_all = NULL;
  
  g_assert (del_obj != NULL);

  all = obj->obj_list;

  while (all)
    {
      if (all->obj == del_obj)
	{
	  /* Found the one to delete */
#ifdef DEBUG
	  printf ("Found the one to delete\n");
#endif /* DEBUG */

	  if (prev_all)
	    prev_all->next = all->next;
	  else
	    obj->obj_list = all->next;

	  /* Draw obj (which will actually undraw it! */
	  del_obj->drawfunc (del_obj);

	  free_one_obj (del_obj);
	  g_free (all);

	  if (obj_show_single != -1)
	    {
	      /* We've just deleted the only visible one */
	      draw_grid_clear ();
	      obj_show_single = -1; /* Show all again */
	    }
	  return;
	}
      prev_all = all;
      all = all->next;
    }
  g_warning (_("Hey where has the object gone ?"));
}

static void
do_move_all_obj (GdkPoint *to_pnt)
{
  /* Move all objects in one go */
  /* Undraw/then draw in new pos */
  gint16 xdiff = move_all_pnt->x - to_pnt->x;
  gint16 ydiff = move_all_pnt->y - to_pnt->y;
  
  if (xdiff || ydiff)
    {
      DAllObjs *all;
  
      for (all = current_obj->obj_list; all; all = all->next)
	{
	  Dobject *obj = all->obj;
	  
	  /* undraw ! */
	  draw_one_obj (obj);
	  
	  update_pnts (obj, xdiff, ydiff);
	  
	  /* Draw in new pos */
	  draw_one_obj (obj);
	}
      
      *move_all_pnt = *to_pnt;
    }
}

void
do_save_obj (Dobject *obj, FILE *to)
{
  DobjPoints *spnt;

  for (spnt = obj->points; spnt; spnt = spnt->next)
    {
      fprintf (to, "%d %d\n", spnt->pnt.x, spnt->pnt.y);
    }
}

static void
do_move_obj (Dobject  *obj,
	     GdkPoint *to_pnt)
{
  /* Move the whole line - undraw the line to start with */
  /* Then draw in new pos */
  gint16 xdiff = 0;
  gint16 ydiff = 0;
  
  get_diffs (obj, &xdiff, &ydiff, to_pnt);
  
  if (xdiff || ydiff)
    {  
      /* undraw ! */
      draw_one_obj (obj);
      
      update_pnts (obj, xdiff, ydiff);
      
      /* Draw in new pos */
      draw_one_obj (obj);
    }
}

static void
do_move_obj_pnt (Dobject  *obj,
		 GdkPoint *to_pnt)
{
  /* Move the whole line - undraw the line to start with */
  /* Then draw in new pos */
  DobjPoints *spnt;
  gint16 xdiff = 0;
  gint16 ydiff = 0;
  
  spnt = get_diffs (obj, &xdiff, &ydiff, to_pnt);
  
  if ((!xdiff && !ydiff) || !spnt)
    return;
  
  /* undraw ! */
  draw_one_obj (obj);

  spnt->pnt.x = spnt->pnt.x - xdiff;
  spnt->pnt.y = spnt->pnt.y - ydiff;
  
  /* Draw in new pos */
  draw_one_obj (obj);
}

/* copy objs */
DAllObjs *
copy_all_objs (DAllObjs *objs)
{
  DAllObjs *nobj;
  DAllObjs *new_all_objs = NULL;
  DAllObjs *ret = NULL;

  while (objs)
    {
      nobj = g_new0 (DAllObjs, 1);

     if (!ret)
	{
	  ret = new_all_objs = nobj;
	}
      else
	{
	  new_all_objs->next = nobj;
	  new_all_objs = nobj;
	}

      nobj->obj = (Dobject *) objs->obj->copyfunc (objs->obj);

      objs = objs->next;
    }

  return ret;
}

/* Screen refresh */
static void
draw_one_obj (Dobject * obj)
{
  obj->drawfunc (obj);
}

void
draw_objects (DAllObjs *objs,
	      gint      show_single)
{
  /* Show_single - only one object to draw Unless shift 
   * is down in which case show all.
   */

  gint count = 0;

  while (objs)
    {
      if (!show_single || count == obj_show_single || obj_show_single == -1)
	draw_one_obj (objs->obj);
      
      objs = objs->next;
      count++;
    }
}

void
prepend_to_all_obj (GFigObj  *fobj,
		    DAllObjs *nobj)
{
  DAllObjs *cobj;

  setup_undo (); /* Remember ME */

  if (!fobj->obj_list)
    {
      fobj->obj_list = nobj;
      return;
    }

  cobj = fobj->obj_list;

  while (cobj->next)
    {
      cobj = cobj->next;
    }

  cobj->next = nobj;
}

static void
scale_obj_points (DobjPoints *opnt,
		  gdouble     scale_x,
		  gdouble     scale_y)
{
  while (opnt)
    {
      opnt->pnt.x = (gint) (opnt->pnt.x * scale_x);
      opnt->pnt.y = (gint) (opnt->pnt.y * scale_y);
      opnt = opnt->next;
    }
}

void
add_to_all_obj (GFigObj *fobj,
		Dobject *obj)
{
  DAllObjs *nobj;
  
  nobj = g_new0 (DAllObjs, 1);

  nobj->obj = obj;

  if (need_to_scale)
    scale_obj_points (obj->points, scale_x_factor, scale_y_factor);

  prepend_to_all_obj (fobj, nobj);
}

/* First button press -- start drawing object */
void
object_start (GdkPoint *pnt,
	      gint      shift_down)
{
  /* start for the current object */
  if (!selvals.scaletoimage)
    {
      need_to_scale = 1;
      selvals.scaletoimage = 1;
    }
  else
    {
      need_to_scale = 0;
    }

  switch (selvals.otype)
    {
    case LINE:
      /* Shift means we are still drawing */
      if (!shift_down || !obj_creating)
	draw_sqr (pnt);
      d_line_start (pnt, shift_down);
      break;
    case CIRCLE:
      draw_sqr (pnt);
      d_circle_start (pnt, shift_down);
      break;
    case ELLIPSE:
      draw_sqr (pnt);
      d_ellipse_start (pnt, shift_down);
      break;
    case POLY:
      draw_sqr (pnt);
      d_poly_start (pnt, shift_down);
      break;
    case ARC:
      d_arc_start (pnt, shift_down);
      break;
    case STAR:
      draw_sqr (pnt);
      d_star_start (pnt, shift_down);
      break;
    case SPIRAL:
      draw_sqr (pnt);
      d_spiral_start (pnt, shift_down);
      break;
    case BEZIER:
      if (!tmp_bezier)
	draw_sqr (pnt);
      d_bezier_start (pnt, shift_down);
      break;
    default:
      /* Internal error */
      break;
    }
}
  
void
object_end (GdkPoint *pnt,
	    gint      shift_down)
{
  /* end for the current object */
  /* Add onto global object list */

  /* If shift is down may carry on drawing */
  switch (selvals.otype)
    {
    case LINE:
      d_line_end (pnt, shift_down);
      draw_sqr (pnt);
      break;
    case CIRCLE:
      draw_sqr (pnt);
      d_circle_end (pnt, shift_down);
      break;
    case ELLIPSE:
      draw_sqr (pnt);
      d_ellipse_end (pnt, shift_down);
      break;
    case POLY:
      draw_sqr (pnt);
      d_poly_end (pnt, shift_down);
      break;
    case STAR:
      draw_sqr (pnt);
      d_star_end (pnt, shift_down);
      break;
    case ARC:
      draw_sqr (pnt);
      d_arc_end (pnt, shift_down);
      break;
    case SPIRAL:
      draw_sqr (pnt);
      d_spiral_end (pnt, shift_down);
      break;
    case BEZIER:
      d_bezier_end (pnt, shift_down);
      break;
    default:
      /* Internal error */
      break;
    }

  if (need_to_scale)
    {
      need_to_scale = 0;
      selvals.scaletoimage = 0;
    }
}

void
object_update (GdkPoint *pnt)
{
  /* update for the current object */
  /* New position xy */
  switch (selvals.otype)
    {
    case LINE:
      d_update_line (pnt);
      break;
    case CIRCLE:
      d_update_circle (pnt);
      break;
    case ELLIPSE:
      d_update_ellipse (pnt);
      break;
    case POLY:
      d_update_poly (pnt);
      break;
    case STAR:
      d_update_star (pnt);
      break;
    case ARC:
      d_update_arc (pnt);
      break;
    case SPIRAL:
      d_update_spiral (pnt);
      break;
    case BEZIER:
      d_update_bezier (pnt);
      break;
    default:
      /* Internal error */
      break;
    }
}


