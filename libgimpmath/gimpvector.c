/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpvector.c
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

#include "config.h"

#include <glib.h>

#include "gimpmath.h"


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
/* Two   dimensional vector functions */
/**************************************/

/**
 * gimp_vector2_inner_product:
 * @vector1: the first vector (by address),
 * @vector2: the second vector (by address).
 *
 * Computes the inner (dot) product of two 2D vectors.
 * This product is nul iff the two vectors are orthognal.
 *
 * Returns: The inner product.
 **/
gdouble
gimp_vector2_inner_product (const GimpVector2 *vector1,
                            const GimpVector2 *vector2)
{
  return (vector1->x * vector2->x + vector1->y * vector2->y);
}

/**
 * gimp_vector2_inner_product_val:
 * @vector1: the first vector (by value),
 * @vector2: the second vector (by value).
 *
 * Computes the inner (dot) product of two 2D vectors.
 * This product is nul iff the two vectors are orthognal.
 *
 * Returns: The inner product.
 **/
gdouble
gimp_vector2_inner_product_val (GimpVector2 vector1,
                                GimpVector2 vector2)
{
  return (vector1.x * vector2.x + vector1.y * vector2.y);
}

/**
 * gimp_vector2_cross_product:
 * @vector1: the first vector (by address)
 * @vector2: the second vector (by address)
 *
 * Compute the cross product of two vectors.
 * The result is a vector (a #GimpVector2) which is orthognal to both
 * of vector1 and vector2. If vector1 and vector2 and parallel, the
 * result will be the nul vector.
 *
 * Note that in 2D, this function is mostly useful to test if two
 * vectors are parallel or not, or to compute the area spawned by two
 * vectors.
 *
 * Returns: The cross product.
 **/
GimpVector2
gimp_vector2_cross_product (const GimpVector2 *vector1,
                            const GimpVector2 *vector2)
{
  GimpVector2 normal;

  normal.x = vector1->x * vector2->y - vector1->y * vector2->x;
  normal.y = vector1->y * vector2->x - vector1->x * vector2->y;

  return normal;
}

/**
 * gimp_vector2_cross_product_val:
 * @vector1: the first vector (by value)
 * @vector2: the second vector (by value)
 *
 * This function is mainly another implementation of
 * gimp_vector2_cross_product where the arguments are passed by value
 * rather than by address.
 *
 * Returns: The cross product.
 **/
GimpVector2
gimp_vector2_cross_product_val (GimpVector2 vector1,
                                GimpVector2 vector2)
{
  GimpVector2 normal;

  normal.x = vector1.x * vector2.y - vector1.y * vector2.x;
  normal.y = vector1.y * vector2.x - vector1.x * vector2.y;

  return normal;
}

/**
 * gimp_vector2_length:
 * @vector: a #GimpVector2 (by address)
 *
 * Computes the length of a 2D vector.
 *
 * Returns: the length of the given vector (a positive gdouble)
 **/
gdouble
gimp_vector2_length (const GimpVector2 *vector)
{
  return (sqrt (vector->x * vector->x + vector->y * vector->y));
}

/**
 * gimp_vector2_length_val:
 * @vector: a #GimpVector2 (by value)
 *
 * Computes the length of a 2D vector.
 *
 * Returns: the length of the given vector (a positive gdouble)
 **/
gdouble
gimp_vector2_length_val (GimpVector2 vector)
{
  return (sqrt (vector.x * vector.x + vector.y * vector.y));
}

/**
 * gimp_vector2_normalize:
 * @vector: a #GimpVector2 (by address)
 *
 * Normalizes the vector pointed by the argument, so the length of the
 * pointed vector will be 1.0 after this. The nul vector will not be changed,
 * though.
 **/
void
gimp_vector2_normalize (GimpVector2 *vector)
{
  gdouble len;

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

/**
 * gimp_vector2_normalize_val:
 * @vector: a #GimpVector2 (by value)
 *
 * Computes and returns the normalized vector corresponding with the one
 * passed in argument.
 *
 * Returns: a #GimpVector2 parallel to @vector, pointing in the same
 *          direction but with a length of 1.0.
 **/
GimpVector2
gimp_vector2_normalize_val (GimpVector2 vector)
{
  GimpVector2 normalized;
  gdouble     len;

  len = gimp_vector2_length_val (vector);

  if (len != 0.0)
    {
      len = 1.0 / len;
      normalized.x = vector.x * len;
      normalized.y = vector.y * len;
      return normalized;
    }
  else
    {
      return gimp_vector2_zero;
    }
}

/**
 * gimp_vector2_mul:
 * @vector: a #GimpVector2 (by address)
 * @factor: a scalar
 *
 * Multiplies each component of the @vector by @factor.
 * Note that the vector's length will be multiplied by @factor.
 **/
void
gimp_vector2_mul (GimpVector2 *vector,
                  gdouble      factor)
{
  vector->x *= factor;
  vector->y *= factor;
}

/**
 * gimp_vector2_mul_val:
 * @vector: a #GimpVector2 (by value)
 * @factor: a scalar.
 *
 * Computes and returns a #GimpVector2 pointing in the same direction
 * than @vector, but with a length multiplied by @factor.
 *
 * Returns: the resulting #GimpVector2.
 **/
GimpVector2
gimp_vector2_mul_val (GimpVector2 vector,
                      gdouble     factor)
{
  GimpVector2 result;

  result.x = vector.x * factor;
  result.y = vector.y * factor;

  return result;
}

/**
 * gimp_vector2_add:
 * @result: a placeholder for the resulting #GimpVector2
 * @vector1: a #GimpVector2 (by address)
 * @vector2: a #GimpVector2 (by address)
 *
 * Computes the sum of two 2D vectors.
 * The result is stored in the #GimpVector2 pointed by @result.
 **/
void
gimp_vector2_add (GimpVector2       *result,
                  const GimpVector2 *vector1,
                  const GimpVector2 *vector2)
{
  result->x = vector1->x + vector2->x;
  result->y = vector1->y + vector2->y;
}

/**
 * gimp_vector2_add_val:
 * @vector1: a #GimpVector2 (by value)
 * @vector2: a #GimpVector2 (by value)
 *
 * Computes and returns the sum of two 2D vectors.
 *
 * Returns: the resulting #GimpVector2.
 **/
GimpVector2
gimp_vector2_add_val (GimpVector2 vector1,
                      GimpVector2 vector2)
{
  GimpVector2 result;

  result.x = vector1.x + vector2.x;
  result.y = vector1.y + vector2.y;

  return result;
}

/**
 * gimp_vector2_sub:
 * @result: a placeholder for the resulting #GimpVector2
 * @vector1: a #GimpVector2 (by address)
 * @vector2: a #GimpVector2 (by address)
 *
 * Computes the difference of two 2D vectors (@vector1 minus @vector2).
 * The result is stored in the #GimpVector2 pointed by @result.
 **/
void
gimp_vector2_sub (GimpVector2       *result,
                  const GimpVector2 *vector1,
                  const GimpVector2 *vector2)
{
  result->x = vector1->x - vector2->x;
  result->y = vector1->y - vector2->y;
}

/**
 * gimp_vector2_sub_val:
 * @vector1: a #GimpVector2 (by value)
 * @vector2: a #GimpVector2 (by value)
 *
 * Computes and returns the difference of two 2D vectors (@vector1 minus
 * @vector2).
 *
 * Returns: the resulting #GimpVector2.
 **/
GimpVector2
gimp_vector2_sub_val (GimpVector2 vector1,
                      GimpVector2 vector2)
{
  GimpVector2 result;

  result.x = vector1.x - vector2.x;
  result.y = vector1.y - vector2.y;

  return result;
}

/**
 * gimp_vector2_set:
 * @vector: a #GimpVector2 (by address)
 * @x: a gdouble used as first coordinate
 * @y: a gdouble used as second coordinate
 *
 * Sets the first and second coordinates of @vector to @x and @y.
 **/
void
gimp_vector2_set (GimpVector2 *vector,
                  gdouble      x,
                  gdouble      y)
{
  vector->x = x;
  vector->y = y;
}

/**
 * gimp_vector2_new:
 * @x: a gdouble used as first coordinate
 * @y: a gdouble used as second coordinate
 *
 * Creates a #GimpVector2 of coordinate @x and @y.
 *
 * Returns: the resulting GimpVector2.
 **/
GimpVector2
gimp_vector2_new (gdouble x,
                  gdouble y)
{
  GimpVector2 vector;

  vector.x = x;
  vector.y = y;

  return vector;
}

/**
 * gimp_vector2_neg:
 * @vector: a #GimpVector2 (by address)
 *
 * Negates the @vector (i.e. negate all its coordinates).
 **/
void
gimp_vector2_neg (GimpVector2 *vector)
{
  vector->x *= -1.0;
  vector->y *= -1.0;
}

/**
 * gimp_vector2_neg_val:
 * @vector: a #GimpVector2 (by value)
 *
 * Computes and returns the negation of the @vector.
 *
 * Returns: the negated vector.
 **/
GimpVector2
gimp_vector2_neg_val (GimpVector2 vector)
{
  GimpVector2 result;

  result.x = vector.x * -1.0;
  result.y = vector.y * -1.0;

  return result;
}

/**
 * gimp_vector2_rotate:
 * @vector: a #GimpVector2 (by address)
 * @alpha: an angle (in radians)
 *
 * Rotates the @vector by @alpha radians, counterclockwize.
 **/
void
gimp_vector2_rotate (GimpVector2 *vector,
                     gdouble      alpha)
{
  GimpVector2 result;

  result.x = cos (alpha) * vector->x + sin (alpha) * vector->y;
  result.y = cos (alpha) * vector->y - sin (alpha) * vector->x;

  *vector = result;
}

/**
 * gimp_vector2_rotate_val:
 * @vector: a #GimpVector2 (by value)
 * @alpha: an angle (in radians)
 *
 * Computes and returns the rotation of the @vector by @alpha radians,
 * counterclockwize.
 *
 * Returns: the @vector rotated by @alpha radians.
 **/
GimpVector2
gimp_vector2_rotate_val (GimpVector2 vector,
                         gdouble     alpha)
{
  GimpVector2 result;

  result.x = cos (alpha) * vector.x + sin (alpha) * vector.y;
  result.y = cos (alpha) * vector.y - sin (alpha) * vector.x;

  return result;
}

/**************************************/
/* Three dimensional vector functions */
/**************************************/

/**
 * gimp_vector3_inner_product:
 * @vector1: the first #GimpVector3 (by address),
 * @vector2: the second #GimpVector3 (by address).
 *
 * Computes the inner (dot) product of two 23 vectors.
 * This product is nul iff the two vectors are orthognal.
 *
 * Returns: The inner product.
 **/
gdouble
gimp_vector3_inner_product (const GimpVector3 *vector1,
                            const GimpVector3 *vector2)
{
  return (vector1->x * vector2->x +
          vector1->y * vector2->y +
          vector1->z * vector2->z);
}

/**
 * gimp_vector3_inner_product_val:
 * @vector1: the first #GimpVector3 (by value),
 * @vector2: the second #GimpVector3 (by value).
 *
 * Computes the inner (dot) product of two 3D vectors.
 * This product is nul iff the two vectors are orthognal.
 *
 * Returns: The inner product.
 **/
gdouble
gimp_vector3_inner_product_val (GimpVector3 vector1,
                                GimpVector3 vector2)
{
  return (vector1.x * vector2.x +
          vector1.y * vector2.y +
          vector1.z * vector2.z);
}

/**
 * gimp_vector3_cross_product:
 * @vector1: the first #GimpVector3 (by address)
 * @vector2: the second #GimpVector3 (by address)
 *
 * Compute the cross product of two vectors.
 * The result is a #GimpVector3 which is orthognal to both
 * of vector1 and vector2. If vector1 and vector2 and parallel, the
 * result will be the nul vector.
 *
 * This function can be used to compute the normal of the plan defined by
 * @vector1 and @vector2.
 *
 * Returns: The cross product.
 **/
GimpVector3
gimp_vector3_cross_product (const GimpVector3 *vector1,
                            const GimpVector3 *vector2)
{
  GimpVector3 normal;

  normal.x = vector1->y * vector2->z - vector1->z * vector2->y;
  normal.y = vector1->z * vector2->x - vector1->x * vector2->z;
  normal.z = vector1->x * vector2->y - vector1->y * vector2->x;

  return normal;
}

/**
 * gimp_vector3_cross_product_val:
 * @vector1: the first #GimpVector3 (by value)
 * @vector2: the second #GimpVector3 (by value)
 *
 * This function is mainly another implementation of
 * #gimp_vector3_cross_product where the arguments are passed by value
 * rather than by address.
 *
 * Returns: The cross product.
 **/
GimpVector3
gimp_vector3_cross_product_val (GimpVector3 vector1,
                                GimpVector3 vector2)
{
  GimpVector3 normal;

  normal.x = vector1.y * vector2.z - vector1.z * vector2.y;
  normal.y = vector1.z * vector2.x - vector1.x * vector2.z;
  normal.z = vector1.x * vector2.y - vector1.y * vector2.x;

  return normal;
}

/**
 * gimp_vector3_length:
 * @vector: a #GimpVector3 (by address)
 *
 * Computes and returns the length of a 3D vector.
 *
 * Returns: the length of @vector.
 **/
gdouble
gimp_vector3_length (const GimpVector3 *vector)
{
  return (sqrt (vector->x * vector->x +
                vector->y * vector->y +
                vector->z * vector->z));
}

/**
 * gimp_vector3_length_val:
 * @vector: a #GimpVector3 (by value)
 *
 * Computes and returns the length of a 3D vector.
 *
 * Returns: the length of @vector.
 **/
gdouble
gimp_vector3_length_val (GimpVector3 vector)
{
  return (sqrt (vector.x * vector.x +
                vector.y * vector.y +
                vector.z * vector.z));
}

/**
 * gimp_vector3_normalize:
 * @vector: a #GimpVector3 (by address)
 *
 * Normalizes the vector pointed by @vector, so its length will be 1.0.
 * The nul vector will not be changed though.
 **/
void
gimp_vector3_normalize (GimpVector3 *vector)
{
  gdouble len;

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

/**
 * gimp_vector3_normalize_val:
 * @vector: a #GimpVector3 (by value)
 *
 * Computes and returns the normalized vector corresponding with the one
 * passed in argument.
 *
 * Returns: a #GimpVector2 parallel to @vector, pointing in the same
 *          direction but with a length of 1.0.
 **/
GimpVector3
gimp_vector3_normalize_val (GimpVector3 vector)
{
  GimpVector3 result;
  gdouble     len;

  len = gimp_vector3_length_val (vector);

  if (len != 0.0)
    {
      len = 1.0 / len;
      result.x = vector.x * len;
      result.y = vector.y * len;
      result.z = vector.z * len;
      return result;
    }
  else
    {
      return gimp_vector3_zero;
    }
}

/**
 * gimp_vector3_mul:
 * @vector: a #GimpVector3 (by address)
 * @factor: a scalar
 *
 * Multiplies each component of the @vector by @factor.
 * Note that this is equivalent to multiplied the length of @vector
 * by @factor.
 **/
void
gimp_vector3_mul (GimpVector3 *vector,
                  gdouble      factor)
{
  vector->x *= factor;
  vector->y *= factor;
  vector->z *= factor;
}

/**
 * gimp_vector3_mul_val:
 * @vector: a #GimpVector3 (by value)
 * @factor: a scalar.
 *
 * Computes and returns a #GimpVector3 pointing in the same direction
 * than @vector, but with a length multiplied by @factor.
 *
 * Returns: the resulting #GimpVector3.
 **/
GimpVector3
gimp_vector3_mul_val (GimpVector3 vector,
                      gdouble     factor)
{
  GimpVector3 result;

  result.x = vector.x * factor;
  result.y = vector.y * factor;
  result.z = vector.z * factor;

  return result;
}

/**
 * gimp_vector3_sub:
 * @result: a placeholder for the resulting #GimpVector3
 * @vector1: a #GimpVector3 (by address)
 * @vector2: a #GimpVector3 (by address)
 *
 * Computes the difference of two 3D vectors (@vector1 minus @vector2).
 * The result is stored in the #GimpVector3 pointed by @result.
 **/
void
gimp_vector3_sub (GimpVector3       *result,
                  const GimpVector3 *vector1,
                  const GimpVector3 *vector2)
{
  result->x = vector1->x - vector2->x;
  result->y = vector1->y - vector2->y;
  result->z = vector1->z - vector2->z;
}

/**
 * gimp_vector3_sub_val:
 * @vector1: a #GimpVector3 (by value)
 * @vector2: a #GimpVector3 (by value)
 *
 * Computes and returns the difference of two 3D vectors (@vector1 minus
 * @vector2).
 *
 * Returns: the resulting #GimpVector3.
 **/
GimpVector3
gimp_vector3_sub_val (GimpVector3 vector1,
                     GimpVector3 vector2)
{
  GimpVector3 result;

  result.x = vector1.x - vector2.x;
  result.y = vector1.y - vector2.y;
  result.z = vector1.z - vector2.z;

  return result;
}

/**
 * gimp_vector3_set:
 * @vector: a #GimpVector3 (by address)
 * @x: a gdouble used as first coordinate
 * @y: a gdouble used as second coordinate
 * @z: a gdouble used as third coordinate
 *
 * Sets the three coordinates of @vector to @x, @y and @z.
 **/
void
gimp_vector3_set (GimpVector3 *vector,
                  gdouble      x,
                  gdouble      y,
                  gdouble      z)
{
  vector->x = x;
  vector->y = y;
  vector->z = z;
}

/**
 * gimp_vector3_new:
 * @x: a gdouble used as first coordinate
 * @y: a gdouble used as second coordinate
 * @z: a gdouble used as third coordinate
 *
 * Creates a #GimpVector3 of coordinate @x, @y and @z.
 *
 * Returns: the resulting GimpVector3.
 **/
GimpVector3
gimp_vector3_new (gdouble  x,
                  gdouble  y,
                  gdouble  z)
{
  GimpVector3 vector;

  vector.x = x;
  vector.y = y;
  vector.z = z;

  return vector;
}

/**
 * gimp_vector3_add:
 * @result: a placeholder for the resulting #GimpVector3
 * @vector1: a #GimpVector3 (by address)
 * @vector2: a #GimpVector3 (by address)
 *
 * Computes the sum of two 3D vectors.
 * The result is stored in the #GimpVector3 pointed by @result.
 **/
void
gimp_vector3_add (GimpVector3       *result,
                  const GimpVector3 *vector1,
                  const GimpVector3 *vector2)
{
  result->x = vector1->x + vector2->x;
  result->y = vector1->y + vector2->y;
  result->z = vector1->z + vector2->z;
}

/**
 * gimp_vector3_add_val:
 * @vector1: a #GimpVector3 (by value)
 * @vector2: a #GimpVector3 (by value)
 *
 * Computes and returns the sum of two 3D vectors (@vector1 minus
 * @vector2).
 *
 * Returns: the resulting #GimpVector3.
 **/
GimpVector3
gimp_vector3_add_val (GimpVector3 vector1,
                      GimpVector3 vector2)
{
  GimpVector3 result;

  result.x = vector1.x + vector2.x;
  result.y = vector1.y + vector2.y;
  result.z = vector1.z + vector2.z;

  return result;
}

/**
 * gimp_vector3_neg:
 * @vector: a #GimpVector3 (by address)
 *
 * Negates the @vector (i.e. negate all its coordinates).
 **/
void
gimp_vector3_neg (GimpVector3 *vector)
{
  vector->x *= -1.0;
  vector->y *= -1.0;
  vector->z *= -1.0;
}

/**
 * gimp_vector3_neg_val:
 * @vector: a #GimpVector3 (by value)
 *
 * Computes and returns the negation of the @vector.
 *
 * Returns: the negated vector.
 **/
GimpVector3
gimp_vector3_neg_val (GimpVector3 vector)
{
  GimpVector3 result;

  result.x = vector.x * -1.0;
  result.y = vector.y * -1.0;
  result.z = vector.z * -1.0;

  return result;
}

/**
 * gimp_vector3_rotate:
 * @vector: a #GimpVector3 (by address)
 * @alpha: the angle (in radian) of rotation around the Z axis.
 * @beta: the angle (in radian) of rotation around the Y axis.
 * @gamma: the angle (in radian) of rotation around the X axis.
 *
 * Rotates the @vector around the three axis (Z, Y, and X) by
 * @alpha, @beta and @gamma, respectively.
 *
 * Note that the order of the rotation is very important.  If you expect
 * a vector to be rotated around X, and then around Y, you will have to
 * call this function twice.  Also, it is often wise to call this function
 * with only one of @alpha, @beta and @gamma non-nul.
 **/
void
gimp_vector3_rotate (GimpVector3 *vector,
                     gdouble      alpha,
                     gdouble      beta,
                     gdouble      gamma)
{
  GimpVector3 s, t;

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

/**
 * gimp_vector3_rotate_val:
 * @vector: a #GimpVector3 (by value)
 * @alpha: the angle (in radian) of rotation around the Z axis.
 * @beta: the angle (in radian) of rotation around the Y axis.
 * @gamma: the angle (in radian) of rotation around the X axis.
 *
 * Rotates the @vector around the three axis (Z, Y, and X) by
 * @alpha, @beta and @gamma, respectively, and return the resulting
 * vector.
 *
 * Note that the order of the rotation is very important.  If you expect
 * a vector to be rotated around X, and then around Y, you will have to
 * call this function twice. Also, it is often wise to call this function
 * with only one of @alpha, @beta and @gamma non-nul.
 *
 * Returns: the rotated vector.
 **/
GimpVector3
gimp_vector3_rotate_val (GimpVector3 vector,
                         gdouble     alpha,
                         gdouble     beta,
                         gdouble     gamma)
{
  GimpVector3 s, t, result;

  /* First we rotate it around the Z axis (XY plane).. */
  /* ================================================= */

  s.x = cos (alpha) * vector.x + sin (alpha) * vector.y;
  s.y = cos (alpha) * vector.y - sin (alpha) * vector.x;

  /* ..then around the Y axis (XZ plane).. */
  /* ===================================== */

  t = s;

  result.x = cos (beta) *t.x      + sin (beta) * vector.z;
  s.z      = cos (beta) *vector.z - sin (beta) * t.x;

  /* ..and at last around the X axis (YZ plane) */
  /* ========================================== */

  result.y = cos (gamma) * t.y + sin (gamma) * s.z;
  result.z = cos (gamma) * s.z - sin (gamma) * t.y;

  return result;
}

/**
 * gimp_vector_2d_to_3d:
 * @sx: the abscisse of the upper-left screen rectangle.
 * @sy: the ordinate of the upper-left screen rectangle.
 * @w: the width of the screen rectangle.
 * @h: the height of the screen rectangle.
 * @x: the abscisse of the point in the screen rectangle to map.
 * @y: the ordinate of the point in the screen rectangle to map.
 * @vp: position of the observer (by address).
 * @p: the resulting point (by address).
 *
 * \"Compute screen (sx,sy)-(sx+w,sy+h) to 3D unit square mapping.
 * The plane to map to is given in the z field of p. The observer
 * is located at position vp (vp->z!=0.0).\"
 *
 * In other words, this computes the projection of the point (@x ,@y) to
 * the plane z = @p->z (parallel to XY), from the @vp point of view through
 * the screen (@sx, @sy)->(@sx+@w, @sy+@h)
 **/

void
gimp_vector_2d_to_3d (gint               sx,
                      gint               sy,
                      gint               w,
                      gint               h,
                      gint               x,
                      gint               y,
                      const GimpVector3 *vp,
                      GimpVector3       *p)
{
  gdouble t = 0.0;

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

/**
 * gimp_vector_2d_to_3d_val:
 * @sx: the abscisse of the upper-left screen rectangle.
 * @sy: the ordinate of the upper-left screen rectangle.
 * @w: the width of the screen rectangle.
 * @h: the height of the screen rectangle.
 * @x: the abscisse of the point in the screen rectangle to map.
 * @y: the ordinate of the point in the screen rectangle to map.
 * @vp: position of the observer (by value).
 * @p: the resulting point (by value).
 *
 * This is mostly the same function as #gimp_vector_2d_to_3d, with
 * the position of the observer and the projecting plane given by value
 * rather than by address.  Also, the resulting point is returned.
 *
 * Returns: the computed #GimpVector3 point.
 **/
GimpVector3
gimp_vector_2d_to_3d_val (gint        sx,
                          gint        sy,
                          gint        w,
                          gint        h,
                          gint        x,
                          gint        y,
                          GimpVector3 vp,
                          GimpVector3 p)
{
  GimpVector3 result;
  gdouble     t = 0.0;

  if (vp.x != 0.0)
    t = (p.z - vp.z) / vp.z;

  if (t != 0.0)
    {
      result.x = vp.x + t * (vp.x - ((gdouble) (x - sx) / (gdouble) w));
      result.y = vp.y + t * (vp.y - ((gdouble) (y - sy) / (gdouble) h));
    }
  else
    {
      result.x = (gdouble) (x - sx) / (gdouble) w;
      result.y = (gdouble) (y - sy) / (gdouble) h;
    }

  return result;
}

/**
 * gimp_vector_3d_to_2d:
 * @sx: the abscisse of the upper-left screen rectangle.
 * @sy: the ordinate of the upper-left screen rectangle.
 * @w: the width of the screen rectangle.
 * @h: the height of the screen rectangle.
 * @x: the abscisse of the point in the screen rectangle to map (return value).
 * @y: the ordinate of the point in the screen rectangle to map (return value).
 * @vp: position of the observer (by address).
 * @p: the 3D point to project to the plane. (by address).
 *
 * Convert the given 3D point to 2D (project it onto the
 * viewing plane, (sx,sy,0)-(sx+w,sy+h,0). The input is
 * assumed to be in the unit square (0,0,z)-(1,1,z).
 * The viewpoint of the observer is passed in vp.
 *
 * This is basically the opposite of the #gimp_vector_2d_to_3d function.
 **/
void
gimp_vector_3d_to_2d (gint               sx,
                      gint               sy,
                      gint               w,
                      gint               h,
                      gdouble           *x,
                      gdouble           *y,
                      const GimpVector3 *vp,
                      const GimpVector3 *p)
{
  gdouble     t;
  GimpVector3 dir;

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
