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
/* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.               */
/***************************************************************************/

/**********************************************/
/* A little collection of useful vector stuff */
/**********************************************/

#include <stdlib.h>
#include <math.h>
#include <gck/gck.h>

/*************************/
/* Some useful constants */
/*************************/

const GckVector2 gck_vector2_zero = {0.0,0.0};
const GckVector2 gck_vector2_unit_x = {1.0,0.0};
const GckVector2 gck_vector2_unit_y = {0.0,1.0};

const GckVector3 gck_vector3_zero = {0.0,0.0,0.0};
const GckVector3 gck_vector3_unit_x = {1.0,0.0,0.0};
const GckVector3 gck_vector3_unit_y = {0.0,1.0,0.0};
const GckVector3 gck_vector3_unit_z = {0.0,0.0,1.0};

/**************************************/
/* Three dimensional vector functions */
/**************************************/

double gck_vector2_inner_product(GckVector2 *a,GckVector2 *b)
{
  g_function_enter("gck_vector2_inner_product");
  g_assert(a!=NULL);
  g_assert(b!=NULL);
  g_function_leave("gck_vector2_inner_product");
  return(a->x*b->x+a->y*b->y);
}

GckVector3 gck_vector2_cross_product(GckVector2 *a,GckVector2 *b)
{
  GckVector3 normal;

  g_function_enter("gck_vector2_cross_product");
  g_assert(a!=NULL);
  g_assert(b!=NULL);

/*  normal.x=a->y*b->z-a->z*b->y;
  normal.y=a->z*b->x-a->x*b->z;
  normal.z=a->x*b->y-a->y*b->x;*/

  g_function_leave("gck_vector2_cross_product");
  return(normal);
}

double gck_vector2_length(GckVector2 *a)
{
  g_function_enter("gck_vector2_length");
  g_assert(a!=NULL);
  g_function_leave("gck_vector2_length");
  return(sqrt(a->x*a->x+a->y*a->y));
}

void gck_vector2_normalize(GckVector2 *a)
{
  double len;
  
  g_function_enter("gck_vector2_normalize");
  g_assert(a!=NULL);

  len=gck_vector2_length(a);
  if (len!=0.0) 
    {
      len=1.0/len;
      a->x=a->x*len;
      a->y=a->y*len;
    }
  else *a=gck_vector2_zero;

  g_function_leave("gck_vector2_normalize");
}

void gck_vector2_mul(GckVector2 *a,double b)
{
  g_function_enter("gck_vector2_mul");
  g_assert(a!=NULL);

  a->x=a->x*b;
  a->y=a->y*b;

  g_function_leave("gck_vector2_mul");
}

void gck_vector2_sub(GckVector2 *c,GckVector2 *a,GckVector2 *b)
{
  g_function_enter("gck_vector2_sub");
  g_assert(a!=NULL);
  g_assert(b!=NULL);
  g_assert(c!=NULL);

  c->x=a->x-b->x;
  c->y=a->y-b->y;

  g_function_leave("gck_vector2_sub");
}

void gck_vector2_set(GckVector2 *a, double x,double y)
{
  g_function_enter("gck_vector2_set");
  g_assert(a!=NULL);

  a->x=x;
  a->y=y;

  g_function_leave("gck_vector2_set");
}

void gck_vector2_add(GckVector2 *c,GckVector2 *a,GckVector2 *b)
{
  g_function_enter("gck_vector2_add");
  g_assert(a!=NULL);
  g_assert(b!=NULL);
  g_assert(c!=NULL);

  c->x=a->x+b->x;
  c->y=a->y+b->y;

  g_function_leave("gck_vector2_add");
}

void gck_vector2_neg(GckVector2 *a)
{
  g_function_enter("gck_vector2_neg");
  g_assert(a!=NULL);

  a->x*=-1.0;
  a->y*=-1.0;

  g_function_leave("gck_vector2_neg");
}

void gck_vector2_rotate(GckVector2 *v,double alpha)
{
  GckVector2 s;

  g_function_enter("gck_vector2_rotate");
  g_assert(v!=NULL);

  s.x=cos(alpha)*v->x+sin(alpha)*v->y;
  s.y=cos(alpha)*v->y-sin(alpha)*v->x;
  
  *v=s;

  g_function_leave("gck_vector2_rotate");
}

/**************************************/
/* Three dimensional vector functions */
/**************************************/

double gck_vector3_inner_product(GckVector3 *a,GckVector3 *b)
{
  g_function_enter("gck_vector3_inner_product");
  g_assert(a!=NULL);
  g_assert(b!=NULL);
  g_function_leave("gck_vector3_inner_product");

  return(a->x*b->x+a->y*b->y+a->z*b->z);
}

GckVector3 gck_vector3_cross_product(GckVector3 *a,GckVector3 *b)
{
  GckVector3 normal;

  g_function_enter("gck_vector3_cross_product");
  g_assert(a!=NULL);
  g_assert(b!=NULL);

  normal.x=a->y*b->z-a->z*b->y;
  normal.y=a->z*b->x-a->x*b->z;
  normal.z=a->x*b->y-a->y*b->x;

  g_function_leave("gck_vector3_cross_product");
  return(normal);
}

double gck_vector3_length(GckVector3 *a)
{
  g_function_enter("gck_vector3_length");
  g_assert(a!=NULL);
  g_function_leave("gck_vector3_length");
  return(sqrt(a->x*a->x+a->y*a->y+a->z*a->z));
}

void gck_vector3_normalize(GckVector3 *a)
{
  double len;
  
  g_function_enter("gck_vector3_normalize");
  g_assert(a!=NULL);

  len=gck_vector3_length(a);
  if (len!=0.0)
    {
      len=1.0/len;
      a->x=a->x*len;
      a->y=a->y*len;
      a->z=a->z*len;
    }
  else *a=gck_vector3_zero;

  g_function_leave("gck_vector3_normalize");
}

void gck_vector3_mul(GckVector3 *a,double b)
{
  g_function_enter("gck_vector3_mul");
  g_assert(a!=NULL);

  a->x=a->x*b;
  a->y=a->y*b;
  a->z=a->z*b;

  g_function_leave("gck_vector3_mul");
}

void gck_vector3_sub(GckVector3 *c,GckVector3 *a,GckVector3 *b)
{
  g_function_enter("gck_vector3_sub");
  g_assert(a!=NULL);
  g_assert(b!=NULL);
  g_assert(c!=NULL);

  c->x=a->x-b->x;
  c->y=a->y-b->y;
  c->z=a->z-b->z;

  g_function_leave("gck_vector3_sub");
}

void gck_vector3_set(GckVector3 *a, double x,double y,double z)
{
  g_function_enter("gck_vector3_set");
  g_assert(a!=NULL);

  a->x=x;
  a->y=y;
  a->z=z;

  g_function_leave("gck_vector3_set");
}

void gck_vector3_add(GckVector3 *c,GckVector3 *a,GckVector3 *b)
{
  g_function_enter("gck_vector3_add");
  g_assert(a!=NULL);
  g_assert(b!=NULL);
  g_assert(c!=NULL);

  c->x=a->x+b->x;
  c->y=a->y+b->y;
  c->z=a->z+b->z;

  g_function_leave("gck_vector3_add");
}

void gck_vector3_neg(GckVector3 *a)
{
  g_function_enter("gck_vector3_neg");
  g_assert(a!=NULL);

  a->x*=-1.0;
  a->y*=-1.0;
  a->z*=-1.0;

  g_function_leave("gck_vector3_neg");
}

void gck_vector3_rotate(GckVector3 *v,double alpha,double beta,double gamma)
{
  GckVector3 s,t;

  g_function_enter("gck_vector3_rotate");
  g_assert(v!=NULL);

  /* First we rotate it around the Z axis (XY plane).. */
  /* ================================================= */

  s.x=cos(alpha)*v->x+sin(alpha)*v->y;
  s.y=cos(alpha)*v->y-sin(alpha)*v->x;

  /* ..then around the Y axis (XZ plane).. */
  /* ===================================== */

  t=s;
    
  v->x=cos(beta)*t.x+sin(beta)*v->z;
  s.z=cos(beta)*v->z-sin(beta)*t.x;

  /* ..and at last around the X axis (YZ plane) */
  /* ========================================== */
  
  v->y=cos(gamma)*t.y+sin(gamma)*s.z;
  v->z=cos(gamma)*s.z-sin(gamma)*t.y;

  g_function_leave("gck_vector3_rotate");
}

/******************************************************************/
/* Compute screen (sx,sy)-(sx+w,sy+h) to 3D unit square mapping.  */
/* The plane to map to is given in the z field of p. The observer */
/* is located at position vp (vp->z!=0.0).                        */
/******************************************************************/

void gck_2d_to_3d(int sx,int sy,int w,int h,int x,int y,GckVector3 *vp,GckVector3 *p)
{
  double t=0.0;

  g_function_enter("gck_2d_to_3d");
  g_assert(vp!=NULL);
  g_assert(p!=NULL);

  if (vp->x!=0.0) t=(p->z-vp->z)/vp->z;

  if (t!=0.0)
    {
      p->x=vp->x+t*(vp->x-((double)(x-sx)/(double)w));
      p->y=vp->y+t*(vp->y-((double)(y-sy)/(double)h));
    }
  else
    {
      p->x=(double)(x-sx)/(double)w;
      p->y=(double)(y-sy)/(double)h;     
    }

  g_function_leave("gck_2d_to_3d");
}

/*********************************************************/
/* Convert the given 3D point to 2D (project it onto the */
/* viewing plane, (sx,sy,0)-(sx+w,sy+h,0). The input is  */
/* assumed to be in the unit square (0,0,z)-(1,1,z).     */
/* The viewpoint of the observer is passed in vp.        */
/*********************************************************/

void gck_3d_to_2d(int sx,int sy,int w,int h,double *x,double *y,GckVector3 *vp,GckVector3 *p)
{
  double t;
  GckVector3 dir;

  g_function_enter("gck_3d_to_2d");
  g_assert(vp!=NULL);
  g_assert(p!=NULL);

  gck_vector3_sub(&dir,p,vp);
  gck_vector3_normalize(&dir);
  
  if (dir.z!=0.0)
    {
      t=(-1.0*vp->z)/dir.z;
      *x=(double)sx+((vp->x+t*dir.x)*(double)w);
      *y=(double)sy+((vp->y+t*dir.y)*(double)h);
    }
  else
    {
      *x=(double)sx+(p->x*(double)w);
      *y=(double)sy+(p->y*(double)h);
    }

  g_function_leave("gck_3d_to_2d");
}

