/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmamathtypes.h
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

#ifndef __LIGMA_MATH_TYPES_H__
#define __LIGMA_MATH_TYPES_H__


#include <libligmabase/ligmabasetypes.h>


G_BEGIN_DECLS

typedef struct _LigmaMatrix2 LigmaMatrix2;
typedef struct _LigmaMatrix3 LigmaMatrix3;
typedef struct _LigmaMatrix4 LigmaMatrix4;

/**
 * LigmaMatrix2
 * @coeff: the coefficients
 *
 * A two by two matrix.
 **/
struct _LigmaMatrix2
{
  gdouble coeff[2][2];
};

/**
 * LigmaMatrix3
 * @coeff: the coefficients
 *
 * A three by three matrix.
 **/
struct _LigmaMatrix3
{
  gdouble coeff[3][3];
};

/**
 * LigmaMatrix4
 * @coeff: the coefficients
 *
 * A four by four matrix.
 **/
struct _LigmaMatrix4
{
  gdouble coeff[4][4];
};


typedef struct _LigmaVector2 LigmaVector2;
typedef struct _LigmaVector3 LigmaVector3;
typedef struct _LigmaVector4 LigmaVector4;

/**
 * LigmaVector2:
 * @x: the x axis
 * @y: the y axis
 *
 * A two dimensional vector.
 **/
struct _LigmaVector2
{
  gdouble x, y;
};

/**
 * LIGMA_TYPE_VECTOR2:
 *
 * Boxed type representing a two-dimensional vector.
 */
#define LIGMA_TYPE_VECTOR2 (ligma_vector2_get_type ())

GType ligma_vector2_get_type (void) G_GNUC_CONST;


/**
 * LigmaVector3:
 * @x: the x axis
 * @y: the y axis
 * @z: the z axis
 *
 * A three dimensional vector.
 **/
struct _LigmaVector3
{
  gdouble x, y, z;
};

/**
 * LIGMA_TYPE_VECTOR3:
 *
 * Boxed type representing a three-dimensional vector.
 */
#define LIGMA_TYPE_VECTOR3 (ligma_vector3_get_type ())

GType ligma_vector3_get_type (void) G_GNUC_CONST;


/**
 * LigmaVector4:
 * @x: the x axis
 * @y: the y axis
 * @z: the z axis
 * @w: the w axis
 *
 * A four dimensional vector.
 **/
struct _LigmaVector4
{
  gdouble x, y, z, w;
};


G_END_DECLS

#endif /* __LIGMA_MATH_TYPES_H__ */
