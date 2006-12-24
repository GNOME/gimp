/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-2001 Spencer Kimball, Peter Mattis, and others
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

#include "config.h"

#include <glib-object.h>

#include "libgimpmath/gimpmath.h"

#include "core-types.h"

#include "gimp-transform-resize.h"


#if defined (HAVE_FINITE)
#define FINITE(x) finite(x)
#elif defined (HAVE_ISFINITE)
#define FINITE(x) isfinite(x)
#elif defined (G_OS_WIN32)
#define FINITE(x) _finite(x)
#else
#error "no FINITE() implementation available?!"
#endif

#define MIN4(a,b,c,d) MIN(MIN((a),(b)),MIN((c),(d)))
#define MAX4(a,b,c,d) MAX(MAX((a),(b)),MAX((c),(d)))


typedef struct
{
  gint      x, y;
} Point;

typedef struct
{
  gint      xmin, xmax;
  gint      ymin, ymax;
  gdouble   m, b;  /* y = mx + b */
  gboolean  top, right;
} Edge;



static void  gimp_transform_resize_adjust (gdouble  dx1,
                                           gdouble  dy1,
                                           gdouble  dx2,
                                           gdouble  dy2,
                                           gdouble  dx3,
                                           gdouble  dy3,
                                           gdouble  dx4,
                                           gdouble  dy4,
                                           gint    *x1,
                                           gint    *y1,
                                           gint    *x2,
                                           gint    *y2);
static void  gimp_transform_resize_crop   (gdouble  dx1,
                                           gdouble  dy1,
                                           gdouble  dx2,
                                           gdouble  dy2,
                                           gdouble  dx3,
                                           gdouble  dy3,
                                           gdouble  dx4,
                                           gdouble  dy4,
                                           gdouble  aspect,
                                           gint    *x1,
                                           gint    *y1,
                                           gint    *x2,
                                           gint    *y2);


/*
 * This function wants to be passed the inverse transformation matrix!!
 */
void
gimp_transform_resize_boundary (const GimpMatrix3   *inv,
                                GimpTransformResize  resize,
                                gint                 u1,
                                gint                 v1,
                                gint                 u2,
                                gint                 v2,
                                gint                *x1,
                                gint                *y1,
                                gint                *x2,
                                gint                *y2)
{
  gdouble dx1, dx2, dx3, dx4;
  gdouble dy1, dy2, dy3, dy4;

  g_return_if_fail (inv != NULL);
  
  /*  initialize with the original boundary  */
  *x1 = u1;
  *y1 = v1;
  *x2 = u2;
  *y2 = v2;
  
  if (resize == GIMP_TRANSFORM_RESIZE_CLIP)
    return;

  gimp_matrix3_transform_point (inv, u1, v1, &dx1, &dy1);
  gimp_matrix3_transform_point (inv, u2, v1, &dx2, &dy2);
  gimp_matrix3_transform_point (inv, u1, v2, &dx3, &dy3);
  gimp_matrix3_transform_point (inv, u2, v2, &dx4, &dy4);

  /*  check if the transformation matrix is valid at all  */
  if (! FINITE (dx1) || ! FINITE (dy1) ||
      ! FINITE (dx2) || ! FINITE (dy2) ||
      ! FINITE (dx3) || ! FINITE (dy3) ||
      ! FINITE (dx4) || ! FINITE (dy4))
    {
      g_warning ("invalid transform matrix");
      resize = GIMP_TRANSFORM_RESIZE_CLIP;
    }

  switch (resize)
    {
    case GIMP_TRANSFORM_RESIZE_ADJUST:
      gimp_transform_resize_adjust (dx1, dy1, dx2, dy2, dx3, dy3, dx4, dy4,
                                    x1, y1, x2, y2);
      break;

    case GIMP_TRANSFORM_RESIZE_CLIP:
      /*  we are all done already  */
      break;

    case GIMP_TRANSFORM_RESIZE_CROP:
      gimp_transform_resize_crop (dx1, dy1, dx2, dy2, dx3, dy3, dx4, dy4,
                                  0.0,
                                  x1, y1, x2, y2);
      break;

    case GIMP_TRANSFORM_RESIZE_CROP_WITH_ASPECT:
      gimp_transform_resize_crop (dx1, dy1, dx2, dy2, dx3, dy3, dx4, dy4,
                                  ((gdouble) u2 - u1) / (v2 - v1),
                                  x1, y1, x2, y2);
      break;
    }

  if (*x1 == *x2)
    (*x2)++;

  if (*y1 == *y2)
    (*y2)++;
}

static void
gimp_transform_resize_adjust (gdouble  dx1,
                              gdouble  dy1,
                              gdouble  dx2,
                              gdouble  dy2,
                              gdouble  dx3,
                              gdouble  dy3,
                              gdouble  dx4,
                              gdouble  dy4,
                              gint    *x1,
                              gint    *y1,
                              gint    *x2,
                              gint    *y2)
{
  *x1 = (gint) floor (MIN4 (dx1, dx2, dx3, dx4));
  *y1 = (gint) floor (MIN4 (dy1, dy2, dy3, dy4));

  *x2 = (gint) ceil (MAX4 (dx1, dx2, dx3, dx4));
  *y2 = (gint) ceil (MAX4 (dy1, dy2, dy3, dy4));
}

static void
edge_init (Edge        *edge,
           const Point *p,
           const Point *q)
{
  gdouble den;

  edge->xmin  = MIN ( (p->x),  (q->x));
  edge->xmax  = MAX ( (p->x),  (q->x));

  edge->ymin  = MIN ( (p->y),  (q->y));
  edge->ymax  = MAX ( (p->y),  (q->y));

  edge->top   = p->x > q->x;
  edge->right = p->y > q->y;

  den = q->x - p->x;

  if (den == 0)
    den = 0.001;

  edge->m   = ((gdouble) q->y - p->y) / den;
  edge->b     = p->y - edge->m * p->x;
}

static const Edge *
find_edge (const Edge *edges,
           gint        x,
           gboolean    top)
{
  const Edge *emax = edges;
  const Edge *e = edges;
  gint        i;

  for (i = 0; i < 4; i++)
    {
      if ((e->xmin == x) && (e->xmax != e->xmin) &&
          ((e->top && top) || (!e->top && !top)))
            emax = e;
      e++;
    }

  return emax;
}

/* find largest pixel completely inside;
 * look through all edges for intersection
 */
static gint
intersect_x (const Edge *edges,
             gint        y)
{
  gdouble x0 = 0;
  gdouble x1 = 0;
  gint    i;

  for (i = 0; i < 4; i++)
    if (edges[i].right && edges[i].ymin <= y && edges[i].ymax >= y)
      {
        x0 = (y + 0.5 - edges[i].b) / edges[i].m;
        x1 = (y - 0.5 - edges[i].b) / edges[i].m;
      }

  return (gint) floor (MIN (x0, x1));
}

static gint
intersect_y (const Edge *edge,
             gint        xi)
{
  gdouble yfirst = edge->m * (xi - 0.5) + edge->b;
  gdouble ylast  = edge->m * (xi + 0.5) + edge->b;

  return (gint) (edge->top ? ceil (MAX (yfirst, ylast)) : floor (MIN (yfirst, ylast)));
}

static void
gimp_transform_resize_crop (gdouble  dx1,
                            gdouble  dy1,
                            gdouble  dx2,
                            gdouble  dy2,
                            gdouble  dx3,
                            gdouble  dy3,
                            gdouble  dx4,
                            gdouble  dy4,
                            gdouble  aspect,
                            gint    *x1,
                            gint    *y1,
                            gint    *x2,
                            gint    *y2)
{
  Point        points[4];
  gint         ax, ay;
  int          min;
  gint         tx, ty;
  
  Edge         edges[4];
  const Point *a;
  const Point *b;
  const Edge  *top;
  const Edge  *bottom;
  gint         cxmin, cymin;
  gint         cxmax, cymax;
  Point       *xint;
  
  gint         ymin, ymax;
  gint         maxarea = 0;
  gint         xi;
  gint         i;

  /*  fill in the points array  */
  points[0].x = floor (dx1);
  points[0].y = floor (dy1);
  points[1].x = floor (dx2);
  points[1].y = floor (dy2);
  points[2].x = floor (dx3);
  points[2].y = floor (dy3);
  points[3].x = floor (dx4);
  points[3].y = floor (dy4);
  
  /*  first, translate the vertices into the first quadrant */

  ax = 0;
  ay = 0;
  
  for (i = 0; i < 4; i++)
    {
      if (points[i].x < ax)
        ax = points[i].x;
      if (points[i].y < ay)
        ay = points[i].y;
    }
  
  for (i = 0; i < 4; i++)
    {
      points[i].x += (-ax) * 2;
      points[i].y += (-ay) * 2;
    }
  
  /* find the convex hull using Jarvis's March as the points are passed
   * in different orders due to gimp_matrix3_transform_point()
   */
  
  min = 0;
  for (i = 0; i < 4; i++)
    {
      if (points[i].y < points[min].y)
        min = i;
    }

  tx = points[0].x;
  ty = points[0].y;
  
  points[0].x = points[min].x;
  points[0].y = points[min].y;
  
  points[min].x = tx;
  points[min].y = ty;
  
  for (i = 1; i < 4; i++)
    {
      gdouble theta, theta_m = 2 * G_PI;
      gdouble theta_v = 0;
      gint    j;
      
      min = 3;

      for (j = i; j < 4; j++)
      {
        gdouble sy = points[j].y - points[i - 1].y;
        gdouble sx = points[j].x - points[i - 1].x;
        theta = atan2 (sy, sx);

        if ((theta < theta_m) && ((theta > theta_v) || ((theta == theta_v) && (sx > 0))))
          {
            theta_m = theta;
            min = j;
          }
      }

      theta_v = theta_m;

      tx = points[i].x;
      ty = points[i].y;
      
      points[i].x = points[min].x;
      points[i].y = points[min].y;
      
      points[min].x = tx;
      points[min].y = ty;
    }
  
  /* reverse the order of points */
  
  tx = points[0].x; ty = points[0].y;
  points[0].x = points[3].x; points[0].y = points[3].y;
  points[3].x = tx; points[3].y = ty;

  tx = points[1].x; ty = points[1].y;
  points[1].x = points[2].x; points[1].y = points[2].y;
  points[2].x = tx; points[2].y = ty;
  

  /* now, find the largest rectangle using the method described in
   * "Computing the Largest Inscribed Isothetic Rectangle" by
   * D. Hsu, J. Snoeyink, et al.
   */
  
  /*  first create an array of edges  */
  
  cxmin = cxmax = points[3].x;
  cymin = cymax = points[3].y;
  
  for (i = 0, a = points + 3, b = points; i < 4; i++, a = b, b++)
    {
      if (G_UNLIKELY(i == 0))
        {
          cxmin = cxmax = a->x;
          cymin = cymax = a->y;
        }
      else
        {
          if (a->x < cxmin)
            cxmin = a->x;

          if (a->x > cxmax)
              cxmax = a->x;

          if (a->y < cymin)
            cymin = a->y;

          if (a->y > cymax)
            cymax = a->y;
        }
        
      edge_init (edges + i, a, b);
    }

  xint = g_new (Point, cymax);
  
  for (i = 0; i < cymax; i++)
    {
      xint[i].x = intersect_x (edges, i);
      xint[i].y = i;
    }

  top    = find_edge (edges, cxmin, TRUE);
  bottom = find_edge (edges, cxmin, FALSE);

  for (xi = cxmin; xi < cxmax; xi++)
    {
      gint ylo, yhi;

      ymin = intersect_y (top, xi);
      ymax = intersect_y (bottom, xi);

      for (ylo = ymax; ylo > ymin; ylo--)
        {
          for (yhi = ymin; yhi < ymax; yhi++)
            {
              if (yhi > ylo)
                {
                  gint     xlo, xhi;
                  gint     xright;
                  gint     height, width, fixed_width;
                  gint     area;

                  xlo = xint[ylo].x;
                  xhi = xint[yhi].x;
                        
                  xright = MIN (xlo, xhi);

                  height = yhi - ylo;
                  width = xright - xi;

                  if (aspect != 0)
                    {
                      fixed_width = (int) ceil((gdouble) height * aspect);

                      if (fixed_width <= width)
                        width = fixed_width;
                      else
                        width = 0;
                    }
                  
                  area = width * height;

                  if (area > maxarea)
                    {
                      maxarea = area;
                      
                      *x1 = xi;
                      *y1 = ylo;
                      *x2 = xi + width;
                      *y2 = ylo + height;
                    }
                }
            }
        }

      if (xi == top->xmax)
        top = find_edge (edges, xi, TRUE);

      if (xi == bottom->xmax)
        bottom = find_edge (edges, xi, FALSE);
    }

  g_free (xint);
  
  /* translate back the vertices */

  *x1 = *x1 - ((-ax) * 2);
  *y1 = *y1 - ((-ay) * 2);
  *x2 = *x2 - ((-ax) * 2);
  *y2 = *y2 - ((-ay) * 2);
}
