/* blob.c: routines for manipulating scan converted convex
 *         polygons.
 *  
 * Copyright 1998-1999, Owen Taylor <otaylor@gtk.org>
 *
 * > Please contact the above author before modifying the copy <
 * > of this file in the GIMP distribution. Thanks.            <
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

#include "blob.h"
#include <glib.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ROUND(A) floor((A)+0.5)

static Blob *
blob_new (int y, int height)
{
  Blob *result;

  result = g_malloc (sizeof (Blob) +  sizeof(BlobSpan) * (height-1));
  result->y = y;
  result->height = height;

  return result;
}

typedef enum {
  EDGE_NONE = 0,
  EDGE_LEFT = 1 << 0,
  EDGE_RIGHT = 1 << 1
} EdgeType;

static void
blob_fill (Blob *b, EdgeType *present)
{
  int start;
  int y, x1, x2, y1, y2, i1, i2;
  int i, j;

  /* Mark empty lines at top and bottom as unused */

  start = 0;
  while (!(present[start])) 
    {
      b->data[start].left = 0;
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

  for (i=b->height-1;!present[i];i--)
    {
      b->data[i].left = 0;
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

  /* We fill only interior regions of convex hull, as if we were filling
     polygons. But since we draw ellipses with nearest points, not interior
     points, maybe it would look better if we did the same here. Probably
     not a big deal either way after anti-aliasing */

  /*     left edge */
  for (i1=start; i1<b->height-2; i1++)
    {
      /* Find empty gaps */
      if (!(present[i1+1] & EDGE_LEFT))
	{
	  int increment;	/* fractional part */
	  int denom;		/* denominator of fraction */
	  int step;		/* integral step */
	  int frac;		/* fractional step */
	  int reverse;

	  /* find bottom of gap */
	  i2 = i1+2;
	  while (!(present[i2] & EDGE_LEFT) && i2 < b->height) i2++;
	  
	  if (i2 < b->height)
	    {
	      denom = i2-i1;
	      x1 = b->data[i1].left;
	      x2 = b->data[i2].left;
	      step = (x2-x1)/denom;
	      frac = x2-x1 - step*denom;
	      if (frac < 0)
		{
		  frac = -frac;
		  reverse = 1;
		}
	      else
		reverse = 0;
	      
	      increment = 0;
	      for (i=i1+1; i<i2; i++)
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
	  i1 = i2-1;		/* advance to next possibility */
	}
    }

  /*     right edge */
  for (i1=start; i1<b->height-2; i1++)
    {
      /* Find empty gaps */
      if (!(present[i1+1] & EDGE_RIGHT))
	{
	  int increment;	/* fractional part */
	  int denom;		/* denominator of fraction */
	  int step;		/* integral step */
	  int frac;		/* fractional step */
	  int reverse;

	  /* find bottom of gap */
	  i2 = i1+2;
	  while (!(present[i2] & EDGE_RIGHT) && i2 < b->height) i2++;
	  
	  if (i2 < b->height)
	    {
	      denom = i2-i1;
	      x1 = b->data[i1].right;
	      x2 = b->data[i2].right;
	      step = (x2-x1)/denom;
	      frac = x2-x1 - step*denom;
	      if (frac < 0)
		{
		  frac = -frac;
		  reverse = 1;
		}
	      else
		reverse = 0;
	      
	      increment = 0;
	      for (i=i1+1; i<i2; i++)
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
	  i1 = i2-1;		/* advance to next possibility */
	}
    }

}

static void
blob_make_convex (Blob *b, EdgeType *present)
{
  int y, x1, x2, y1, y2, i1, i2;
  int i, j;
  int start;

  /* Walk through edges, deleting points that aren't on convex hull */

  start = 0;
  while (!(present[start])) start++;

  /*    left edge */

  i1 = start-1; 
  i2 = start;
  x1 = b->data[start].left - b->data[start].right;
  y1 = 0;
  
  for (i=start+1;i<b->height;i++)
    {
      if (!(present[i] & EDGE_LEFT))
	continue;

      x2 = b->data[i].left - b->data[i2].left;
      y2 = i-i2;
      
      while (x2*y1 - x1*y2 < 0) /* clockwise rotation */
	{
	  present[i2] &= ~EDGE_LEFT;
	  i2 = i1;
	  while (!(present[--i1] & EDGE_LEFT) && i1>=start);

	  if (i1<start)
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
  
  for (i=start+1;i<b->height;i++)
    {
      if (!(present[i] & EDGE_RIGHT))
	continue;

      x2 = b->data[i].right - b->data[i2].right;
      y2 = i-i2;
      
      while (x2*y1 - x1*y2 > 0) /* counter-clockwise rotation */
	{
	  present[i2] &= ~EDGE_RIGHT;
	  i2 = i1;
	  while (!(present[--i1] & EDGE_RIGHT) && i1>=start);

	  if (i1<start)
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

  blob_fill (b, present);
}

Blob *
blob_convex_union (Blob *b1, Blob *b2)
{
  Blob *result;
  int y, x1, x2, y1, y2, i1, i2;
  int i, j;
  int start;
  EdgeType *present;

  /* Create the storage for the result */

  y = MIN(b1->y,b2->y);
  result = blob_new (y, MAX(b1->y+b1->height,b2->y+b2->height)-y);

  if (result->height == 0)
    return result;

  present = g_new0 (EdgeType, result->height);

  /* Initialize spans from original objects */

  for (i=0, j=b1->y-y; i<b1->height; i++,j++)
    {
      if (b1->data[i].right >= b1->data[i].left)
	{
	  present[j] = EDGE_LEFT | EDGE_RIGHT;
	  result->data[j].left = b1->data[i].left;
	  result->data[j].right = b1->data[i].right;
	}
    }

  for (i=0, j=b2->y-y; i<b2->height; i++,j++)
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
  
  blob_make_convex (result, present);

  g_free (present);
  return result;
}

static void
blob_line_add_pixel (Blob *b, int x, int y)
{
  if (b->data[y-b->y].left > b->data[y-b->y].right)
    b->data[y-b->y].left = b->data[y-b->y].right = x;
  else
    {
      b->data[y-b->y].left = MIN (b->data[y-b->y].left, x);
      b->data[y-b->y].right = MAX (b->data[y-b->y].right, x);
    }
}

void
blob_line (Blob *b, int x0, int y0, int x1, int y1)
{
  int dx, dy, d;
  int incrE, incrNE;
  int x, y;

  int xstep = 1;
  int ystep = 1;
  
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
      d = 2*dy - dx;		/* initial value of d */
      incrE = 2 * dy;		/* increment used for move to E */
      incrNE = 2 * (dy-dx);		/* increment used for move to NE */
      
      blob_line_add_pixel (b, x, y);
      
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
	  blob_line_add_pixel (b, x, y);
	}
    }
  else
    {
      d = 2*dx - dy;		/* initial value of d */
      incrE = 2 * dx;		/* increment used for move to E */
      incrNE = 2 * (dx-dy);		/* increment used for move to NE */
      
      blob_line_add_pixel (b, x, y);
      
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
	  blob_line_add_pixel (b, x, y);
	}
    }
}

#define TABLE_SIZE 256

#define ELLIPSE_SHIFT 2
#define TABLE_SHIFT 14
#define TOTAL_SHIFT ELLIPSE_SHIFT + TABLE_SHIFT

static int trig_initialized = 0;
static int trig_table[TABLE_SIZE];

/* Scan convert an ellipse specified by _offsets_ of major and
   minor axes, and by center into a blob */
Blob *
blob_ellipse (double xc, double yc, double xp, double yp, double xq, double yq)
{
  int i;
  Blob *r;
  gdouble r1, r2;
  gint maxy, miny;
  gint step;
  double max_radius;

  gint xc_shift, yc_shift;
  gint xp_shift, yp_shift;
  gint xq_shift, yq_shift;

  EdgeType *present;
  
  if (!trig_initialized)
    {
      trig_initialized = 1;
      for (i=0; i<256; i++)
	trig_table[i] = 0.5 + sin(i * (M_PI / 128.)) * (1 << TABLE_SHIFT);
    }

  /* Make sure we traverse ellipse in ccw direction */

  if (xp * yq - yq * xp < 0)
    {
      xq = -xq;
      yq = -yq;

    }

  /* Compute bounds as if we were drawing a rectangle */

  maxy = ceil (yc + fabs (yp) + fabs(yq));
  miny = floor (yc - fabs (yp) - fabs(yq));

  r = blob_new (miny, maxy - miny + 1);
  present = g_new0 (EdgeType, r->height);

  /* Figure out a step that will draw most of the points */

  r1 = sqrt (xp * xp + yp * yp);
  r2 = sqrt (xp * xp + yp * yp);
  max_radius = MAX (r1, r2);
  step = TABLE_SIZE;

  while (step > 1 && (TABLE_SIZE / step < 4*max_radius))
    step >>= 1;

  /* Fill in the edge points */

  xc_shift = 0.5 + xc * (1 << TOTAL_SHIFT);
  yc_shift = 0.5 + yc * (1 << TOTAL_SHIFT);
  xp_shift = 0.5 + xp * (1 << ELLIPSE_SHIFT);
  yp_shift = 0.5 + yp * (1 << ELLIPSE_SHIFT);
  xq_shift = 0.5 + xq * (1 << ELLIPSE_SHIFT);
  yq_shift = 0.5 + yq * (1 << ELLIPSE_SHIFT);

  for (i = 0 ; i < TABLE_SIZE ; i += step)
    {
      gint s = trig_table[i];
      gint c = trig_table[(TABLE_SIZE + TABLE_SIZE/4 - i) % TABLE_SIZE];
      
      gint x = (xc_shift + c * xp_shift + s * xq_shift +
		(1 << (TOTAL_SHIFT - 1))) >> TOTAL_SHIFT;
      gint y = ((yc_shift + c * yp_shift + s * yq_shift +
		(1 << (TOTAL_SHIFT - 1))) >> TOTAL_SHIFT) - r->y;

      gint dydi = c * yq_shift  - s * yp_shift;

      if (dydi <= 0) /* left edge */
	{
	  if (present[y] & EDGE_LEFT)
	    {
	      r->data[y].left = MIN (r->data[y].left, x);
	    }
	  else
	    {
	      present[y] |= EDGE_LEFT;
	      r->data[y].left = x;
	    }
	}
      
      if (dydi >= 0) /* right edge */
	{
	  if (present[y] & EDGE_RIGHT)
	    {
	      r->data[y].right = MAX (r->data[y].right, x);
	    }
	  else
	    {
	      present[y] |= EDGE_RIGHT;
	      r->data[y].right = x;
	    }
	}
    }

  /* Now fill in missing points */

  blob_fill (r, present);
  g_free (present);

  return r;
}

void
blob_bounds(Blob *b, int *x, int *y, int *width, int *height)
{
  int i;
  int x0, x1, y0, y1;

  i = 0;
  while (i<b->height && b->data[i].left > b->data[i].right)
    i++;

  if (i<b->height)
    {
      y0 = b->y + i;
      x0 = b->data[i].left;
      x1 = b->data[i].right + 1;
      while (i<b->height && b->data[i].left <= b->data[i].right)
	{
	  x0 = MIN(b->data[i].left, x0);
	  x1 = MAX(b->data[i].right+1, x1);
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

void
blob_dump(Blob *b) {
  int i,j;
  for (i=0; i<b->height; i++)
    {
      for (j=0;j<b->data[i].left;j++)
	putchar(' ');
      for (j=b->data[i].left;j<=b->data[i].right;j++)
	putchar('*');
      putchar('\n');
    }
}
