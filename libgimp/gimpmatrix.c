/* gimpmatrix.c
 * Copyright (C) 1998 Jay Cox <jaycox@earthlink.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <glib.h>
#include <math.h>
#include <string.h>
#include "gimpmatrix.h"

#define EPSILON 1e-6


/**
 * gimp_matrix_transform_point:
 * @matrix: The transformation matrix.
 * @x: The source X coordinate.
 * @y: The source Y coordinate.
 * @newx: The transformed X coordinate.
 * @newy: The transformed Y coordinate.
 * 
 * Transforms a point in 2D as specified by the transformation matrix.
 */
void
gimp_matrix_transform_point (GimpMatrix  matrix, 
			     gdouble     x, 
			     gdouble     y,
			     gdouble    *newx, 
			     gdouble    *newy)
{
  gdouble w;

  w = matrix[2][0]*x + matrix[2][1]*y + matrix[2][2];

  if (w == 0.0)
    w = 1.0;
  else
    w = 1.0/w;

  *newx = (matrix[0][0]*x + matrix[0][1]*y + matrix[0][2])*w;
  *newy = (matrix[1][0]*x + matrix[1][1]*y + matrix[1][2])*w;
}

/**
 * gimp_matrix_mult:
 * @matrix1: The first input matrix.
 * @matrix2: The second input matrix which will be oeverwritten ba the result.
 * 
 * Multiplies two matrices and puts the result into the second one.
 */
void
gimp_matrix_mult (GimpMatrix matrix1, 
		  GimpMatrix matrix2)
{
  gint i, j;
  GimpMatrix tmp;
  gdouble t1, t2, t3;

  for (i = 0; i < 3; i++)
    {
      t1 = matrix1[i][0];
      t2 = matrix1[i][1];
      t3 = matrix1[i][2];
      for (j = 0; j < 3; j++)
	{
	  tmp[i][j]  = t1 * matrix2[0][j];
	  tmp[i][j] += t2 * matrix2[1][j];
	  tmp[i][j] += t3 * matrix2[2][j];
	}
    }

  /*  put the results in matrix2 */
  memcpy (&matrix2[0][0], &tmp[0][0], sizeof(GimpMatrix));
}

/**
 * gimp_matrix_identity:
 * @matrix: A matrix.
 * 
 * Sets the matrix to the identity matrix.
 */
void
gimp_matrix_identity (GimpMatrix matrix)
{
  static GimpMatrix identity = { {1.0, 0.0, 0.0},
				 {0.0, 1.0, 0.0},
				 {0.0, 0.0, 1.0} };

  memcpy (&matrix[0][0], &identity[0][0], sizeof(GimpMatrix));
}

/**
 * gimp_matrix_translate:
 * @matrix: The matrix that is to be translated.
 * @x: Translation in X direction.
 * @y: Translation in Y direction.
 * 
 * Translates the matrix by x and y.
 */
void
gimp_matrix_translate (GimpMatrix matrix, 
		       gdouble    x, 
		       gdouble    y)
{
  gdouble g, h, i;

  g = matrix[2][0];
  h = matrix[2][1];
  i = matrix[2][2];

  matrix[0][0] += x*g;
  matrix[0][1] += x*h;
  matrix[0][2] += x*i;
  matrix[1][0] += y*g;
  matrix[1][1] += y*h;
  matrix[1][2] += y*i;
}

/**
 * gimp_matrix_scale:
 * @matrix: The matrix that is to be scaled.
 * @x: X scale factor.
 * @y: Y scale factor.
 * 
 * Scales the matrix by x and y 
 */
void
gimp_matrix_scale (GimpMatrix matrix, 
		   gdouble    x, 
		   gdouble    y)
{
  matrix[0][0] *= x;
  matrix[0][1] *= x;
  matrix[0][2] *= x;

  matrix[1][0] *= y;
  matrix[1][1] *= y;
  matrix[1][2] *= y;
}

/**
 * gimp_matrix_rotate:
 * @matrix: The matrix that is to be rotated.
 * @theta: The angle of rotation (in radians).
 * 
 * Rotates the matrix by theta degrees.
 */
void
gimp_matrix_rotate (GimpMatrix matrix, 
		    gdouble theta)
{
  gdouble t1, t2;
  gdouble cost, sint;

  cost = cos(theta);
  sint = sin(theta);
  
  t1 = matrix[0][0];
  t2 = matrix[1][0];
  matrix[0][0] = cost*t1 - sint*t2;
  matrix[1][0] = sint*t1 + cost*t2;

  t1 = matrix[0][1];
  t2 = matrix[1][1];
  matrix[0][1] = cost*t1 - sint*t2;
  matrix[1][1] = sint*t1 + cost*t2;

  t1 = matrix[0][2];
  t2 = matrix[1][2];
  matrix[0][2] = cost*t1 - sint*t2;
  matrix[1][2] = sint*t1 + cost*t2;
}

/**
 * gimp_matrix_xshear:
 * @matrix: The matrix that is to be sheared.
 * @amount: X shear amount.
 * 
 * Shears the matrix in the X direction.
 */
void
gimp_matrix_xshear (GimpMatrix matrix, 
		    gdouble    amount)
{
  matrix[0][0] += amount * matrix[1][0];
  matrix[0][1] += amount * matrix[1][1];
  matrix[0][2] += amount * matrix[1][2];
}

/**
 * gimp_matrix_yshear:
 * @matrix: The matrix that is to be sheared.
 * @amount: Y shear amount.
 * 
 * Shears the matrix in the Y direction.
 */
void
gimp_matrix_yshear (GimpMatrix matrix, 
		    gdouble    amount)
{
  matrix[1][0] += amount * matrix[0][0];
  matrix[1][1] += amount * matrix[0][1];
  matrix[1][2] += amount * matrix[0][2];
}

/**
 * gimp_matrix_determinant:
 * @matrix: The input matrix. 
 * 
 * Calculates the determinant of the given matrix.
 * 
 * Returns: The determinant.
 */
gdouble
gimp_matrix_determinant (GimpMatrix matrix)
{
  gdouble determinant;

  determinant  = 
    matrix[0][0] * (matrix[1][1]*matrix[2][2] - matrix[1][2]*matrix[2][1]);
  determinant -= 
    matrix[1][0] * (matrix[0][1]*matrix[2][2] - matrix[0][2]*matrix[2][1]);
  determinant += 
    matrix[2][0] * (matrix[0][1]*matrix[1][2] - matrix[0][2]*matrix[1][1]);

  return determinant;
}

/**
 * gimp_matrix_invert:
 * @matrix: The matrix that is to be inverted.
 * @matrix_inv: A matrix the inverted matrix should be written into. 
 * 
 * Inverts the given matrix.
 */
void
gimp_matrix_invert (GimpMatrix matrix, 
		    GimpMatrix matrix_inv)
{
  gdouble det_1;

  det_1 = gimp_matrix_determinant (matrix);

  if (det_1 == 0.0)
    return;

  det_1 = 1.0 / det_1;

  matrix_inv[0][0] =   
    (matrix[1][1] * matrix[2][2] - matrix[1][2] * matrix[2][1]) * det_1;
 
  matrix_inv[1][0] = 
    - (matrix[1][0] * matrix[2][2] - matrix[1][2] * matrix[2][0]) * det_1;

  matrix_inv[2][0] =   
    (matrix[1][0] * matrix[2][1] - matrix[1][1] * matrix[2][0]) * det_1;

  matrix_inv[0][1] = 
    - (matrix[0][1] * matrix[2][2] - matrix[0][2] * matrix[2][1] ) * det_1;

  matrix_inv[1][1] = 
    (matrix[0][0] * matrix[2][2] - matrix[0][2] * matrix[2][0]) * det_1;

  matrix_inv[2][1] = 
    - (matrix[0][0] * matrix[2][1] - matrix[0][1] * matrix[2][0]) * det_1;
  
  matrix_inv[0][2] =
    (matrix[0][1] * matrix[1][2] - matrix[0][2] * matrix[1][1]) * det_1;
  
  matrix_inv[1][2] = 
    - (matrix[0][0] * matrix[1][2] - matrix[0][2] * matrix[1][0]) * det_1;
  
  matrix_inv[2][2] = 
    (matrix[0][0] * matrix[1][1] - matrix[0][1] * matrix[1][0]) * det_1;
}


/**
 * gimp_matrix_duplicate:
 * @src: The source matrix.
 * @target: The destination matrix. 
 * 
 * Copies the source matrix to the destination matrix.
 */
void
gimp_matrix_duplicate (GimpMatrix src, 
		       GimpMatrix target)
{
  memcpy (&target[0][0], &src[0][0], sizeof(GimpMatrix));
}


/*  functions to test for matrix properties  */


/**
 * gimp_matrix_is_diagonal:
 * @matrix: The matrix that is to be tested.
 * 
 * Checks if the given matrix is diagonal.
 * 
 * Returns: TRUE if the matrix is diagonal.
 */
gboolean
gimp_matrix_is_diagonal (GimpMatrix matrix)
{
  gint i, j;

  for (i = 0; i < 3; i++)
    {
      for (j = 0; j < 3; j++)
	{
	  if (i != j && fabs (matrix[i][j]) > EPSILON)
	    return FALSE;
	}
    }

  return TRUE;
}

/**
 * gimp_matrix_is_identity:
 * @matrix: The matrix that is to be tested.
 * 
 * Checks if the given matrix is the identity matrix.
 * 
 * Returns: TRUE if the matrix is the identity matrix.
 */
gboolean
gimp_matrix_is_identity (GimpMatrix matrix)
{
  gint i,j;

  for (i = 0; i < 3; i++)
    {
      for (j = 0; j < 3; j++)
	{
	  if (i == j)
	    {
	      if (fabs (matrix[i][j] - 1.0) > EPSILON)
		return FALSE;
	    }
	  else
	    {
	      if (fabs (matrix[i][j]) > EPSILON)
		return FALSE;
	    }
	}
    }

  return TRUE;
}

/*  Check if we'll need to interpolate when applying this matrix. 
    This function returns TRUE if all entries of the upper left 
    2x2 matrix are either 0 or 1 
 */


/**
 * gimp_matrix_is_simple:
 * @matrix: The matrix that is to be tested.
 * 
 * Checks if we'll need to interpolate when applying this matrix as
 * a transformation.
 * 
 * Returns: TRUE if all entries of the upper left 2x2 matrix are either 
 * 0 or 1
 */
gboolean
gimp_matrix_is_simple (GimpMatrix matrix)
{
  gdouble absm;
  gint i, j;

  for (i = 0; i < 2; i++)
    {
      for (j = 0; j < 2; j++)
	{
	  absm = fabs (matrix[i][j]);
	  if (absm > EPSILON && fabs (absm - 1.0) > EPSILON)
	    return FALSE;
	}
    }

  return TRUE;
}
