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
#define EPSILON  0.00000001
#define MIN4(a,b,c,d) MIN(MIN((a),(b)),MIN((c),(d)))
#define MAX4(a,b,c,d) MAX(MAX((a),(b)),MAX((c),(d)))


typedef struct
{
  gdouble  x, y;
} Point;

typedef struct
{
  Point   a, b, c, d;
  gdouble area;
  gdouble aspect;
} Rectangle;

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

static void      add_rectangle   (Point      points[4],
                                  Rectangle *r,
                                  Point      a,
                                  Point      b,
                                  Point      c,
                                  Point      d);
static gboolean  intersect       (Point      a,
                                  Point      b,
                                  Point      c,
                                  Point      d,
                                  Point     *i);
static gboolean  intersect_x     (Point      a,
                                  Point      b,
                                  Point      c,
                                  Point     *i);
static gboolean  intersect_y     (Point      a,
                                  Point      b,
                                  Point      c,
                                  Point     *i);
static gboolean  in_poly         (Point      points[4],
                                  Point      p);
static gboolean  point_on_border (Point      points[4],
                                  Point      p);

static void      find_two_point_rectangle            (Rectangle *r,
                                                      Point      points[4],
                                                      gint       p);
static void      find_three_point_rectangle_corner   (Rectangle *r,
                                                      Point      points[4],
                                                      gint       p);
static void      find_three_point_rectangle          (Rectangle *r,
                                                      Point      points[4],
                                                      gint       p);
static void      find_three_point_rectangle_triangle (Rectangle *r,
                                                      Point      points[4],
                                                      gint       p);
static void      find_maximum_aspect_rectangle       (Rectangle *r,
                                                      Point      points[4],
                                                      gint       p);
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
  Point     points[4];
  Rectangle r;
  gint      ax, ay, tx, ty;
  gint      i, j;
  gint      min;

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
      gdouble theta, theta_m = 2.0 * G_PI;
      gdouble theta_v        = 0;

      min = 3;

      for (j = i; j < 4; j++)
        {
          gdouble sy = points[j].y - points[i - 1].y;
          gdouble sx = points[j].x - points[i - 1].x;

          theta = atan2 (sy, sx);

          if ((theta < theta_m) &&
              ((theta > theta_v) || ((theta == theta_v) && (sx > 0))))
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

  tx = points[0].x;
  ty = points[0].y;

  points[0].x = points[3].x;
  points[0].y = points[3].y;
  points[3].x = tx;
  points[3].y = ty;

  tx = points[1].x;
  ty = points[1].y;

  points[1].x = points[2].x;
  points[1].y = points[2].y;
  points[2].x = tx;
  points[2].y = ty;

  r.a.x = r.a.y = r.b.x = r.b.y = r.c.x = r.c.x = r.d.x = r.d.x = r.area = 0;
  r.aspect = aspect;

  if (aspect != 0)
    {
      for (i = 0; i < 4; i++)
        find_maximum_aspect_rectangle (&r, points, i);
    }
  else
    {
      for (i = 0; i < 4; i++)
        {
          find_three_point_rectangle          (&r, points, i);
          find_three_point_rectangle_corner   (&r, points, i);
          find_two_point_rectangle            (&r, points, i);
          find_three_point_rectangle_triangle (&r, points, i);
        }
    }

  *x1 = floor(r.a.x + 0.5);
  *y1 = floor(r.a.y + 0.5);
  *x2 = ceil(r.c.x - 0.5);
  *y2 = ceil(r.c.y - 0.5);

  *x1 = *x1 - ((-ax) * 2);
  *y1 = *y1 - ((-ay) * 2);
  *x2 = *x2 - ((-ax) * 2);
  *y2 = *y2 - ((-ay) * 2);

}

static void
find_three_point_rectangle (Rectangle *r,
                            Point      points[4],
                            gint       p)
{
  Point a = points[p       % 4];  /* 0 1 2 3 */
  Point b = points[(p + 1) % 4];  /* 1 2 3 0 */
  Point c = points[(p + 2) % 4];  /* 2 3 0 2 */
  Point d = points[(p + 3) % 4];  /* 3 0 1 1 */
  Point i1;     /* intersection point */
  Point i2;     /* intersection point */
  Point i3;     /* intersection point */

  if (intersect_x (b, c,  a,  &i1) &&
      intersect_y (c, d,  i1, &i2) &&
      intersect_x (d, a,  i2, &i3))
    add_rectangle (points, r, i3, i3, i1, i1);

  if (intersect_y (b, c,  a,  &i1) &&
      intersect_x (c, d,  i1, &i2) &&
      intersect_y (a, i1, i2, &i3))
    add_rectangle (points, r, i3, i3, i1, i2);

  if (intersect_x (c, d,  a,  &i1) &&
      intersect_y (b, c,  i1, &i2) &&
      intersect_x (a, i1, i2, &i3))
    add_rectangle (points, r, i3, i3, i1, i2);

  if ( intersect_y (c, d,  a,  &i1) &&
       intersect_x (b, c,  i1, &i2) &&
       intersect_y (a, i1, i2, &i3))
    add_rectangle (points, r, i3, i3, i1, i2);
}

static void
find_three_point_rectangle_corner (Rectangle *r,
                                   Point      points[4],
                                   gint       p)
{
  Point a = points[p       % 4];  /* 0 1 2 3 */
  Point b = points[(p + 1) % 4];  /* 1 2 3 0 */
  Point c = points[(p + 2) % 4];  /* 2 3 0 2 */
  Point d = points[(p + 3) % 4];  /* 3 0 1 1 */
  Point i1;     /* intersection point */
  Point i2;     /* intersection point */

  if (intersect_x (b, c, a , &i1) &&
      intersect_y (c, d, i1, &i2))
    add_rectangle (points, r, a, a, i1, i2);

  if (intersect_y (b, c, a , &i1) &&
      intersect_x (c, d, i1, &i2))
    add_rectangle (points, r, a, a, i1, i2);

  if (intersect_x (c, d, a , &i1) &&
      intersect_y (b, c, i1, &i2))
    add_rectangle (points, r, a, a, i1, i2);

  if (intersect_y (c, d, a , &i1) &&
      intersect_x (b, c, i1, &i2))
    add_rectangle (points, r, a, a, i1, i2);
}

static void
find_two_point_rectangle (Rectangle *r,
                          Point      points[4],
                          gint       p)
{
  Point a = points[ p      % 4];  /* 0 1 2 3 */
  Point b = points[(p + 1) % 4];  /* 1 2 3 0 */
  Point c = points[(p + 2) % 4];  /* 2 3 0 2 */
  Point d = points[(p + 3) % 4];  /* 2 3 0 2 */
  Point i1;     /* intersection point */
  Point i2;     /* intersection point */
  Point mid;    /* Mid point */

  add_rectangle (points, r, a, a, c, c);
  add_rectangle (points, r, b, b, d, d);

  if (intersect_x (c, b, a, &i1) &&
      intersect_y (c, b, a, &i2))
    {
      mid.x = ( i1.x + i2.x ) / 2.0;
      mid.y = ( i1.y + i2.y ) / 2.0;

      add_rectangle (points, r, a, a, mid, mid);
    }
}

static void
find_three_point_rectangle_triangle (Rectangle *r,
                                     Point      points[4],
                                     gint       p)
{
  Point a = points[p % 4];          /* 0 1 2 3 */
  Point b = points[(p + 1) % 4];    /* 1 2 3 0 */
  Point c = points[(p + 2) % 4];    /* 2 3 0 2 */
  Point d = points[(p + 3) % 4];    /* 3 0 1 1 */
  Point i1;                         /* intersection point */
  Point i2;                         /* intersection point */
  Point mid;

  mid.x = (a.x + b.x) / 2.0;
  mid.y = (a.y + b.y) / 2.0;

  if (intersect_x (b, c, mid, &i1) &&
      intersect_y (a, d, mid, &i2))
    add_rectangle (points, r, mid, mid, i1, i2);

  if (intersect_y (b, c, mid, &i1) &&
      intersect_x (a, d, mid, &i2))
    add_rectangle (points, r, mid, mid, i1, i2);

  if (intersect_x (a, d, mid, &i1) &&
      intersect_y (b, c, mid, &i2))
    add_rectangle (points, r, mid, mid, i1, i2);

  if (intersect_y (a, d, mid, &i1) &&
      intersect_x (b, c, mid, &i2))
    add_rectangle (points, r, mid, mid, i1, i2);
}

static void
find_maximum_aspect_rectangle (Rectangle *r,
                               Point      points[4],
                               gint       p)
{
  Point a = points[ p      % 4];  /* 0 1 2 3 */
  Point b = points[(p + 1) % 4];  /* 1 2 3 0 */
  Point c = points[(p + 2) % 4];  /* 2 3 0 2 */
  Point d = points[(p + 3) % 4];  /* 2 3 0 2 */
  Point i1;     /* intersection point */
  Point i2;     /* intersection point */
  Point i3;     /* intersection point */

  if (intersect_x (b, c, a, &i1))
    {
      i2.x = i1.x + 1.0 * r->aspect;
      i2.y = i1.y + 1.0;

      if (intersect (d, a, i1, i2, &i3))
        add_rectangle (points, r, i1, i3, i1, i3);

      if (intersect (a, b, i1, i2, &i3))
        add_rectangle (points, r, i1, i3, i1, i3);

      if (intersect (c, d, i1, i2, &i3))
        add_rectangle (points, r, i1, i3, i1, i3);
    }

  if (intersect_x (b, c, a, &i1))
    {
      i2.x = i1.x - 1.0 * r->aspect;
      i2.y = i1.y + 1.0;

      if (intersect (d, a, i1, i2, &i3))
        add_rectangle (points, r, i1, i3, i1, i3);

      if (intersect (a, b, i1, i2, &i3))
        add_rectangle (points, r, i1, i3, i1, i3);

      if (intersect (c, d, i1, i2, &i3))
        add_rectangle (points, r, i1, i3, i1, i3);
    }

  if (intersect_x (c, d, a,  &i1))
    {
      i2.x = i1.x + 1.0 * r->aspect;
      i2.y = i1.y + 1.0;

      if (intersect (d, a, i1, i2, &i3))
        add_rectangle (points, r, i1, i3, i1, i3);

      if (intersect (a, b, i1, i2, &i3))
        add_rectangle (points, r, i1, i3, i1, i3);

      if (intersect (b, c, i1, i2, &i3))
        add_rectangle (points, r, i1, i3, i1, i3);
    }

  if (intersect_x (c, d, a,  &i1))
    {
      i2.x = i1.x - 1.0 * r->aspect;
      i2.y = i1.y + 1.0;

      if (intersect (d, a, i1, i2, &i3))
        add_rectangle (points, r, i1, i3, i1, i3);

      if (intersect (a, b, i1, i2, &i3))
        add_rectangle (points, r, i1, i3, i1, i3);

      if (intersect (b, c, i1, i2, &i3))
        add_rectangle (points, r, i1, i3, i1, i3);
    }
}

static gboolean
in_poly (Point points[4],
         Point p)
{
  Point p1, p2;
  gint  counter = 0;
  gint  i;

  p1 = points[0];

  for (i = 1; i <= 4; i++)
    {
      p2 = points[i % 4];

      if (p.y > MIN (p1.y, p2.y))
        {
          if (p.y <= MAX (p1.y, p2.y))
            {
              if (p.x <= MAX (p1.x, p2.x))
                {
                  if (p1.y != p2.y)
                    {
                      gdouble xinters = ((p.y - p1.y) * (p2.x - p1.x) /
                                         (p2.y - p1.y) + p1.x);

                      if (p1.x == p2.x || p.x <= xinters)
                        counter++;
                    }
                }
            }
        }

      p1 = p2;
    }

  /* border check */
  if (point_on_border (points, p))
    return TRUE;

  return (counter % 2 != 0);
}

static gboolean
point_on_border (Point points[4],
                 Point p)
{
  gint i;

  for (i = 0; i <= 4; i++)
    {
      Point   a  = points[i % 4];
      Point   b  = points[(i + 1) % 4];
      gdouble a1 = (b.y - a.y);
      gdouble b1 = (a.x - b.x);
      gdouble c1 = a1 * a.x + b1 * a.y;
      gdouble c2 = a1 * p.x + b1 * p.y;

      if (ABS (c1 - c2) < EPSILON &&
          MIN (a.x, b.x) <= p.x   &&
          MAX (a.x, b.x) >= p.x   &&
          MIN (a.y, b.y) <= p.y   &&
          MAX (a.y, b.y) >= p.y)
        return TRUE;
    }

  return FALSE;
}

static gboolean
intersect (Point  a,
           Point  b,
           Point  c,
           Point  d,
           Point *i)
{
  gdouble a1  = (b.y - a.y);
  gdouble b1  = (a.x - b.x);
  gdouble c1  = a1 * a.x + b1 * a.y;

  gdouble a2  = (d.y - c.y);
  gdouble b2  = (c.x - d.x);
  gdouble c2  = a2 * c.x + b2 * c.y;
  gdouble det = a1*b2 - a2*b1;

  if (det == 0)
    return FALSE;

  i->x = (b2*c1 - b1*c2) / det;
  i->y = (a1*c2 - a2*c1) / det;

  return TRUE;
}

static gboolean
intersect_x (Point  a,
             Point  b,
             Point  c,
             Point *i)
{
  gdouble a1, b1, c1;
  gdouble a2, b2, c2;
  gdouble det;
  Point   d = c;

  d.y += 1;
  a1 = (b.y - a.y);
  b1 = (a.x - b.x);
  c1 = a1 * a.x + b1 * a.y;

  a2 = (d.y - c.y);
  b2 = (c.x - d.x);
  c2 = a2 * c.x + b2 * c.y;
  det = a1*b2 - a2*b1;

  if (det == 0)
    return FALSE;

  i->x = (b2*c1 - b1*c2) / det;
  i->y = (a1*c2 - a2*c1) / det;

  return TRUE;
}

static gboolean
intersect_y (Point  a,
             Point  b,
             Point  c,
             Point *i)
{
  gdouble a1, b1, c1, a2, b2, c2;
  gdouble det;
  Point   d = c;

  d.x += 1;

  a1 = (b.y - a.y);
  b1 = (a.x - b.x);
  c1 = a1 * a.x + b1 * a.y;

  a2 = (d.y - c.y);
  b2 = (c.x - d.x);
  c2 = a2 * c.x + b2 * c.y;
  det = a1*b2 - a2*b1;

  if (det == 0)
    return FALSE;

  i->x = (b2*c1 - b1*c2) / det;
  i->y = (a1*c2 - a2*c1) / det;

  return TRUE;
}

static void
add_rectangle (Point      points[4],
               Rectangle *r,
               Point      a,
               Point      b,
               Point      c,
               Point      d)
{
  gdouble width;
  gdouble height;
  gdouble minx, maxx;
  gdouble miny, maxy;
  gint    i;

  minx = MIN4(a.x,b.x,c.x,d.x);
  maxx = MAX4(a.x,b.x,c.x,d.x);
  miny = MIN4(a.y,b.y,c.y,d.y);
  maxy = MAX4(a.y,b.y,c.y,d.y);

  a.x = minx;
  a.y = miny;

  b.x = maxx;
  b.y = miny;

  c.x = maxx;
  c.y = maxy;

  d.x = minx;
  d.y = maxy;

  width  =  maxx - minx;
  height =  maxy - miny;

  for ( i = 0 ; i < 4 ; i++)
    {
      if (in_poly(points, a) &&
          in_poly(points, b) &&
          in_poly(points, c) &&
          in_poly(points, d))
        {
          gdouble area = width * height;

          if (r->area <= area)
            {
              r->a.x = a.x;
              r->a.y = a.y;

              r->b.x = b.x;
              r->b.y = b.y;

              r->c.x = c.x;
              r->c.y = c.y;

              r->d.x = d.x;
              r->d.y = d.y;

              r->area = area;
            }
        }
    }
}
