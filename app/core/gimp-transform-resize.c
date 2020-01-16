/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-2001 Spencer Kimball, Peter Mattis, and others
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
 */

#include "config.h"

#include <string.h>

#include <gio/gio.h>
#include <gegl.h>

#include "libgimpmath/gimpmath.h"

#include "core-types.h"

#include "gimp-transform-resize.h"
#include "gimp-transform-utils.h"
#include "gimp-utils.h"


#define EPSILON 0.00000001


typedef struct
{
  GimpVector2 a, b, c, d;
  gdouble     area;
  gdouble     aspect;
} Rectangle;


static void      gimp_transform_resize_adjust        (const GimpVector2 *points,
                                                      gint               n_points,
                                                      gint              *x1,
                                                      gint              *y1,
                                                      gint              *x2,
                                                      gint              *y2);
static void      gimp_transform_resize_crop          (const GimpVector2 *points,
                                                      gint               n_points,
                                                      gdouble            aspect,
                                                      gint              *x1,
                                                      gint              *y1,
                                                      gint              *x2,
                                                      gint              *y2);

static void      add_rectangle                       (const GimpVector2 *points,
                                                      gint               n_points,
                                                      Rectangle         *r,
                                                      GimpVector2        a,
                                                      GimpVector2        b,
                                                      GimpVector2        c,
                                                      GimpVector2        d);
static gboolean  intersect                           (GimpVector2        a,
                                                      GimpVector2        b,
                                                      GimpVector2        c,
                                                      GimpVector2        d,
                                                      GimpVector2       *i);
static gboolean  intersect_x                         (GimpVector2        a,
                                                      GimpVector2        b,
                                                      GimpVector2        c,
                                                      GimpVector2       *i);
static gboolean  intersect_y                         (GimpVector2        a,
                                                      GimpVector2        b,
                                                      GimpVector2        c,
                                                      GimpVector2       *i);
static gboolean  in_poly                             (const GimpVector2 *points,
                                                      gint               n_points,
                                                      GimpVector2        p);
static gboolean  point_on_border                     (const GimpVector2 *points,
                                                      gint               n_points,
                                                      GimpVector2        p);

static void      find_two_point_rectangle            (Rectangle         *r,
                                                      const GimpVector2 *points,
                                                      gint               n_points,
                                                      gint               p);
static void      find_three_point_rectangle_corner   (Rectangle         *r,
                                                      const GimpVector2 *points,
                                                      gint               n_points,
                                                      gint               p);
static void      find_three_point_rectangle          (Rectangle          *r,
                                                      const GimpVector2 *points,
                                                      gint               n_points,
                                                      gint               p);
static void      find_three_point_rectangle_triangle (Rectangle         *r,
                                                      const GimpVector2 *points,
                                                      gint               n_points,
                                                      gint               p);
static void      find_maximum_aspect_rectangle       (Rectangle          *r,
                                                      const GimpVector2 *points,
                                                      gint               n_points,
                                                      gint               p);


/*
 * This function wants to be passed the inverse transformation matrix!!
 */
gboolean
gimp_transform_resize_boundary (const GimpMatrix3   *inv,
                                GimpTransformResize  resize,
                                gdouble              u1,
                                gdouble              v1,
                                gdouble              u2,
                                gdouble              v2,
                                gint                *x1,
                                gint                *y1,
                                gint                *x2,
                                gint                *y2)
{
  GimpVector2 bounds[4];
  GimpVector2 points[5];
  gint        n_points;
  gboolean    valid;
  gint        i;

  g_return_val_if_fail (inv != NULL, FALSE);

  /*  initialize with the original boundary  */
  *x1 = floor (u1);
  *y1 = floor (v1);
  *x2 = ceil  (u2);
  *y2 = ceil  (v2);

  /* if clipping then just return the original rectangle */
  if (resize == GIMP_TRANSFORM_RESIZE_CLIP)
    return TRUE;

  bounds[0] = (GimpVector2) { u1, v1 };
  bounds[1] = (GimpVector2) { u2, v1 };
  bounds[2] = (GimpVector2) { u2, v2 };
  bounds[3] = (GimpVector2) { u1, v2 };

  gimp_transform_polygon (inv, bounds, 4, TRUE,
                          points, &n_points);

  valid = (n_points >= 2);

  /*  check if the transformation matrix is valid at all  */
  for (i = 0; i < n_points && valid; i++)
    valid = (isfinite (points[i].x) && isfinite (points[i].y));

  if (! valid)
    {
      /* since there is no sensible way to deal with this, just do the same as
       * with GIMP_TRANSFORM_RESIZE_CLIP: return
       */
      return FALSE;
    }

  switch (resize)
    {
    case GIMP_TRANSFORM_RESIZE_ADJUST:
      /* return smallest rectangle (with sides parallel to x- and y-axis)
       * that surrounds the new points */
      gimp_transform_resize_adjust (points, n_points,
                                    x1, y1, x2, y2);
      break;

    case GIMP_TRANSFORM_RESIZE_CROP:
      gimp_transform_resize_crop (points, n_points,
                                  0.0,
                                  x1, y1, x2, y2);
      break;

    case GIMP_TRANSFORM_RESIZE_CROP_WITH_ASPECT:
      gimp_transform_resize_crop (points, n_points,
                                  (u2 - u1) / (v2 - v1),
                                  x1, y1, x2, y2);
      break;

    case GIMP_TRANSFORM_RESIZE_CLIP:
      /* Remove warning about not handling all enum values. We handle
       * this case in the beginning of the function
       */
      break;
    }

  /* ensure that resulting rectangle has at least area 1 */
  if (*x1 == *x2)
    (*x2)++;

  if (*y1 == *y2)
    (*y2)++;

  return TRUE;
}

/* this calculates the smallest rectangle (with sides parallel to x- and
 * y-axis) that contains the points d1 to d4
 */
static void
gimp_transform_resize_adjust (const GimpVector2 *points,
                              gint               n_points,
                              gint              *x1,
                              gint              *y1,
                              gint              *x2,
                              gint              *y2)
{
  GimpVector2 top_left;
  GimpVector2 bottom_right;
  gint        i;

  top_left = bottom_right = points[0];

  for (i = 1; i < n_points; i++)
    {
      top_left.x     = MIN (top_left.x,     points[i].x);
      top_left.y     = MIN (top_left.y,     points[i].y);

      bottom_right.x = MAX (bottom_right.x, points[i].x);
      bottom_right.y = MAX (bottom_right.y, points[i].y);
    }

  *x1 = (gint) floor (top_left.x + EPSILON);
  *y1 = (gint) floor (top_left.y + EPSILON);

  *x2 = (gint) ceil (bottom_right.x - EPSILON);
  *y2 = (gint) ceil (bottom_right.y - EPSILON);
}

static void
gimp_transform_resize_crop (const GimpVector2 *orig_points,
                            gint               n_points,
                            gdouble            aspect,
                            gint              *x1,
                            gint              *y1,
                            gint              *x2,
                            gint              *y2)
{
  GimpVector2 points[5];
  Rectangle   r;
  GimpVector2 t,a;
  gint        i, j;
  gint        min;

  memcpy (points, orig_points, sizeof (GimpVector2) * n_points);

  /* find lowest, rightmost corner of surrounding rectangle */
  a.x = 0;
  a.y = 0;
  for (i = 0; i < 4; i++)
    {
      if (points[i].x < a.x)
        a.x = points[i].x;

      if (points[i].y < a.y)
        a.y = points[i].y;
    }

  /* and translate all the points to the first quadrant */
  for (i = 0; i < n_points; i++)
    {
      points[i].x += (-a.x) * 2;
      points[i].y += (-a.y) * 2;
    }

  /* find the convex hull using Jarvis's March as the points are passed
   * in different orders due to gimp_matrix3_transform_point()
   */
  min = 0;
  for (i = 0; i < n_points; i++)
    {
      if (points[i].y < points[min].y)
        min = i;
    }

  t = points[0];
  points[0] = points[min];
  points[min] = t;

  for (i = 1; i < n_points - 1; i++)
    {
      gdouble min_theta;
      gdouble min_mag;
      int next;

      next = n_points - 1;
      min_theta = 2.0 * G_PI;
      min_mag = DBL_MAX;

      for (j = i; j < n_points; j++)
        {
          gdouble theta;
          gdouble sy;
          gdouble sx;
          gdouble mag;

          sy = points[j].y - points[i - 1].y;
          sx = points[j].x - points[i - 1].x;

          if ((sx == 0.0) && (sy == 0.0))
            {
              next = j;
              break;
            }

          theta = atan2 (-sy, -sx);
          mag = (sx * sx) + (sy * sy);

          if ((theta < min_theta) ||
              ((theta == min_theta) && (mag < min_mag)))
            {
              min_theta = theta;
              min_mag = mag;
              next = j;
            }
        }

      t = points[i];
      points[i] = points[next];
      points[next] = t;
    }

  /* reverse the order of points */
  for (i = 0; i < n_points / 2; i++)
    {
      t                        = points[i];
      points[i]                = points[n_points - i - 1];
      points[n_points - i - 1] = t;
    }

  r.a.x = r.a.y = r.b.x = r.b.y = r.c.x = r.c.y = r.d.x = r.d.y = r.area = 0;
  r.aspect = aspect;

  if (aspect != 0)
    {
      for (i = 0; i < n_points; i++)
        find_maximum_aspect_rectangle (&r, points, n_points, i);
    }
  else
    {
      for (i = 0; i < n_points; i++)
        {
          find_three_point_rectangle          (&r, points, n_points, i);
          find_three_point_rectangle_corner   (&r, points, n_points, i);
          find_two_point_rectangle            (&r, points, n_points, i);
          find_three_point_rectangle_triangle (&r, points, n_points, i);
        }
    }

  if (r.area == 0)
    {
      /* saveguard if something went wrong, adjust and give warning */
      gimp_transform_resize_adjust (orig_points, n_points,
                                    x1, y1, x2, y2);
      g_printerr ("no rectangle found by algorithm, no cropping done\n");
      return;
    }
  else
    {
      /* round and translate the calculated points back */
      *x1 = floor (r.a.x + 0.5);
      *y1 = floor (r.a.y + 0.5);
      *x2 = ceil  (r.c.x - 0.5);
      *y2 = ceil  (r.c.y - 0.5);

      *x1 = *x1 - ((-a.x) * 2);
      *y1 = *y1 - ((-a.y) * 2);
      *x2 = *x2 - ((-a.x) * 2);
      *y2 = *y2 - ((-a.y) * 2);
      return;
    }
}

static void
find_three_point_rectangle (Rectangle         *r,
                            const GimpVector2 *points,
                            gint               n_points,
                            gint               p)
{
  GimpVector2 a = points[p       % n_points];  /* 0 1 2 3 */
  GimpVector2 b = points[(p + 1) % n_points];  /* 1 2 3 0 */
  GimpVector2 c = points[(p + 2) % n_points];  /* 2 3 0 1 */
  GimpVector2 d = points[(p + 3) % n_points];  /* 3 0 1 2 */
  GimpVector2 i1;                              /* intersection point */
  GimpVector2 i2;                              /* intersection point */
  GimpVector2 i3;                              /* intersection point */

  if (intersect_x (b, c, a,  &i1) &&
      intersect_y (c, d, i1, &i2) &&
      intersect_x (d, a, i2, &i3))
    add_rectangle (points, n_points, r, i3, i3, i1, i1);

  if (intersect_y (b, c, a,  &i1) &&
      intersect_x (c, d, i1, &i2) &&
      intersect_y (d, a, i2, &i3))
    add_rectangle (points, n_points, r, i3, i3, i1, i1);

  if (intersect_x (d, c, a,  &i1) &&
      intersect_y (c, b, i1, &i2) &&
      intersect_x (b, a, i2, &i3))
    add_rectangle (points, n_points, r, i3, i3, i1, i1);

  if (intersect_y (d, c, a,  &i1) &&
      intersect_x (c, b, i1, &i2) &&
      intersect_y (b, a, i2, &i3))
    add_rectangle (points, n_points, r, i3, i3, i1, i1);
}

static void
find_three_point_rectangle_corner (Rectangle         *r,
                                   const GimpVector2 *points,
                                   gint               n_points,
                                   gint               p)
{
  GimpVector2 a = points[p       % n_points];  /* 0 1 2 3 */
  GimpVector2 b = points[(p + 1) % n_points];  /* 1 2 3 0 */
  GimpVector2 c = points[(p + 2) % n_points];  /* 2 3 0 2 */
  GimpVector2 d = points[(p + 3) % n_points];  /* 3 0 2 1 */
  GimpVector2 i1;                              /* intersection point */
  GimpVector2 i2;                              /* intersection point */

  if (intersect_x (b, c, a , &i1) &&
      intersect_y (c, d, i1, &i2))
    add_rectangle (points, n_points, r, a, a, i1, i2);

  if (intersect_y (b, c, a , &i1) &&
      intersect_x (c, d, i1, &i2))
    add_rectangle (points, n_points, r, a, a, i1, i2);

  if (intersect_x (c, d, a , &i1) &&
      intersect_y (b, c, i1, &i2))
    add_rectangle (points, n_points, r, a, a, i1, i2);

  if (intersect_y (c, d, a , &i1) &&
      intersect_x (b, c, i1, &i2))
    add_rectangle (points, n_points, r, a, a, i1, i2);
}

static void
find_two_point_rectangle (Rectangle         *r,
                          const GimpVector2 *points,
                          gint               n_points,
                          gint               p)
{
  GimpVector2 a = points[ p      % n_points];  /* 0 1 2 3 */
  GimpVector2 b = points[(p + 1) % n_points];  /* 1 2 3 0 */
  GimpVector2 c = points[(p + 2) % n_points];  /* 2 3 0 1 */
  GimpVector2 d = points[(p + 3) % n_points];  /* 3 0 1 2 */
  GimpVector2 i1;                              /* intersection point */
  GimpVector2 i2;                              /* intersection point */
  GimpVector2 mid;                             /* Mid point */

  add_rectangle (points, n_points, r, a, a, c, c);
  add_rectangle (points, n_points, r, b, b, d, d);

  if (intersect_x (c, b, a, &i1) &&
      intersect_y (c, b, a, &i2))
    {
      mid.x = ( i1.x + i2.x ) / 2.0;
      mid.y = ( i1.y + i2.y ) / 2.0;

      add_rectangle (points, n_points, r, a, a, mid, mid);
    }
}

static void
find_three_point_rectangle_triangle (Rectangle         *r,
                                     const GimpVector2 *points,
                                     gint               n_points,
                                     gint               p)
{
  GimpVector2 a = points[p       % n_points];  /* 0 1 2 3 */
  GimpVector2 b = points[(p + 1) % n_points];  /* 1 2 3 0 */
  GimpVector2 c = points[(p + 2) % n_points];  /* 2 3 0 1 */
  GimpVector2 d = points[(p + 3) % n_points];  /* 3 0 1 2 */
  GimpVector2 i1;                              /* intersection point */
  GimpVector2 i2;                              /* intersection point */
  GimpVector2 mid;

  mid.x = (a.x + b.x) / 2.0;
  mid.y = (a.y + b.y) / 2.0;

  if (intersect_x (b, c, mid, &i1) &&
      intersect_y (a, d, mid, &i2))
    add_rectangle (points, n_points, r, mid, mid, i1, i2);

  if (intersect_y (b, c, mid, &i1) &&
      intersect_x (a, d, mid, &i2))
    add_rectangle (points, n_points, r, mid, mid, i1, i2);

  if (intersect_x (a, d, mid, &i1) &&
      intersect_y (b, c, mid, &i2))
    add_rectangle (points, n_points, r, mid, mid, i1, i2);

  if (intersect_y (a, d, mid, &i1) &&
      intersect_x (b, c, mid, &i2))
    add_rectangle (points, n_points, r, mid, mid, i1, i2);
}

static void
find_maximum_aspect_rectangle (Rectangle         *r,
                               const GimpVector2 *points,
                               gint               n_points,
                               gint               p)
{
  GimpVector2 a = points[ p      % n_points];  /* 0 1 2 3 */
  GimpVector2 b = points[(p + 1) % n_points];  /* 1 2 3 0 */
  GimpVector2 c = points[(p + 2) % n_points];  /* 2 3 0 1 */
  GimpVector2 d = points[(p + 3) % n_points];  /* 3 0 1 2 */
  GimpVector2 i1;                              /* intersection point */
  GimpVector2 i2;                              /* intersection point */
  GimpVector2 i3;                              /* intersection point */

  if (intersect_x (b, c, a, &i1))
    {
      i2.x = i1.x + 1.0 * r->aspect;
      i2.y = i1.y + 1.0;

      if (intersect (d, a, i1, i2, &i3))
        add_rectangle (points, n_points, r, i1, i3, i1, i3);

      if (intersect (a, b, i1, i2, &i3))
        add_rectangle (points, n_points, r, i1, i3, i1, i3);

      if (intersect (c, d, i1, i2, &i3))
        add_rectangle (points, n_points, r, i1, i3, i1, i3);

      i2.x = i1.x - 1.0 * r->aspect;
      i2.y = i1.y + 1.0;

      if (intersect (d, a, i1, i2, &i3))
        add_rectangle (points, n_points, r, i1, i3, i1, i3);

      if (intersect (a, b, i1, i2, &i3))
        add_rectangle (points, n_points, r, i1, i3, i1, i3);

      if (intersect (c, d, i1, i2, &i3))
        add_rectangle (points, n_points, r, i1, i3, i1, i3);
    }

  if (intersect_y (b, c, a, &i1))
    {
      i2.x = i1.x + 1.0 * r->aspect;
      i2.y = i1.y + 1.0;

      if (intersect (d, a, i1, i2, &i3))
        add_rectangle (points, n_points, r, i1, i3, i1, i3);

      if (intersect (a, b, i1, i2, &i3))
        add_rectangle (points, n_points, r, i1, i3, i1, i3);

      if (intersect (c, d, i1, i2, &i3))
        add_rectangle (points, n_points, r, i1, i3, i1, i3);

      i2.x = i1.x - 1.0 * r->aspect;
      i2.y = i1.y + 1.0;

      if (intersect (d, a, i1, i2, &i3))
        add_rectangle (points, n_points, r, i1, i3, i1, i3);

      if (intersect (a, b, i1, i2, &i3))
        add_rectangle (points, n_points, r, i1, i3, i1, i3);

      if (intersect (c, d, i1, i2, &i3))
        add_rectangle (points, n_points, r, i1, i3, i1, i3);
    }

  if (intersect_x (c, d, a,  &i1))
    {
      i2.x = i1.x + 1.0 * r->aspect;
      i2.y = i1.y + 1.0;

      if (intersect (d, a, i1, i2, &i3))
        add_rectangle (points, n_points, r, i1, i3, i1, i3);

      if (intersect (a, b, i1, i2, &i3))
        add_rectangle (points, n_points, r, i1, i3, i1, i3);

      if (intersect (b, c, i1, i2, &i3))
        add_rectangle (points, n_points, r, i1, i3, i1, i3);

      i2.x = i1.x - 1.0 * r->aspect;
      i2.y = i1.y + 1.0;

      if (intersect (d, a, i1, i2, &i3))
        add_rectangle (points, n_points, r, i1, i3, i1, i3);

      if (intersect (a, b, i1, i2, &i3))
        add_rectangle (points, n_points, r, i1, i3, i1, i3);

      if (intersect (b, c, i1, i2, &i3))
        add_rectangle (points, n_points, r, i1, i3, i1, i3);
    }

  if (intersect_y (c, d, a,  &i1))
    {
      i2.x = i1.x + 1.0 * r->aspect;
      i2.y = i1.y + 1.0;

      if (intersect (d, a, i1, i2, &i3))
        add_rectangle (points, n_points, r, i1, i3, i1, i3);

      if (intersect (a, b, i1, i2, &i3))
        add_rectangle (points, n_points, r, i1, i3, i1, i3);

      if (intersect (b, c, i1, i2, &i3))
        add_rectangle (points, n_points, r, i1, i3, i1, i3);

      i2.x = i1.x - 1.0 * r->aspect;
      i2.y = i1.y + 1.0;

      if (intersect (d, a, i1, i2, &i3))
        add_rectangle (points, n_points, r, i1, i3, i1, i3);

      if (intersect (a, b, i1, i2, &i3))
        add_rectangle (points, n_points, r, i1, i3, i1, i3);

      if (intersect (b, c, i1, i2, &i3))
        add_rectangle (points, n_points, r, i1, i3, i1, i3);
    }
}

/* check if point is inside the polygon "points", if point is on border
 * its still inside.
 */
static gboolean
in_poly (const GimpVector2 *points,
         gint               n_points,
         GimpVector2        p)
{
  GimpVector2 p1, p2;
  gint  counter = 0;
  gint  i;

  p1 = points[0];

  for (i = 1; i <= n_points; i++)
    {
      p2 = points[i % n_points];

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
  if (point_on_border (points, n_points, p))
    return TRUE;

  return (counter % 2 != 0);
}

/* check if the point p lies on the polygon "points"
 */
static gboolean
point_on_border (const GimpVector2 *points,
                 gint               n_points,
                 GimpVector2        p)
{
  gint i;

  for (i = 0; i <= n_points; i++)
    {
      GimpVector2   a  = points[i       % n_points];
      GimpVector2   b  = points[(i + 1) % n_points];
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

/* calculate the intersection point of the line a-b with the line c-d
 * and write it to i, if existing.
 */
static gboolean
intersect (GimpVector2  a,
           GimpVector2  b,
           GimpVector2  c,
           GimpVector2  d,
           GimpVector2 *i)
{
  gdouble a1  = (b.y - a.y);
  gdouble b1  = (a.x - b.x);
  gdouble c1  = a1 * a.x + b1 * a.y;

  gdouble a2  = (d.y - c.y);
  gdouble b2  = (c.x - d.x);
  gdouble c2  = a2 * c.x + b2 * c.y;
  gdouble det = a1 * b2 - a2 * b1;

  if (det == 0)
    return FALSE;

  i->x = (b2 * c1 - b1 * c2) / det;
  i->y = (a1 * c2 - a2 * c1) / det;

  return TRUE;
}

/* calculate the intersection point of the line a-b with the vertical line
 * through c and write it to i, if existing.
 */
static gboolean
intersect_x (GimpVector2  a,
             GimpVector2  b,
             GimpVector2  c,
             GimpVector2 *i)
{
  GimpVector2   d = c;
  d.y += 1;

  return intersect(a,b,c,d,i);
}

/* calculate the intersection point of the line a-b with the horizontal line
 * through c and write it to i, if existing.
 */
static gboolean
intersect_y (GimpVector2  a,
             GimpVector2  b,
             GimpVector2  c,
             GimpVector2 *i)
{
  GimpVector2   d = c;
  d.x += 1;

  return intersect(a,b,c,d,i);
}

/* this takes the smallest ortho-aligned (the sides of the rectangle are
 * parallel to the x- and y-axis) rectangle fitting around the points a to d,
 * checks if the whole rectangle is inside the polygon described by points and
 * writes it to r if the area is bigger than the rectangle already stored in r.
 */
static void
add_rectangle (const GimpVector2 *points,
               gint               n_points,
               Rectangle         *r,
               GimpVector2        a,
               GimpVector2        b,
               GimpVector2        c,
               GimpVector2        d)
{
  gdouble width;
  gdouble height;
  gdouble minx, maxx;
  gdouble miny, maxy;

  /* get the orthoaligned (the sides of the rectangle are parallel to the x-
   * and y-axis) rectangle surrounding the points a to d.
   */
  minx = MIN4 (a.x, b.x, c.x, d.x);
  maxx = MAX4 (a.x, b.x, c.x, d.x);
  miny = MIN4 (a.y, b.y, c.y, d.y);
  maxy = MAX4 (a.y, b.y, c.y, d.y);

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

  /* check if this rectangle is inside the polygon "points" */
  if (in_poly (points, n_points, a) &&
      in_poly (points, n_points, b) &&
      in_poly (points, n_points, c) &&
      in_poly (points, n_points, d))
    {
      gdouble area = width * height;

      /* check if the new rectangle is larger (in terms of area)
       * than the currently stored rectangle in r, if yes store
       * new rectangle to r
       */
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
