/* blob.c: routines for manipulating scan converted convex
 *         polygons.
 *  
 * Copyright 1998, Owen Taylor <otaylor@gtk.org>
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
#include "glib.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ROUND(A) floor((A)+0.5)

static Blob *
blob_new (int y, int height)
{
  Blob *result;

  result = g_malloc (sizeof (Blob) +  sizeof(BlobSpan) * height);
  result->y = y;
  result->height = height;

  return result;
}

typedef enum {
  NONE = 0,
  LEFT = 1 << 0,
  RIGHT = 1 << 1,
} EdgeType;

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

  present = g_new (EdgeType, result->height);
  memset (present, 0, result->height * sizeof(EdgeType));

  /* Initialize spans from original objects */

  for (i=0, j=b1->y-y; i<b1->height; i++,j++)
    {
      if (b1->data[i].right >= b1->data[i].left)
	{
	  present[j] = LEFT | RIGHT;
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
	      present[j] = LEFT | RIGHT;
	      result->data[j].left = b2->data[i].left;
	      result->data[j].right = b2->data[i].right;
	    }
	}
    }

  /* Now walk through edges, deleting points that aren't on convex hull */

  start = 0;
  while (!(present[start])) start++;

  /*    left edge */

  i1 = start-1; 
  i2 = start;
  x1 = result->data[start].left - result->data[start].right;
  y1 = 0;
  
  for (i=start+1;i<result->height;i++)
    {
      if (!(present[i] & LEFT))
	continue;

      x2 = result->data[i].left - result->data[i2].left;
      y2 = i-i2;
      
      while (x2*y1 - x1*y2 < 0) /* clockwise rotation */
	{
	  present[i2] &= ~LEFT;
	  i2 = i1;
	  while (!(present[--i1] & LEFT) && i1>=start);

	  if (i1<start)
	    {
	      x1 = result->data[start].left - result->data[start].right;
	      y1 = 0;
	    }
	  else
	    {
	      x1 = result->data[i2].left - result->data[i1].left;
	      y1 = i2 - i1;
	    }
	  x2 = result->data[i].left - result->data[i2].left;
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
  x1 = result->data[start].right - result->data[start].left;
  y1 = 0;
  
  for (i=start+1;i<result->height;i++)
    {
      if (!(present[i] & RIGHT))
	continue;

      x2 = result->data[i].right - result->data[i2].right;
      y2 = i-i2;
      
      while (x2*y1 - x1*y2 > 0) /* counter-clockwise rotation */
	{
	  present[i2] &= ~RIGHT;
	  i2 = i1;
	  while (!(present[--i1] & RIGHT) && i1>=start);

	  if (i1<start)
	    {
	      x1 = result->data[start].right - result->data[start].left;
	      y1 = 0;
	    }
	  else
	    {
	      x1 = result->data[i2].right - result->data[i1].right;
	      y1 = i2 - i1;
	    }
	  x2 = result->data[i].right - result->data[i2].right;
	  y2 = i - i2;
	}
      x1 = x2;
      y1 = y2;
      i1 = i2;
      i2 = i;
    }

  /* Restore edges of spans that were deleted in last step or never present */

  /* We fill only interior regions of convex hull, as if we were filling
     polygons. But since we draw ellipses with nearest points, not interior
     points, maybe it would look better if we did the same here. Probably
     not a big deal either way after anti-aliasing */

  /*     left edge */
  for (i1=start; i1<result->height-2; i1++)
    {
      /* Find empty gaps */
      if (!(present[i1+1] & LEFT))
	{
	  int increment;	/* fractional part */
	  int denom;		/* denominator of fraction */
	  int step;		/* integral step */
	  int frac;		/* fractional step */
	  int reverse;

	  /* find bottom of gap */
	  i2 = i1+2;
	  while (!(present[i2] & LEFT) && i2 < result->height) i2++;
	  
	  if (i2 < result->height)
	    {
	      denom = i2-i1;
	      x1 = result->data[i1].left;
	      x2 = result->data[i2].left;
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
		    result->data[i].left = x1;
		  else
		    result->data[i].left = x1 + 1;
		}
	    }
	  i1 = i2-1;		/* advance to next possibility */
	}
    }

  /*     right edge */
  for (i1=start; i1<result->height-2; i1++)
    {
      /* Find empty gaps */
      if (!(present[i1+1] & RIGHT))
	{
	  int increment;	/* fractional part */
	  int denom;		/* denominator of fraction */
	  int step;		/* integral step */
	  int frac;		/* fractional step */
	  int reverse;

	  /* find bottom of gap */
	  i2 = i1+2;
	  while (!(present[i2] & RIGHT) && i2 < result->height) i2++;
	  
	  if (i2 < result->height)
	    {
	      denom = i2-i1;
	      x1 = result->data[i1].right;
	      x2 = result->data[i2].right;
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
		    result->data[i].right = x1 - 1;
		  else
		    result->data[i].right = x1;
		}
	    }
	  i1 = i2-1;		/* advance to next possibility */
	}
    }
  
  /* Mark empty lines at top and bottom as unused */
  for (i=0;i<start;i++)
    {
      result->data[i].left = 0;
      result->data[i].right = -1;
    }
  for (i=result->height-1;!present[i];i--)
    {
      result->data[i].left = 0;
      result->data[i].right = -1;
    }

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

/****************************************************************
 * Code to scan convert an arbitrary ellipse into a Blob. Based
 * on Van Aken's conic algorithm in Foley and Van Damn 
 ****************************************************************/

/* Return octant from gradient */
static int
blob_get_octant (int D, int E)
{
  if (D>=0)
    {
    if (E<0)
      return (D<-E) ? 1 : 2;
    else
      return (D>E) ? 3 : 4;
    }
  else
    if (E>0)
      return (-D<E) ? 5 : 6;
    else
      return (-D>-E) ? 7 : 8;
}

static void
blob_conic_add_pixel (Blob *b, EdgeType *present, int x, int y, int octant)
{
  /*  printf ("%d %d\n",x,y); */
  if (y<b->y || y>=b->y+b->height)
    {
      /*      g_warning("Out of bounds!\n"); */
    }
  else
    {
      if (octant <= 4)
	{
	  if (present[y-b->y] & RIGHT)
	    b->data[y-b->y].right = MAX(b->data[y-b->y].right,x);
	  else
	    {
	      b->data[y-b->y].right = x;
	      present[y-b->y] |= RIGHT;
	    }
	}
      else
	{
	  if (present[y-b->y] & LEFT)
	    b->data[y-b->y].left = MIN(b->data[y-b->y].left,x);
	  else
	    {
	      b->data[y-b->y].left = x;
	      present[y-b->y] |= LEFT;
	    }
	}
    }
}

static void
blob_conic (Blob *b, int xs, int ys,
	    int A, int B, int C, int D, int E, int F)
{
  int x,y;			/* current point */
  int octant;			/* current octant */
  int dxsquare, dysquare;	/* change in (x,y) for square moves */
  int dxdiag, dydiag;		/* change in (x,y) for diagonal moves */
  int d,u,v,k1,k2,k3;		/* decision variables and increments */
  int octantCount;		/* number of octants to be drawn */
  int count;			/* number of steps for last octant */
  int tmp, i;

  EdgeType *present;
  
  present = g_new (EdgeType, b->height);
  memset (present, 0, b->height * sizeof(EdgeType));

  octant = blob_get_octant (D,E);

  switch (octant)
    {
    case 1:
      d = ROUND (A+B/2.+C/4.+D+E/2.+F);
      u = ROUND (A+B/2.+D);
      v = ROUND (A+B/2.+D+E);
      k1 = 2*A;
      k2 = 2*A + B;
      k3 = k2 + B + 2*C;
      dxsquare = 1;
      dysquare = 0;
      dxdiag = 1;
      dydiag = 1;
      break;
    case 2:
      d = ROUND (A/4.+B/2.+C+D/2.+E+F);
      u = ROUND (B/2.+C+E);
      v = ROUND (B/2.+C+D+E);
      k1 = 2*C;
      k2 = B + 2*C;
      k3 = 2*A + 2*B + 2*C;
      dxsquare = 0;
      dysquare = 1;
      dxdiag = 1;
      dydiag = 1;
      break;
    case 3:
      d = ROUND (A/4.-B/2.+C-D/2.+E+F);
      u = ROUND (-B/2.+C+E);
      v = ROUND (-B/2.+C-D+E);
      k1 = 2*C;
      k2 = 2*C - B;
      k3 = 2*A - 2*B + 2*C;
      dxsquare = 0;
      dysquare = 1;
      dxdiag = -1;
      dydiag = 1;
      break;
    case 4:
      d = ROUND (A-B/2.+C/4.-D+E/2.+F);
      u = ROUND (A-B/2.-D);
      v = ROUND (A-B/2.-D+E);
      k1 = 2*A;
      k2 = 2*A - B;
      k3 = k2 - B + 2*C;
      dxsquare = -1;
      dysquare = 0;
      dxdiag = -1;
      dydiag = 1;
      break;
    case 5:
      d = ROUND (A+B/2.+C/4.-D-E/2.+F);
      u = ROUND (A+B/2.-D);
      v = ROUND (A+B/2.-D-E);
      k1 = 2*A;
      k2 = 2*A + B;
      k3 = k2 + B + 2*C;
      dxsquare = -1;
      dysquare = 0;
      dxdiag = -1;
      dydiag = -1;
      break;
    case 6:
      d = ROUND (A/4.+B/2.+C-D/2.-E+F);
      u = ROUND (B/2.+C-E);
      v = ROUND (B/2.+C-D-E);
      k1 = 2*C;
      k2 = B + 2*C;
      k3 = 2*A + 2*B + 2*C;
      dxsquare = 0;
      dysquare = -1;
      dxdiag = -1;
      dydiag = -1;
      break;
    case 7:
      d = ROUND (A/4.-B/2.+C+D/2.-E+F);
      u = ROUND (-B/2.+C-E);
      v = ROUND (-B/2.+C+D-E);
      k1 = 2*C;
      k2 = 2*C - B;
      k3 = 2*A - 2*B + 2*C;
      dxsquare = 0;
      dysquare = -1;
      dxdiag = 1;
      dydiag = -1;
      break;
    default:			/* case 8: */
      d = ROUND (A-B/2.+C/4.+D-E/2.+F);
      u = ROUND (A-B/2.+D);
      v = ROUND (A-B/2.+D-E);
      k1 = 2*A;
      k2 = 2*A - B;
      k3 = k2 - B + 2*C;
      dxsquare = 1;
      dysquare = 0;
      dxdiag = 1;
      dydiag = -1;
      break;
    }

  octantCount = 8;
  x = xs;
  y = ys;
  count = 0;			/* ignore until last octant */

  /* Initialize boundary checking - we keep track of the discriminants
     for the conic as quadratics in x and y, and when they go negative
     we know we are beyond the boundaries of the conic. */

  while (1)
    {
      if (octantCount == 0)
	{
	  /* figure out remaining steps in square direction */
	  switch (octant)
	    {
	    case 1:
	    case 8:
	      count = xs - x;
	      break;
	    case 2:
	    case 3:
	      count = ys - y;
	      break;
	    case 4:
	    case 5:
	      count = x - xs;
	      break;
	    case 6:
	    case 7:
	      count = y - ys;
	      break;
	    }
	  /*	  if (count < 0)
	    g_warning("Negative count (%d) in octant %d\n",count,octant); */
	  if (count <= 0)
	    goto done;
	     
	}
      if (octant %2)	/* odd octants */
	{
	  while (v < k2/2)
	    {
	      blob_conic_add_pixel (b, present, x, y, octant);
	      if (d<0)
		{
		  x += dxsquare; y += dysquare;
		  u += k1;
		  v += k2;
		  d += u;
		}
	      else
		{
		  x += dxdiag; y += dydiag;
		  u += k2;
		  v += k3;
		  d += v;
		}
	      if (count && --count == 0)
		goto done;
	    }
	  /* We now cross diagonal octant boundary */
	  d = ROUND (d - u + v/2. - k2/2. + 3*k3/8.);
	  u = ROUND (-u + v - k2/2. + k3/2.);
	  v = ROUND (v - k2 + k3/2.); /* could be v + A - C */
	  k1 = k1 - 2*k2 + k3;
	  k2 = k3 - k2;
	  tmp = dxsquare;
	  dxsquare = -dysquare;
	  dysquare = tmp;
	}
      else			/* Even octants */
	{
	  while (u < k2 /2)
	    {
	      blob_conic_add_pixel (b, present, x, y, octant);
	      if (d<0)
		{
		  x += dxdiag; y += dydiag;
		  u += k2;
		  v += k3;
		  d += v;
		}
	      else
		{
		  x += dxsquare; y += dysquare;
		  u += k1;
		  v += k2;
		  d += u;
		}
	      if (count && --count <= 0)
		goto done;
	    }
	  /* We now cross square octant boundary */
	  d = d + u - v + k1 - k2;
	  v = 2*u - v + k1 - k2;
	  /* Do v first; it depends on u */
	  u = u + k1 - k2;
	  k3 = 4 * (k1 - k2) + k3;
	  k2 = 2 * k1 - k2;
	  tmp = dxdiag;
	  dxdiag = -dydiag;
	  dydiag = tmp;
	}
      octant++;
      if (octant > 8) octant -= 8;
      octantCount--;
    }
  
done:				/* jump out of two levels */
  
  for (i=0; i<b->height; i++)
    if (present[i] != (LEFT | RIGHT))
      {
	b->data[i].left = 0;
	b->data[i].right = -1;
      }

  g_free (present);
}

/* Scan convert an ellipse specified by _offsets_ of major and
   minor axes, and by center into a blob */
Blob *
blob_ellipse (double xc, double yc, double xp, double yp, double xq, double yq)
{
  Blob *r;
  double A,B,C,D,E,F;		/* coefficients of conic */
  double xprod,tmp;
  double height;
  /*  double dpx, dpy; */
  int y;

  xprod = xp*yq - xq*yp;

  if (xprod == 0)		/* colinear points */
    {
      g_print("Colinear points!\n");
      g_on_error_query("gimp");
    }
  
  if (xprod < 0)
    {
      tmp = xp; xp = xq; xq = tmp;
      tmp = yp; yp = yq; yq = tmp;
      xprod = -xprod;
    }
  
  A = yp*yp + yq*yq;
  B = -2 * (xp*yp + xq*yq);
  C = xp*xp + xq*xq;
  D = 2*yq*xprod;
  E = -2*xq*xprod;
  F = 0;
  /* Now offset the ellipse so that the center is exact, but the
     starting point is no longer exactly on the ellipse */

  /* This needs a change to blob_conic to work. blob_conic assumes
   * we start at (0,0)
   */

  /*  dpx = ROUND(xp+xc)-xp-xc;
  dpy = ROUND(yp+yc)-yp-yc;

  F += dpx*(A*dpx+D+B*dpy) + dpy*(C*dpy+E);
  D += 2*A*dpx + B*dpy;
  E += B*dpx + 2*C*dpy;
  */
  height = sqrt(A);
  y = floor(yc-height-0.5);

  /* We allow an extra pixel of slop on top and bottom to deal with
     round-off error */
  r = blob_new (y-1,ceil(yc+height+0.5)-y+3);

  /* Although it seems that multiplying A-F by a constant would improve
     things, this seems not to be the case in practice. Several test
     cases showed about equal improvement and degradation */
  blob_conic (r,ROUND(xp+xc),ROUND(yp+yc),ROUND(1*A),ROUND(1*B),ROUND(1*C),
	      ROUND(1*D),ROUND(1*E),ROUND(1*F));

  /* Add a line through the center to improve things a bit. (Doesn't
   * work perfectly because sometimes we overshoot
   */
  {
    int x0 = floor (0.5 + xc - xp);
    int x1 = floor (0.5 + xc + xp);
    int y0 = floor (0.5 + yc - yp);
    int y1 = floor (0.5 + yc + yp);
 /*    r = blob_new (MIN(y0,y1), abs(y1-y0)+1); */
    blob_line (r, x0, y0, x1, y1);
  }

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
