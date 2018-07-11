/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * gimpink-blob.c: routines for manipulating scan converted convex polygons.
 * Copyright 1998-1999, Owen Taylor <otaylor@gtk.org>
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

#include <glib-object.h>

#include "libgimpmath/gimpmath.h"

#include "paint-types.h"

#include "gimpink-blob.h"


typedef enum
{
  EDGE_NONE  = 0,
  EDGE_LEFT  = 1 << 0,
  EDGE_RIGHT = 1 << 1
} EdgeType;


/*  local function prototypes  */

static GimpBlob * gimp_blob_new            (gint      y,
                                            gint      height);
static void       gimp_blob_fill           (GimpBlob *b,
                                            EdgeType *present);
static void       gimp_blob_make_convex    (GimpBlob *b,
                                            EdgeType *present);

#if 0
static void       gimp_blob_line_add_pixel (GimpBlob *b,
                                            gint      x,
                                            gint      y);
static void       gimp_blob_line           (GimpBlob *b,
                                            gint      x0,
                                            gint      y0,
                                            gint      x1,
                                            gint      y1);
#endif


/*  public functions  */

/* Return blob for the given (convex) polygon
 */
GimpBlob *
gimp_blob_polygon (GimpBlobPoint *points,
                   gint           n_points)
{
  GimpBlob *result;
  EdgeType *present;
  gint      i;
  gint      im1;
  gint      ip1;
  gint      ymin, ymax;

  ymax = points[0].y;
  ymin = points[0].y;

  for (i = 1; i < n_points; i++)
    {
      if (points[i].y > ymax)
        ymax = points[i].y;
      if (points[i].y < ymin)
        ymin = points[i].y;
    }

  result = gimp_blob_new (ymin, ymax - ymin + 1);
  present = g_new0 (EdgeType, result->height);

  im1 = n_points - 1;
  i = 0;
  ip1 = 1;

  for (; i < n_points ; i++)
    {
      gint sides = 0;
      gint j     = points[i].y - ymin;

      if (points[i].y < points[im1].y)
        sides |= EDGE_RIGHT;
      else if (points[i].y > points[im1].y)
        sides |= EDGE_LEFT;

      if (points[ip1].y < points[i].y)
        sides |= EDGE_RIGHT;
      else if (points[ip1].y > points[i].y)
        sides |= EDGE_LEFT;

      if (sides & EDGE_RIGHT)
        {
          if (present[j] & EDGE_RIGHT)
            {
              result->data[j].right = MAX (result->data[j].right, points[i].x);
            }
          else
            {
              present[j] |= EDGE_RIGHT;
              result->data[j].right = points[i].x;
            }
        }

      if (sides & EDGE_LEFT)
        {
          if (present[j] & EDGE_LEFT)
            {
              result->data[j].left = MIN (result->data[j].left, points[i].x);
            }
          else
            {
              present[j] |= EDGE_LEFT;
              result->data[j].left = points[i].x;
            }
        }

      im1 = i;
      ip1++;
      if (ip1 == n_points)
        ip1 = 0;
    }

  gimp_blob_fill (result, present);
  g_free (present);

  return result;
}

/* Scan convert a square specified by _offsets_ of major and minor
 * axes, and by center into a blob
 */
GimpBlob *
gimp_blob_square (gdouble xc,
                  gdouble yc,
                  gdouble xp,
                  gdouble yp,
                  gdouble xq,
                  gdouble yq)
{
  GimpBlobPoint points[4];

  /* Make sure we order points ccw */

  if (xp * yq - xq * yp < 0)
    {
      xq = -xq;
      yq = -yq;
    }

  points[0].x = xc + xp + xq;
  points[0].y = yc + yp + yq;
  points[1].x = xc + xp - xq;
  points[1].y = yc + yp - yq;
  points[2].x = xc - xp - xq;
  points[2].y = yc - yp - yq;
  points[3].x = xc - xp + xq;
  points[3].y = yc - yp + yq;

  return gimp_blob_polygon (points, 4);
}

/* Scan convert a diamond specified by _offsets_ of major and minor
 * axes, and by center into a blob
 */
GimpBlob *
gimp_blob_diamond (gdouble xc,
                   gdouble yc,
                   gdouble xp,
                   gdouble yp,
                   gdouble xq,
                   gdouble yq)
{
  GimpBlobPoint points[4];

  /* Make sure we order points ccw */

  if (xp * yq - xq * yp < 0)
    {
      xq = -xq;
      yq = -yq;
    }

  points[0].x = xc + xp;
  points[0].y = yc + yp;
  points[1].x = xc - xq;
  points[1].y = yc - yq;
  points[2].x = xc - xp;
  points[2].y = yc - yp;
  points[3].x = xc + xq;
  points[3].y = yc + yq;

  return gimp_blob_polygon (points, 4);
}


#define TABLE_SIZE 256

#define ELLIPSE_SHIFT 2
#define TABLE_SHIFT   12
#define TOTAL_SHIFT   (ELLIPSE_SHIFT + TABLE_SHIFT)

/*
 * The choose of this values limits the maximal image_size to
 * 16384 x 16384 pixels. The values will overflow as soon as
 * x or y > INT_MAX / (1 << (ELLIPSE_SHIFT + TABLE_SHIFT)) / SUBSAMPLE
 *
 * Alternatively the code could be change the code as follows:
 *
 *   xc_base = floor (xc)
 *   xc_shift = 0.5 + (xc - xc_base) * (1 << TOTAL_SHIFT);
 *
 *    gint x = xc_base + (xc_shift + c * xp_shift + s * xq_shift +
 *             (1 << (TOTAL_SHIFT - 1))) >> TOTAL_SHIFT;
 *
 * which would change the limit from the image to the ellipse size
 *
 * Update: this change was done, and now there apparently is a limit
 * on the ellipse size. I'm too lazy to fully understand what's going
 * on here and simply leave this comment here for
 * documentation. --Mitch
 */

static gboolean trig_initialized = FALSE;
static gint     trig_table[TABLE_SIZE];

/* Scan convert an ellipse specified by _offsets_ of major and
 * minor axes, and by center into a blob
 */
GimpBlob *
gimp_blob_ellipse (gdouble xc,
                   gdouble yc,
                   gdouble xp,
                   gdouble yp,
                   gdouble xq,
                   gdouble yq)
{
  GimpBlob *result;
  EdgeType *present;
  gint      i;
  gdouble   r1, r2;
  gint      maxy, miny;
  gint      step;
  gdouble   max_radius;

  gint      xc_shift, yc_shift;
  gint      xp_shift, yp_shift;
  gint      xq_shift, yq_shift;
  gint      xc_base, yc_base;

  if (! trig_initialized)
    {
      trig_initialized = TRUE;

      for (i = 0; i < 256; i++)
        trig_table[i] = 0.5 + sin (i * (G_PI / 128.0)) * (1 << TABLE_SHIFT);
    }

  /* Make sure we traverse ellipse in ccw direction */

  if (xp * yq - xq * yp < 0)
    {
      xq = -xq;
      yq = -yq;
    }

  /* Compute bounds as if we were drawing a rectangle */

  maxy = ceil  (yc + fabs (yp) + fabs (yq));
  miny = floor (yc - fabs (yp) - fabs (yq));

  result = gimp_blob_new (miny, maxy - miny + 1);
  present = g_new0 (EdgeType, result->height);

  xc_base = floor (xc);
  yc_base = floor (yc);

  /* Figure out a step that will draw most of the points */

  r1 = sqrt (xp * xp + yp * yp);
  r2 = sqrt (xq * xq + yq * yq);
  max_radius = MAX (r1, r2);
  step = TABLE_SIZE;

  while (step > 1 && (TABLE_SIZE / step < 4 * max_radius))
    step >>= 1;

  /* Fill in the edge points */

  xc_shift = 0.5 + (xc - xc_base) * (1 << TOTAL_SHIFT);
  yc_shift = 0.5 + (yc - yc_base) * (1 << TOTAL_SHIFT);
  xp_shift = 0.5 + xp * (1 << ELLIPSE_SHIFT);
  yp_shift = 0.5 + yp * (1 << ELLIPSE_SHIFT);
  xq_shift = 0.5 + xq * (1 << ELLIPSE_SHIFT);
  yq_shift = 0.5 + yq * (1 << ELLIPSE_SHIFT);

  for (i = 0 ; i < TABLE_SIZE ; i += step)
    {
      gint s = trig_table[i];
      gint c = trig_table[(TABLE_SIZE + TABLE_SIZE / 4 - i) % TABLE_SIZE];

      gint x = ((xc_shift + c * xp_shift + s * xq_shift +
                 (1 << (TOTAL_SHIFT - 1))) >> TOTAL_SHIFT) + xc_base;
      gint y = (((yc_shift + c * yp_shift + s * yq_shift +
                 (1 << (TOTAL_SHIFT - 1))) >> TOTAL_SHIFT)) + yc_base
                  - result->y;

      gint dydi = c * yq_shift  - s * yp_shift;

      if (dydi <= 0) /* left edge */
        {
          if (present[y] & EDGE_LEFT)
            {
              result->data[y].left = MIN (result->data[y].left, x);
            }
          else
            {
              present[y] |= EDGE_LEFT;
              result->data[y].left = x;
            }
        }

      if (dydi >= 0) /* right edge */
        {
          if (present[y] & EDGE_RIGHT)
            {
              result->data[y].right = MAX (result->data[y].right, x);
            }
          else
            {
              present[y] |= EDGE_RIGHT;
              result->data[y].right = x;
            }
        }
    }

  /* Now fill in missing points */

  gimp_blob_fill (result, present);
  g_free (present);

  return result;
}

void
gimp_blob_bounds (GimpBlob *b,
                  gint     *x,
                  gint     *y,
                  gint     *width,
                  gint     *height)
{
  gint i;
  gint x0, x1, y0, y1;

  i = 0;
  while (i < b->height && b->data[i].left > b->data[i].right)
    i++;

  if (i < b->height)
    {
      y0 = b->y + i;
      x0 = b->data[i].left;
      x1 = b->data[i].right + 1;

      while (i < b->height && b->data[i].left <= b->data[i].right)
        {
          x0 = MIN (b->data[i].left,      x0);
          x1 = MAX (b->data[i].right + 1, x1);
          i++;
        }

      y1 = b->y + i;
    }
  else
    {
      x0 = y0 = 0;
      x1 = y1 = 0;
    }

  *x = x0;
  *y = y0;
  *width = x1 - x0;
  *height = y1 - y0;
}

GimpBlob *
gimp_blob_convex_union (GimpBlob *b1,
                        GimpBlob *b2)
{
  GimpBlob *result;
  gint      y;
  gint      i, j;
  EdgeType *present;

  /* Create the storage for the result */

  y = MIN (b1->y, b2->y);
  result = gimp_blob_new (y, MAX (b1->y + b1->height, b2->y + b2->height)-y);

  if (result->height == 0)
    return result;

  present = g_new0 (EdgeType, result->height);

  /* Initialize spans from original objects */

  for (i = 0, j = b1->y-y; i < b1->height; i++, j++)
    {
      if (b1->data[i].right >= b1->data[i].left)
        {
          present[j] = EDGE_LEFT | EDGE_RIGHT;
          result->data[j].left  = b1->data[i].left;
          result->data[j].right = b1->data[i].right;
        }
    }

  for (i = 0, j = b2->y - y; i < b2->height; i++, j++)
    {
      if (b2->data[i].right >= b2->data[i].left)
        {
          if (present[j])
            {
              if (result->data[j].left > b2->data[i].left)
                result->data[j].left = b2->data[i].left;
              if (result->data[j].right < b2->data[i].right)
                result->data[j].right = b2->data[i].right;
            }
          else
            {
              present[j] = EDGE_LEFT | EDGE_RIGHT;
              result->data[j].left = b2->data[i].left;
              result->data[j].right = b2->data[i].right;
            }
        }
    }

  gimp_blob_make_convex (result, present);

  g_free (present);

  return result;
}

GimpBlob *
gimp_blob_duplicate (GimpBlob *b)
{
  g_return_val_if_fail (b != NULL, NULL);

  return g_memdup (b, sizeof (GimpBlob) +  sizeof (GimpBlobSpan) * (b->height - 1));
}

#if 0
void
gimp_blob_dump (GimpBlob *b)
{
  gint i,j;

  for (i = 0; i < b->height; i++)
    {
      for (j = 0; j < b->data[i].left; j++)
        putchar (' ');

      for (j = b->data[i].left; j <= b->data[i].right; j++)
        putchar ('*');

      putchar ('\n');
    }
}
#endif


/*  private functions  */

static GimpBlob *
gimp_blob_new (gint y,
               gint height)
{
  GimpBlob *result;

  result = g_malloc (sizeof (GimpBlob) +  sizeof (GimpBlobSpan) * (height - 1));

  result->y      = y;
  result->height = height;

  return result;
}

static void
gimp_blob_fill (GimpBlob *b,
                EdgeType *present)
{
  gint start;
  gint x1, x2, i1, i2;
  gint i;

  /* Mark empty lines at top and bottom as unused */

  start = 0;
  while (! present[start])
    {
      b->data[start].left  = 0;
      b->data[start].right = -1;
      start++;
    }

  if (present[start] != (EDGE_RIGHT | EDGE_LEFT))
    {
      if (present[start] == EDGE_RIGHT)
        b->data[start].left = b->data[start].right;
      else
        b->data[start].right = b->data[start].left;

      present[start] = EDGE_RIGHT | EDGE_LEFT;
    }

  for (i = b->height - 1; ! present[i]; i--)
    {
      b->data[i].left  = 0;
      b->data[i].right = -1;
    }

  if (present[i] != (EDGE_RIGHT | EDGE_LEFT))
    {
      if (present[i] == EDGE_RIGHT)
        b->data[i].left = b->data[i].right;
      else
        b->data[i].right = b->data[i].left;

      present[i] = EDGE_RIGHT | EDGE_LEFT;
    }


  /* Restore missing edges  */

  /* We fill only interior regions of convex hull, as if we were
   * filling polygons. But since we draw ellipses with nearest points,
   * not interior points, maybe it would look better if we did the
   * same here. Probably not a big deal either way after anti-aliasing
   */

  /* left edge */
  for (i1 = start; i1 < b->height - 2; i1++)
    {
      /* Find empty gaps */
      if (! (present[i1 + 1] & EDGE_LEFT))
        {
          gint increment; /* fractional part */
          gint denom;     /* denominator of fraction */
          gint step;      /* integral step */
          gint frac;      /* fractional step */
          gint reverse;

          /* find bottom of gap */
          i2 = i1 + 2;
          while (i2 < b->height && ! (present[i2] & EDGE_LEFT))
            i2++;

          if (i2 < b->height)
            {
              denom = i2 - i1;
              x1 = b->data[i1].left;
              x2 = b->data[i2].left;
              step = (x2 - x1) / denom;
              frac = x2 - x1 - step * denom;
              if (frac < 0)
                {
                  frac = -frac;
                  reverse = 1;
                }
              else
                reverse = 0;

              increment = 0;
              for (i = i1 + 1; i < i2; i++)
                {
                  x1 += step;
                  increment += frac;
                  if (increment >= denom)
                    {
                      increment -= denom;
                      x1 += reverse ? -1 : 1;
                    }
                  if (increment == 0 || reverse)
                    b->data[i].left = x1;
                  else
                    b->data[i].left = x1 + 1;
                }
            }

          i1 = i2 - 1;        /* advance to next possibility */
        }
    }

  /* right edge */
  for (i1 = start; i1 < b->height - 2; i1++)
    {
      /* Find empty gaps */
      if (! (present[i1 + 1] & EDGE_RIGHT))
        {
          gint increment; /* fractional part */
          gint denom;     /* denominator of fraction */
          gint step;      /* integral step */
          gint frac;      /* fractional step */
          gint reverse;

          /* find bottom of gap */
          i2 = i1 + 2;
          while (i2 < b->height && ! (present[i2] & EDGE_RIGHT))
            i2++;

          if (i2 < b->height)
            {
              denom = i2 - i1;
              x1 = b->data[i1].right;
              x2 = b->data[i2].right;
              step = (x2 - x1) / denom;
              frac = x2 - x1 - step * denom;
              if (frac < 0)
                {
                  frac = -frac;
                  reverse = 1;
                }
              else
                reverse = 0;

              increment = 0;
              for (i = i1 + 1; i<i2; i++)
                {
                  x1 += step;
                  increment += frac;
                  if (increment >= denom)
                    {
                      increment -= denom;
                      x1 += reverse ? -1 : 1;
                    }
                  if (reverse && increment != 0)
                    b->data[i].right = x1 - 1;
                  else
                    b->data[i].right = x1;
                }
            }

          i1 = i2 - 1;        /* advance to next possibility */
        }
    }

}

static void
gimp_blob_make_convex (GimpBlob *b,
                       EdgeType *present)
{
  gint x1, x2, y1, y2, i1, i2;
  gint i;
  gint start;

  /* Walk through edges, deleting points that aren't on convex hull */

  start = 0;
  while (! present[start])
    start++;

  /*    left edge */

  i1 = start - 1;
  i2 = start;
  x1 = b->data[start].left - b->data[start].right;
  y1 = 0;

  for (i = start + 1; i < b->height; i++)
    {
      if (! (present[i] & EDGE_LEFT))
        continue;

      x2 = b->data[i].left - b->data[i2].left;
      y2 = i - i2;

      while (x2 * y1 - x1 * y2 < 0) /* clockwise rotation */
        {
          present[i2] &= ~EDGE_LEFT;
          i2 = i1;
          while ((--i1) >= start && (! (present[i1] & EDGE_LEFT)));

          if (i1 < start)
            {
              x1 = b->data[start].left - b->data[start].right;
              y1 = 0;
            }
          else
            {
              x1 = b->data[i2].left - b->data[i1].left;
              y1 = i2 - i1;
            }
          x2 = b->data[i].left - b->data[i2].left;
          y2 = i - i2;
        }

      x1 = x2;
      y1 = y2;
      i1 = i2;
      i2 = i;
    }

  /*     Right edge */

  i1 = start -1;
  i2 = start;
  x1 = b->data[start].right - b->data[start].left;
  y1 = 0;

  for (i = start + 1; i < b->height; i++)
    {
      if (! (present[i] & EDGE_RIGHT))
        continue;

      x2 = b->data[i].right - b->data[i2].right;
      y2 = i - i2;

      while (x2 * y1 - x1 * y2 > 0) /* counter-clockwise rotation */
        {
          present[i2] &= ~EDGE_RIGHT;
          i2 = i1;
          while ((--i1) >= start && (! (present[i1] & EDGE_RIGHT)));

          if (i1 < start)
            {
              x1 = b->data[start].right - b->data[start].left;
              y1 = 0;
            }
          else
            {
              x1 = b->data[i2].right - b->data[i1].right;
              y1 = i2 - i1;
            }

          x2 = b->data[i].right - b->data[i2].right;
          y2 = i - i2;
        }

      x1 = x2;
      y1 = y2;
      i1 = i2;
      i2 = i;
    }

  gimp_blob_fill (b, present);
}


#if 0

static void
gimp_blob_line_add_pixel (GimpBlob *b,
                          gint      x,
                          gint      y)
{
  if (b->data[y - b->y].left > b->data[y - b->y].right)
    {
      b->data[y - b->y].left = b->data[y - b->y].right = x;
    }
  else
    {
      b->data[y - b->y].left  = MIN (b->data[y - b->y].left,  x);
      b->data[y - b->y].right = MAX (b->data[y - b->y].right, x);
    }
}

static void
gimp_blob_line (GimpBlob *b,
                gint      x0,
                gint      y0,
                gint      x1,
                gint      y1)
{
  gint dx, dy, d;
  gint incrE, incrNE;
  gint x, y;

  gint xstep = 1;
  gint ystep = 1;

  dx = x1 - x0;
  dy = y1 - y0;

  if (dx < 0)
    {
      dx = -dx;
      xstep = -1;
    }

  if (dy < 0)
    {
      dy = -dy;
      ystep = -1;
    }

  /*  for (y = y0; y != y1 + ystep ; y += ystep)
    {
      b->data[y-b->y].left = 0;
      b->data[y-b->y].right = -1;
      }*/

  x = x0;
  y = y0;

  if (dy < dx)
    {
      d = 2 * dy - dx;        /* initial value of d */
      incrE  = 2 * dy;        /* increment used for move to E */
      incrNE = 2 * (dy - dx); /* increment used for move to NE */

      gimp_blob_line_add_pixel (b, x, y);

      while (x != x1)
        {
          if (d <= 0)
            {
              d += incrE;
              x += xstep;
            }
          else
            {
              d += incrNE;
              x += xstep;
              y += ystep;
            }

          gimp_blob_line_add_pixel (b, x, y);
        }
    }
  else
    {
      d = 2 * dx - dy;        /* initial value of d */
      incrE  = 2 * dx;        /* increment used for move to E */
      incrNE = 2 * (dx - dy); /* increment used for move to NE */

      gimp_blob_line_add_pixel (b, x, y);

      while (y != y1)
        {
          if (d <= 0)
            {
              d += incrE;
              y += ystep;
            }
          else
            {
              d += incrNE;
              x += xstep;
              y += ystep;
            }

          gimp_blob_line_add_pixel (b, x, y);
        }
    }
}

#endif
