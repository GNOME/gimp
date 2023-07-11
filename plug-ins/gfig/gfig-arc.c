/*
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This is a plug-in for GIMP.
 *
 * Generates images containing vector type drawings.
 *
 * Copyright (C) 1997 Andy Thomas  alt@picnic.demon.co.uk
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "config.h"

#include <math.h>
#include <stdlib.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>

#include "gfig.h"
#include "gfig-dobject.h"
#include "gfig-arc.h"

#include "libgimp/stdplugins-intl.h"

static gdouble     dist                (gdouble   x1,
                                        gdouble   y1,
                                        gdouble   x2,
                                        gdouble   y2);

static void        mid_point           (gdouble   x1,
                                        gdouble   y1,
                                        gdouble   x2,
                                        gdouble   y2,
                                        gdouble  *mx,
                                        gdouble  *my);

static gdouble     line_grad           (gdouble   x1,
                                        gdouble   y1,
                                        gdouble   x2,
                                        gdouble   y2);

static gdouble     line_cons           (gdouble   x,
                                        gdouble   y,
                                        gdouble   lgrad);

static void        line_definition     (gdouble   x1,
                                        gdouble   y1,
                                        gdouble   x2,
                                        gdouble   y2,
                                        gdouble  *lgrad,
                                        gdouble  *lconst);

static void        arc_details         (GdkPoint *vert_a,
                                        GdkPoint *vert_b,
                                        GdkPoint *vert_c,
                                        GdkPoint *center_pnt,
                                        gdouble  *radius);

static gdouble     arc_angle           (GdkPoint *pnt,
                                        GdkPoint *center);

static void        arc_drawing_details (GfigObject *obj,
                                        gdouble    *minang,
                                        GdkPoint   *center_pnt,
                                        gdouble    *arcang,
                                        gdouble    *radius,
                                        gboolean    draw_cnts,
                                        gboolean    do_scale);

static void        d_draw_arc          (GfigObject *obj,
                                        cairo_t    *cr);

static void        d_paint_arc         (GfigObject *obj);

static GfigObject *d_copy_arc          (GfigObject *obj);

static void        d_update_arc_line   (GdkPoint   *pnt);
static void        d_update_arc        (GdkPoint   *pnt);
static void        d_arc_line_start    (GdkPoint   *pnt,
                                        gboolean    shift_down);
static void        d_arc_line_end      (GimpGfig   *gfig,
                                        GdkPoint   *pnt,
                                        gboolean    shift_down);

/* Distance between two points. */
static gdouble
dist (gdouble x1,
      gdouble y1,
      gdouble x2,
      gdouble y2)
{

  double s1 = x1 - x2;
  double s2 = y1 - y2;

  return sqrt (s1 * s1 + s2 * s2);
}

/* Mid point of line returned */
static void
mid_point (gdouble  x1,
           gdouble  y1,
           gdouble  x2,
           gdouble  y2,
           gdouble *mx,
           gdouble *my)
{
  *mx = (x1 + x2) / 2.0;
  *my = (y1 + y2) / 2.0;
}

/* Careful about infinite grads */
static gdouble
line_grad (gdouble x1,
           gdouble y1,
           gdouble x2,
           gdouble y2)
{
  double dx, dy;

  dx = x1 - x2;
  dy = y1 - y2;

  return (dx == 0.0) ? 0.0 : dy / dx;
}

/* Constant of line that goes through x, y with grad lgrad */
static gdouble
line_cons (gdouble x,
           gdouble y,
           gdouble lgrad)
{
  return y - lgrad * x;
}

/* Get grad & const for perpend. line to given points */
static void
line_definition (gdouble  x1,
                 gdouble  y1,
                 gdouble  x2,
                 gdouble  y2,
                 gdouble *lgrad,
                 gdouble *lconst)
{
  double grad1;
  double midx, midy;

  grad1 = line_grad (x1, y1, x2, y2);

  if (grad1 == 0.0)
    {
#ifdef DEBUG
      printf ("Infinite grad....\n");
#endif /* DEBUG */
      return;
    }

  mid_point (x1, y1, x2, y2, &midx, &midy);

  /* Invert grad for perpen gradient */

  *lgrad = -1.0 / grad1;

  *lconst = line_cons (midx, midy,*lgrad);
}

/* Arch details
 * Given three points get arc radius and the co-ords
 * of center point.
 */

static void
arc_details (GdkPoint *vert_a,
             GdkPoint *vert_b,
             GdkPoint *vert_c,
             GdkPoint *center_pnt,
             gdouble  *radius)
{
  /* Only vertices are in whole numbers - everything else is in doubles */
  double ax, ay;
  double bx, by;
  double cx, cy;

  double len_a, len_b, len_c;
  double sum_sides2;
  double area;
  double circumcircle_R;
  double line1_grad = 0, line1_const = 0;
  double line2_grad = 0, line2_const = 0;
  double inter_x = 0.0, inter_y = 0.0;
  int    got_x = 0, got_y = 0;

  ax = (double) (vert_a->x);
  ay = (double) (vert_a->y);
  bx = (double) (vert_b->x);
  by = (double) (vert_b->y);
  cx = (double) (vert_c->x);
  cy = (double) (vert_c->y);

  len_a = dist (ax, ay, bx, by);
  len_b = dist (bx, by, cx, cy);
  len_c = dist (cx, cy, ax, ay);

  sum_sides2 = (fabs (len_a) + fabs (len_b) + fabs (len_c))/2;

  /* Area */
  area = sqrt (sum_sides2 * (sum_sides2 - len_a) *
                            (sum_sides2 - len_b) *
                            (sum_sides2 - len_c));

  /* Circumcircle */
  circumcircle_R = len_a * len_b * len_c / (4 * area);
  *radius = circumcircle_R;

  /* Deal with exceptions - I hate exceptions */

  if (ax == bx || ax == cx || cx == bx)
    {
      /* vert line -> mid point gives inter_x */
      if (ax == bx && bx == cx)
        {
          /* Straight line */
          double miny = ay;
          double maxy = ay;

          if (by > maxy)
            maxy = by;

          if (by < miny)
            miny = by;

          if (cy > maxy)
            maxy = cy;

          if (cy < miny)
            miny = cy;

          inter_y = (maxy - miny) / 2 + miny;
        }
      else if (ax == bx)
        {
          inter_y = (ay - by) / 2 + by;
        }
      else if (bx == cx)
        {
          inter_y = (by - cy) / 2 + cy;
        }
      else
        {
          inter_y = (cy - ay) / 2 + ay;
        }
      got_y = 1;
    }

  if (ay == by || by == cy || ay == cy)
    {
      /* Horz line -> midpoint gives inter_y */
      if (ay == by && by == cy)
        {
          /* Straight line */
          double minx = ax;
          double maxx = ax;

          if (bx > maxx)
            maxx = bx;

          if (bx < minx)
            minx = bx;

          if (cx > maxx)
            maxx = cx;

          if (cx < minx)
            minx = cx;

          inter_x = (maxx - minx) / 2 + minx;
        }
      else if (ay == by)
        {
          inter_x = (ax - bx) / 2 + bx;
        }
      else if (by == cy)
        {
          inter_x = (bx - cx) / 2 + cx;
        }
      else
        {
          inter_x = (cx - ax) / 2 + ax;
        }
      got_x = 1;
    }

  if (!got_x || !got_y)
    {
      /* At least two of the lines are not parallel to the axis */
      /*first line */
      if (ax != bx && ay != by)
        line_definition (ax, ay, bx, by, &line1_grad, &line1_const);
      else
        line_definition (ax, ay, cx, cy, &line1_grad, &line1_const);
      /* second line */
      if (bx != cx && by != cy)
        line_definition (bx, by, cx, cy, &line2_grad, &line2_const);
      else
        line_definition (ax, ay, cx, cy, &line2_grad, &line2_const);
    }

  /* Intersection point */

  if (!got_x)
    inter_x = (line2_const - line1_const) / (line1_grad - line2_grad);
  if (!got_y)
    inter_y = line1_grad * inter_x + line1_const;

  center_pnt->x = (gint) inter_x;
  center_pnt->y = (gint) inter_y;
}

static gdouble
arc_angle (GdkPoint *pnt,
           GdkPoint *center)
{
  /* Get angle (in degrees) of point given origin of center */
  gint16  shift_x;
  gint16  shift_y;
  gdouble offset_angle;

  shift_x =  pnt->x - center->x;
  shift_y = -pnt->y + center->y;
  offset_angle = atan2 (shift_y, shift_x);

  if (offset_angle < 0)
    offset_angle += 2.0 * G_PI;

  return offset_angle * 360 / (2.0 * G_PI);
}

static void
arc_drawing_details (GfigObject *obj,
                     gdouble    *minang,
                     GdkPoint   *center_pnt,
                     gdouble    *arcang,
                     gdouble    *radius,
                     gboolean    draw_cnts,
                     gboolean    do_scale)
{
  DobjPoints *pnt1 = NULL;
  DobjPoints *pnt2 = NULL;
  DobjPoints *pnt3 = NULL;
  DobjPoints  dpnts[3];
  gdouble     ang1, ang2, ang3;
  gdouble     maxang;

  pnt1 = obj->points;

  if (!pnt1)
    return; /* Not fully drawn */

  pnt2 = pnt1->next;

  if (!pnt2)
    return; /* Not fully drawn */

  pnt3 = pnt2->next;

  if (!pnt3)
    return; /* Still not fully drawn */

  if (do_scale)
    {
      /* Adjust pnts for scaling */
      /* Warning struct copies here! and casting to double <-> int */
      /* Too complex fix me - to much hacking */
      gdouble xy[2];
      int     j;

      dpnts[0] = *pnt1;
      dpnts[1] = *pnt2;
      dpnts[2] = *pnt3;

      pnt1 = &dpnts[0];
      pnt2 = &dpnts[1];
      pnt3 = &dpnts[2];

      for (j = 0 ; j < 3; j++)
        {
          xy[0] = dpnts[j].pnt.x;
          xy[1] = dpnts[j].pnt.y;
          if (selvals.scaletoimage)
            scale_to_original_xy (&xy[0], 1);
          else
            scale_to_xy (&xy[0], 1);
          dpnts[j].pnt.x = xy[0];
          dpnts[j].pnt.y = xy[1];
        }
    }

  arc_details (&pnt1->pnt, &pnt2->pnt, &pnt3->pnt, center_pnt, radius);

  ang1 = arc_angle (&pnt1->pnt, center_pnt);
  ang2 = arc_angle (&pnt2->pnt, center_pnt);
  ang3 = arc_angle (&pnt3->pnt, center_pnt);

  /* Find min/max angle */

  maxang = ang1;

  if (ang3 > maxang)
    maxang = ang3;

  *minang = ang1;

  if (ang3 < *minang)
    *minang = ang3;

  if (ang2 > *minang && ang2 < maxang)
    *arcang = maxang - *minang;
  else
    *arcang = maxang - *minang - 360;
}

static void
d_draw_arc (GfigObject *obj,
            cairo_t    *cr)
{
  DobjPoints *pnt1, *pnt2, *pnt3;
  GdkPoint center_pnt;
  gdouble  radius, minang, arcang;

  g_assert (obj != NULL);

  if (!obj)
    return;

  pnt1 = obj->points;
  pnt2 = pnt1 ? pnt1->next : NULL;
  pnt3 = pnt2 ? pnt2->next : NULL;

  if (! pnt3)
    return;

  draw_sqr (&pnt1->pnt, obj == gfig_context->selected_obj, cr);
  draw_sqr (&pnt2->pnt, obj == gfig_context->selected_obj, cr);
  draw_sqr (&pnt3->pnt, obj == gfig_context->selected_obj, cr);

  arc_drawing_details (obj, &minang, &center_pnt, &arcang, &radius,
                       TRUE, FALSE);
  gfig_draw_arc (center_pnt.x, center_pnt.y, radius, radius, -minang, -(minang + arcang), cr);
}

static void
d_paint_arc (GfigObject *obj)
{
  /* first point center */
  /* Next point is radius */
  gdouble *line_pnts;
  gint     seg_count = 0;
  gint     i = 0;
  gdouble  ang_grid;
  gdouble  ang_loop;
  gdouble  radius;
  gint     loop;
  GdkPoint last_pnt = { 0, 0 };
  gboolean first = TRUE;
  GdkPoint center_pnt;
  gdouble  minang, arcang;

  g_assert (obj != NULL);

  if (!obj)
    return;

  /* No cnt pnts & must scale */
  arc_drawing_details (obj, &minang, &center_pnt, &arcang, &radius,
                       FALSE, TRUE);

  seg_count = 360; /* Should make a smoth-ish curve */

  /* +3 because we MIGHT do pie selection */
  line_pnts = g_new0 (gdouble, 2 * seg_count + 3);

  /* Lines */
  ang_grid = 2.0 * G_PI / 360.0;

  if (arcang < 0.0)
    {
      /* Swap - since we always draw anti-clock wise */
      minang += arcang;
      arcang  = -arcang;
    }

  minang = minang * (2.0 * G_PI / 360.0); /* min ang is in degrees - need in rads */

  for (loop = 0 ; loop < abs ((gint)arcang) ; loop++)
    {
      gdouble  lx, ly;
      GdkPoint calc_pnt;

      ang_loop = (gdouble)loop * ang_grid + minang;

      lx =  radius * cos (ang_loop);
      ly = -radius * sin (ang_loop); /* y grows down screen and angs measured from x clockwise */

      calc_pnt.x = RINT (lx + center_pnt.x);
      calc_pnt.y = RINT (ly + center_pnt.y);

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
          first = FALSE;
        }
    }

  /* One go */
  if (obj->style.paint_type == PAINT_BRUSH_TYPE)
    {
      gfig_paint (selvals.brshtype,
                  gfig_context->drawable,
                  i, line_pnts);
    }

  g_free (line_pnts);
}

static GfigObject *
d_copy_arc (GfigObject *obj)
{
  GfigObject *nc;

  g_assert (obj->type == ARC);

  nc = d_new_object (ARC, obj->points->pnt.x, obj->points->pnt.y);
  nc->points->next = d_copy_dobjpoints (obj->points->next);

  return nc;
}

void
d_arc_object_class_init (void)
{
  GfigObjectClass *class = &dobj_class[ARC];

  class->type      = ARC;
  class->name      = "ARC";
  class->drawfunc  = d_draw_arc;
  class->paintfunc = d_paint_arc;
  class->copyfunc  = d_copy_arc;
  class->update    = d_update_arc;
}

/* Update end point of line */
static void
d_update_arc_line (GdkPoint *pnt)
{
  DobjPoints *spnt, *epnt;
  /* Get last but one segment and undraw it -
   * Then draw new segment in.
   * always dealing with the static object.
   */

  /* Get start of segments */
  spnt = obj_creating->points;

  if (!spnt)
    return; /* No points */

  if ((epnt = spnt->next))
    {
      g_free (epnt);
    }

  epnt = new_dobjpoint (pnt->x, pnt->y);
  spnt->next = epnt;
}

static void
d_update_arc (GdkPoint *pnt)
{
  DobjPoints *pnt1 = NULL;
  DobjPoints *pnt2 = NULL;
  DobjPoints *pnt3 = NULL;

  /* First two points as line only become arch when third
   * point is placed on canvas.
   */

  pnt1 = obj_creating->points;

  if (!pnt1                ||
      !(pnt2 = pnt1->next) ||
      !(pnt3 = pnt2->next))
    {
      d_update_arc_line (pnt);
      return; /* Not fully drawn */
    }

  /* Update a real curve */
  /* Nothing to be done ... */
}

static void
d_arc_line_start (GdkPoint *pnt,
                  gboolean  shift_down)
{
  if (!obj_creating || !shift_down)
    {
      /* Must delete obj_creating if we have one */
      obj_creating = d_new_object (LINE, pnt->x, pnt->y);
    }
  else
    {
      /* Contniuation */
      d_update_arc_line (pnt);
    }
}

void
d_arc_start (GdkPoint *pnt,
             gboolean  shift_down)
{
  /* Draw lines to start with -- then convert to an arc */
  d_arc_line_start (pnt, TRUE); /* TRUE means multiple pointed line */
}

static void
d_arc_line_end (GimpGfig *gfig,
                GdkPoint *pnt,
                gboolean  shift_down)
{
  if (shift_down)
    {
      if (tmp_line)
        {
          GdkPoint tmp_pnt = *pnt;

          if (need_to_scale)
            {
              tmp_pnt.x = pnt->x * scale_x_factor;
              tmp_pnt.y = pnt->y * scale_y_factor;
            }

          d_pnt_add_line (tmp_line, tmp_pnt.x, tmp_pnt.y, -1);
          free_one_obj (obj_creating);
          /* Must free obj_creating */
        }
      else
        {
          tmp_line = obj_creating;
          add_to_all_obj (gfig, gfig_context->current_obj, obj_creating);
        }

      obj_creating = d_new_object (LINE, pnt->x, pnt->y);
    }
  else
    {
      if (tmp_line)
        {
          GdkPoint tmp_pnt = *pnt;

          if (need_to_scale)
            {
              tmp_pnt.x = pnt->x * scale_x_factor;
              tmp_pnt.y = pnt->y * scale_y_factor;
            }

          d_pnt_add_line (tmp_line, tmp_pnt.x, tmp_pnt.y, -1);
          free_one_obj (obj_creating);
          /* Must free obj_creating */
        }
      else
        {
          add_to_all_obj (gfig, gfig_context->current_obj, obj_creating);
        }
      obj_creating = NULL;
      tmp_line = NULL;
    }
  /*gtk_widget_queue_draw (gfig_context->preview);*/
}

void
d_arc_end (GimpGfig *gfig,
           GdkPoint *pnt,
           gboolean  shift_down)
{
  /* Under control point */
  if (!tmp_line               ||
      !tmp_line->points       ||
      !tmp_line->points->next)
    {
      /* No arc created  - yet. Must have three points */
      d_arc_line_end (gfig, pnt, TRUE);
    }
  else
    {
      /* Complete arc */
      /* Convert to an arc ... */
      tmp_line->type = ARC;
      tmp_line->class = &dobj_class[ARC];
      d_arc_line_end (gfig, pnt, FALSE);
      if (need_to_scale)
        {
          selvals.scaletoimage = 0;
        }
      gtk_widget_queue_draw (gfig_context->preview);
      if (need_to_scale)
        {
          selvals.scaletoimage = 1;
        }
    }
}

