/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmamatrix.c
 * Copyright (C) 1998 Jay Cox <jaycox@ligma.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <glib-object.h>

#include "ligmamath.h"


/**
 * SECTION: ligmamatrix
 * @title: LigmaMatrix
 * @short_description: Utilities to set up and manipulate 3x3
 *                     transformation matrices.
 * @see_also: #LigmaVector2, #LigmaVector3, #LigmaVector4
 *
 * When doing image manipulation you will often need 3x3
 * transformation matrices that define translation, rotation, scaling,
 * shearing and arbitrary perspective transformations using a 3x3
 * matrix. Here you'll find a set of utility functions to set up those
 * matrices and to perform basic matrix manipulations and tests.
 *
 * Each matrix class has a 2 dimensional gdouble coeff member. The
 * element for row r and column c of the matrix is coeff[r][c].
 **/


#define EPSILON 1e-6


static LigmaMatrix2 * matrix2_copy                  (const LigmaMatrix2 *matrix);

G_DEFINE_BOXED_TYPE (LigmaMatrix2, ligma_matrix2, matrix2_copy, g_free)


/*
 * LIGMA_TYPE_PARAM_MATRIX2
 */

#define LIGMA_PARAM_SPEC_MATRIX2(pspec) (G_TYPE_CHECK_INSTANCE_CAST ((pspec), LIGMA_TYPE_PARAM_MATRIX2, LigmaParamSpecMatrix2))

static void   ligma_param_matrix2_class_init  (GParamSpecClass *class);
static void   ligma_param_matrix2_init        (GParamSpec      *pspec);
static void   ligma_param_matrix2_set_default (GParamSpec      *pspec,
                                              GValue          *value);
static gint   ligma_param_matrix2_values_cmp  (GParamSpec      *pspec,
                                              const GValue    *value1,
                                              const GValue    *value2);

typedef struct _LigmaParamSpecMatrix2 LigmaParamSpecMatrix2;

struct _LigmaParamSpecMatrix2
{
  GParamSpecBoxed      parent_instance;

  LigmaMatrix2          default_value;
};

/**
 * ligma_param_matrix2_get_type:
 *
 * Reveals the object type
 *
 * Returns: the #GType for a LigmaMatrix2 object
 *
 * Since: 2.4
 **/
GType
ligma_param_matrix2_get_type (void)
{
  static GType spec_type = 0;

  if (!spec_type)
    {
      static const GTypeInfo type_info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) ligma_param_matrix2_class_init,
        NULL, NULL,
        sizeof (LigmaParamSpecMatrix2),
        0,
        (GInstanceInitFunc) ligma_param_matrix2_init
      };

      spec_type = g_type_register_static (G_TYPE_PARAM_BOXED,
                                          "LigmaParamMatrix2",
                                          &type_info, 0);
    }

  return spec_type;
}

static void
ligma_param_matrix2_class_init (GParamSpecClass *class)
{
  class->value_type        = LIGMA_TYPE_MATRIX2;
  class->value_set_default = ligma_param_matrix2_set_default;
  class->values_cmp        = ligma_param_matrix2_values_cmp;
}

static void
ligma_param_matrix2_init (GParamSpec *pspec)
{
  LigmaParamSpecMatrix2 *cspec = LIGMA_PARAM_SPEC_MATRIX2 (pspec);

  ligma_matrix2_identity (&cspec->default_value);
}

static void
ligma_param_matrix2_set_default (GParamSpec *pspec,
                                GValue     *value)
{
  LigmaParamSpecMatrix2 *cspec = LIGMA_PARAM_SPEC_MATRIX2 (pspec);

  g_value_set_static_boxed (value, &cspec->default_value);
}

static gint
ligma_param_matrix2_values_cmp (GParamSpec   *pspec,
                               const GValue *value1,
                               const GValue *value2)
{
  LigmaMatrix2 *matrix1;
  LigmaMatrix2 *matrix2;
  gint         i, j;

  matrix1 = value1->data[0].v_pointer;
  matrix2 = value2->data[0].v_pointer;

  /*  try to return at least *something*, it's useless anyway...  */

  if (! matrix1)
    return matrix2 != NULL ? -1 : 0;
  else if (! matrix2)
    return matrix1 != NULL;

  for (i = 0; i < 2; i++)
    for (j = 0; j < 2; j++)
      if (matrix1->coeff[i][j] != matrix2->coeff[i][j])
        return 1;

  return 0;
}

/**
 * ligma_param_spec_matrix2:
 * @name:          Canonical name of the param
 * @nick:          Nickname of the param
 * @blurb:         Brief description of param.
 * @default_value: Value to use if none is assigned.
 * @flags:         a combination of #GParamFlags
 *
 * Creates a param spec to hold a #LigmaMatrix2 value.
 * See g_param_spec_internal() for more information.
 *
 * Returns: (transfer full): a newly allocated #GParamSpec instance
 *
 * Since: 2.4
 **/
GParamSpec *
ligma_param_spec_matrix2 (const gchar       *name,
                         const gchar       *nick,
                         const gchar       *blurb,
                         const LigmaMatrix2 *default_value,
                         GParamFlags        flags)
{
  LigmaParamSpecMatrix2 *cspec;

  g_return_val_if_fail (default_value != NULL, NULL);

  cspec = g_param_spec_internal (LIGMA_TYPE_PARAM_MATRIX2,
                                 name, nick, blurb, flags);

  cspec->default_value = *default_value;

  return G_PARAM_SPEC (cspec);
}


static LigmaMatrix2 *
matrix2_copy (const LigmaMatrix2 *matrix)
{
  return (LigmaMatrix2 *) g_memdup2 (matrix, sizeof (LigmaMatrix2));
}


/**
 * ligma_matrix2_identity:
 * @matrix: A matrix.
 *
 * Sets the matrix to the identity matrix.
 */
void
ligma_matrix2_identity (LigmaMatrix2 *matrix)
{
  static const LigmaMatrix2 identity = { { { 1.0, 0.0 },
                                          { 0.0, 1.0 } } };

  *matrix = identity;
}

/**
 * ligma_matrix2_mult:
 * @matrix1: The first input matrix.
 * @matrix2: The second input matrix which will be overwritten by the result.
 *
 * Multiplies two matrices and puts the result into the second one.
 */
void
ligma_matrix2_mult (const LigmaMatrix2 *matrix1,
                   LigmaMatrix2       *matrix2)
{
  LigmaMatrix2  tmp;

  tmp.coeff[0][0] = (matrix1->coeff[0][0] * matrix2->coeff[0][0] +
                     matrix1->coeff[0][1] * matrix2->coeff[1][0]);
  tmp.coeff[0][1] = (matrix1->coeff[0][0] * matrix2->coeff[0][1] +
                     matrix1->coeff[0][1] * matrix2->coeff[1][1]);
  tmp.coeff[1][0] = (matrix1->coeff[1][0] * matrix2->coeff[0][0] +
                     matrix1->coeff[1][1] * matrix2->coeff[1][0]);
  tmp.coeff[1][1] = (matrix1->coeff[1][0] * matrix2->coeff[0][1] +
                     matrix1->coeff[1][1] * matrix2->coeff[1][1]);

  *matrix2 = tmp;
}

/**
 * ligma_matrix2_determinant:
 * @matrix: The input matrix.
 *
 * Calculates the determinant of the given matrix.
 *
 * Returns: The determinant.
 *
 * Since: 2.10.16
 */

gdouble
ligma_matrix2_determinant (const LigmaMatrix2 *matrix)
{
  return matrix->coeff[0][0] * matrix->coeff[1][1] -
         matrix->coeff[0][1] * matrix->coeff[1][0];
}

/**
 * ligma_matrix2_invert:
 * @matrix: The matrix that is to be inverted.
 *
 * Inverts the given matrix.
 *
 * Since: 2.10.16
 */
void
ligma_matrix2_invert (LigmaMatrix2 *matrix)
{
  gdouble det = ligma_matrix2_determinant (matrix);
  gdouble temp;

  if (fabs (det) <= EPSILON)
    return;

  temp = matrix->coeff[0][0];

  matrix->coeff[0][0]  = matrix->coeff[1][1] / det;
  matrix->coeff[0][1] /= -det;
  matrix->coeff[1][0] /= -det;
  matrix->coeff[1][1]  = temp / det;
}

/**
 * ligma_matrix2_transform_point:
 * @matrix: The transformation matrix.
 * @x: The source X coordinate.
 * @y: The source Y coordinate.
 * @newx: (out): The transformed X coordinate.
 * @newy: (out): The transformed Y coordinate.
 *
 * Transforms a point in 2D as specified by the transformation matrix.
 *
 * Since: 2.10.16
 */
void
ligma_matrix2_transform_point (const LigmaMatrix2 *matrix,
                              gdouble            x,
                              gdouble            y,
                              gdouble           *newx,
                              gdouble           *newy)
{
  *newx = matrix->coeff[0][0] * x + matrix->coeff[0][1] * y;
  *newy = matrix->coeff[1][0] * x + matrix->coeff[1][1] * y;
}


static LigmaMatrix3 * matrix3_copy                  (const LigmaMatrix3 *matrix);

G_DEFINE_BOXED_TYPE (LigmaMatrix3, ligma_matrix3, matrix3_copy, g_free)


/*
 * LIGMA_TYPE_PARAM_MATRIX3
 */

#define LIGMA_PARAM_SPEC_MATRIX3(pspec) (G_TYPE_CHECK_INSTANCE_CAST ((pspec), LIGMA_TYPE_PARAM_MATRIX3, LigmaParamSpecMatrix3))

static void   ligma_param_matrix3_class_init  (GParamSpecClass *class);
static void   ligma_param_matrix3_init        (GParamSpec      *pspec);
static void   ligma_param_matrix3_set_default (GParamSpec      *pspec,
                                              GValue          *value);
static gint   ligma_param_matrix3_values_cmp  (GParamSpec      *pspec,
                                              const GValue    *value1,
                                              const GValue    *value2);

typedef struct _LigmaParamSpecMatrix3 LigmaParamSpecMatrix3;

struct _LigmaParamSpecMatrix3
{
  GParamSpecBoxed      parent_instance;

  LigmaMatrix3          default_value;
};

/**
 * ligma_param_matrix3_get_type:
 *
 * Reveals the object type
 *
 * Returns: the #GType for a LigmaMatrix3 object
 *
 * Since: 2.8
 **/
GType
ligma_param_matrix3_get_type (void)
{
  static GType spec_type = 0;

  if (!spec_type)
    {
      static const GTypeInfo type_info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) ligma_param_matrix3_class_init,
        NULL, NULL,
        sizeof (LigmaParamSpecMatrix3),
        0,
        (GInstanceInitFunc) ligma_param_matrix3_init
      };

      spec_type = g_type_register_static (G_TYPE_PARAM_BOXED,
                                          "LigmaParamMatrix3",
                                          &type_info, 0);
    }

  return spec_type;
}

static void
ligma_param_matrix3_class_init (GParamSpecClass *class)
{
  class->value_type        = LIGMA_TYPE_MATRIX3;
  class->value_set_default = ligma_param_matrix3_set_default;
  class->values_cmp        = ligma_param_matrix3_values_cmp;
}

static void
ligma_param_matrix3_init (GParamSpec *pspec)
{
  LigmaParamSpecMatrix3 *cspec = LIGMA_PARAM_SPEC_MATRIX3 (pspec);

  ligma_matrix3_identity (&cspec->default_value);
}

static void
ligma_param_matrix3_set_default (GParamSpec *pspec,
                                GValue     *value)
{
  LigmaParamSpecMatrix3 *cspec = LIGMA_PARAM_SPEC_MATRIX3 (pspec);

  g_value_set_static_boxed (value, &cspec->default_value);
}

static gint
ligma_param_matrix3_values_cmp (GParamSpec   *pspec,
                               const GValue *value1,
                               const GValue *value2)
{
  LigmaMatrix3 *matrix1;
  LigmaMatrix3 *matrix2;
  gint         i, j;

  matrix1 = value1->data[0].v_pointer;
  matrix2 = value2->data[0].v_pointer;

  /*  try to return at least *something*, it's useless anyway...  */

  if (! matrix1)
    return matrix2 != NULL ? -1 : 0;
  else if (! matrix2)
    return matrix1 != NULL;

  for (i = 0; i < 3; i++)
    for (j = 0; j < 3; j++)
      if (matrix1->coeff[i][j] != matrix2->coeff[i][j])
        return 1;

  return 0;
}

/**
 * ligma_param_spec_matrix3:
 * @name:          Canonical name of the param
 * @nick:          Nickname of the param
 * @blurb:         Brief description of param.
 * @default_value: Value to use if none is assigned.
 * @flags:         a combination of #GParamFlags
 *
 * Creates a param spec to hold a #LigmaMatrix3 value.
 * See g_param_spec_internal() for more information.
 *
 * Returns: (transfer full): a newly allocated #GParamSpec instance
 *
 * Since: 2.8
 **/
GParamSpec *
ligma_param_spec_matrix3 (const gchar       *name,
                         const gchar       *nick,
                         const gchar       *blurb,
                         const LigmaMatrix3 *default_value,
                         GParamFlags        flags)
{
  LigmaParamSpecMatrix3 *cspec;

  cspec = g_param_spec_internal (LIGMA_TYPE_PARAM_MATRIX3,
                                 name, nick, blurb, flags);

  if (default_value)
    cspec->default_value = *default_value;

  return G_PARAM_SPEC (cspec);
}


static LigmaMatrix3 *
matrix3_copy (const LigmaMatrix3 *matrix)
{
  return (LigmaMatrix3 *) g_memdup2 (matrix, sizeof (LigmaMatrix3));
}


/**
 * ligma_matrix3_identity:
 * @matrix: A matrix.
 *
 * Sets the matrix to the identity matrix.
 */
void
ligma_matrix3_identity (LigmaMatrix3 *matrix)
{
  static const LigmaMatrix3 identity = { { { 1.0, 0.0, 0.0 },
                                          { 0.0, 1.0, 0.0 },
                                          { 0.0, 0.0, 1.0 } } };

  *matrix = identity;
}

/**
 * ligma_matrix3_transform_point:
 * @matrix: The transformation matrix.
 * @x: The source X coordinate.
 * @y: The source Y coordinate.
 * @newx: The transformed X coordinate.
 * @newy: The transformed Y coordinate.
 *
 * Transforms a point in 2D as specified by the transformation matrix.
 */
void
ligma_matrix3_transform_point (const LigmaMatrix3 *matrix,
                              gdouble            x,
                              gdouble            y,
                              gdouble           *newx,
                              gdouble           *newy)
{
  gdouble  w;

  w = matrix->coeff[2][0] * x + matrix->coeff[2][1] * y + matrix->coeff[2][2];

  if (w == 0.0)
    w = 1.0;
  else
    w = 1.0/w;

  *newx = (matrix->coeff[0][0] * x +
           matrix->coeff[0][1] * y +
           matrix->coeff[0][2]) * w;
  *newy = (matrix->coeff[1][0] * x +
           matrix->coeff[1][1] * y +
           matrix->coeff[1][2]) * w;
}

/**
 * ligma_matrix3_mult:
 * @matrix1: The first input matrix.
 * @matrix2: The second input matrix which will be overwritten by the result.
 *
 * Multiplies two matrices and puts the result into the second one.
 */
void
ligma_matrix3_mult (const LigmaMatrix3 *matrix1,
                   LigmaMatrix3       *matrix2)
{
  gint         i, j;
  LigmaMatrix3  tmp;
  gdouble      t1, t2, t3;

  for (i = 0; i < 3; i++)
    {
      t1 = matrix1->coeff[i][0];
      t2 = matrix1->coeff[i][1];
      t3 = matrix1->coeff[i][2];

      for (j = 0; j < 3; j++)
        {
          tmp.coeff[i][j]  = t1 * matrix2->coeff[0][j];
          tmp.coeff[i][j] += t2 * matrix2->coeff[1][j];
          tmp.coeff[i][j] += t3 * matrix2->coeff[2][j];
        }
    }

  *matrix2 = tmp;
}

/**
 * ligma_matrix3_translate:
 * @matrix: The matrix that is to be translated.
 * @x: Translation in X direction.
 * @y: Translation in Y direction.
 *
 * Translates the matrix by x and y.
 */
void
ligma_matrix3_translate (LigmaMatrix3 *matrix,
                        gdouble      x,
                        gdouble      y)
{
  gdouble g, h, i;

  g = matrix->coeff[2][0];
  h = matrix->coeff[2][1];
  i = matrix->coeff[2][2];

  matrix->coeff[0][0] += x * g;
  matrix->coeff[0][1] += x * h;
  matrix->coeff[0][2] += x * i;
  matrix->coeff[1][0] += y * g;
  matrix->coeff[1][1] += y * h;
  matrix->coeff[1][2] += y * i;
}

/**
 * ligma_matrix3_scale:
 * @matrix: The matrix that is to be scaled.
 * @x: X scale factor.
 * @y: Y scale factor.
 *
 * Scales the matrix by x and y
 */
void
ligma_matrix3_scale (LigmaMatrix3 *matrix,
                    gdouble      x,
                    gdouble      y)
{
  matrix->coeff[0][0] *= x;
  matrix->coeff[0][1] *= x;
  matrix->coeff[0][2] *= x;

  matrix->coeff[1][0] *= y;
  matrix->coeff[1][1] *= y;
  matrix->coeff[1][2] *= y;
}

/**
 * ligma_matrix3_rotate:
 * @matrix: The matrix that is to be rotated.
 * @theta: The angle of rotation (in radians).
 *
 * Rotates the matrix by theta degrees.
 */
void
ligma_matrix3_rotate (LigmaMatrix3 *matrix,
                     gdouble      theta)
{
  gdouble t1, t2;
  gdouble cost, sint;

  cost = cos (theta);
  sint = sin (theta);

  t1 = matrix->coeff[0][0];
  t2 = matrix->coeff[1][0];
  matrix->coeff[0][0] = cost * t1 - sint * t2;
  matrix->coeff[1][0] = sint * t1 + cost * t2;

  t1 = matrix->coeff[0][1];
  t2 = matrix->coeff[1][1];
  matrix->coeff[0][1] = cost * t1 - sint * t2;
  matrix->coeff[1][1] = sint * t1 + cost * t2;

  t1 = matrix->coeff[0][2];
  t2 = matrix->coeff[1][2];
  matrix->coeff[0][2] = cost * t1 - sint * t2;
  matrix->coeff[1][2] = sint * t1 + cost * t2;
}

/**
 * ligma_matrix3_xshear:
 * @matrix: The matrix that is to be sheared.
 * @amount: X shear amount.
 *
 * Shears the matrix in the X direction.
 */
void
ligma_matrix3_xshear (LigmaMatrix3 *matrix,
                     gdouble      amount)
{
  matrix->coeff[0][0] += amount * matrix->coeff[1][0];
  matrix->coeff[0][1] += amount * matrix->coeff[1][1];
  matrix->coeff[0][2] += amount * matrix->coeff[1][2];
}

/**
 * ligma_matrix3_yshear:
 * @matrix: The matrix that is to be sheared.
 * @amount: Y shear amount.
 *
 * Shears the matrix in the Y direction.
 */
void
ligma_matrix3_yshear (LigmaMatrix3 *matrix,
                     gdouble      amount)
{
  matrix->coeff[1][0] += amount * matrix->coeff[0][0];
  matrix->coeff[1][1] += amount * matrix->coeff[0][1];
  matrix->coeff[1][2] += amount * matrix->coeff[0][2];
}

/**
 * ligma_matrix3_affine:
 * @matrix: The input matrix.
 * @a: the 'a' coefficient
 * @b: the 'b' coefficient
 * @c: the 'c' coefficient
 * @d: the 'd' coefficient
 * @e: the 'e' coefficient
 * @f: the 'f' coefficient
 *
 * Applies the affine transformation given by six values to @matrix.
 * The six values form define an affine transformation matrix as
 * illustrated below:
 *
 *  ( a c e )
 *  ( b d f )
 *  ( 0 0 1 )
 **/
void
ligma_matrix3_affine (LigmaMatrix3 *matrix,
                     gdouble      a,
                     gdouble      b,
                     gdouble      c,
                     gdouble      d,
                     gdouble      e,
                     gdouble      f)
{
  LigmaMatrix3 affine;

  affine.coeff[0][0] = a;
  affine.coeff[1][0] = b;
  affine.coeff[2][0] = 0.0;

  affine.coeff[0][1] = c;
  affine.coeff[1][1] = d;
  affine.coeff[2][1] = 0.0;

  affine.coeff[0][2] = e;
  affine.coeff[1][2] = f;
  affine.coeff[2][2] = 1.0;

  ligma_matrix3_mult (&affine, matrix);
}

/**
 * ligma_matrix3_determinant:
 * @matrix: The input matrix.
 *
 * Calculates the determinant of the given matrix.
 *
 * Returns: The determinant.
 */
gdouble
ligma_matrix3_determinant (const LigmaMatrix3 *matrix)
{
  gdouble determinant;

  determinant  = (matrix->coeff[0][0] *
                  (matrix->coeff[1][1] * matrix->coeff[2][2] -
                   matrix->coeff[1][2] * matrix->coeff[2][1]));
  determinant -= (matrix->coeff[1][0] *
                  (matrix->coeff[0][1] * matrix->coeff[2][2] -
                   matrix->coeff[0][2] * matrix->coeff[2][1]));
  determinant += (matrix->coeff[2][0] *
                  (matrix->coeff[0][1] * matrix->coeff[1][2] -
                   matrix->coeff[0][2] * matrix->coeff[1][1]));

  return determinant;
}

/**
 * ligma_matrix3_invert:
 * @matrix: The matrix that is to be inverted.
 *
 * Inverts the given matrix.
 */
void
ligma_matrix3_invert (LigmaMatrix3 *matrix)
{
  LigmaMatrix3 inv;
  gdouble     det;

  det = ligma_matrix3_determinant (matrix);

  if (det == 0.0)
    return;

  det = 1.0 / det;

  inv.coeff[0][0] =   (matrix->coeff[1][1] * matrix->coeff[2][2] -
                       matrix->coeff[1][2] * matrix->coeff[2][1]) * det;

  inv.coeff[1][0] = - (matrix->coeff[1][0] * matrix->coeff[2][2] -
                       matrix->coeff[1][2] * matrix->coeff[2][0]) * det;

  inv.coeff[2][0] =   (matrix->coeff[1][0] * matrix->coeff[2][1] -
                       matrix->coeff[1][1] * matrix->coeff[2][0]) * det;

  inv.coeff[0][1] = - (matrix->coeff[0][1] * matrix->coeff[2][2] -
                       matrix->coeff[0][2] * matrix->coeff[2][1]) * det;

  inv.coeff[1][1] =   (matrix->coeff[0][0] * matrix->coeff[2][2] -
                       matrix->coeff[0][2] * matrix->coeff[2][0]) * det;

  inv.coeff[2][1] = - (matrix->coeff[0][0] * matrix->coeff[2][1] -
                       matrix->coeff[0][1] * matrix->coeff[2][0]) * det;

  inv.coeff[0][2] =   (matrix->coeff[0][1] * matrix->coeff[1][2] -
                       matrix->coeff[0][2] * matrix->coeff[1][1]) * det;

  inv.coeff[1][2] = - (matrix->coeff[0][0] * matrix->coeff[1][2] -
                       matrix->coeff[0][2] * matrix->coeff[1][0]) * det;

  inv.coeff[2][2] =   (matrix->coeff[0][0] * matrix->coeff[1][1] -
                       matrix->coeff[0][1] * matrix->coeff[1][0]) * det;

  *matrix = inv;
}


/*  functions to test for matrix properties  */

/**
 * ligma_matrix3_is_identity:
 * @matrix: The matrix that is to be tested.
 *
 * Checks if the given matrix is the identity matrix.
 *
 * Returns: %TRUE if the matrix is the identity matrix, %FALSE otherwise
 */
gboolean
ligma_matrix3_is_identity (const LigmaMatrix3 *matrix)
{
  gint i, j;

  for (i = 0; i < 3; i++)
    {
      for (j = 0; j < 3; j++)
        {
          if (i == j)
            {
              if (fabs (matrix->coeff[i][j] - 1.0) > EPSILON)
                return FALSE;
            }
          else
            {
              if (fabs (matrix->coeff[i][j]) > EPSILON)
                return FALSE;
            }
        }
    }

  return TRUE;
}

/**
 * ligma_matrix3_is_diagonal:
 * @matrix: The matrix that is to be tested.
 *
 * Checks if the given matrix is diagonal.
 *
 * Returns: %TRUE if the matrix is diagonal, %FALSE otherwise
 */
gboolean
ligma_matrix3_is_diagonal (const LigmaMatrix3 *matrix)
{
  gint i, j;

  for (i = 0; i < 3; i++)
    {
      for (j = 0; j < 3; j++)
        {
          if (i != j && fabs (matrix->coeff[i][j]) > EPSILON)
            return FALSE;
        }
    }

  return TRUE;
}

/**
 * ligma_matrix3_is_affine:
 * @matrix: The matrix that is to be tested.
 *
 * Checks if the given matrix defines an affine transformation.
 *
 * Returns: %TRUE if the matrix defines an affine transformation,
 *          %FALSE otherwise
 *
 * Since: 2.4
 */
gboolean
ligma_matrix3_is_affine (const LigmaMatrix3 *matrix)
{
  return (fabs (matrix->coeff[2][0]) < EPSILON &&
          fabs (matrix->coeff[2][1]) < EPSILON &&
          fabs (matrix->coeff[2][2] - 1.0) < EPSILON);
}

/**
 * ligma_matrix3_is_simple:
 * @matrix: The matrix that is to be tested.
 *
 * Checks if we'll need to interpolate when applying this matrix as
 * a transformation.
 *
 * Returns: %TRUE if all entries of the upper left 2x2 matrix are
 *          either 0 or 1, %FALSE otherwise
 */
gboolean
ligma_matrix3_is_simple (const LigmaMatrix3 *matrix)
{
  gdouble absm;
  gint    i, j;

  for (i = 0; i < 2; i++)
    {
      for (j = 0; j < 2; j++)
        {
          absm = fabs (matrix->coeff[i][j]);
          if (absm > EPSILON && fabs (absm - 1.0) > EPSILON)
            return FALSE;
        }
    }

  return TRUE;
}

/**
 * ligma_matrix3_equal:
 * @matrix1: The first matrix
 * @matrix2: The second matrix
 *
 * Checks if two matrices are equal.
 *
 * Returns: %TRUE the matrices are equal, %FALSE otherwise
 *
 * Since: 2.10.16
 */
gboolean
ligma_matrix3_equal (const LigmaMatrix3 *matrix1,
                    const LigmaMatrix3 *matrix2)
{
  gint i, j;

  for (i = 0; i < 3; i++)
    {
      for (j = 0; j < 3; j++)
        {
          if (fabs (matrix1->coeff[i][j] - matrix2->coeff[i][j]) > EPSILON)
            return FALSE;
        }
    }

  return TRUE;
}

/**
 * ligma_matrix4_identity:
 * @matrix: A matrix.
 *
 * Sets the matrix to the identity matrix.
 *
 * Since: 2.10.16
 */
void
ligma_matrix4_identity (LigmaMatrix4 *matrix)
{
  gint i, j;

  for (i = 0; i < 4; i++)
    {
      for (j = 0; j < 4; j++)
        matrix->coeff[i][j] = i == j;
    }
}

/**
 * ligma_matrix4_mult:
 * @matrix1: The first input matrix.
 * @matrix2: The second input matrix which will be overwritten by the result.
 *
 * Multiplies two matrices and puts the result into the second one.
 *
 * Since: 2.10.16
 */
void
ligma_matrix4_mult (const LigmaMatrix4 *matrix1,
                   LigmaMatrix4       *matrix2)
{
  LigmaMatrix4 result = {};
  gint        i, j, k;

  for (i = 0; i < 4; i++)
    {
      for (j = 0; j < 4; j++)
        {
          for (k = 0; k < 4; k++)
            result.coeff[i][j] += matrix1->coeff[i][k] * matrix2->coeff[k][j];
        }
    }

  *matrix2 = result;
}

/**
 * ligma_matrix4_to_deg:
 * @matrix:
 * @a: (out):
 * @b: (out):
 * @c: (out):
 *
 **/
void
ligma_matrix4_to_deg (const LigmaMatrix4 *matrix,
                     gdouble           *a,
                     gdouble           *b,
                     gdouble           *c)
{
  *a = 180 * (asin (matrix->coeff[1][0]) / G_PI_2);
  *b = 180 * (asin (matrix->coeff[2][0]) / G_PI_2);
  *c = 180 * (asin (matrix->coeff[2][1]) / G_PI_2);
}

/**
 * ligma_matrix4_transform_point:
 * @matrix: The transformation matrix.
 * @x: The source X coordinate.
 * @y: The source Y coordinate.
 * @z: The source Z coordinate.
 * @newx: (out): The transformed X coordinate.
 * @newy: (out): The transformed Y coordinate.
 * @newz: (out): The transformed Z coordinate.
 *
 * Transforms a point in 3D as specified by the transformation matrix.
 *
 * Returns: The transformed W coordinate.
 *
 * Since: 2.10.16
 */
gdouble
ligma_matrix4_transform_point (const LigmaMatrix4 *matrix,
                              gdouble            x,
                              gdouble            y,
                              gdouble            z,
                              gdouble           *newx,
                              gdouble           *newy,
                              gdouble           *newz)
{
  gdouble neww;

  *newx = matrix->coeff[0][0] * x +
          matrix->coeff[0][1] * y +
          matrix->coeff[0][2] * z +
          matrix->coeff[0][3];
  *newy = matrix->coeff[1][0] * x +
          matrix->coeff[1][1] * y +
          matrix->coeff[1][2] * z +
          matrix->coeff[1][3];
  *newz = matrix->coeff[2][0] * x +
          matrix->coeff[2][1] * y +
          matrix->coeff[2][2] * z +
          matrix->coeff[2][3];
  neww  = matrix->coeff[3][0] * x +
          matrix->coeff[3][1] * y +
          matrix->coeff[3][2] * z +
          matrix->coeff[3][3];

  *newx /= neww;
  *newy /= neww;
  *newz /= neww;

  return neww;
}
