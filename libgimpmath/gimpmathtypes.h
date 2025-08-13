/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpmathtypes.h
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

#ifndef __GIMP_MATH_TYPES_H__
#define __GIMP_MATH_TYPES_H__


#include <libgimpbase/gimpbasetypes.h>


G_BEGIN_DECLS

typedef struct _GimpMatrix2 GimpMatrix2;
typedef struct _GimpMatrix3 GimpMatrix3;
typedef struct _GimpMatrix4 GimpMatrix4;

/**
 * GimpMatrix2
 * @coeff: the coefficients
 *
 * A two by two matrix.
 **/
struct _GimpMatrix2
{
  gdouble coeff[2][2];
};

/**
 * GimpMatrix3
 * @coeff: the coefficients
 *
 * A three by three matrix.
 **/
struct _GimpMatrix3
{
  gdouble coeff[3][3];
};

/**
 * GimpMatrix4
 * @coeff: the coefficients
 *
 * A four by four matrix.
 **/
struct _GimpMatrix4
{
  gdouble coeff[4][4];
};


typedef struct _GimpVector2 GimpVector2;
typedef struct _GimpVector3 GimpVector3;
typedef struct _GimpVector4 GimpVector4;

/**
 * GimpVector2:
 * @x: the x axis
 * @y: the y axis
 *
 * A two dimensional vector.
 **/
struct _GimpVector2
{
  gdouble x, y;
};

/**
 * GIMP_TYPE_VECTOR2:
 *
 * Boxed type representing a two-dimensional vector.
 */
#define GIMP_TYPE_VECTOR2 (gimp_vector2_get_type ())

GType gimp_vector2_get_type (void) G_GNUC_CONST;


/**
 * GimpVector3:
 * @x: the x axis
 * @y: the y axis
 * @z: the z axis
 *
 * A three dimensional vector.
 **/
struct _GimpVector3
{
  gdouble x, y, z;
};

/**
 * GIMP_TYPE_VECTOR3:
 *
 * Boxed type representing a three-dimensional vector.
 */
#define GIMP_TYPE_VECTOR3 (gimp_vector3_get_type ())

GType gimp_vector3_get_type (void) G_GNUC_CONST;


/**
 * GimpVector4:
 * @x: the x axis
 * @y: the y axis
 * @z: the z axis
 * @w: the w axis
 *
 * A four dimensional vector.
 **/
struct _GimpVector4
{
  gdouble x, y, z, w;
};


G_END_DECLS

#endif /* __GIMP_MATH_TYPES_H__ */
