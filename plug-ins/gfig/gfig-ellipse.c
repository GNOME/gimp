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

#include "config.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "gfig.h"
#include "gfig-dobject.h"
#include "gfig-poly.h"

#include "libgimp/stdplugins-intl.h"

static void
d_draw_ellipse (Dobject * obj)
{
  DobjPoints *center_pnt;
  DobjPoints *edge_pnt;
  gint        bound_wx;
  gint        bound_wy;
  gint        top_x;
  gint        top_y;

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

  bound_wx = abs (center_pnt->pnt.x - edge_pnt->pnt.x);
  bound_wy = abs (center_pnt->pnt.y - edge_pnt->pnt.y);

  if (edge_pnt->pnt.x > center_pnt->pnt.x)
    top_x = 2 * center_pnt->pnt.x - edge_pnt->pnt.x;
  else
    top_x = edge_pnt->pnt.x;

  if (edge_pnt->pnt.y > center_pnt->pnt.y)
    top_y = 2 * center_pnt->pnt.y - edge_pnt->pnt.y;
  else
    top_y = edge_pnt->pnt.y;

  gfig_draw_arc (center_pnt->pnt.x, center_pnt->pnt.y, bound_wx, bound_wy, 0, 360);
}

static void
d_paint_approx_ellipse (Dobject *obj)
{
  /* first point center */
  /* Next point is radius */
  gdouble    *line_pnts;
  gint        seg_count = 0;
  gint        i = 0;
  DobjPoints *center_pnt;
  DobjPoints *radius_pnt;
  gdouble     a_axis;
  gdouble     b_axis;
  gdouble     ang_grid;
  gdouble     ang_loop;
  gdouble     radius;
  gint        loop;
  GdkPoint    first_pnt, last_pnt;
  gboolean    first = TRUE;

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

      line_pnts[i++] = calc_pnt.x;
      line_pnts[i++] = calc_pnt.y;
      last_pnt = calc_pnt;

      if (first)
        {
          first_pnt = calc_pnt;
          first = FALSE;
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
                  gfig_context->drawable_id,
                  i, line_pnts);
    }
  else
    {
      gimp_free_select (gfig_context->image_id,
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
  DobjPoints *center_pnt;
  DobjPoints *edge_pnt;
  gint        bound_wx;
  gint        bound_wy;
  gint        top_x;
  gint        top_y;
  gdouble     dpnts[4];

  /* Drawing ellipse is hard .
   * 1) select circle
   * 2) stroke it
   */

  g_assert (obj != NULL);

  if (selvals.approxcircles)
    {
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

  gimp_ellipse_select (gfig_context->image_id,
                       dpnts[0], dpnts[1],
                       dpnts[2], dpnts[3],
                       selopt.type,
                       selopt.antia,
                       selopt.feather,
                       selopt.feather_radius);

  paint_layer_fill ();

  gimp_edit_stroke (gfig_context->drawable_id);
}

static Dobject *
d_copy_ellipse (Dobject * obj)
{
  Dobject *nc;

  g_assert (obj->type == ELLIPSE);

  nc = d_new_object (ELLIPSE, obj->points->pnt.x, obj->points->pnt.y);
  nc->points->next = d_copy_dobjpoints (obj->points->next);

  return nc;
}

void
d_ellipse_object_class_init ()
{
  DobjClass *class = &dobj_class[ELLIPSE];

  class->type      = ELLIPSE;
  class->name      = "Ellipse";
  class->drawfunc  = d_draw_ellipse;
  class->paintfunc = d_paint_ellipse;
  class->copyfunc  = d_copy_ellipse;
}

void
d_update_ellipse (GdkPoint *pnt)
{
  DobjPoints *center_pnt, *edge_pnt;
  gint        bound_wx;
  gint        bound_wy;
  gint        top_x;
  gint        top_y;

  /* Undraw last one then draw new one */
  center_pnt = obj_creating->points;

  if (!center_pnt)
    return; /* No points */

  if ((edge_pnt = center_pnt->next))
    {
      /* Undraw current */
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

      draw_circle (&edge_pnt->pnt);

      gdk_draw_arc (gfig_context->preview->window,
                    gfig_gc,
                    0,
                    top_x,
                    top_y,
                    bound_wx,
                    bound_wy,
                    0,
                    360 * 64);
    }

  draw_circle (pnt);

  edge_pnt = new_dobjpoint (pnt->x, pnt->y);

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

  gdk_draw_arc (gfig_context->preview->window,
                gfig_gc,
                0,
                top_x,
                top_y,
                bound_wx,
                bound_wy,
                0,
                360 * 64);

  center_pnt->next = edge_pnt;
}

void
d_ellipse_start (GdkPoint *pnt, gint shift_down)
{
  obj_creating = d_new_object (ELLIPSE, pnt->x, pnt->y);
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
      add_to_all_obj (gfig_context->current_obj, obj_creating);
    }

  obj_creating = NULL;
}
