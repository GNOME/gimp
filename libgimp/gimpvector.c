/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * The gimp_vector* functions were taken from:
 * GCK - The General Convenience Kit
 * Copyright (C) 1996 Tom Bech
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/**********************************************/
/* A little collection of useful vector stuff */
/**********************************************/

#include "gimpmath.h"
#include "gimpvector.h"

/*************************/
/* Some useful constants */
/*************************/

const GimpVector2 gimp_vector2_zero =   { 0.0, 0.0 };
const GimpVector2 gimp_vector2_unit_x = { 1.0, 0.0 };
const GimpVector2 gimp_vector2_unit_y = { 0.0, 1.0 };

const GimpVector3 gimp_vector3_zero =   { 0.0, 0.0, 0.0 };
const GimpVector3 gimp_vector3_unit_x = { 1.0, 0.0, 0.0 };
const GimpVector3 gimp_vector3_unit_y = { 0.0, 1.0, 0.0 };
const GimpVector3 gimp_vector3_unit_z = { 0.0, 0.0, 1.0 };

/**************************************/
/* Three dimensional vector functions */
/**************************************/

gdouble
gimp_vector2_inner_product (GimpVector2 *a,
			    GimpVector2 *b)
{
  g_assert (a != NULL);
  g_assert (b != NULL);

  return (a->x * b->x + a->y * b->y);
}

GimpVector2
gimp_vector2_cross_product (GimpVector2 *a,
			    GimpVector2 *b)
{
  GimpVector2 normal;

  g_assert (a != NULL);
  g_assert (b != NULL);

  normal.x = a->x * b->y - a->y * b->x;
  normal.y = a->y * b->x - a->x * b->y;

  return normal;
}

gdouble
gimp_vector2_length (GimpVector2 *a)
{
  g_assert (a != NULL);

  return (sqrt (a->x * a->x + a->y * a->y));
}

void
gimp_vector2_normalize (GimpVector2 *a)
{
  gdouble len;

  g_assert (a != NULL);

  len = gimp_vector2_length (a);
  if (len != 0.0) 
    {
      len = 1.0 / len;
      a->x = a->x * len;
      a->y = a->y * len;
    }
  else
    {
      *a = gimp_vector2_zero;
    }
}

void
gimp_vector2_mul (GimpVector2 *a,
		  double       b)
{
  g_assert (a != NULL);

  a->x = a->x * b;
  a->y = a->y * b;
}

void
gimp_vector2_sub (GimpVector2 *c,
		  GimpVector2 *a,
		  GimpVector2 *b)
{
  g_assert (a != NULL);
  g_assert (b != NULL);
  g_assert (c != NULL);

  c->x = a->x - b->x;
  c->y = a->y - b->y;
}

void
gimp_vector2_set (GimpVector2 *a,
		  gdouble      x,
		  gdouble      y)
{
  g_assert (a != NULL);

  a->x = x;
  a->y = y;
}

void
gimp_vector2_add (GimpVector2 *c,
		  GimpVector2 *a,
		  GimpVector2 *b)
{
  g_assert (a != NULL);
  g_assert (b != NULL);
  g_assert (c != NULL);

  c->x = a->x + b->x;
  c->y = a->y + b->y;
}

void
gimp_vector2_neg (GimpVector2 *a)
{
  g_assert (a != NULL);

  a->x *= -1.0;
  a->y *= -1.0;
}

void
gimp_vector2_rotate (GimpVector2 *v,
		     gdouble      alpha)
{
  GimpVector2 s;

  g_assert (v != NULL);

  s.x = cos (alpha) * v->x + sin (alpha) * v->y;
  s.y = cos (alpha) * v->y - sin (alpha) * v->x;

  *v = s;
}

/**************************************/
/* Three dimensional vector functions */
/**************************************/

gdouble
gimp_vector3_inner_product (GimpVector3 *a,
			    GimpVector3 *b)
{
  g_assert (a != NULL);
  g_assert (b != NULL);

  return (a->x * b->x + a->y * b->y + a->z * b->z);
}

GimpVector3
gimp_vector3_cross_product (GimpVector3 *a,
			    GimpVector3 *b)
{
  GimpVector3 normal;

  g_assert (a != NULL);
  g_assert (b != NULL);

  normal.x = a->y * b->z - a->z * b->y;
  normal.y = a->z * b->x - a->x * b->z;
  normal.z = a->x * b->y - a->y * b->x;

  return normal;
}

gdouble
gimp_vector3_length (GimpVector3 *a)
{
  g_assert (a != NULL);

  return (sqrt (a->x * a->x + a->y * a->y + a->z * a->z));
}

void
gimp_vector3_normalize (GimpVector3 *a)
{
  gdouble len;

  g_assert (a != NULL);

  len = gimp_vector3_length (a);
  if (len != 0.0)
    {
      len = 1.0 / len;
      a->x = a->x * len;
      a->y = a->y * len;
      a->z = a->z * len;
    }
  else
    {
      *a = gimp_vector3_zero;
    }
}

void
gimp_vector3_mul (GimpVector3 *a,
		  gdouble      b)
{
  g_assert (a != NULL);

  a->x = a->x * b;
  a->y = a->y * b;
  a->z = a->z * b;
}

void
gimp_vector3_sub (GimpVector3 *c,
		  GimpVector3 *a,
		  GimpVector3 *b)
{
  g_assert (a != NULL);
  g_assert (b != NULL);
  g_assert (c != NULL);

  c->x = a->x - b->x;
  c->y = a->y - b->y;
  c->z = a->z - b->z;
}

void
gimp_vector3_set (GimpVector3 *a,
		  gdouble      x,
		  gdouble      y,
		  gdouble      z)
{
  g_assert (a != NULL);

  a->x = x;
  a->y = y;
  a->z = z;
}

void
gimp_vector3_add (GimpVector3 *c,
		  GimpVector3 *a,
		  GimpVector3 *b)
{
  g_assert (a != NULL);
  g_assert (b != NULL);
  g_assert (c != NULL);

  c->x = a->x + b->x;
  c->y = a->y + b->y;
  c->z = a->z + b->z;
}

void
gimp_vector3_neg (GimpVector3 *a)
{
  g_assert (a != NULL);

  a->x *= -1.0;
  a->y *= -1.0;
  a->z *= -1.0;
}

void
gimp_vector3_rotate (GimpVector3 *v,
		     gdouble      alpha,
		     gdouble      beta,
		     gdouble      gamma)
{
  GimpVector3 s, t;

  g_assert (v != NULL);

  /* First we rotate it around the Z axis (XY plane).. */
  /* ================================================= */

  s.x = cos (alpha) * v->x + sin (alpha) * v->y;
  s.y = cos (alpha) * v->y - sin (alpha) * v->x;

  /* ..then around the Y axis (XZ plane).. */
  /* ===================================== */

  t = s;

  v->x = cos (beta) *t.x  + sin (beta) * v->z;
  s.z  = cos (beta) *v->z - sin (beta) * t.x;

  /* ..and at last around the X axis (YZ plane) */
  /* ========================================== */

  v->y = cos (gamma) * t.y + sin (gamma) * s.z;
  v->z = cos (gamma) * s.z - sin (gamma) * t.y;
}

/******************************************************************/
/* Compute screen (sx,sy)-(sx+w,sy+h) to 3D unit square mapping.  */
/* The plane to map to is given in the z field of p. The observer */
/* is located at position vp (vp->z!=0.0).                        */
/******************************************************************/

void
gimp_vector_2d_to_3d (gint         sx,
		      gint         sy,
		      gint         w,
		      gint         h,
		      gint         x,
		      gint         y,
		      GimpVector3 *vp,
		      GimpVector3 *p)
{
  gdouble t = 0.0;

  g_assert (vp != NULL);
  g_assert (p != NULL);

  if (vp->x != 0.0)
    t = (p->z - vp->z) / vp->z;

  if (t != 0.0)
    {
      p->x = vp->x + t * (vp->x - ((gdouble) (x - sx) / (gdouble) w));
      p->y = vp->y + t * (vp->y - ((gdouble) (y - sy) / (gdouble) h));
    }
  else
    {
      p->x = (gdouble) (x - sx) / (gdouble) w;
      p->y = (gdouble) (y - sy) / (gdouble) h;     
    }
}

/*********************************************************/
/* Convert the given 3D point to 2D (project it onto the */
/* viewing plane, (sx,sy,0)-(sx+w,sy+h,0). The input is  */
/* assumed to be in the unit square (0,0,z)-(1,1,z).     */
/* The viewpoint of the observer is passed in vp.        */
/*********************************************************/

void
gimp_vector_3d_to_2d (gint         sx,
		      gint         sy,
		      gint         w,
		      gint         h,
		      gdouble     *x,
		      gdouble     *y,
		      GimpVector3 *vp,
		      GimpVector3 *p)
{
  gdouble t;
  GimpVector3 dir;

  g_assert (vp != NULL);
  g_assert (p != NULL);

  gimp_vector3_sub (&dir, p, vp);
  gimp_vector3_normalize (&dir);

  if (dir.z != 0.0)
    {
      t = (-1.0 * vp->z) / dir.z;
      *x = (gdouble) sx + ((vp->x + t * dir.x) * (gdouble) w);
      *y = (gdouble) sy + ((vp->y + t * dir.y) * (gdouble) h);
    }
  else
    {
      *x = (gdouble) sx + (p->x * (gdouble) w);
      *y = (gdouble) sy + (p->y * (gdouble) h);
    }
}
