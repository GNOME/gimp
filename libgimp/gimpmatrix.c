/* gimpmatrix.c
 * Copyright (C) 1998 Jay Cox <jaycox@earthlink.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "gimpmatrix.h"
#include <math.h>

void
gimp_matrix_transform_point (GimpMatrix m, double x, double y,
			     double *newx, double *newy)
{
  double w;
  w = m[2][0]*x + m[2][1]*y + m[2][2];

  if (w == 0.0)
    w = 1.0;
  else
    w = 1.0/w;

  *newx = (m[0][0]*x + m[0][1]*y + m[0][2])*w;
  *newy = (m[1][0]*x + m[1][1]*y + m[1][2])*w;
}

void
gimp_matrix_mult (GimpMatrix m1, GimpMatrix m2)
{
  int i, j;
  GimpMatrix tmp;
  double t1, t2, t3;

  for (i = 0; i < 3; i++)
  {
    t1 = m1[i][0];
    t2 = m1[i][1];
    t3 = m1[i][2];
    for (j = 0; j < 3; j++)
    {
      tmp[i][j]  = t1 * m2[0][j];
      tmp[i][j] += t2 * m2[1][j];
      tmp[i][j] += t3 * m2[2][j];
    }
  }
  /*  put the results in m2 */
  memcpy(&m2[0][0], &tmp[0][0], sizeof(double)*9);
}

void
gimp_matrix_identity (GimpMatrix m)
{
  static GimpMatrix identity = {{1.0, 0.0, 0.0},
				{0.0, 1.0, 0.0},
				{0.0, 0.0, 1.0} };
  memcpy(&m[0][0], &identity[0][0], sizeof(double)*9);
}

void
gimp_matrix_translate (GimpMatrix m, double x, double y)
{
  double g, h, i;
  g = m[2][0];
  h = m[2][1];
  i = m[2][2];
  m[0][0] += x*g;
  m[0][1] += x*h;
  m[0][2] += x*i;
  m[1][0] += y*g;
  m[1][1] += y*h;
  m[1][2] += y*i;
}

void
gimp_matrix_scale (GimpMatrix m, double x, double y)
{
  m[0][0] *= x;
  m[0][1] *= x;
  m[0][2] *= x;

  m[1][0] *= y;
  m[1][1] *= y;
  m[1][2] *= y;
}

void
gimp_matrix_rotate (GimpMatrix m, double theta)
{
  double t1, t2;
  double cost, sint;

  cost = cos(theta);
  sint = sin(theta);
  
  t1 = m[0][0];
  t2 = m[1][0];
  m[0][0] = cost*t1 - sint*t2;
  m[1][0] = sint*t1 + cost*t2;

  t1 = m[0][1];
  t2 = m[1][1];
  m[0][1] = cost*t1 - sint*t2;
  m[1][1] = sint*t1 + cost*t2;

  t1 = m[0][2];
  t2 = m[1][2];
  m[0][2] = cost*t1 - sint*t2;
  m[1][2] = sint*t1 + cost*t2;
}

void
gimp_matrix_xshear (GimpMatrix m, double amnt)
{
  m[0][0] += amnt * m[1][0];
  m[0][1] += amnt * m[1][1];
  m[0][2] += amnt * m[1][2];
}

void
gimp_matrix_yshear (GimpMatrix m, double amnt)
{
  m[1][0] += amnt * m[0][0];
  m[1][1] += amnt * m[0][1];
  m[1][2] += amnt * m[0][2];
}


double
gimp_matrix_determinant (GimpMatrix m)
{
  double determinant;

  determinant  = m[0][0] * (m[1][1]*m[2][2] - m[1][2]*m[2][1]);
  determinant -= m[1][0] * (m[0][1]*m[2][2] - m[0][2]*m[2][1]);
  determinant += m[2][0] * (m[0][1]*m[1][2] - m[0][2]*m[1][1]);

  return determinant;
}

void
gimp_matrix_invert (GimpMatrix m, GimpMatrix m_inv)
{
  double det_1;

  det_1 = gimp_matrix_determinant (m);
  if (det_1 == 0.0)
    return;
  det_1 = 1.0 / det_1;

  m_inv[0][0] =   ( m[1][1] * m[2][2] - m[1][2] * m[2][1] ) * det_1;
  m_inv[1][0] = - ( m[1][0] * m[2][2] - m[1][2] * m[2][0] ) * det_1;
  m_inv[2][0] =   ( m[1][0] * m[2][1] - m[1][1] * m[2][0] ) * det_1;
  m_inv[0][1] = - ( m[0][1] * m[2][2] - m[0][2] * m[2][1] ) * det_1;
  m_inv[1][1] =   ( m[0][0] * m[2][2] - m[0][2] * m[2][0] ) * det_1;
  m_inv[2][1] = - ( m[0][0] * m[2][1] - m[0][1] * m[2][0] ) * det_1;
  m_inv[0][2] =   ( m[0][1] * m[1][2] - m[0][2] * m[1][1] ) * det_1;
  m_inv[1][2] = - ( m[0][0] * m[1][2] - m[0][2] * m[1][0] ) * det_1;
  m_inv[2][2] =   ( m[0][0] * m[1][1] - m[0][1] * m[1][0] ) * det_1;
}
