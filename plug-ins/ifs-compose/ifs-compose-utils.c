/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * IfsCompose is a interface for creating IFS fractals by
 * direct manipulation.
 * Copyright (C) 1997 Owen Taylor
 *
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

#include <stdlib.h>
#include <string.h>

#include <gdk/gdk.h>

#include <libgimp/gimp.h>

#include "ifs-compose.h"


typedef struct
{
  GdkPoint point;
  gdouble angle;
} SortPoint;


/* local functions */
static void     aff_element_compute_click_boundary (AffElement     *elem,
                                                    gint            num_elements,
                                                    gdouble        *points_x,
                                                    gdouble        *points_y);
static guchar * create_brush                       (IfsComposeVals *ifsvals,
                                                    gint           *brush_size,
                                                    gdouble        *brush_offset);


void
aff2_translate (Aff2    *naff,
                gdouble  x,
                gdouble  y)
{
  naff->a11 = 1.0;
  naff->a12 = 0;
  naff->a21 = 0;
  naff->a22 = 1.0;
  naff->b1 = x;
  naff->b2 = y;
}

void
aff2_rotate (Aff2    *naff,
             gdouble  theta)
{
  naff->a11 = cos(theta);
  naff->a12 = sin(theta);
  naff->a21 = -naff->a12;
  naff->a22 = naff->a11;
  naff->b1 = 0;
  naff->b2 = 0;
}

void
aff2_scale (Aff2    *naff,
            gdouble  s,
            gboolean flip)
{
  if (flip)
    naff->a11 = -s;
  else
    naff->a11 = s;

  naff->a12 = 0;
  naff->a21 = 0;
  naff->a22 = s;
  naff->b1 = 0;
  naff->b2 = 0;
}

/* Create a unitary transform with given x-y asymmetry and shear */
void
aff2_distort (Aff2    *naff,
              gdouble  asym,
              gdouble  shear)
{
  naff->a11 = asym;
  naff->a22 = 1/asym;
  naff->a12 = shear;
  naff->a21 = 0;
  naff->b1 = 0;
  naff->b2 = 0;
}

/* Find a pure stretch in some direction that brings xo,yo to xn,yn */
void
aff2_compute_stretch (Aff2    *naff,
                      gdouble  xo,
                      gdouble  yo,
                      gdouble  xn,
                      gdouble  yn)
{
  gdouble denom = xo*xn + yo*yn;

  if (denom == 0.0)     /* singular */
    {
      naff->a11 = 1.0;
      naff->a12 = 0.0;
      naff->a21 = 0.0;
      naff->a22 = 1.0;
    }
  else
    {
      naff->a11 = (SQR(xn) + SQR(yo)) / denom;
      naff->a22 = (SQR(xo) + SQR(yn)) / denom;
      naff->a12 = naff->a21 = (xn * yn - xo * yo) / denom;
    }

  naff->b1 = 0.0;
  naff->b2 = 0.0;
}

void
aff2_compose (Aff2 *naff,
              Aff2 *aff1,
              Aff2 *aff2)
{
  naff->a11 = aff1->a11 * aff2->a11 + aff1->a12 * aff2->a21;
  naff->a12 = aff1->a11 * aff2->a12 + aff1->a12 * aff2->a22;
  naff->b1  = aff1->a11 * aff2->b1  + aff1->a12 * aff2->b2 + aff1->b1;
  naff->a21 = aff1->a21 * aff2->a11 + aff1->a22 * aff2->a21;
  naff->a22 = aff1->a21 * aff2->a12 + aff1->a22 * aff2->a22;
  naff->b2  = aff1->a21 * aff2->b1  + aff1->a22 * aff2->b2 + aff1->b2;
}

/* Returns the identity matrix if the original matrix was singular */
void
aff2_invert (Aff2 *naff,
             Aff2 *aff)
{
  gdouble det = aff->a11 * aff->a22 - aff->a12 * aff->a21;

  if (det==0)
    {
      aff2_scale (naff, 1.0, 0);
    }
  else
    {
      naff->a11 = aff->a22 / det;
      naff->a22 = aff->a11 / det;
      naff->a21 = - aff->a21 / det;
      naff->a12 = - aff->a12 / det;
      naff->b1 = - naff->a11 * aff->b1 - naff->a12 * aff->b2;
      naff->b2 = - naff->a21 * aff->b1 - naff->a22 * aff->b2;
    }
}

void
aff2_apply (Aff2    *aff,
            gdouble  x,
            gdouble  y,
            gdouble *xf,
            gdouble *yf)
{
  gdouble xt = aff->a11 * x + aff->a12 * y + aff->b1;
  gdouble yt = aff->a21 * x + aff->a22 * y + aff->b2;

  *xf = xt;
  *yf = yt;
}

/* Find the fixed point of an affine transformation
   (Will return garbage for pure translations) */

void
aff2_fixed_point (Aff2    *aff,
                  gdouble *xf,
                  gdouble *yf)
{
  Aff2 t1,t2;

  t1.a11 = 1 - aff->a11;
  t1.a22 = 1 - aff->a22;
  t1.a12 = -aff->a12;
  t1.a21 = -aff->a21;
  t1.b1  = 0;
  t1.b2  = 0;

  aff2_invert (&t2, &t1);
  aff2_apply (&t2, aff->b1, aff->b2, xf, yf);
}

void
aff3_apply (Aff3    *t,
            gdouble  x,
            gdouble  y,
            gdouble  z,
            gdouble *xf,
            gdouble *yf,
            gdouble *zf)
{
  gdouble xt = (t->vals[0][0] * x +
                t->vals[0][1] * y +
                t->vals[0][2] * z + t->vals[0][3]);
  gdouble yt = (t->vals[1][0] * x +
                t->vals[1][1] * y +
                t->vals[1][2] * z + t->vals[1][3]);
  gdouble zt = (t->vals[2][0] * x +
                t->vals[2][1] * y +
                t->vals[2][2] * z + t->vals[2][3]);

  *xf = xt;
  *yf = yt;
  *zf = zt;
}

static int
ipolygon_sort_func (const void *a,
                    const void *b)
{
  if (((SortPoint *)a)->angle < ((SortPoint *)b)->angle)
    return -1;
  else if (((SortPoint *)a)->angle > ((SortPoint *)b)->angle)
    return 1;
  else
    return 0;
}

/* Return a newly-allocated polygon which is the convex hull
   of the given polygon.

   Uses the Graham scan. see
   http://www.cs.curtin.edu.au/units/cg201/notes/node77.html

   for a description
*/

IPolygon *
ipolygon_convex_hull (IPolygon *poly)
{
  gint       num_new     = poly->npoints;
  GdkPoint  *new_points  = g_new (GdkPoint, num_new);
  SortPoint *sort_points = g_new (SortPoint, num_new);
  IPolygon  *new_poly    = g_new (IPolygon, 1);

  gint       i, j;
  gint       x1, x2, y1, y2;
  gint       lowest;
  GdkPoint   lowest_pt;

  new_poly->points = new_points;
  if (num_new <= 3)
    {
      memcpy (new_points, poly->points, num_new * sizeof (GdkPoint));
      new_poly->npoints = num_new;
      g_free (sort_points);
      return new_poly;
    }

  /* scan for the lowest point */
  lowest_pt = poly->points[0];
  lowest = 0;

  for (i = 1; i < num_new; i++)
    if (poly->points[i].y < lowest_pt.y)
      {
        lowest_pt = poly->points[i];
        lowest = i;
      }

  /* sort by angle from lowest point */

  for (i = 0, j = 0; i < num_new; i++, j++)
    {
      if (i==lowest)
        j--;
      else
        {
          gdouble dy = poly->points[i].y - lowest_pt.y;
          gdouble dx = poly->points[i].x - lowest_pt.x;

          if (dy == 0 && dx == 0)
            {
              j--;
              num_new--;
              continue;
            }
          sort_points[j].point = poly->points[i];
          sort_points[j].angle = atan2 (dy, dx);
        }
    }

  qsort (sort_points, num_new - 1, sizeof (SortPoint), ipolygon_sort_func);

  /* now ensure that all turns as we trace the perimeter are
     counter-clockwise */

  new_points[0] = lowest_pt;
  new_points[1] = sort_points[0].point;
  x1 = new_points[1].x - new_points[0].x;
  y1 = new_points[1].y - new_points[0].y;

  for (i = 1, j = 2; j < num_new; i++, j++)
    {
      x2 = sort_points[i].point.x - new_points[j - 1].x;
      y2 = sort_points[i].point.y - new_points[j - 1].y;

      if (x2 == 0 && y2 == 0)
        {
          num_new--;
          j--;
          continue;
        }

      while (x1 * y2 - x2 * y1 < 0) /* clockwise rotation */
        {
          num_new--;
          j--;
          x1 = new_points[j - 1].x - new_points[j - 2].x;
          y1 = new_points[j - 1].y - new_points[j - 2].y;
          x2 = sort_points[i].point.x - new_points[j - 1].x;
          y2 = sort_points[i].point.y - new_points[j - 1].y;
        }
      new_points[j] = sort_points[i].point;
      x1 = x2;
      y1 = y2;
    }

  g_free (sort_points);

  new_poly->npoints = num_new;

  return new_poly;
}

/* Determines whether a specified point is in the given polygon.
   Based on

   inpoly.c by Bob Stein and Craig Yap.

   (Linux Journal, Issue 35 (March 1997), p 68)
   */

gint
ipolygon_contains (IPolygon *poly,
                   gint      xt,
                   gint      yt)
{
  gint xnew, ynew;
  gint xold, yold;
  gint x1,y1;
  gint x2,y2;

  gint i;
  gint inside = 0;

  if (poly->npoints < 3)
    return 0;

  xold=poly->points[poly->npoints - 1].x;
  yold=poly->points[poly->npoints - 1].y;
  for (i = 0; i < poly->npoints; i++)
    {
      xnew = poly->points[i].x;
      ynew = poly->points[i].y;
      if (xnew > xold)
        {
          x1 = xold;
          x2 = xnew;
          y1 = yold;
          y2 = ynew;
        }
      else
        {
          x1 = xnew;
          x2 = xold;
          y1 = ynew;
          y2 = yold;
        }
      if ((xnew < xt) == (xt <= xold) &&
          (yt - y1)*(x2 - x1) < (y2 - y1)*(xt - x1))
        inside = !inside;
      xold = xnew;
      yold = ynew;
    }
  return inside;
}

void
aff_element_compute_color_trans (AffElement *elem)
{
  gdouble red_rgb[3];
  gdouble green_rgb[3];
  gdouble blue_rgb[3];
  gdouble black_rgb[3];
  gdouble target_rgb[3];
  int     i, j;

  gegl_color_get_pixel (elem->v.red_color, babl_format ("R'G'B' double"), red_rgb);
  gegl_color_get_pixel (elem->v.green_color, babl_format ("R'G'B' double"), green_rgb);
  gegl_color_get_pixel (elem->v.blue_color, babl_format ("R'G'B' double"), blue_rgb);
  gegl_color_get_pixel (elem->v.black_color, babl_format ("R'G'B' double"), black_rgb);
  gegl_color_get_pixel (elem->v.target_color, babl_format ("R'G'B' double"), target_rgb);

  if (elem->v.simple_color)
    {
      gdouble mag2;

      mag2  = SQR (target_rgb[0]);
      mag2 += SQR (target_rgb[1]);
      mag2 += SQR (target_rgb[2]);

      /* For mag2 == 0, the transformation blows up in general
         but is well defined for hue_scale == value_scale, so
         we assume that special case. */
      if (mag2 == 0)
        for (i = 0; i < 3; i++)
          {
            for (j = 0; j < 4; j++)
              elem->color_trans.vals[i][j] = 0.0;

            elem->color_trans.vals[i][i] = elem->v.hue_scale;
          }
      else
        {
          /*  red  */
          for (j = 0; j < 3; j++)
            {
              elem->color_trans.vals[0][j] = target_rgb[0]
                / mag2 * (elem->v.value_scale - elem->v.hue_scale);
            }

          /*  green  */
          for (j = 0; j < 3; j++)
            {
              elem->color_trans.vals[1][j] = target_rgb[1]
                / mag2 * (elem->v.value_scale - elem->v.hue_scale);
            }

          /*  blue  */
          for (j = 0; j < 3; j++)
            {
              elem->color_trans.vals[2][j] = target_rgb[2]
                / mag2 * (elem->v.value_scale - elem->v.hue_scale);
            }

          elem->color_trans.vals[0][0] += elem->v.hue_scale;
          elem->color_trans.vals[1][1] += elem->v.hue_scale;
          elem->color_trans.vals[2][2] += elem->v.hue_scale;

          elem->color_trans.vals[0][3] =
            (1 - elem->v.value_scale) * target_rgb[0];
          elem->color_trans.vals[1][3] =
            (1 - elem->v.value_scale) * target_rgb[1];
          elem->color_trans.vals[2][3] =
            (1 - elem->v.value_scale) * target_rgb[2];

          }


      aff3_apply (&elem->color_trans, 1.0, 0.0, 0.0,
                  &red_rgb[0], &red_rgb[1], &red_rgb[2]);
      aff3_apply (&elem->color_trans, 0.0, 1.0, 0.0,
                  &green_rgb[0], &green_rgb[1], &green_rgb[2]);
      aff3_apply (&elem->color_trans, 0.0, 0.0, 1.0,
                  &blue_rgb[0], &blue_rgb[1], &blue_rgb[2]);
      aff3_apply (&elem->color_trans, 0.0, 0.0, 0.0,
                  &black_rgb[0], &black_rgb[1], &black_rgb[2]);

      gegl_color_set_pixel (elem->v.red_color, babl_format ("R'G'B' double"), red_rgb);
      gegl_color_set_pixel (elem->v.green_color, babl_format ("R'G'B' double"), green_rgb);
      gegl_color_set_pixel (elem->v.blue_color, babl_format ("R'G'B' double"), blue_rgb);
      gegl_color_set_pixel (elem->v.black_color, babl_format ("R'G'B' double"), black_rgb);
    }
  else
    {
      elem->color_trans.vals[0][0] = red_rgb[0] - black_rgb[0];
      elem->color_trans.vals[1][0] = red_rgb[1] - black_rgb[1];
      elem->color_trans.vals[2][0] = red_rgb[2] - black_rgb [2];

      elem->color_trans.vals[0][1] = green_rgb[0] - black_rgb[0];
      elem->color_trans.vals[1][1] = green_rgb[1] - black_rgb[1];
      elem->color_trans.vals[2][1] = green_rgb[2] - black_rgb[2];

      elem->color_trans.vals[0][2] = blue_rgb[0] - black_rgb[0];
      elem->color_trans.vals[1][2] = blue_rgb[1] - black_rgb[1];
      elem->color_trans.vals[2][2] = blue_rgb[2] - black_rgb[2];

      elem->color_trans.vals[0][3] = black_rgb[0];
      elem->color_trans.vals[1][3] = black_rgb[1];
      elem->color_trans.vals[2][3] = black_rgb[2];
    }
}

void
aff_element_compute_trans (AffElement *elem,
                           gdouble     width,
                           gdouble     height,
                           gdouble     center_x,
                           gdouble     center_y)
{
  Aff2 t1, t2, t3;

  /* create the rotation, scaling and shearing part of the transform */
  aff2_distort (&t1, elem->v.asym, elem->v.shear);
  aff2_scale (&t2, elem->v.scale, elem->v.flip);
  aff2_compose (&t3, &t2, &t1);
  aff2_rotate (&t2, elem->v.theta);
  aff2_compose (&t1, &t2, &t3);

  /* now create the translational part */
  aff2_translate (&t2, -center_x*width, -center_y*width);
  aff2_compose (&t3, &t1, &t2);
  aff2_translate (&t2, elem->v.x*width, elem->v.y*width);
  aff2_compose (&elem->trans, &t2, &t3);
}

void
aff_element_decompose_trans (AffElement *elem,
                             Aff2       *aff,
                             gdouble     width,
                             gdouble     height,
                             gdouble     center_x,
                             gdouble     center_y)
{
  Aff2    t1, t2;
  gdouble det, scale, sign;

  /* pull of the translational parts */
  aff2_translate (&t1,center_x * width, center_y * width);
  aff2_compose (&t2, aff, &t1);

  elem->v.x = t2.b1 / width;
  elem->v.y = t2.b2 / width;

  det = t2.a11 * t2.a22 - t2.a12 * t2.a21;

  if (det == 0.0)
    {
      elem->v.scale = 0.0;
      elem->v.theta = 0.0;
      elem->v.asym  = 1.0;
      elem->v.shear = 0.0;
      elem->v.flip  = 0;
    }
  else
    {
      if (det >= 0)
        {
          scale = elem->v.scale = sqrt (det);
          sign = 1;
          elem->v.flip = 0;
        }
      else
        {
          scale = elem->v.scale = sqrt (-det);
          sign = -1;
          elem->v.flip = 1;
        }

      elem->v.theta = atan2 (-t2.a21, t2.a11);

      if (cos (elem->v.theta) == 0.0)
        {
          elem->v.asym = - t2.a21 / scale / sin (elem->v.theta);
          elem->v.shear = - sign * t2.a22 / scale / sin (elem->v.theta);
        }
      else
        {
          elem->v.asym = sign * t2.a11 / scale / cos (elem->v.theta);
          elem->v.shear = sign *
            (t2.a12/scale - sin (elem->v.theta)/elem->v.asym)
            / cos (elem->v.theta);
        }
    }
}

static void
aff_element_compute_click_boundary (AffElement *elem,
                                    int         num_elements,
                                    gdouble    *points_x,
                                    gdouble    *points_y)
{
  gint    i;
  gdouble xtot = 0;
  gdouble ytot = 0;
  gdouble xc, yc;
  gdouble theta;
  gdouble sth, cth;              /* sin(theta), cos(theta) */
  gdouble axis1, axis2;
  gdouble axis1max, axis2max, axis1min, axis2min;

  /* compute the center of mass of the points */
  for (i = 0; i < num_elements; i++)
    {
      xtot += points_x[i];
      ytot += points_y[i];
    }
  xc = xtot / num_elements;
  yc = ytot / num_elements;

  /* compute the sum of the (x+iy)^2, and take half the the resulting
     angle (xtot+iytot = A*exp(2i*theta)), to get an average direction */

  xtot = 0;
  ytot = 0;
  for (i = 0; i < num_elements; i++)
    {
      xtot += SQR (points_x[i] - xc) - SQR (points_y[i] - yc);
      ytot += 2 * (points_x[i] - xc) * (points_y[i] - yc);
    }
  theta = 0.5 * atan2 (ytot, xtot);
  sth = sin (theta);
  cth = cos (theta);

  /* compute the minimum rectangle at angle theta that bounds the points,
     1/2 side lengths left in axis1, axis2, center in xc, yc */

  axis1max = axis1min = 0.0;
  axis2max = axis2min = 0.0;
  for (i = 0; i < num_elements; i++)
    {
      gdouble proj1 =  (points_x[i] - xc) * cth + (points_y[i] - yc) * sth;
      gdouble proj2 = -(points_x[i] - xc) * sth + (points_y[i] - yc) * cth;
      if (proj1 < axis1min)
        axis1min = proj1;
      if (proj1 > axis1max)
        axis1max = proj1;
      if (proj2 < axis2min)
        axis2min = proj2;
      if (proj2 > axis2max)
        axis2max = proj2;
    }
  axis1 = 0.5 * (axis1max - axis1min);
  axis2 = 0.5 * (axis2max - axis2min);
  xc += 0.5 * ((axis1max + axis1min) * cth - (axis2max + axis2min) * sth);
  yc += 0.5 * ((axis1max + axis1min) * sth + (axis2max + axis2min) * cth);

  /* if the the rectangle is less than 10 pixels in any dimension,
     make it click_boundary, otherwise set click_boundary = draw_boundary */

  if (axis1 < 8.0 || axis2 < 8.0)
    {
      GdkPoint *points = g_new (GdkPoint, 4);

      elem->click_boundary = g_new (IPolygon, 1);
      elem->click_boundary->points = points;
      elem->click_boundary->npoints = 4;

      if (axis1 < 8.0) axis1 = 8.0;
      if (axis2 < 8.0) axis2 = 8.0;

      points[0].x = xc + axis1 * cth - axis2 * sth;
      points[0].y = yc + axis1 * sth + axis2 * cth;
      points[1].x = xc - axis1 * cth - axis2 * sth;
      points[1].y = yc - axis1 * sth + axis2 * cth;
      points[2].x = xc - axis1 * cth + axis2 * sth;
      points[2].y = yc - axis1 * sth - axis2 * cth;
      points[3].x = xc + axis1 * cth + axis2 * sth;
      points[3].y = yc + axis1 * sth - axis2 * cth;
    }
  else
    elem->click_boundary = elem->draw_boundary;
}

void
aff_element_compute_boundary (AffElement  *elem,
                              gint         width,
                              gint         height,
                              AffElement **elements,
                              gint         num_elements)
{
  gint      i;
  IPolygon  tmp_poly;
  gdouble  *points_x;
  gdouble  *points_y;

  if (elem->click_boundary && elem->click_boundary != elem->draw_boundary)
    g_free (elem->click_boundary);
  if (elem->draw_boundary)
    g_free (elem->draw_boundary);

  tmp_poly.npoints = num_elements;
  tmp_poly.points  = g_new (GdkPoint, num_elements);
  points_x = g_new (gdouble, num_elements);
  points_y = g_new (gdouble, num_elements);

  for (i = 0; i < num_elements; i++)
    {
      aff2_apply (&elem->trans,
                  elements[i]->v.x * width, elements[i]->v.y * width,
                  &points_x[i],&points_y[i]);
      tmp_poly.points[i].x = (gint)points_x[i];
      tmp_poly.points[i].y = (gint)points_y[i];
    }

  elem->draw_boundary = ipolygon_convex_hull (&tmp_poly);
  aff_element_compute_click_boundary (elem, num_elements, points_x, points_y);

  g_free (tmp_poly.points);
}

void
aff_element_draw (AffElement  *elem,
                  gboolean     selected,
                  gint         width,
                  gint         height,
                  cairo_t     *cr,
                  GdkRGBA     *color,
                  PangoLayout *layout)
{
  PangoRectangle rect;
  gint           i;

  pango_layout_set_text (layout, elem->name, -1);
  pango_layout_get_pixel_extents (layout, NULL, &rect);

  gdk_cairo_set_source_rgba (cr, color);

  cairo_move_to (cr,
                 elem->v.x * width - rect.width  / 2,
                 elem->v.y * width + rect.height / 2);
  pango_cairo_show_layout (cr, layout);
  cairo_fill (cr);

  cairo_set_line_width (cr, 1.0);

  if (elem->click_boundary != elem->draw_boundary)
    {
      cairo_move_to (cr,
                     elem->click_boundary->points[0].x,
                     elem->click_boundary->points[0].y);

      for (i = 1; i < elem->click_boundary->npoints; i++)
        cairo_line_to (cr,
                       elem->click_boundary->points[i].x,
                       elem->click_boundary->points[i].y);

      cairo_close_path (cr);

      cairo_stroke (cr);
    }

  if (selected)
    cairo_set_line_width (cr, 3.0);

  cairo_move_to (cr,
                 elem->draw_boundary->points[0].x,
                 elem->draw_boundary->points[0].y);

  for (i = 1; i < elem->draw_boundary->npoints; i++)
    cairo_line_to (cr,
                   elem->draw_boundary->points[i].x,
                   elem->draw_boundary->points[i].y);

  cairo_close_path (cr);

  cairo_stroke (cr);
}

AffElement *
aff_element_new (gdouble    x,
                 gdouble    y,
                 GeglColor *color,
                 gint       count)
{
  AffElement *elem = g_new (AffElement, 1);
  gchar buffer[16];

  elem->v.x = x;
  elem->v.y = y;
  elem->v.theta = 0.0;
  elem->v.scale = 0.5;
  elem->v.asym = 1.0;
  elem->v.shear = 0.0;
  elem->v.flip = 0;

  elem->v.red_color   = gegl_color_duplicate (color);
  elem->v.blue_color  = gegl_color_duplicate (color);
  elem->v.green_color = gegl_color_duplicate (color);
  elem->v.black_color = gegl_color_duplicate (color);

  elem->v.target_color = gegl_color_duplicate (color);
  elem->v.hue_scale = 0.5;
  elem->v.value_scale = 0.5;

  elem->v.simple_color = TRUE;

  elem->draw_boundary = NULL;
  elem->click_boundary = NULL;

  aff_element_compute_color_trans (elem);

  elem->v.prob = 1.0;

  sprintf (buffer,"%d", count);
  elem->name = g_strdup (buffer);

  return elem;
}

void
aff_element_free (AffElement *elem)
{
  if (elem->click_boundary != elem->draw_boundary)
    g_free (elem->click_boundary);

  g_clear_object (&elem->v.red_color);
  g_clear_object (&elem->v.blue_color);
  g_clear_object (&elem->v.green_color);
  g_clear_object (&elem->v.black_color);
  g_clear_object (&elem->v.target_color);

  g_free (elem->draw_boundary);
  g_free (elem);
}

#ifdef DEBUG_BRUSH
static brush_chars[] = {' ',':','*','@'};
#endif

static guchar *
create_brush (IfsComposeVals *ifsvals,
              gint           *brush_size,
              gdouble        *brush_offset)
{
  gint     i, j;
  gint     ii, jj;
  guchar  *brush;
#ifdef DEBUG_BRUSH
  gdouble  totpix = 0.0;
#endif

  gdouble radius = ifsvals->radius * ifsvals->subdivide;

  *brush_size = ceil (2 * radius);
  *brush_offset = 0.5 * (*brush_size - 1);

  brush = g_new (guchar, SQR (*brush_size));

  for (i = 0; i < *brush_size; i++)
    {
      for (j = 0; j < *brush_size; j++)
        {
          gdouble pixel = 0.0;
          gdouble d     = sqrt (SQR (i - *brush_offset) +
                                SQR (j - *brush_offset));

          if (d - 0.5 * G_SQRT2 > radius)
            pixel = 0.0;
          else if (d + 0.5 * G_SQRT2 < radius)
            pixel = 1.0;
          else
            for (ii = 0; ii < 10; ii++)
              for (jj = 0; jj < 10; jj++)
                {
                  d = sqrt (SQR (i - *brush_offset + ii * 0.1 - 0.45) +
                            SQR (j - *brush_offset + jj * 0.1 - 0.45));
                  pixel += (d < radius) / 100.0;
                }

          brush[i * *brush_size + j] = 255.999 * pixel;

#ifdef DEBUG_BRUSH
          putchar(brush_chars[(gint)(pixel * 3.999)]);
          totpix += pixel;
#endif /* DEBUG_BRUSH */
        }
#ifdef DEBUG_BRUSH
      putchar('\n');
#endif /* DEBUG_BRUSH */
    }
#ifdef DEBUG_BRUSH
  printf ("Brush total / area = %f\n", totpix / SQR (ifsvals->subdivide));
#endif /* DEBUG_BRUSH */
  return brush;
}

void
ifs_render (AffElement     **elements,
            gint             num_elements,
            gint             width,
            gint             height,
            gint             nsteps,
            IfsComposeVals  *vals,
            gint             band_y,
            gint             band_height,
            guchar          *data,
            guchar          *mask,
            guchar          *nhits,
            gboolean         preview)
{
  gint     i, k, n;
  gdouble  x, y;
  gdouble  r, g, b;
  gint     ri, gi, bi;
  guint32  p0, psum;
  gdouble  pt;
  guchar  *ptr;
  guint32 *prob;
  gdouble *fprob;
  gint     subdivide;
  guchar  *brush = NULL;
  gint     brush_size   = 1;
  gdouble  brush_offset = 0.0;

  if (preview)
    subdivide = 1;
  else
    subdivide = vals->subdivide;

  /* compute the probabilities and transforms */
  fprob = g_new (gdouble, num_elements);
  prob = g_new (guint32, num_elements);
  pt = 0.0;

  for (i = 0; i < num_elements; i++)
    {
      aff_element_compute_trans(elements[i],
                                width * subdivide,
                                height * subdivide,
                                vals->center_x,
                                vals->center_y);
      fprob[i] = fabs(
          elements[i]->trans.a11 * elements[i]->trans.a22
        - elements[i]->trans.a12 * elements[i]->trans.a21);

      /* As a heuristic, if the determinant is really small, it's
         probably a line element, so increase the probability so
         it gets rendered */

      /* FIXME: figure out what 0.01 really should be */
      if (fprob[i] < 0.01)
        fprob[i] = 0.01;

      fprob[i] *= elements[i]->v.prob;

      pt += fprob[i];
    }

  psum = 0;
  for (i = 0; i < num_elements; i++)
    {
      psum += (guint32) -1 * (fprob[i] / pt);
      prob[i] = psum;
    }

  prob[i - 1] = (guint32) -1;  /* make sure we don't get bitten by roundoff */

  /* create the brush */
  if (!preview)
    brush = create_brush (vals, &brush_size, &brush_offset);

  x = y = 0;
  r = g = b = 0;

  /* n is used to limit the number of progress updates */
  n = nsteps / 32;

  /* now run the iteration */
  for (i = 0; i < nsteps; i++)
    {
      if (!preview && ((i % n) == 0))
        gimp_progress_update ((gdouble) i / (gdouble) nsteps);

      p0 = g_random_int ();
      k = 0;

      while (p0 > prob[k])
        k++;

      aff2_apply (&elements[k]->trans, x, y, &x, &y);
      aff3_apply (&elements[k]->color_trans, r, g, b, &r, &g, &b);

      if (i < 50)
        continue;

      ri = (gint) (255.0 * r + 0.5);
      gi = (gint) (255.0 * g + 0.5);
      bi = (gint) (255.0 * b + 0.5);

      if ((ri < 0) || (ri > 255) ||
          (gi < 0) || (gi > 255) ||
          (bi < 0) || (bi > 255))
        continue;

      if (preview)
        {
          if ((x < width) && (y < (band_y + band_height)) &&
              (x >= 0) && (y >= band_y))
            {
              ptr = data + 3 * (((gint) (y - band_y)) * width + (gint) x);

              *ptr++ = ri;
              *ptr++ = gi;
              *ptr   = bi;
            }
        }
      else
        {
          if ((x < width * subdivide) && (y < height * subdivide) &&
              (x >= 0) && (y >= 0))
            {
              gint ii;
              gint jj;
              gint jj0   = floor (y - brush_offset - band_y * subdivide);
              gint ii0   = floor (x - brush_offset);
              gint jjmin = 0;
              gint iimin = 0;
              gint jjmax;
              gint iimax;

              if (ii0 < 0)
                iimin = - ii0;
              else
                iimin = 0;

              if (jj0 < 0)
                jjmin = - jj0;
              else
                jjmin = 0;

              if (jj0 + brush_size >= subdivide * band_height)
                jjmax = subdivide * band_height - jj0;
              else
                jjmax = brush_size;

              if (ii0 + brush_size >= subdivide * width)
                iimax = subdivide * width - ii0;
              else
                iimax = brush_size;

              for (jj = jjmin; jj < jjmax; jj++)
                for (ii = iimin; ii < iimax; ii++)
                  {
                    guint m_old;
                    guint m_new;
                    guint m_pix;
                    guint n_hits;
                    guint old_scale;
                    guint pix_scale;
                    gint  index = (jj0 + jj) * width * subdivide + ii0 + ii;

                    n_hits = nhits[index];
                    if (n_hits == 255)
                      continue;

                    m_pix = brush[jj * brush_size + ii];
                    if (!m_pix)
                      continue;

                    nhits[index] = ++n_hits;
                    m_old = mask[index];
                    m_new = m_old + m_pix - m_old * m_pix / 255;
                    mask[index] = m_new;

                    /* relative probability that old colored pixel is on top */
                    old_scale = m_old * (255 * n_hits - m_pix);

                    /* relative probability that new colored pixel is on top */
                    pix_scale = m_pix * ((255 - m_old) * n_hits + m_old);

                    ptr = data + 3 * index;

                    *ptr = ((old_scale * (*ptr) + pix_scale * ri) /
                            (old_scale + pix_scale));
                    ptr++;

                    *ptr = ((old_scale * (*ptr) + pix_scale * gi) /
                            (old_scale + pix_scale));
                    ptr++;

                    *ptr = ((old_scale * (*ptr) + pix_scale * bi) /
                            (old_scale + pix_scale));
                  }
            }
        }
    } /* main iteration */

  if (!preview )
    gimp_progress_update (1.0);

  g_free (brush);
  g_free (prob);
  g_free (fprob);
}
