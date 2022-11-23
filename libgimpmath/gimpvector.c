/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmavector.c
 *
 * The ligma_vector* functions were taken from:
 * GCK - The General Convenience Kit
 * Copyright (C) 1996 Tom Bech
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

/**********************************************/
/* A little collection of useful vector stuff */
/**********************************************/

#include "config.h"

#include <glib-object.h>

#include "ligmamath.h"


/**
 * SECTION: ligmavector
 * @title: LigmaVector
 * @short_description: Utilities to set up and manipulate vectors.
 * @see_also: #LigmaMatrix2, #LigmaMatrix3, #LigmaMatrix4
 *
 * Utilities to set up and manipulate vectors.
 **/


static gpointer  ligma_vector2_copy (gpointer boxed);
static void      ligma_vector2_free (gpointer boxed);
static gpointer  ligma_vector3_copy (gpointer boxed);
static void      ligma_vector3_free (gpointer boxed);


/*************************/
/* Some useful constants */
/*************************/

static const LigmaVector2 ligma_vector2_zero =   { 0.0, 0.0 };
#if 0
static const LigmaVector2 ligma_vector2_unit_x = { 1.0, 0.0 };
static const LigmaVector2 ligma_vector2_unit_y = { 0.0, 1.0 };
#endif

static const LigmaVector3 ligma_vector3_zero =   { 0.0, 0.0, 0.0 };
#if 0
static const LigmaVector3 ligma_vector3_unit_x = { 1.0, 0.0, 0.0 };
static const LigmaVector3 ligma_vector3_unit_y = { 0.0, 1.0, 0.0 };
static const LigmaVector3 ligma_vector3_unit_z = { 0.0, 0.0, 1.0 };
#endif

/**************************************/
/* Two   dimensional vector functions */
/**************************************/

/**
 * ligma_vector2_new:
 * @x: the X coordinate.
 * @y: the Y coordinate.
 *
 * Creates a [struct@Vector2] of coordinates @x and @y.
 *
 * Returns: the resulting vector
 **/
LigmaVector2
ligma_vector2_new (gdouble x,
                  gdouble y)
{
  LigmaVector2 vector;

  vector.x = x;
  vector.y = y;

  return vector;
}

/**
 * ligma_vector2_set:
 * @vector: a pointer to a #LigmaVector2.
 * @x: the X coordinate.
 * @y: the Y coordinate.
 *
 * Sets the X and Y coordinates of @vector to @x and @y.
 **/
void
ligma_vector2_set (LigmaVector2 *vector,
                  gdouble      x,
                  gdouble      y)
{
  vector->x = x;
  vector->y = y;
}

/**
 * ligma_vector2_length:
 * @vector: a pointer to a #LigmaVector2.
 *
 * Computes the length of a 2D vector.
 *
 * Returns: the length of @vector (a positive gdouble).
 **/
gdouble
ligma_vector2_length (const LigmaVector2 *vector)
{
  return (sqrt (vector->x * vector->x + vector->y * vector->y));
}

/**
 * ligma_vector2_length_val:
 * @vector: a #LigmaVector2.
 *
 * Identical to [method@Vector2.length], but the vector is passed by value
 * rather than by reference.
 *
 * Returns: the length of @vector (a positive gdouble).
 **/
gdouble
ligma_vector2_length_val (LigmaVector2 vector)
{
  return (sqrt (vector.x * vector.x + vector.y * vector.y));
}

/**
 * ligma_vector2_mul:
 * @vector: a pointer to a #LigmaVector2.
 * @factor: a scalar.
 *
 * Multiplies each component of the @vector by @factor. Note that this
 * is equivalent to multiplying the vectors length by @factor.
 **/
void
ligma_vector2_mul (LigmaVector2 *vector,
                  gdouble      factor)
{
  vector->x *= factor;
  vector->y *= factor;
}

/**
 * ligma_vector2_mul_val:
 * @vector: a #LigmaVector2.
 * @factor: a scalar.
 *
 * Identical to [method@Vector2.mul], but the vector is passed by value rather
 * than by reference.
 *
 * Returns: the resulting #LigmaVector2.
 **/
LigmaVector2
ligma_vector2_mul_val (LigmaVector2 vector,
                      gdouble     factor)
{
  LigmaVector2 result;

  result.x = vector.x * factor;
  result.y = vector.y * factor;

  return result;
}


/**
 * ligma_vector2_normalize:
 * @vector: a pointer to a #LigmaVector2.
 *
 * Normalizes the @vector so the length of the @vector is 1.0. The nul
 * vector will not be changed.
 **/
void
ligma_vector2_normalize (LigmaVector2 *vector)
{
  gdouble len;

  len = ligma_vector2_length (vector);

  if (len != 0.0)
    {
      len = 1.0 / len;
      vector->x *= len;
      vector->y *= len;
    }
  else
    {
      *vector = ligma_vector2_zero;
    }
}

/**
 * ligma_vector2_normalize_val:
 * @vector: a #LigmaVector2.
 *
 * Identical to [method@Vector2.normalize], but the
 * vector is passed by value rather than by reference.
 *
 * Returns: a #LigmaVector2 parallel to @vector, pointing in the same
 * direction but with a length of 1.0.
 **/
LigmaVector2
ligma_vector2_normalize_val (LigmaVector2 vector)
{
  LigmaVector2 normalized;
  gdouble     len;

  len = ligma_vector2_length_val (vector);

  if (len != 0.0)
    {
      len = 1.0 / len;
      normalized.x = vector.x * len;
      normalized.y = vector.y * len;
      return normalized;
    }
  else
    {
      return ligma_vector2_zero;
    }
}

/**
 * ligma_vector2_neg:
 * @vector: a pointer to a #LigmaVector2.
 *
 * Negates the @vector (i.e. negate all its coordinates).
 **/
void
ligma_vector2_neg (LigmaVector2 *vector)
{
  vector->x *= -1.0;
  vector->y *= -1.0;
}

/**
 * ligma_vector2_neg_val:
 * @vector: a #LigmaVector2.
 *
 * Identical to [method@Vector2.neg], but the vector
 * is passed by value rather than by reference.
 *
 * Returns: the negated #LigmaVector2.
 **/
LigmaVector2
ligma_vector2_neg_val (LigmaVector2 vector)
{
  LigmaVector2 result;

  result.x = vector.x * -1.0;
  result.y = vector.y * -1.0;

  return result;
}

/**
 * ligma_vector2_add:
 * @result: (out caller-allocates): destination for the resulting #LigmaVector2.
 * @vector1: a pointer to the first #LigmaVector2.
 * @vector2: a pointer to the second #LigmaVector2.
 *
 * Computes the sum of two 2D vectors. The resulting #LigmaVector2 is
 * stored in @result.
 **/
void
ligma_vector2_add (LigmaVector2       *result,
                  const LigmaVector2 *vector1,
                  const LigmaVector2 *vector2)
{
  result->x = vector1->x + vector2->x;
  result->y = vector1->y + vector2->y;
}

/**
 * ligma_vector2_add_val:
 * @vector1: the first #LigmaVector2.
 * @vector2: the second #LigmaVector2.
 *
 * This function is identical to ligma_vector2_add() but the vectors
 * are passed by value rather than by reference.
 *
 * Returns: the resulting #LigmaVector2.
 **/
LigmaVector2
ligma_vector2_add_val (LigmaVector2 vector1,
                      LigmaVector2 vector2)
{
  LigmaVector2 result;

  result.x = vector1.x + vector2.x;
  result.y = vector1.y + vector2.y;

  return result;
}

/**
 * ligma_vector2_sub:
 * @result: (out caller-allocates): the destination for the resulting #LigmaVector2.
 * @vector1: a pointer to the first #LigmaVector2.
 * @vector2: a pointer to the second #LigmaVector2.
 *
 * Computes the difference of two 2D vectors (@vector1 minus @vector2).
 * The resulting #LigmaVector2 is stored in @result.
 **/
void
ligma_vector2_sub (LigmaVector2       *result,
                  const LigmaVector2 *vector1,
                  const LigmaVector2 *vector2)
{
  result->x = vector1->x - vector2->x;
  result->y = vector1->y - vector2->y;
}

/**
 * ligma_vector2_sub_val:
 * @vector1: the first #LigmaVector2.
 * @vector2: the second #LigmaVector2.
 *
 * This function is identical to ligma_vector2_sub() but the vectors
 * are passed by value rather than by reference.
 *
 * Returns: the resulting #LigmaVector2.
 **/
LigmaVector2
ligma_vector2_sub_val (LigmaVector2 vector1,
                      LigmaVector2 vector2)
{
  LigmaVector2 result;

  result.x = vector1.x - vector2.x;
  result.y = vector1.y - vector2.y;

  return result;
}

/**
 * ligma_vector2_inner_product:
 * @vector1: a pointer to the first #LigmaVector2.
 * @vector2: a pointer to the second #LigmaVector2.
 *
 * Computes the inner (dot) product of two 2D vectors.
 * This product is zero if and only if the two vectors are orthogonal.
 *
 * Returns: The inner product.
 **/
gdouble
ligma_vector2_inner_product (const LigmaVector2 *vector1,
                            const LigmaVector2 *vector2)
{
  return (vector1->x * vector2->x + vector1->y * vector2->y);
}

/**
 * ligma_vector2_inner_product_val:
 * @vector1: the first #LigmaVector2.
 * @vector2: the second #LigmaVector2.
 *
 * Identical to [method@Vector2.inner_product], but the
 * vectors are passed by value rather than by reference.
 *
 * Returns: The inner product.
 **/
gdouble
ligma_vector2_inner_product_val (LigmaVector2 vector1,
                                LigmaVector2 vector2)
{
  return (vector1.x * vector2.x + vector1.y * vector2.y);
}

/**
 * ligma_vector2_cross_product:
 * @vector1: a pointer to the first #LigmaVector2.
 * @vector2: a pointer to the second #LigmaVector2.
 *
 * Compute the cross product of two vectors. The result is a
 * #LigmaVector2 which is orthogonal to both @vector1 and @vector2. If
 * @vector1 and @vector2 are parallel, the result will be the nul
 * vector.
 *
 * Note that in 2D, this function is useful to test if two vectors are
 * parallel or not, or to compute the area spawned by two vectors.
 *
 * Returns: The cross product.
 **/
LigmaVector2
ligma_vector2_cross_product (const LigmaVector2 *vector1,
                            const LigmaVector2 *vector2)
{
  LigmaVector2 normal;

  normal.x = vector1->x * vector2->y - vector1->y * vector2->x;
  normal.y = vector1->y * vector2->x - vector1->x * vector2->y;

  return normal;
}

/**
 * ligma_vector2_cross_product_val:
 * @vector1: the first #LigmaVector2.
 * @vector2: the second #LigmaVector2.
 *
 * Identical to [method@Vector2.cross_product], but the
 * vectors are passed by value rather than by reference.
 *
 * Returns: The cross product.
 **/
LigmaVector2
ligma_vector2_cross_product_val (LigmaVector2 vector1,
                                LigmaVector2 vector2)
{
  LigmaVector2 normal;

  normal.x = vector1.x * vector2.y - vector1.y * vector2.x;
  normal.y = vector1.y * vector2.x - vector1.x * vector2.y;

  return normal;
}

/**
 * ligma_vector2_rotate:
 * @vector: a pointer to a #LigmaVector2.
 * @alpha: an angle (in radians).
 *
 * Rotates the @vector counterclockwise by @alpha radians.
 **/
void
ligma_vector2_rotate (LigmaVector2 *vector,
                     gdouble      alpha)
{
  LigmaVector2 result;

  result.x = cos (alpha) * vector->x + sin (alpha) * vector->y;
  result.y = cos (alpha) * vector->y - sin (alpha) * vector->x;

  *vector = result;
}

/**
 * ligma_vector2_rotate_val:
 * @vector: a #LigmaVector2.
 * @alpha: an angle (in radians).
 *
 * Identical to [method@Vector2.rotate], but the vector
 * is passed by value rather than by reference.
 *
 * Returns: a #LigmaVector2 representing @vector rotated by @alpha
 * radians.
 **/
LigmaVector2
ligma_vector2_rotate_val (LigmaVector2 vector,
                         gdouble     alpha)
{
  LigmaVector2 result;

  result.x = cos (alpha) * vector.x + sin (alpha) * vector.y;
  result.y = cos (alpha) * vector.y - sin (alpha) * vector.x;

  return result;
}

/**
 * ligma_vector2_normal:
 * @vector: a pointer to a #LigmaVector2.
 *
 * Compute a normalized perpendicular vector to @vector
 *
 * Returns: a #LigmaVector2 perpendicular to @vector, with a length of 1.0.
 *
 * Since: 2.8
 **/
LigmaVector2
ligma_vector2_normal (LigmaVector2 *vector)
{
  LigmaVector2 result;

  result.x = - vector->y;
  result.y = vector->x;

  ligma_vector2_normalize (&result);

  return result;
}

/**
 * ligma_vector2_normal_val:
 * @vector: a #LigmaVector2.
 *
 * Identical to [method@Vector2.normal], but the vector
 * is passed by value rather than by reference.
 *
 * Returns: a #LigmaVector2 perpendicular to @vector, with a length of 1.0.
 *
 * Since: 2.8
 **/
LigmaVector2
ligma_vector2_normal_val (LigmaVector2 vector)
{
  LigmaVector2 result;

  result.x = - vector.y;
  result.y = vector.x;

  ligma_vector2_normalize (&result);

  return result;
}
/**************************************/
/* Three dimensional vector functions */
/**************************************/

/**
 * ligma_vector3_new:
 * @x: the X coordinate.
 * @y: the Y coordinate.
 * @z: the Z coordinate.
 *
 * Creates a #LigmaVector3 of coordinate @x, @y and @z.
 *
 * Returns: the resulting #LigmaVector3.
 **/
LigmaVector3
ligma_vector3_new (gdouble  x,
                  gdouble  y,
                  gdouble  z)
{
  LigmaVector3 vector;

  vector.x = x;
  vector.y = y;
  vector.z = z;

  return vector;
}

/**
 * ligma_vector3_set:
 * @vector: a pointer to a #LigmaVector3.
 * @x: the X coordinate.
 * @y: the Y coordinate.
 * @z: the Z coordinate.
 *
 * Sets the X, Y and Z coordinates of @vector to @x, @y and @z.
 **/
void
ligma_vector3_set (LigmaVector3 *vector,
                  gdouble      x,
                  gdouble      y,
                  gdouble      z)
{
  vector->x = x;
  vector->y = y;
  vector->z = z;
}

/**
 * ligma_vector3_length:
 * @vector: a pointer to a #LigmaVector3.
 *
 * Computes the length of a 3D vector.
 *
 * Returns: the length of @vector (a positive gdouble).
 **/
gdouble
ligma_vector3_length (const LigmaVector3 *vector)
{
  return (sqrt (vector->x * vector->x +
                vector->y * vector->y +
                vector->z * vector->z));
}

/**
 * ligma_vector3_length_val:
 * @vector: a #LigmaVector3.
 *
 * Identical to [method@Vector3.length], but the vector
 * is passed by value rather than by reference.
 *
 * Returns: the length of @vector (a positive gdouble).
 **/
gdouble
ligma_vector3_length_val (LigmaVector3 vector)
{
  return (sqrt (vector.x * vector.x +
                vector.y * vector.y +
                vector.z * vector.z));
}

/**
 * ligma_vector3_mul:
 * @vector: a pointer to a #LigmaVector3.
 * @factor: a scalar.
 *
 * Multiplies each component of the @vector by @factor. Note that
 * this is equivalent to multiplying the vectors length by @factor.
 **/
void
ligma_vector3_mul (LigmaVector3 *vector,
                  gdouble      factor)
{
  vector->x *= factor;
  vector->y *= factor;
  vector->z *= factor;
}

/**
 * ligma_vector3_mul_val:
 * @vector: a #LigmaVector3.
 * @factor: a scalar.
 *
 * Identical to [method@Vector3.mul], but the vector is
 * passed by value rather than by reference.
 *
 * Returns: the resulting #LigmaVector3.
 **/
LigmaVector3
ligma_vector3_mul_val (LigmaVector3 vector,
                      gdouble     factor)
{
  LigmaVector3 result;

  result.x = vector.x * factor;
  result.y = vector.y * factor;
  result.z = vector.z * factor;

  return result;
}

/**
 * ligma_vector3_normalize:
 * @vector: a pointer to a #LigmaVector3.
 *
 * Normalizes the @vector so the length of the @vector is 1.0. The nul
 * vector will not be changed.
 **/
void
ligma_vector3_normalize (LigmaVector3 *vector)
{
  gdouble len;

  len = ligma_vector3_length (vector);

  if (len != 0.0)
    {
      len = 1.0 / len;
      vector->x *= len;
      vector->y *= len;
      vector->z *= len;
    }
  else
    {
      *vector = ligma_vector3_zero;
    }
}

/**
 * ligma_vector3_normalize_val:
 * @vector: a #LigmaVector3.
 *
 * Identical to [method@Vector3.normalize], but the
 * vector is passed by value rather than by reference.
 *
 * Returns: a #LigmaVector3 parallel to @vector, pointing in the same
 * direction but with a length of 1.0.
 **/
LigmaVector3
ligma_vector3_normalize_val (LigmaVector3 vector)
{
  LigmaVector3 result;
  gdouble     len;

  len = ligma_vector3_length_val (vector);

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
      return ligma_vector3_zero;
    }
}

/**
 * ligma_vector3_neg:
 * @vector: a pointer to a #LigmaVector3.
 *
 * Negates the @vector (i.e. negate all its coordinates).
 **/
void
ligma_vector3_neg (LigmaVector3 *vector)
{
  vector->x *= -1.0;
  vector->y *= -1.0;
  vector->z *= -1.0;
}

/**
 * ligma_vector3_neg_val:
 * @vector: a #LigmaVector3.
 *
 * Identical to [method@Vector3.neg], but the vector
 * is passed by value rather than by reference.
 *
 * Returns: the negated #LigmaVector3.
 **/
LigmaVector3
ligma_vector3_neg_val (LigmaVector3 vector)
{
  LigmaVector3 result;

  result.x = vector.x * -1.0;
  result.y = vector.y * -1.0;
  result.z = vector.z * -1.0;

  return result;
}

/**
 * ligma_vector3_add:
 * @result: (out caller-allocates): destination for the resulting #LigmaVector3.
 * @vector1: a pointer to the first #LigmaVector3.
 * @vector2: a pointer to the second #LigmaVector3.
 *
 * Computes the sum of two 3D vectors. The resulting #LigmaVector3 is
 * stored in @result.
 **/
void
ligma_vector3_add (LigmaVector3       *result,
                  const LigmaVector3 *vector1,
                  const LigmaVector3 *vector2)
{
  result->x = vector1->x + vector2->x;
  result->y = vector1->y + vector2->y;
  result->z = vector1->z + vector2->z;
}

/**
 * ligma_vector3_add_val:
 * @vector1: a #LigmaVector3.
 * @vector2: a #LigmaVector3.
 *
 * This function is identical to ligma_vector3_add() but the vectors
 * are passed by value rather than by reference.
 *
 * Returns: the resulting #LigmaVector3.
 **/
LigmaVector3
ligma_vector3_add_val (LigmaVector3 vector1,
                      LigmaVector3 vector2)
{
  LigmaVector3 result;

  result.x = vector1.x + vector2.x;
  result.y = vector1.y + vector2.y;
  result.z = vector1.z + vector2.z;

  return result;
}

/**
 * ligma_vector3_sub:
 * @result: (out caller-allocates): the destination for the resulting #LigmaVector3.
 * @vector1: a pointer to the first #LigmaVector3.
 * @vector2: a pointer to the second #LigmaVector3.
 *
 * Computes the difference of two 3D vectors (@vector1 minus @vector2).
 * The resulting #LigmaVector3 is stored in @result.
 **/
void
ligma_vector3_sub (LigmaVector3       *result,
                  const LigmaVector3 *vector1,
                  const LigmaVector3 *vector2)
{
  result->x = vector1->x - vector2->x;
  result->y = vector1->y - vector2->y;
  result->z = vector1->z - vector2->z;
}

/**
 * ligma_vector3_sub_val:
 * @vector1: a #LigmaVector3.
 * @vector2: a #LigmaVector3.
 *
 * This function is identical to ligma_vector3_sub() but the vectors
 * are passed by value rather than by reference.
 *
 * Returns: the resulting #LigmaVector3.
 **/
LigmaVector3
ligma_vector3_sub_val (LigmaVector3 vector1,
                     LigmaVector3 vector2)
{
  LigmaVector3 result;

  result.x = vector1.x - vector2.x;
  result.y = vector1.y - vector2.y;
  result.z = vector1.z - vector2.z;

  return result;
}

/**
 * ligma_vector3_inner_product:
 * @vector1: a pointer to the first #LigmaVector3.
 * @vector2: a pointer to the second #LigmaVector3.
 *
 * Computes the inner (dot) product of two 3D vectors. This product
 * is zero if and only if the two vectors are orthogonal.
 *
 * Returns: The inner product.
 **/
gdouble
ligma_vector3_inner_product (const LigmaVector3 *vector1,
                            const LigmaVector3 *vector2)
{
  return (vector1->x * vector2->x +
          vector1->y * vector2->y +
          vector1->z * vector2->z);
}

/**
 * ligma_vector3_inner_product_val:
 * @vector1: the first #LigmaVector3.
 * @vector2: the second #LigmaVector3.
 *
 * Identical to [method@Vector3.inner_product], but the
 * vectors are passed by value rather than by reference.
 *
 * Returns: The inner product.
 **/
gdouble
ligma_vector3_inner_product_val (LigmaVector3 vector1,
                                LigmaVector3 vector2)
{
  return (vector1.x * vector2.x +
          vector1.y * vector2.y +
          vector1.z * vector2.z);
}

/**
 * ligma_vector3_cross_product:
 * @vector1: a pointer to the first #LigmaVector3.
 * @vector2: a pointer to the second #LigmaVector3.
 *
 * Compute the cross product of two vectors. The result is a
 * #LigmaVector3 which is orthogonal to both @vector1 and @vector2. If
 * @vector1 and @vector2 and parallel, the result will be the nul
 * vector.
 *
 * This function can be used to compute the normal of the plane
 * defined by @vector1 and @vector2.
 *
 * Returns: The cross product.
 **/
LigmaVector3
ligma_vector3_cross_product (const LigmaVector3 *vector1,
                            const LigmaVector3 *vector2)
{
  LigmaVector3 normal;

  normal.x = vector1->y * vector2->z - vector1->z * vector2->y;
  normal.y = vector1->z * vector2->x - vector1->x * vector2->z;
  normal.z = vector1->x * vector2->y - vector1->y * vector2->x;

  return normal;
}

/**
 * ligma_vector3_cross_product_val:
 * @vector1: the first #LigmaVector3.
 * @vector2: the second #LigmaVector3.
 *
 * Identical to [method@Vector3.cross_product], but the
 * vectors are passed by value rather than by reference.
 *
 * Returns: The cross product.
 **/
LigmaVector3
ligma_vector3_cross_product_val (LigmaVector3 vector1,
                                LigmaVector3 vector2)
{
  LigmaVector3 normal;

  normal.x = vector1.y * vector2.z - vector1.z * vector2.y;
  normal.y = vector1.z * vector2.x - vector1.x * vector2.z;
  normal.z = vector1.x * vector2.y - vector1.y * vector2.x;

  return normal;
}

/**
 * ligma_vector3_rotate:
 * @vector: a pointer to a #LigmaVector3.
 * @alpha: the angle (in radian) of rotation around the Z axis.
 * @beta: the angle (in radian) of rotation around the Y axis.
 * @gamma: the angle (in radian) of rotation around the X axis.
 *
 * Rotates the @vector around the three axis (Z, Y, and X) by @alpha,
 * @beta and @gamma, respectively.
 *
 * Note that the order of the rotation is very important. If you
 * expect a vector to be rotated around X, and then around Y, you will
 * have to call this function twice. Also, it is often wise to call
 * this function with only one of @alpha, @beta and @gamma non-zero.
 **/
void
ligma_vector3_rotate (LigmaVector3 *vector,
                     gdouble      alpha,
                     gdouble      beta,
                     gdouble      gamma)
{
  LigmaVector3 s, t;

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
 * ligma_vector3_rotate_val:
 * @vector: a #LigmaVector3.
 * @alpha: the angle (in radian) of rotation around the Z axis.
 * @beta: the angle (in radian) of rotation around the Y axis.
 * @gamma: the angle (in radian) of rotation around the X axis.
 *
 * Identical to [method@Vector3.rotate], but the vectors
 * are passed by value rather than by reference.
 *
 * Returns: the rotated vector.
 **/
LigmaVector3
ligma_vector3_rotate_val (LigmaVector3 vector,
                         gdouble     alpha,
                         gdouble     beta,
                         gdouble     gamma)
{
  LigmaVector3 s, t, result;

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
 * ligma_vector_2d_to_3d:
 * @sx: the abscissa of the upper-left screen rectangle.
 * @sy: the ordinate of the upper-left screen rectangle.
 * @w: the width of the screen rectangle.
 * @h: the height of the screen rectangle.
 * @x: the abscissa of the point in the screen rectangle to map.
 * @y: the ordinate of the point in the screen rectangle to map.
 * @vp: the position of the observer.
 * @p: the resulting point.
 *
 * \"Compute screen (sx, sy) - (sx + w, sy + h) to 3D unit square
 * mapping. The plane to map to is given in the z field of p. The
 * observer is located at position vp (vp->z != 0.0).\"
 *
 * In other words, this computes the projection of the point (@x, @y)
 * to the plane z = @p->z (parallel to XY), from the @vp point of view
 * through the screen (@sx, @sy)->(@sx + @w, @sy + @h)
 **/
void
ligma_vector_2d_to_3d (gint               sx,
                      gint               sy,
                      gint               w,
                      gint               h,
                      gint               x,
                      gint               y,
                      const LigmaVector3 *vp,
                      LigmaVector3       *p)
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
 * ligma_vector_2d_to_3d_val:
 * @sx: the abscissa of the upper-left screen rectangle.
 * @sy: the ordinate of the upper-left screen rectangle.
 * @w: the width of the screen rectangle.
 * @h: the height of the screen rectangle.
 * @x: the abscissa of the point in the screen rectangle to map.
 * @y: the ordinate of the point in the screen rectangle to map.
 * @vp: position of the observer.
 * @p: the resulting point.
 *
 * This function is identical to ligma_vector_2d_to_3d() but the
 * position of the @observer and the resulting point @p are passed by
 * value rather than by reference.
 *
 * Returns: the computed #LigmaVector3 point.
 **/
LigmaVector3
ligma_vector_2d_to_3d_val (gint        sx,
                          gint        sy,
                          gint        w,
                          gint        h,
                          gint        x,
                          gint        y,
                          LigmaVector3 vp,
                          LigmaVector3 p)
{
  LigmaVector3 result;
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

  result.z = 0;
  return result;
}

/**
 * ligma_vector_3d_to_2d:
 * @sx: the abscissa of the upper-left screen rectangle.
 * @sy: the ordinate of the upper-left screen rectangle.
 * @w: the width of the screen rectangle.
 * @h: the height of the screen rectangle.
 * @x: the abscissa of the point in the screen rectangle to map (return value).
 * @y: the ordinate of the point in the screen rectangle to map (return value).
 * @vp: position of the observer.
 * @p: the 3D point to project to the plane.
 *
 * Convert the given 3D point to 2D (project it onto the viewing
 * plane, (sx, sy, 0) - (sx + w, sy + h, 0). The input is assumed to
 * be in the unit square (0, 0, z) - (1, 1, z). The viewpoint of the
 * observer is passed in vp.
 *
 * This is basically the opposite of ligma_vector_2d_to_3d().
 **/
void
ligma_vector_3d_to_2d (gint               sx,
                      gint               sy,
                      gint               w,
                      gint               h,
                      gdouble           *x,
                      gdouble           *y,
                      const LigmaVector3 *vp,
                      const LigmaVector3 *p)
{
  gdouble     t;
  LigmaVector3 dir;

  ligma_vector3_sub (&dir, p, vp);
  ligma_vector3_normalize (&dir);

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

/* Private functions for boxed type. */

static gpointer
ligma_vector2_copy (gpointer boxed)
{
  LigmaVector2 *vector = boxed;
  LigmaVector2 *new_v;

  new_v = g_slice_new (LigmaVector2);
  new_v->x = vector->x;
  new_v->y = vector->y;

  return new_v;
}

static void
ligma_vector2_free (gpointer boxed)
{
  g_slice_free (LigmaVector2, boxed);
}

G_DEFINE_BOXED_TYPE (LigmaVector2, ligma_vector2,
                     ligma_vector2_copy,
                     ligma_vector2_free)

static gpointer
ligma_vector3_copy (gpointer boxed)
{
  LigmaVector3 *vector = boxed;
  LigmaVector3 *new_v;

  new_v = g_slice_new (LigmaVector3);
  new_v->x = vector->x;
  new_v->y = vector->y;
  new_v->z = vector->z;

  return new_v;
}

static void
ligma_vector3_free (gpointer boxed)
{
  g_slice_free (LigmaVector3, boxed);
}

G_DEFINE_BOXED_TYPE (LigmaVector3, ligma_vector3,
                     ligma_vector3_copy,
                     ligma_vector3_free)
