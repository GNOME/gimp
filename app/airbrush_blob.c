/* airbrush_blob.c: routines for manipulating scan converted convex
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

#include "airbrush_blob.h"
#include <glib.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define ROUND(A) floor((A)+0.5)

#ifndef M_PI
#define M_PI    3.14159265358979323846
#endif /* M_PI */

#ifndef M_PI_H
#define M_PI_H 3.14159265358979323846/2
#endif

#define SUBSAMPLE 8.0



static AirBrushBlob *
airbrush_blob_new (int y, int height)
{
  
  AirBrushBlob *result;

  result = g_malloc (sizeof (AirBrushBlob) +  sizeof(AirBrushBlobSpan) * (height-1));
  result->y = y;
  result->height = height;
  return result;
}


static AirBlob *
airblob_new(int y) 
{
  AirBlob  *result;
  
  result = g_malloc (sizeof (AirBlob));
  return result;
}

static AirLine *
airline_new(int y)
{
  AirLine *result;

  result = g_malloc(sizeof (AirLine));
  return result;
}





typedef enum {
  EDGE_NONE = 0,
  EDGE_LEFT = 1 << 0,
  EDGE_RIGHT = 1 << 1
} EdgeType;

static void
airbrush_blob_fill (AirBrushBlob *b, EdgeType *present)
{

  int start;
  int x1, x2, i1, i2;
  int i;

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
airbrush_blob_make_convex (AirBrushBlob *b, EdgeType *present)
{
  int x1, x2, y1, y2, i1, i2;
  int i;
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

  airbrush_blob_fill (b, present);
}

AirBrushBlob *
airbrush_blob_convex_union (AirBrushBlob *b1, AirBrushBlob *b2)
{
  AirBrushBlob *result;
  int y;
  int i, j;
  EdgeType *present;


  /* Create the storage for the result */

  y = MIN(b1->y,b2->y);
  result = airbrush_blob_new (y, MAX(b1->y+b1->height,b2->y+b2->height)-y);

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
  
  airbrush_blob_make_convex (result, present);

  g_free (present);
  return result;
}

/* You must be able to divide TABLE_SIZE with 4*/

#define TABLE_SIZE 256
#define TABLE_QUARTER TABLE_SIZE/4   

#define ELLIPSE_SHIFT 2
#define TABLE_SHIFT 14
#define TOTAL_SHIFT (ELLIPSE_SHIFT + TABLE_SHIFT)

static int trig_initialized = 0;
static int trig_table[TABLE_SIZE];
#define POWFAC 0.999



/* Scan convert an ellipse specified by _offsets_ of major and
   minor axes, and by center into a airbrush_blob */

/* Warning UGLY code ahead :-)*/

AirBrushBlob *
airbrush_blob_ellipse (double xc, double yc, double xt, double yt, double xr, double yr, double xb, double yb, double xl, double yl)
{
  int i;
  AirBrushBlob *rma, *rmi, *rtot;
  gint maxyma, minyma, maxymi, minymi;
  gint step;
  double max_radius;
  double ma_ang1, ma_ang2;
  double x1, x2, y1, y2;
  double xtotma, ytotma;
  double xcma, ycma, ytma, xrma, ybma, xlma;
  double xcmi, ycmi, ytmi, xrmi, ybmi, xlmi;
  double mapoint_i, mapoint; 

  gint xcma_shift, ycma_shift;
  gint xcmi_shift, ycmi_shift; 
  gint ytma_shift, ytmi_shift;
  gint xrma_shift, xrmi_shift;
  gint ybma_shift, ybmi_shift;
  gint xlma_shift, xlmi_shift;

  EdgeType *presentma;
  EdgeType *presentmi;


  if ((yt == yb) && (xr == xl) && (yt == xr))
	{
	/*Zero*/

	ytma = ytmi = xrma = xrmi = ybma = ybmi = xlma = xlmi = yt;
	ycma = ycmi = yc;
	xcma = xcmi = xc;

	}
  else if (xr == xl)
	{
	if (yt > yb)
		{
		/*South*/

		/*The Max circle*/ 
		ytma = ybma = xrma = xlma = xl;
		mapoint_i = (((yt * yt) / yb) - yt);
		mapoint = mapoint_i * pow(POWFAC, mapoint_i);	
		xcma = xc;
		ycma = yc + mapoint;

		/*The Min Circle*/
		ytmi = xrmi = ybmi = xlmi = xl/2;
		xcmi = xc;
		ycmi = yc - mapoint/2;

		}
	else
	        {	
		/*North*/

                /*The Max circle*/
                ytma = ybma = xrma = xlma = xl;
                mapoint_i = (((yb * yb) / yt) - yb);
                mapoint = mapoint_i * pow(POWFAC, mapoint_i);
                xcma = xc;
                ycma = yc - mapoint;

                /*The Min Circle*/
                ytmi = xrmi = ybmi = xlmi = xl/2;
                xcmi = xc;
                ycmi = yc + mapoint/2; 

		}	
	}	 
  else if (yt == yb)
	{
	if (xr > xl)
		{
		/*East*/
		
		/*The Max circle*/
		ytma = ybma = xrma = xlma = yt;
		mapoint_i = (((xr * xr) /xl) -xr);
		mapoint = mapoint_i * pow(POWFAC, mapoint_i);
		xcma = mapoint + xc;
		ycma = yc;

		/*The Min Circle*/
		ytmi = ybmi = xrmi = xlmi = yt/2;
		xcmi = xc - mapoint/2; 	
		ycmi = yc;
		
		}
	else
		{
		/*West*/

                /*The Max circle*/
                ytma = ybma = xrma = xlma = yt;
                mapoint_i = (((xl * xl) /xr) - xl);
                mapoint = mapoint_i * pow(POWFAC, mapoint_i);
                xcma = xc - mapoint;
                ycma = yc;
  

                /*The Min Circle*/
                ytmi = ybmi = xrmi = xlmi = yt/2;
                xcmi = xc + mapoint/2;
                ycmi = yc; 
		
		}
	}
  else if ((yt > yb) && (xr > xl))
  	{
	/*SouthEast*/

        /*The Max circle*/
        ma_ang1 = atan(yt/xr);
        x1 = cos(ma_ang1) * xl;
        y1 = sin(ma_ang1) * yt;
        ma_ang2 = M_PI/2 - ma_ang1;
        x2 = cos(ma_ang2) * xr;
        y2 = sin(ma_ang2) * yb;
        xtotma = x1 + x2;
        ytotma = y1 + y2;
        ytma = ybma = xrma = xlma = hypot(ytotma, xtotma)/2;
        mapoint_i = (((yt * yt + xr * xr) / hypot(yb, xl)) - hypot(yt, xr));
        mapoint = mapoint_i  * pow(POWFAC , mapoint_i);
        xcma = xc + (cos(ma_ang1) * mapoint);
        ycma = yc + (sin(ma_ang1) * mapoint);

        /*The Min Circle*/
        ytmi = xrmi = ybmi = xlmi = hypot(yb, xl)/2;
        xcmi = xc - (cos(ma_ang1) * mapoint * 0.5);
        ycmi = yc - (sin(ma_ang1) * mapoint * 0.5);         

	}
  else if ((yt > yb) && (xl > xr))
        {	
	/*SouthWest*/

        /*The Max circle*/
        ma_ang1 = atan(yt/xl);
        x1 = cos(ma_ang1) * xr;
        y1 = sin(ma_ang1) * yt;
        ma_ang2 = M_PI/2 - ma_ang1;
        x2 = cos(ma_ang2) * xl;
        y2 = sin(ma_ang2) * yb;
        xtotma = x1 + x2;
        ytotma = y1 + y2;
        ytma = ybma = xrma = xlma = hypot(ytotma, xtotma)/2;
        mapoint_i = (((yt * yt + xl * xl) / hypot(yb, xr)) - hypot(yt, xl));
        mapoint = mapoint_i * pow(POWFAC, mapoint_i);
        xcma = xc - (cos(ma_ang1) * mapoint);
        ycma = yc + (sin(ma_ang1) * mapoint);

        /*The Min Circle*/
        ytmi = xrmi = ybmi = xlmi = hypot(yb, xr)/2;
        xcmi = xc + (cos(ma_ang1) * mapoint * 0.5);
        ycmi = yc - (sin(ma_ang1) * mapoint * 0.5);       

	}	
  else if ((yb > yt) && (xl > xr))
	{	
	/*NorthWest*/

        /*The Max circle*/
        ma_ang1 = atan(yb/xl);
        x1 = cos(ma_ang1) * xl;
        y1 = sin(ma_ang1) * yt;
        ma_ang2 = M_PI/2 - ma_ang1;
        x2 = cos(ma_ang2) * xr;
        y2 = sin(ma_ang2) * yb;
        xtotma = x1 + x2;
        ytotma = y1 + y2;
        ytma = ybma = xrma = xlma = hypot(ytotma, xtotma)/2;
        mapoint_i = (((yb * yb + xl * xl) / hypot(yt, xr)) - hypot(yb, xl));
        mapoint = mapoint_i * pow(POWFAC, mapoint_i);
        xcma = xc - (cos(ma_ang1) * mapoint);
        ycma = yc - (sin(ma_ang1) * mapoint);

        /*The Min Circle*/
        ytmi = xrmi = ybmi = xlmi = hypot(yt, xr)/2;
        xcmi = xc + (cos(ma_ang1) * mapoint * 0.5);
        ycmi = yc + (sin(ma_ang1) * mapoint * 0.5);       

	}	
  else 
/*if ((yb > yt) && (xr > xl))*/
	{
	/*NorthEast*/

        /*The Max circle*/
        ma_ang1 = atan(yb/xr);
        x1 = cos(ma_ang1) * xr;
        y1 = sin(ma_ang1) * yt;
        ma_ang2 = M_PI/2 - ma_ang1;
        x2 = cos(ma_ang2) * xl;
        y2 = sin(ma_ang2) * yb;
        xtotma = x1 + x2;
        ytotma = y1 + y2;
        ytma = ybma = xrma = xlma = hypot(ytotma, xtotma)/2;
        mapoint_i = (((yb * yb + xr * xr) / hypot(yt, xl)) - hypot(yb, xr));
        mapoint = mapoint_i * pow(POWFAC, mapoint_i);
        xcma = xc + (cos(ma_ang1) * mapoint);
        ycma = yc - (sin(ma_ang1) * mapoint);

        /*The Min Circle*/
        ytmi = xrmi = ybmi = xlmi = hypot(yt, xl)/2;
        xcmi = xc - (cos(ma_ang1) * mapoint * 0.5);
        ycmi = yc + (sin(ma_ang1) * mapoint * 0.5);       

	}
  if (ytmi <= 0)
	{
	ytmi = ybmi = xrmi = xlmi = 1;
	} 
		
  if (ytma <= 0)
        {
        ytma = ybma = xrma = xlma = 1;
        } 	
  
  if (!trig_initialized)
    {
      trig_initialized = 1;
      for (i=0; i<256; i++)
	trig_table[i] = 0.5 + sin(i * (M_PI / 128.)) * (1 << TABLE_SHIFT);
    }

 
/*Make the Max circle*/
   
  maxyma = ceil (ycma + fabs (ytma));
  minyma = floor (ycma - fabs (ybma));



  rma = airbrush_blob_new (minyma, maxyma - minyma + 1);
  
  
  presentma = g_new0 (EdgeType, rma->height);

  max_radius = ytma;

  step = TABLE_SIZE;

  while (step > 1 && (TABLE_SIZE / step < 4*max_radius))
    step >>= 1;

  /* Fill in the edge points */

  xcma_shift = 0.5 + xcma * (1 << TOTAL_SHIFT);
  ycma_shift = 0.5 + ycma * (1 << TOTAL_SHIFT);
  ytma_shift = 0.5 + ytma * (1 << ELLIPSE_SHIFT);
  xrma_shift = 0.5 + xrma * (1 << ELLIPSE_SHIFT);
  ybma_shift = 0.5 + ybma * (1 << ELLIPSE_SHIFT); 
  xlma_shift = 0.5 + xlma * (1 << ELLIPSE_SHIFT);


  for (i = 0 ; i < TABLE_SIZE ; i += step)
    {

      gint x, y, yi, dydi;

      gint s = trig_table[i];
      gint c = trig_table[(TABLE_SIZE + TABLE_SIZE/4 - i) % TABLE_SIZE];

      if (i < TABLE_QUARTER )
        {
          x = (xcma_shift + c * xrma_shift +
               (1 << (TOTAL_SHIFT - 1))) >> TOTAL_SHIFT;
          yi = (ycma_shift + s * ytma_shift +
               (1 << (TOTAL_SHIFT - 1))) >> TOTAL_SHIFT;
          y = yi -  rma->y;
          dydi = 1;

        }
      else if ( i < (2 * TABLE_QUARTER) )
        {
          x = (xcma_shift + c * xlma_shift +
               (1 << (TOTAL_SHIFT - 1))) >> TOTAL_SHIFT;
          yi = (ycma_shift + s * ytma_shift +
               (1 << (TOTAL_SHIFT - 1))) >> TOTAL_SHIFT;
 
          y = yi - rma->y; 
           
          dydi = -1;
        }
      else if ( i < (3 * TABLE_QUARTER) )
        {
          x = (xcma_shift + c * xlma_shift +
               (1 << (TOTAL_SHIFT - 1))) >> TOTAL_SHIFT;
          yi = (ycma_shift + s * ybma_shift +
               (1 << (TOTAL_SHIFT - 1))) >> TOTAL_SHIFT; 
          y = yi - rma->y;
          dydi = -1;

        }
      else
        {
          x = (xcma_shift + c * xrma_shift +
               (1 << (TOTAL_SHIFT - 1))) >> TOTAL_SHIFT;
          y = ((ycma_shift + s * ybma_shift +
               (1 << (TOTAL_SHIFT - 1))) >> TOTAL_SHIFT) - rma->y;

          dydi = 1;

        }




      if (dydi <= 0) /* left edge */
	{
	  if (presentma[y] & EDGE_LEFT)
	    {
	      rma->data[y].left = MIN (rma->data[y].left, x);
	    }
	  else
	    {
	      presentma[y] |= EDGE_LEFT;
	      rma->data[y].left = x;
	    }
	}
      
      if (dydi > 0) /* right edge */
	{
	  if (presentma[y] & EDGE_RIGHT)
	    {
	      rma->data[y].right = MAX (rma->data[y].right, x);
	    }
	  else
	    {
	      presentma[y] |= EDGE_RIGHT;
	      rma->data[y].right = x;
	    }
	}
    }

  /* Now fill in missing points */

  airbrush_blob_fill (rma, presentma);
  g_free (presentma);

/*Make the Min circle*/


  maxymi = ceil (ycmi + fabs (ytmi));
  minymi = floor (ycmi - fabs (ybmi));


  rmi = airbrush_blob_new (minymi, maxymi - minymi + 1);


  presentmi = g_new0 (EdgeType, rmi->height);

  max_radius = ytmi;

  step = TABLE_SIZE;

  while (step > 1 && (TABLE_SIZE / step < 4*max_radius))
    step >>= 1;

  /* Fill in the edge points */

  xcmi_shift = 0.5 + xcmi * (1 << TOTAL_SHIFT);
  ycmi_shift = 0.5 + ycmi * (1 << TOTAL_SHIFT);
  ytmi_shift = 0.5 + ytmi * (1 << ELLIPSE_SHIFT);
  xrmi_shift = 0.5 + xrmi * (1 << ELLIPSE_SHIFT);
  ybmi_shift = 0.5 + ybmi * (1 << ELLIPSE_SHIFT);
  xlmi_shift = 0.5 + xlmi * (1 << ELLIPSE_SHIFT);



  for (i = 0 ; i < TABLE_SIZE ; i += step)
    {

      gint x, y, yi, dydi;

      gint s = trig_table[i];
      gint c = trig_table[(TABLE_SIZE + TABLE_SIZE/4 - i) % TABLE_SIZE];

      if (i < TABLE_QUARTER )
        {
          x = (xcmi_shift + c * xrmi_shift +
               (1 << (TOTAL_SHIFT - 1))) >> TOTAL_SHIFT;
          yi = (ycmi_shift + s * ytmi_shift +
               (1 << (TOTAL_SHIFT - 1))) >> TOTAL_SHIFT;
          y = yi -  rmi->y;
          dydi = 1;

        }
      else if ( i < (2 * TABLE_QUARTER) )
        {
          x = (xcmi_shift + c * xlmi_shift +
               (1 << (TOTAL_SHIFT - 1))) >> TOTAL_SHIFT;
          yi = (ycmi_shift + s * ytmi_shift +
               (1 << (TOTAL_SHIFT - 1))) >> TOTAL_SHIFT;

          y = yi - rmi->y;

          dydi = -1;
        }
      else if ( i < (3 * TABLE_QUARTER) )
        {
          x = (xcmi_shift + c * xlmi_shift +
               (1 << (TOTAL_SHIFT - 1))) >> TOTAL_SHIFT;
          yi = (ycmi_shift + s * ybmi_shift +
               (1 << (TOTAL_SHIFT - 1))) >> TOTAL_SHIFT;
          y = yi - rmi->y;
          dydi = -1;

        }
      else
        {
          x = (xcmi_shift + c * xrmi_shift +
               (1 << (TOTAL_SHIFT - 1))) >> TOTAL_SHIFT;
          y = ((ycmi_shift + s * ybmi_shift +
               (1 << (TOTAL_SHIFT - 1))) >> TOTAL_SHIFT) - rmi->y;

          dydi = 1;

        }




      if (dydi <= 0) /* left edge */
        {
          if (presentmi[y] & EDGE_LEFT)
            {
              rmi->data[y].left = MIN (rmi->data[y].left, x);
            }
          else
            {
              presentmi[y] |= EDGE_LEFT;
              rmi->data[y].left = x;
            }
        }

      if (dydi > 0) /* right edge */
        {
          if (presentmi[y] & EDGE_RIGHT)
            {
              rmi->data[y].right = MAX (rmi->data[y].right, x);
            }
          else
            {
              presentmi[y] |= EDGE_RIGHT;
              rmi->data[y].right = x;
            }
        }
    }

  /* Now fill in missing points */

  airbrush_blob_fill (rmi, presentmi);
  g_free (presentmi);                     

  rtot = airbrush_blob_convex_union(rma, rmi);

  g_free (rma);
  g_free (rmi);	

  return rtot;
}

void
airbrush_blob_bounds(AirBrushBlob *b, int *x, int *y, int *width, int *height)
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
airbrush_blob_dump(AirBrushBlob *b) {

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

/* This is just a first try to see how it works i.e ugly code :-) */


AirBlob *
create_air_blob (double xc,
		 double yc,
		 double xt,
		 double yt,
		 double xr,
		 double yr,
		 double xb,
		 double yb,
		 double xl,
		 double yl,
		 double direction_abs,
		 double direction)
{
  AirBlob *air_blob;
  double ma_ang1, ma_ang2;
  double x1, x2, y1, y2;
  double xtotma, ytotma;
  double xcma, ycma, ytma, xrma, ybma, xlma;
  double xcmi, ycmi, ytmi, xrmi, ybmi, xlmi;
  double mapoint_i, mapoint; 
  
  air_blob = airblob_new(1);

  air_blob->direction_abs = direction_abs;
  air_blob->direction = direction;
  air_blob->ycenter = yc;
  air_blob->xcenter = xc;




  if ((yt == yb) && (xr == xl) && (yt == xr))
	{
	/*Zero*/

	ytma = ytmi = xrma = xrmi = ybma = ybmi = xlma = xlmi = yt;
	ycma = ycmi = yc;
	xcma = xcmi = xc;
	
	air_blob->main_line.size = yt;
	air_blob->minor_line.size = yt;
	
 	air_blob->maincross_line.size = yt * 2;
	air_blob->maincross_line.dist = 0.0;

 	air_blob->minorcross_line.size = yt * 2;
	air_blob->minorcross_line.dist = 0.0;

	air_blob->direction = M_PI_H;

	}
  else if (xr == xl)
	{
	if (yt > yb)
		{
		/*South*/

		/*The Max circle*/ 
		ytma = ybma = xrma = xlma = xl;
		mapoint_i = (((yt * yt) / yb) - yt);
		mapoint = mapoint_i * pow(POWFAC, mapoint_i);	
		xcma = xc;
		ycma = yc + mapoint;

		/*The Min Circle*/
		ytmi = xrmi = ybmi = xlmi = xl/2;
		xcmi = xc;
		ycmi = yc - mapoint/2;

		air_blob->main_line.size = mapoint + xl;
		air_blob->minor_line.size = mapoint/2 + xl/2;
	
		air_blob->maincross_line.size = xl * 2;
		air_blob->maincross_line.dist = mapoint;

		air_blob->minorcross_line.size = xl/2;
		air_blob->minorcross_line.dist = mapoint/2;



		}
	else
	        {	
		/*North*/

                /*The Max circle*/
                ytma = ybma = xrma = xlma = xl;
                mapoint_i = (((yb * yb) / yt) - yb);
                mapoint = mapoint_i * pow(POWFAC, mapoint_i);
                xcma = xc;
                ycma = yc - mapoint;

                /*The Min Circle*/
                ytmi = xrmi = ybmi = xlmi = xl/2;
                xcmi = xc;
                ycmi = yc + mapoint/2; 


		air_blob->main_line.size = mapoint + xl;
		air_blob->minor_line.size = mapoint/2 + xl/2;
	
		air_blob->maincross_line.size = xl * 2;
		air_blob->maincross_line.dist = mapoint;

		air_blob->minorcross_line.size = xl/2;
		air_blob->minorcross_line.dist = mapoint/2;


		}	
	}	 
  else if (yt == yb)
	{
	if (xr > xl)
		{
		/*East*/
		
		/*The Max circle*/
		ytma = ybma = xrma = xlma = yt;
		mapoint_i = (((xr * xr) /xl) -xr);
		mapoint = mapoint_i * pow(POWFAC, mapoint_i);
		xcma = mapoint + xc;
		ycma = yc;

		/*The Min Circle*/
		ytmi = ybmi = xrmi = xlmi = yt/2;
		xcmi = xc - mapoint/2; 	
		ycmi = yc;

		air_blob->main_line.size = mapoint + yt;
		air_blob->minor_line.size = mapoint/2 + yt/2;

		air_blob->maincross_line.size = xl * 2;
		air_blob->maincross_line.dist = mapoint;

		air_blob->minorcross_line.size = xl/2;
		air_blob->minorcross_line.dist = mapoint/2;

		
		}
	else
		{
		/*West*/

                /*The Max circle*/
                ytma = ybma = xrma = xlma = yt;
                mapoint_i = (((xl * xl) /xr) - xl);
                mapoint = mapoint_i * pow(POWFAC, mapoint_i);
                xcma = xc - mapoint;
                ycma = yc;
  

                /*The Min Circle*/
                ytmi = ybmi = xrmi = xlmi = yt/2;
                xcmi = xc + mapoint/2;
                ycmi = yc; 

		air_blob->main_line.size = mapoint + yt;
		air_blob->minor_line.size = mapoint/2 + yt/2;
	
	     	air_blob->maincross_line.size = xl * 2;
		air_blob->maincross_line.dist = mapoint;

		air_blob->minorcross_line.size = xl/2;
		air_blob->minorcross_line.dist = mapoint/2;

		
		}
	}
  else if ((yt > yb) && (xr > xl))
  	{
	/*SouthEast*/

        /*The Max circle*/
        ma_ang1 = atan(yt/xr);
        x1 = cos(ma_ang1) * xl;
        y1 = sin(ma_ang1) * yt;
        ma_ang2 = M_PI/2 - ma_ang1;
        x2 = cos(ma_ang2) * xr;
        y2 = sin(ma_ang2) * yb;
        xtotma = x1 + x2;
        ytotma = y1 + y2;
        ytma = ybma = xrma = xlma = hypot(ytotma, xtotma)/2;
        mapoint_i = (((yt * yt + xr * xr) / hypot(yb, xl)) - hypot(yt, xr));
        mapoint = mapoint_i  * pow(POWFAC , mapoint_i);
        xcma = xc + (cos(ma_ang1) * mapoint);
        ycma = yc + (sin(ma_ang1) * mapoint);

        /*The Min Circle*/
        ytmi = xrmi = ybmi = xlmi = hypot(yb, xl)/2;
        xcmi = xc - (cos(ma_ang1) * mapoint * 0.5);
        ycmi = yc - (sin(ma_ang1) * mapoint * 0.5);         


	air_blob->main_line.size = mapoint + xlma;
	air_blob->minor_line.size = mapoint/2 + xlmi;

	
	air_blob->maincross_line.size = xlma * 2;
	air_blob->maincross_line.dist = mapoint;

	air_blob->minorcross_line.size = xlmi;
	air_blob->minorcross_line.dist = mapoint/2;




	}
  else if ((yt > yb) && (xl > xr))
        {	
	/*SouthWest*/

        /*The Max circle*/
        ma_ang1 = atan(yt/xl);
        x1 = cos(ma_ang1) * xr;
        y1 = sin(ma_ang1) * yt;
        ma_ang2 = M_PI/2 - ma_ang1;
        x2 = cos(ma_ang2) * xl;
        y2 = sin(ma_ang2) * yb;
        xtotma = x1 + x2;
        ytotma = y1 + y2;
        ytma = ybma = xrma = xlma = hypot(ytotma, xtotma)/2;
        mapoint_i = (((yt * yt + xl * xl) / hypot(yb, xr)) - hypot(yt, xl));
        mapoint = mapoint_i * pow(POWFAC, mapoint_i);
        xcma = xc - (cos(ma_ang1) * mapoint);
        ycma = yc + (sin(ma_ang1) * mapoint);

        /*The Min Circle*/
        ytmi = xrmi = ybmi = xlmi = hypot(yb, xr)/2;
        xcmi = xc + (cos(ma_ang1) * mapoint * 0.5);
        ycmi = yc - (sin(ma_ang1) * mapoint * 0.5);       

	air_blob->main_line.size = mapoint + xlma;
	air_blob->minor_line.size = mapoint/2 + xlmi;

	air_blob->maincross_line.size = xlma * 2;
	air_blob->maincross_line.dist = mapoint;

	air_blob->minorcross_line.size = xlmi;
	air_blob->minorcross_line.dist = mapoint/2;

	}	
  else if ((yb > yt) && (xl > xr))
	{	
	/*NorthWest*/

        /*The Max circle*/
        ma_ang1 = atan(yb/xl);
        x1 = cos(ma_ang1) * xl;
        y1 = sin(ma_ang1) * yt;
        ma_ang2 = M_PI/2 - ma_ang1;
        x2 = cos(ma_ang2) * xr;
        y2 = sin(ma_ang2) * yb;
        xtotma = x1 + x2;
        ytotma = y1 + y2;
        ytma = ybma = xrma = xlma = hypot(ytotma, xtotma)/2;
        mapoint_i = (((yb * yb + xl * xl) / hypot(yt, xr)) - hypot(yb, xl));
        mapoint = mapoint_i * pow(POWFAC, mapoint_i);
        xcma = xc - (cos(ma_ang1) * mapoint);
        ycma = yc - (sin(ma_ang1) * mapoint);

        /*The Min Circle*/
        ytmi = xrmi = ybmi = xlmi = hypot(yt, xr)/2;
        xcmi = xc + (cos(ma_ang1) * mapoint * 0.5);
        ycmi = yc + (sin(ma_ang1) * mapoint * 0.5);

	air_blob->main_line.size = mapoint + xlma;
	air_blob->minor_line.size = mapoint/2 + xlmi;

	air_blob->maincross_line.size = xlma * 2;
	air_blob->maincross_line.dist = mapoint;

	air_blob->minorcross_line.size = xlmi;
	air_blob->minorcross_line.dist = mapoint/2;


       

	}	
  else 
/*if ((yb > yt) && (xr > xl))*/
	{
	/*NorthEast*/

        /*The Max circle*/
        ma_ang1 = atan(yb/xr);
        x1 = cos(ma_ang1) * xr;
        y1 = sin(ma_ang1) * yt;
        ma_ang2 = M_PI/2 - ma_ang1;
        x2 = cos(ma_ang2) * xl;
        y2 = sin(ma_ang2) * yb;
        xtotma = x1 + x2;
        ytotma = y1 + y2;
        ytma = ybma = xrma = xlma = hypot(ytotma, xtotma)/2;
        mapoint_i = (((yb * yb + xr * xr) / hypot(yt, xl)) - hypot(yb, xr));
        mapoint = mapoint_i * pow(POWFAC, mapoint_i);
        xcma = xc + (cos(ma_ang1) * mapoint);
        ycma = yc - (sin(ma_ang1) * mapoint);

        /*The Min Circle*/
        ytmi = xrmi = ybmi = xlmi = hypot(yt, xl)/2;
        xcmi = xc - (cos(ma_ang1) * mapoint * 0.5);
        ycmi = yc + (sin(ma_ang1) * mapoint * 0.5);

	air_blob->main_line.size = mapoint + xlma;
	air_blob->minor_line.size = mapoint/2 + xlmi;


	air_blob->maincross_line.size = xlma * 2;
	air_blob->maincross_line.dist = mapoint;

	air_blob->minorcross_line.size = xlmi;
	air_blob->minorcross_line.dist = mapoint/2;


	}

  return air_blob;
}

AirLine *
create_air_line(AirBlob *airblob)
{

  int i;
  double xcenter, ycenter;

  double direction;

  double masupport, misupport;
  double ma_angsupport, mi_angsupport,  iang;

  int min_x, max_x;
  int min_y, max_y;

  AirLine *airline;

  /* Yes I know I can do a cal of number of lines, but it is for
     the moment much easier to just set a side mem for 16 lines
  */

  airline = airline_new(1);

  xcenter = airblob->xcenter;
  ycenter = airblob->ycenter;
  
  airline->xcenter = xcenter/SUBSAMPLE;
  airline->ycenter = ycenter/SUBSAMPLE;

  direction = airblob->direction_abs;

  if(direction == 0.0)

      {

	airline->line[0].x = (xcenter + airblob->maincross_line.dist)/SUBSAMPLE;
	airline->line[0].y = (ycenter - airblob->maincross_line.size/2)/SUBSAMPLE;
	airline->line[1].x = (xcenter - airblob->minorcross_line.dist)/SUBSAMPLE;
	airline->line[1].y = (ycenter + airblob->minorcross_line.size/2)/SUBSAMPLE;

	airline->line[2].x = (xcenter + airblob->main_line.size)/SUBSAMPLE;
	airline->line[2].y = (ycenter)/SUBSAMPLE;
	airline->line[3].x = (xcenter - airblob->minor_line.size)/SUBSAMPLE;
	airline->line[3].y = (ycenter)/SUBSAMPLE;

	airline->line[4].x = (xcenter + airblob->maincross_line.dist)/SUBSAMPLE;
	airline->line[4].y = (ycenter + airblob->maincross_line.size/2)/SUBSAMPLE;
	airline->line[5].x = (xcenter - airblob->minorcross_line.dist)/SUBSAMPLE;
	airline->line[5].y = (ycenter - airblob->minorcross_line.size/2)/SUBSAMPLE;

	airline->nlines = 6;


      }	
		
  else if(direction == M_PI_H)

      {

        airline->line[0].x = (xcenter - airblob->maincross_line.size/2)/SUBSAMPLE;
	airline->line[0].y = (ycenter - airblob->maincross_line.dist)/SUBSAMPLE;
	airline->line[1].x = (xcenter + airblob->minorcross_line.dist)/SUBSAMPLE;
	airline->line[1].y = (ycenter + airblob->minorcross_line.size/2)/SUBSAMPLE;

	airline->line[2].x = (xcenter)/SUBSAMPLE;
	airline->line[2].y = (ycenter - airblob->main_line.size)/SUBSAMPLE;
	airline->line[3].x = (xcenter)/SUBSAMPLE;
	airline->line[3].y = (ycenter + airblob->minor_line.size)/SUBSAMPLE;

	airline->line[4].x = (xcenter + airblob->maincross_line.dist)/SUBSAMPLE;
	airline->line[4].y = (ycenter - airblob->maincross_line.size/2)/SUBSAMPLE;
	airline->line[5].x = (xcenter - airblob->minorcross_line.dist)/SUBSAMPLE;
	airline->line[5].y = (ycenter + airblob->minorcross_line.size/2)/SUBSAMPLE;

	airline->nlines = 6;

      }


  else if(direction == M_PI)

    {
        airline->line[0].x = (xcenter - airblob->maincross_line.dist)/SUBSAMPLE;
	airline->line[0].y = (ycenter + airblob->maincross_line.size/2)/SUBSAMPLE;
	airline->line[1].x = (xcenter + airblob->minorcross_line.dist)/SUBSAMPLE;
	airline->line[1].y = (ycenter - airblob->minorcross_line.size/2)/SUBSAMPLE;

	airline->line[2].x = (xcenter - airblob->main_line.size)/SUBSAMPLE;
	airline->line[2].y = (ycenter)/SUBSAMPLE;
	airline->line[3].x = (xcenter + airblob->minor_line.size)/SUBSAMPLE;
	airline->line[3].y = (ycenter)/SUBSAMPLE;

	airline->line[4].x = (xcenter - airblob->maincross_line.dist)/SUBSAMPLE;
	airline->line[4].y = (ycenter - airblob->maincross_line.size/2)/SUBSAMPLE;
	airline->line[5].x = (xcenter + airblob->minorcross_line.dist)/SUBSAMPLE;
	airline->line[5].y = (ycenter + airblob->minorcross_line.size/2)/SUBSAMPLE;

	airline->nlines = 6;

    }

  else if(direction == -M_PI_H)

    {
        airline->line[0].x = (xcenter - airblob->maincross_line.dist)/SUBSAMPLE;
	airline->line[0].y = (ycenter + airblob->maincross_line.size/2)/SUBSAMPLE;
	airline->line[1].x = (xcenter + airblob->minorcross_line.dist)/SUBSAMPLE;
	airline->line[1].y = (ycenter - airblob->minorcross_line.size/2)/SUBSAMPLE;

	airline->line[2].x = (xcenter)/SUBSAMPLE;
	airline->line[2].y = (ycenter + airblob->main_line.size)/SUBSAMPLE;
	airline->line[3].x = (xcenter)/SUBSAMPLE;
	airline->line[3].y = (ycenter - airblob->minor_line.size)/SUBSAMPLE;

	airline->line[4].x = (xcenter + airblob->maincross_line.dist)/SUBSAMPLE;
	airline->line[4].y = (ycenter + airblob->maincross_line.size/2)/SUBSAMPLE;
	airline->line[5].x = (xcenter - airblob->minorcross_line.dist)/SUBSAMPLE;
	airline->line[5].y = (ycenter - airblob->minorcross_line.size/2)/SUBSAMPLE;
    }


  else if(direction == -M_PI)
    
    {
        airline->line[0].x = (xcenter - airblob->maincross_line.dist)/SUBSAMPLE;
	airline->line[0].y = (ycenter + airblob->maincross_line.size/2)/SUBSAMPLE;
	airline->line[1].x = (xcenter + airblob->minorcross_line.dist)/SUBSAMPLE;
	airline->line[1].y = (ycenter - airblob->minorcross_line.size/2)/SUBSAMPLE;

	airline->line[2].x = (xcenter - airblob->main_line.size)/SUBSAMPLE;
	airline->line[2].y = (ycenter)/SUBSAMPLE;
	airline->line[3].x = (xcenter + airblob->minor_line.size)/SUBSAMPLE;
	airline->line[3].y = (ycenter)/SUBSAMPLE;

	airline->line[4].x = (xcenter - airblob->maincross_line.dist)/SUBSAMPLE;
	airline->line[4].y = (ycenter - airblob->maincross_line.size/2)/SUBSAMPLE;
	airline->line[5].x = (xcenter + airblob->minorcross_line.dist)/SUBSAMPLE;
	airline->line[5].y = (ycenter + airblob->minorcross_line.size/2)/SUBSAMPLE;

	airline->nlines = 6;
       
    }


  else if ((direction < M_PI) && (direction > M_PI_H))

    {


        masupport = hypot(airblob->maincross_line.dist, airblob->maincross_line.size/2);
	misupport = hypot(airblob->minorcross_line.dist,airblob->minorcross_line.size/2);

	ma_angsupport = atan(airblob->maincross_line.size/2/airblob->maincross_line.dist);
	mi_angsupport = atan(airblob->minorcross_line.size/2/airblob->minorcross_line.dist);
	
	iang = airblob->direction_abs - M_PI_H;

        airline->line[0].x = (xcenter - sin(iang + ma_angsupport) * masupport)/SUBSAMPLE;
	airline->line[0].y = (ycenter - cos(iang + ma_angsupport) * masupport)/SUBSAMPLE;
	airline->line[1].x = (xcenter + sin(iang + mi_angsupport) * misupport)/SUBSAMPLE;
	airline->line[1].y = (ycenter + cos(iang + mi_angsupport) * misupport)/SUBSAMPLE;

	airline->line[2].x = (xcenter - sin(iang) * airblob->main_line.size)/SUBSAMPLE;
	airline->line[2].y = (ycenter - cos(iang) * airblob->main_line.size)/SUBSAMPLE;
	airline->line[3].x = (xcenter + sin(iang) * airblob->minor_line.size)/SUBSAMPLE;
	airline->line[3].y = (ycenter + cos(iang) * airblob->minor_line.size)/SUBSAMPLE;

	airline->line[4].x = (xcenter - sin(iang - ma_angsupport) * masupport)/SUBSAMPLE;
	airline->line[4].y = (ycenter - cos(iang - ma_angsupport) * masupport)/SUBSAMPLE;
	airline->line[5].x = (xcenter + sin(iang - mi_angsupport) * misupport)/SUBSAMPLE;
	airline->line[5].y = (ycenter + cos(iang - mi_angsupport) * misupport)/SUBSAMPLE;

	airline->line[6].x = (xcenter - sin(iang + ma_angsupport/2.) * (masupport + (airblob->main_line.size - masupport)/2.0))/SUBSAMPLE;
	airline->line[6].y = (ycenter - cos(iang + ma_angsupport/2.) * (masupport + (airblob->main_line.size - masupport)/2.0))/SUBSAMPLE;
	airline->line[7].x = (xcenter - sin(iang - ma_angsupport/2.) * (masupport + (airblob->main_line.size - masupport)/2.0))/SUBSAMPLE;
	airline->line[7].y = (ycenter - cos(iang - ma_angsupport/2.) * (masupport + (airblob->main_line.size - masupport)/2.0))/SUBSAMPLE;

	
	
	airline->nlines = 8;	
       
    }

  else if ((direction < M_PI_H) && (direction > 0.0))

    {


        masupport = hypot(airblob->maincross_line.dist, airblob->maincross_line.size/2);
	misupport = hypot(airblob->minorcross_line.dist,airblob->minorcross_line.size/2);

	ma_angsupport = atan(airblob->maincross_line.size/2/airblob->maincross_line.dist);
	mi_angsupport = atan(airblob->minorcross_line.size/2/airblob->minorcross_line.dist);
		
	iang = airblob->direction_abs;

        airline->line[0].x = (xcenter + cos(iang + ma_angsupport) * masupport)/SUBSAMPLE;
	airline->line[0].y = (ycenter - sin(iang + ma_angsupport) * masupport)/SUBSAMPLE;
	airline->line[1].x = (xcenter - cos(iang + mi_angsupport) * misupport)/SUBSAMPLE;
	airline->line[1].y = (ycenter + sin(iang + mi_angsupport) * misupport)/SUBSAMPLE;

	airline->line[2].x = (xcenter + cos(iang) * airblob->main_line.size)/SUBSAMPLE;
	airline->line[2].y = (ycenter - sin(iang) * airblob->main_line.size)/SUBSAMPLE;
	airline->line[3].x = (xcenter - cos(iang) * airblob->minor_line.size)/SUBSAMPLE;
	airline->line[3].y = (ycenter + sin(iang) * airblob->minor_line.size)/SUBSAMPLE;

	airline->line[4].x = (xcenter + cos(iang - ma_angsupport) * masupport)/SUBSAMPLE;
	airline->line[4].y = (ycenter - sin(iang - ma_angsupport) * masupport)/SUBSAMPLE;
	airline->line[5].x = (xcenter - cos(iang - mi_angsupport) * misupport)/SUBSAMPLE;
	airline->line[5].y = (ycenter + sin(iang - mi_angsupport) * misupport)/SUBSAMPLE;
	

	airline->line[6].x = (xcenter + cos(iang + ma_angsupport/2.) * (masupport + (airblob->main_line.size - masupport)/2.0))/SUBSAMPLE;
	airline->line[6].y = (ycenter - sin(iang + ma_angsupport/2.) * (masupport + (airblob->main_line.size - masupport)/2.0))/SUBSAMPLE;
	airline->line[7].x = (xcenter + cos(iang - ma_angsupport/2.) * (masupport + (airblob->main_line.size - masupport)/2.0))/SUBSAMPLE;
	airline->line[7].y = (ycenter - sin(iang - ma_angsupport/2.) * (masupport + (airblob->main_line.size - masupport)/2.0))/SUBSAMPLE;




	airline->nlines = 8;
       
    }


  else if ((direction < 0.0) && (direction > -M_PI_H))

    {


        masupport = hypot(airblob->maincross_line.dist, airblob->maincross_line.size/2);
	misupport = hypot(airblob->minorcross_line.dist,airblob->minorcross_line.size/2);

	ma_angsupport = atan(airblob->maincross_line.size/2/airblob->maincross_line.dist);
	mi_angsupport = atan(airblob->minorcross_line.size/2/airblob->minorcross_line.dist);
		
	iang = fabs(airblob->direction_abs);

        airline->line[0].x = (xcenter + cos(iang + ma_angsupport) * masupport)/SUBSAMPLE;
	airline->line[0].y = (ycenter + sin(iang + ma_angsupport) * masupport)/SUBSAMPLE;
	airline->line[1].x = (xcenter - cos(iang + mi_angsupport) * misupport)/SUBSAMPLE;
	airline->line[1].y = (ycenter - sin(iang + mi_angsupport) * misupport)/SUBSAMPLE;

	airline->line[2].x = (xcenter + cos(iang) * airblob->main_line.size)/SUBSAMPLE;
	airline->line[2].y = (ycenter + sin(iang) * airblob->main_line.size)/SUBSAMPLE;
	airline->line[3].x = (xcenter - cos(iang) * airblob->minor_line.size)/SUBSAMPLE;
	airline->line[3].y = (ycenter - sin(iang) * airblob->minor_line.size)/SUBSAMPLE;

	airline->line[4].x = (xcenter + cos(iang - ma_angsupport) * masupport)/SUBSAMPLE;
	airline->line[4].y = (ycenter + sin(iang - ma_angsupport) * masupport)/SUBSAMPLE;
	airline->line[5].x = (xcenter - cos(iang - mi_angsupport) * misupport)/SUBSAMPLE;
	airline->line[5].y = (ycenter - sin(iang - mi_angsupport) * misupport)/SUBSAMPLE;
	
	airline->nlines = 6;
       
    }


  else if ((direction < -M_PI_H) && (direction > -M_PI))

    {


        masupport = hypot(airblob->maincross_line.dist, airblob->maincross_line.size/2);
	misupport = hypot(airblob->minorcross_line.dist,airblob->minorcross_line.size/2);
	
	ma_angsupport = atan(airblob->maincross_line.size/2/airblob->maincross_line.dist);
	mi_angsupport = atan(airblob->minorcross_line.size/2/airblob->minorcross_line.dist);
	
	iang = fabs(airblob->direction_abs) - M_PI_H;

        airline->line[0].x = (xcenter - sin(iang + ma_angsupport) * masupport)/SUBSAMPLE;
	airline->line[0].y = (ycenter + cos(iang + ma_angsupport) * masupport)/SUBSAMPLE;
	airline->line[1].x = (xcenter + sin(iang + mi_angsupport) * misupport)/SUBSAMPLE;
	airline->line[1].y = (ycenter - cos(iang + mi_angsupport) * misupport)/SUBSAMPLE;

	airline->line[2].x = (xcenter - sin(iang) * airblob->main_line.size)/SUBSAMPLE;
	airline->line[2].y = (ycenter + cos(iang) * airblob->main_line.size)/SUBSAMPLE;
	airline->line[3].x = (xcenter + sin(iang) * airblob->minor_line.size)/SUBSAMPLE;
	airline->line[3].y = (ycenter - cos(iang) * airblob->minor_line.size)/SUBSAMPLE;

	airline->line[4].x = (xcenter - sin(iang - ma_angsupport) * masupport)/SUBSAMPLE;
	airline->line[4].y = (ycenter + cos(iang - ma_angsupport) * masupport)/SUBSAMPLE;
	airline->line[5].x = (xcenter + sin(iang - mi_angsupport) * misupport)/SUBSAMPLE;
	airline->line[5].y = (ycenter - cos(iang - mi_angsupport) * misupport)/SUBSAMPLE;
	
	airline->nlines = 6;
       
    }
 else
   {
     printf("Hmm a bug in the create_air_line");
   }


  min_x = max_x = airline->xcenter;
  min_y = max_y = airline->ycenter;



  for (i=0; i < airline->nlines ; i++)
    {
       min_x = MIN(airline->line[i].x, min_x);
       max_x = MAX(airline->line[i].x, max_x);
       min_y = MIN(airline->line[i].y, min_y);
       max_y = MAX(airline->line[i].y, max_y);
    }

  airline->width = max_x - min_x + 1;
  airline->height = max_y - min_y + 1;

  airline->min_x = min_x;
  airline->min_y = min_y;
  airline->max_x = max_x;
  airline->max_y = max_y;
  
  return airline;

}

AirBlob *
trans_air_blob(AirBlob *airblob_last, AirBlob *airblob_present, double dist, int xcen, int ycen)
{

  AirBlob *trans_airblob;

  double direction_last_abs, direction_present_abs;
  double idirection_abs;

  double direction_last, direction_present;
  double idirection;

  double main_line_present, main_line_last;


  double minor_line_present, minor_line_last;


  double maincross_line_dist_present, maincross_line_dist_last;

  double minorcross_line_dist_present, minorcross_line_dist_last;


  double maincross_line_size_present, maincross_line_size_last;

  double minorcross_line_size_present, minorcross_line_size_last;



  trans_airblob = airblob_new(1);

  direction_last_abs = airblob_last->direction_abs + M_PI;
  direction_present_abs = airblob_present->direction_abs + M_PI;

  idirection_abs = direction_present_abs - direction_last_abs;

  direction_last = airblob_last->direction + M_PI_H;
  direction_present = airblob_present->direction + M_PI_H;

  idirection = direction_present - direction_last;

  main_line_present = airblob_present->main_line.size;
  main_line_last = airblob_last->main_line.size;

  minor_line_present = airblob_present->minor_line.size;
  minor_line_last = airblob_last->minor_line.size;

  maincross_line_dist_present = airblob_present->maincross_line.dist;
  maincross_line_size_present = airblob_present->maincross_line.size;  
  minorcross_line_dist_present = airblob_present->minorcross_line.dist;
  minorcross_line_size_present = airblob_present->minorcross_line.size;  

  maincross_line_dist_last = airblob_last->maincross_line.dist;
  maincross_line_size_last = airblob_last->maincross_line.size;  
  minorcross_line_dist_last = airblob_last->minorcross_line.dist;
  minorcross_line_size_last = airblob_last->minorcross_line.size;  




  /* 
     Now we have to guess a little :-). Why? 
     Well we can't know if the users is painting
     up/down or if she is painting a circle at high speed.
     As you may notice it be so that the last airblob has 
     a direction more or less M_PI rad differernt from the
     present airblob. But we can't know if she tured the 
     airbrush quickly (so that there was no mouse capture 
     during the turn) or if she paints just up and down the
     same direction.

     There for we guess that we want to pait up and down 
     when the diff in direction is bigger than 171 deg and 
     smaller than 189 deg.

  */

  if ((fabs(idirection_abs) > (M_PI - 0.1571)) && (fabs(idirection_abs) < (M_PI + 0.1571)))
    {
      /* We asume that the artist meant to paint in a "strait line" by just tilting the airbrush*/
      
      idirection_abs = idirection_abs - M_PI;
     
      if ((idirection_abs * dist) > (idirection_abs/2))
	{
	  if ((direction_present_abs - idirection_abs * dist) > 2 * M_PI)
	    {
	    trans_airblob->direction_abs = direction_present_abs - idirection_abs * dist - 2 * M_PI - M_PI;
	    }
	  else if ((direction_present_abs - idirection_abs * dist) < 0.0)
	    {
	      trans_airblob->direction_abs = direction_present_abs - idirection_abs * dist + 2 * M_PI - M_PI;
	    }
	  else 
	    {
	      trans_airblob->direction_abs = direction_present_abs - idirection_abs * dist - M_PI;
	    }
	}
      else 
	{
	  if ((direction_present_abs + idirection_abs * dist) > 2 * M_PI)
	    {
	    trans_airblob->direction_abs = direction_present_abs + idirection_abs * dist - 2 * M_PI - M_PI;
	    }
	  else if ((direction_present_abs + idirection_abs * dist) < 0.0)
	    {
	      trans_airblob->direction_abs = direction_present_abs + idirection_abs * dist + 2 * M_PI - M_PI;
	    }
	  else 
	    {
	      trans_airblob->direction_abs = direction_present_abs + idirection_abs * dist - M_PI;
	    }
	}

      trans_airblob->main_line.size = main_line_last + ((minor_line_present - main_line_last) * dist); 
      trans_airblob->minor_line.size = minor_line_last + ((main_line_present - minor_line_last) * dist);

      trans_airblob->maincross_line.dist = maincross_line_dist_last + ((minorcross_line_dist_present - maincross_line_dist_last) * dist);
      trans_airblob->maincross_line.size = maincross_line_size_last + ((minorcross_line_size_present - maincross_line_size_last) * dist);

      trans_airblob->minorcross_line.dist = minorcross_line_dist_last + ((maincross_line_dist_present - minorcross_line_dist_last) * dist);
      trans_airblob->minorcross_line.size = minorcross_line_size_last + ((maincross_line_size_present - minorcross_line_size_last) * dist);

    }
  
  else if (fabs(idirection_abs) < (M_PI - 0.1571))
    {
      if ((direction_last_abs + idirection_abs * dist) > 2*M_PI)
	{
	  trans_airblob->direction_abs = direction_last_abs + idirection_abs * dist - 2 * M_PI - M_PI;
	}
      else if((direction_last_abs + idirection_abs * dist) < 0.0)
	{
	 trans_airblob->direction_abs = direction_last_abs + idirection_abs * dist + 2 * M_PI - M_PI;
	} 
      else 
	{
	  trans_airblob->direction_abs = direction_last_abs + idirection_abs * dist - M_PI;
	}

      trans_airblob->main_line.size = main_line_last + ((main_line_present - main_line_last) * dist); 
      trans_airblob->minor_line.size = minor_line_last + ((minor_line_present - minor_line_last) * dist);

      trans_airblob->maincross_line.dist = maincross_line_dist_last + ((maincross_line_dist_present - maincross_line_dist_last) * dist);
      trans_airblob->maincross_line.size = maincross_line_size_last + ((maincross_line_size_present - maincross_line_size_last) * dist);

      trans_airblob->minorcross_line.dist = minorcross_line_dist_last + ((minorcross_line_dist_present - minorcross_line_dist_last) * dist);
      trans_airblob->minorcross_line.size = minorcross_line_size_last + ((minorcross_line_size_present - minorcross_line_size_last) * dist);
    }
  else
    {

      /* We asume that the artist always travels the shortest way around the "clock" */
      
      idirection_abs = idirection_abs - M_PI;

      if ((direction_last_abs + idirection_abs * dist) > 2*M_PI)
	{
	  trans_airblob->direction_abs = direction_last_abs + idirection_abs * dist - 2 * M_PI - M_PI;
	}
      else if((direction_last_abs + idirection_abs * dist) < 0.0)
	{
	 trans_airblob->direction_abs = direction_last_abs + idirection_abs * dist + 2 * M_PI - M_PI;
	} 
      else 
	{
	  trans_airblob->direction_abs = direction_last_abs + idirection_abs * dist - M_PI;
	}

      trans_airblob->main_line.size = main_line_last + ((main_line_present - main_line_last) * dist); 
      trans_airblob->minor_line.size = minor_line_last + ((minor_line_present - minor_line_last) * dist);

      trans_airblob->maincross_line.dist = maincross_line_dist_last + ((maincross_line_dist_present - maincross_line_dist_last) * dist);
      trans_airblob->maincross_line.size = maincross_line_size_last + ((maincross_line_size_present - maincross_line_size_last) * dist);

      trans_airblob->minorcross_line.dist = minorcross_line_dist_last + ((minorcross_line_dist_present - minorcross_line_dist_last) * dist);
      trans_airblob->minorcross_line.size = minorcross_line_size_last + ((minorcross_line_size_present - minorcross_line_size_last) * dist);

    }


  trans_airblob->xcenter = xcen * SUBSAMPLE;
  trans_airblob->ycenter = ycen * SUBSAMPLE;


  return trans_airblob;

}

int
number_of_steps(int x0, int y0, int x1, int y1)
{


  int dx, dy;

  dx = abs(x0 - x1);
  
  dy = abs(y0 - y1);

  if (dy > dx)
    {
      return dy + 1;
    }
  else
    {
      return dx + 1;
    }
}
   
 


      
      
      
  







