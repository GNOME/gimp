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

static const GimpVector2 gimp_vector2_zero =   { 0.0, 0.0 };
static const GimpVector2 gimp_vector2_unit_x = { 1.0, 0.0 };
static const GimpVector2 gimp_vector2_unit_y = { 0.0, 1.0 };

static const GimpVector3 gimp_vector3_zero =   { 0.0, 0.0, 0.0 };
static const GimpVector3 gimp_vector3_unit_x = { 1.0, 0.0, 0.0 };
static const GimpVector3 gimp_vector3_unit_y = { 0.0, 1.0, 0.0 };
static const GimpVector3 gimp_vector3_unit_z = { 0.0, 0.0, 1.0 };

/**************************************/
/* Three dimensional vector functions */
/**************************************/

gdouble
gimp_vector2_inner_product (GimpVector2 *vector1,
			    GimpVector2 *vector2)
{
  g_assert (vector1 != NULL);
  g_assert (vector2 != NULL);

  return (vector1->x * vector2->x + vector1->y * vector2->y);
}

GimpVector2
gimp_vector2_cross_product (GimpVector2 *vector1,
			    GimpVector2 *vector2)
{
  GimpVector2 normal;

  g_assert (vector1 != NULL);
  g_assert (vector2 != NULL);

  normal.x = vector1->x * vector2->y - vector1->y * vector2->x;
  normal.y = vector1->y * vector2->x - vector1->x * vector2->y;

  return normal;
}

gdouble
gimp_vector2_length (GimpVector2 *vector)
{
  g_assert (vector != NULL);

  return (sqrt (vector->x * vector->x + vector->y * vector->y));
}

void
gimp_vector2_normalize (GimpVector2 *vector)
{
  gdouble len;

  g_assert (vector != NULL);

  len = gimp_vector2_length (vector);
  if (len != 0.0) 
    {
      len = 1.0 / len;
      vector->x *= len;
      vector->y *= len;
    }
  else
    {
      *vector = gimp_vector2_zero;
    }
}

void
gimp_vector2_mul (GimpVector2 *vector,
		  gdouble      factor)
{
  g_assert (vector != NULL);

  vector->x *= factor;
  vector->y *= factor;
}

void
gimp_vector2_sub (GimpVector2 *result,
		  GimpVector2 *vector1,
		  GimpVector2 *vector2)
{
  g_assert (vector1 != NULL);
  g_assert (vector2 != NULL);
  g_assert (result != NULL);

  result->x = vector1->x - vector2->x;
  result->y = vector1->y - vector2->y;
}

void
gimp_vector2_set (GimpVector2 *vector,
		  gdouble      x,
		  gdouble      y)
{
  g_assert (vector != NULL);

  vector->x = x;
  vector->y = y;
}

void
gimp_vector2_add (GimpVector2 *result,
		  GimpVector2 *vector1,
		  GimpVector2 *vector2)
{
  g_assert (vector1 != NULL);
  g_assert (vector2 != NULL);
  g_assert (result != NULL);

  result->x = vector1->x + vector2->x;
  result->y = vector1->y + vector2->y;
}

void
gimp_vector2_neg (GimpVector2 *vector)
{
  g_assert (vector != NULL);

  vector->x *= -1.0;
  vector->y *= -1.0;
}

void
gimp_vector2_rotate (GimpVector2 *vector,
		     gdouble      alpha)
{
  GimpVector2 result;

  g_assert (vector != NULL);

  result.x = cos (alpha) * vector->x + sin (alpha) * vector->y;
  result.y = cos (alpha) * vector->y - sin (alpha) * vector->x;

  *vector = result;
}

/**************************************/
/* Three dimensional vector functions */
/**************************************/

gdouble
gimp_vector3_inner_product (GimpVector3 *vector1,
			    GimpVector3 *vector2)
{
  g_assert (vector1 != NULL);
  g_assert (vector2 != NULL);

  return (vector1->x * vector2->x +
	  vector1->y * vector2->y +
	  vector1->z * vector2->z);
}

GimpVector3
gimp_vector3_cross_product (GimpVector3 *vector1,
			    GimpVector3 *vector2)
{
  GimpVector3 normal;

  g_assert (vector1 != NULL);
  g_assert (vector2 != NULL);

  normal.x = vector1->y * vector2->z - vector1->z * vector2->y;
  normal.y = vector1->z * vector2->x - vector1->x * vector2->z;
  normal.z = vector1->x * vector2->y - vector1->y * vector2->x;

  return normal;
}

gdouble
gimp_vector3_length (GimpVector3 *vector)
{
  g_assert (vector != NULL);

  return (sqrt (vector->x * vector->x +
		vector->y * vector->y +
		vector->z * vector->z));
}

void
gimp_vector3_normalize (GimpVector3 *vector)
{
  gdouble len;

  g_assert (vector != NULL);

  len = gimp_vector3_length (vector);
  if (len != 0.0)
    {
      len = 1.0 / len;
      vector->x *= len;
      vector->y *= len;
      vector->z *= len;
    }
  else
    {
      *vector = gimp_vector3_zero;
    }
}

void
gimp_vector3_mul (GimpVector3 *vector,
		  gdouble      factor)
{
  g_assert (vector != NULL);

  vector->x *= factor;
  vector->y *= factor;
  vector->z *= factor;
}

void
gimp_vector3_sub (GimpVector3 *result,
		  GimpVector3 *vector1,
		  GimpVector3 *vector2)
{
  g_assert (vector1 != NULL);
  g_assert (vector2 != NULL);
  g_assert (result != NULL);

  result->x = vector1->x - vector2->x;
  result->y = vector1->y - vector2->y;
  result->z = vector1->z - vector2->z;
}

void
gimp_vector3_set (GimpVector3 *vector,
		  gdouble      x,
		  gdouble      y,
		  gdouble      z)
{
  g_assert (vector != NULL);

  vector->x = x;
  vector->y = y;
  vector->z = z;
}

void
gimp_vector3_add (GimpVector3 *result,
		  GimpVector3 *vector1,
		  GimpVector3 *vector2)
{
  g_assert (vector1 != NULL);
  g_assert (vector2 != NULL);
  g_assert (result != NULL);

  result->x = vector1->x + vector2->x;
  result->y = vector1->y + vector2->y;
  result->z = vector1->z + vector2->z;
}

void
gimp_vector3_neg (GimpVector3 *vector)
{
  g_assert (vector != NULL);

  vector->x *= -1.0;
  vector->y *= -1.0;
  vector->z *= -1.0;
}

void
gimp_vector3_rotate (GimpVector3 *vector,
		     gdouble      alpha,
		     gdouble      beta,
		     gdouble      gamma)
{
  GimpVector3 s, t;

  g_assert (vector != NULL);

  /* First we rotate it around the Z axis (XY plane).. */
  /* ================================================= */

  s.x = cos (alpha) * vector->x + sin (alpha) * vector->y;
  s.y = cos (alpha) * vector->y - sin (alpha) * vector->x;

  /* ..then around the Y axis (XZ plane).. */
  /* ===================================== */

  t = s;

  vector->x = cos (beta) *t.x       + sin (beta) * vector->z;
  s.z       = cos (beta) *vector->z - sin (beta) * t.x;

  /* ..and at last around the X axis (YZ plane) */
  /* ========================================== */

  vector->y = cos (gamma) * t.y + sin (gamma) * s.z;
  vector->z = cos (gamma) * s.z - sin (gamma) * t.y;
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
