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
  gdouble   x, y;
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

  if (resize == GIMP_TRANSFORM_SIZE_CLIP)
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
      resize = GIMP_TRANSFORM_SIZE_CLIP;
    }

  switch (resize)
    {
    case GIMP_TRANSFORM_SIZE_ADJUST:
      gimp_transform_resize_adjust (dx1, dy1, dx2, dy2, dx3, dy3, dx4, dy4,
                                    x1, y1, x2, y2);
      break;

    case GIMP_TRANSFORM_SIZE_CLIP:
      /*  we are all done already  */
      break;

    case GIMP_TRANSFORM_SIZE_CROP:
      gimp_transform_resize_crop (dx1, dy1, dx2, dy2, dx3, dy3, dx4, dy4,
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
  edge->xmin  = MIN (ceil (p->x), ceil (q->x));
  edge->xmax  = MAX (floor (p->x), floor (q->x));
  edge->ymin  = MIN (ceil (p->y), ceil (q->y));
  edge->ymax  = MAX (floor (p->y), floor (q->y));

  edge->top   = p->x > q->x;
  edge->right = p->y > q->y;

  edge->m     = (q->y - p->y) / (q->x - p->x);
  edge->b     = p->y - edge->m * p->x;
}

static const Edge *
find_edge (const Edge *edges,
           gint        x,
           gboolean    top)
{
  const Edge *emax = edges;
  gint        i;

  for (i = 0; i < 4; i++)
    {
      const Edge *e = edges + i;

      if (e->xmin == x && e->xmax != e->xmin &&
          ((e->top && top) || (!e->top && !top)))
            emax = e;
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
        x0 = (double) (y + 0.5 - edges[i].b) / edges[i].m;
        x1 = (double) (y - 0.5 - edges[i].b) / edges[i].m;
      }

  return (gint) floor (MIN (x0, x1));
}

static gint
intersect_y (const Edge *edge,
             gint        xi)
{
  gdouble yfirst = edge->m * (xi - 0.5) + edge->b;
  gdouble ylast  = edge->m * (xi + 0.5) + edge->b;

  return (gint) floor (edge->top ? MAX (yfirst, ylast) : MIN (yfirst, ylast));
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
                            gint    *x1,
                            gint    *y1,
                            gint    *x2,
                            gint    *y2)
{
  Point        points[4];
  Edge         edges[4];
  const Point *a;
  const Point *b;
  const Edge  *top;
  const Edge  *bottom;
  gint        *px;
  gint         xmin, ymin;
  gint         xmax, ymax;
  gint         maxarea = 0;
  gint         xi, y;
  gint         i;

  /*  fill in the points array  */
  points[0].x = dx1;
  points[0].y = dy1;
  points[1].x = dx2;
  points[1].y = dy2;
  points[2].x = dx3;
  points[2].y = dy3;
  points[3].x = dx4;
  points[3].y = dy4;

  /*  create an array of edges  */
  for (i = 0, a = points + 3, b = points; i < 4; i++, a = b, b++)
    edge_init (edges + i, a, b);

  /*  determine the bounding box  */
  xmin = edges[0].xmin;
  xmax = edges[0].xmax;
  ymin = edges[0].ymin;
  ymax = edges[0].ymax;

  for (i = 1; i < 4; i++)
    {
      const Edge *edge = edges + i;

      if (edge->xmin < xmin)
        xmin = edge->xmin;

      if (edge->xmax > xmax)
        xmax = edge->xmax;

      if (edge->ymin < ymin)
        ymin = edge->ymin;

      if (edge->ymax > ymax)
        ymax = edge->ymax;
    }

  g_printerr ("%d, %d -> %d, %d\n", xmin, ymin, xmax, ymax);

  px = g_new (gint, ymax - ymin + 1);

  for (y = ymin, i = 0; y <= ymax; y++, i++)
    px[i] = intersect_x (edges, y);

  top    = find_edge (edges, xmin, TRUE);
  bottom = find_edge (edges, xmax, FALSE);

  g_printerr ("top: %d, %d -> %d, %d\n",
              top->xmin, top->ymin, top->xmax, top->ymax);
  g_printerr ("bottom: %d, %d -> %d, %d\n",
              bottom->xmin, bottom->ymin, bottom->xmax, bottom->ymax);

  for (xi = xmin; xi < xmax; xi++)
    {
      gint ylo, yhi;

      ymin = intersect_y (top, xi);
      ymax = intersect_y (bottom, xi);

      for (ylo = ymax; ylo >= ymin; ylo--)
        {
          for (yhi = ymin; yhi <= ymax; yhi++)
            {
              if (yhi > ylo)
                {
                  gint xlo    = px[ylo - ymin];
                  gint xhi    = px[yhi - ymin];
                  gint width  = MIN (xlo, xhi) - xi;
                  gint height = yhi - ylo;
                  gint area   = width * height;

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

  g_printerr ("%d, %d -> %d, %d\n", *x1, *y1, *x2, *y2);

  g_free (px);
}
