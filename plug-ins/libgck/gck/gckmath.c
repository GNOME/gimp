/***************************************************************************/
/* GCK - The General Convenience Kit. Generally useful conveniece routines */
/* for GIMP plug-in writers and users of the GDK/GTK libraries.            */
/* Copyright (C) 1996 Tom Bech                                             */
/*                                                                         */
/* This program is free software; you can redistribute it and/or modify    */
/* it under the terms of the GNU General Public License as published by    */
/* the Free Software Foundation; either version 2 of the License, or       */
/* (at your option) any later version.                                     */
/*                                                                         */
/* This program is distributed in the hope that it will be useful,         */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of          */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           */
/* GNU General Public License for more details.                            */
/*                                                                         */
/* You should have received a copy of the GNU General Public License       */
/* along with this program; if not, write to the Free Software             */
/* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,   */
/* USA.                                                                    */
/***************************************************************************/

/***************************/
/* Misc. useful math stuff */
/***************************/

#include <math.h>
#include <gck.h>

double gck_deg_to_rad(double angle)
{
  return((angle/360.0)*(2.0*M_PI));
}

double gck_rad_to_deg(double angle)
{
  return((angle/(2.0*M_PI))*360.0);
}

void gck_mat_to_deg(double mat[4][4],double *a,double *b,double *c)
{
  *a=180*(asin(mat[1][0])/M_PI_2);
  *b=180*(asin(mat[2][0])/M_PI_2);
  *c=180*(asin(mat[2][1])/M_PI_2);
}

/*************************************************/
/* Quick and dirty (and slow) line clip routine. */
/* The function returns FALSE if the line isn't  */
/* visible according to the limits given.        */
/*************************************************/

int gck_clip_line(double *x1,double *y1,double *x2,double *y2,
                  double minx,double miny,double maxx,double maxy)
{
  double tmp;

  g_function_enter("gck_clip_line");
  g_assert(x1!=NULL);
  g_assert(y1!=NULL);
  g_assert(x2!=NULL);
  g_assert(y2!=NULL);

  /* First, check if line is visible at all */
  /* ====================================== */

  if (*x1<minx && *x2<minx) return(FALSE);
  if (*x1>maxx && *x2>maxx) return(FALSE);
  if (*y1<miny && *y2<miny) return(FALSE);
  if (*y1>maxy && *y2>maxy) return(FALSE);

  /* Check for intersection with the four edges. Sort on x first. */
  /* ============================================================ */

  if (*x2<*x1)
    {
      tmp=*x1;
      *x1=*x2;
      *x2=tmp;
      tmp=*y1;
      *y1=*y2;
      *y2=tmp;
    }

  if (*x1<minx)
    {
      if (*y1<*y2) *y1=*y1+(minx-*x1)*((*y2-*y1)/(*x2-*x1));
      else         *y1=*y1-(minx-*x1)*((*y1-*y2)/(*x2-*x1));
      *x1=minx; 
    }

  if (*x2>maxx)
    {
      if (*y1<*y2) *y2=*y2-(*x2-maxx)*((*y2-*y1)/(*x2-*x1));
      else         *y2=*y2+(*x2-maxx)*((*y1-*y2)/(*x2-*x1));
      *x2=maxx;
    }

  if (*y1<miny)
    {
      *x1=*x1+(miny-*y1)*((*x2-*x1)/(*y2-*y1));
      *y1=miny;
    }
  
  if (*y2<miny)
    {
      *x2=*x2-(miny-*y2)*((*x2-*x1)/(*y1-*y2));
      *y2=miny;
    }

  if (*y1>maxy)
    {
      *x1=*x1+(*y1-maxy)*((*x2-*x1)/(*y1-*y2));
      *y1=maxy;
    }
    
  if (*y2>maxy)
    {
      *x2=*x2-(*y2-maxy)*((*x2-*x1)/(*y2-*y1));
      *y2=maxy;
    }

  g_function_leave("gck_clip_line");
  return(TRUE);
}

