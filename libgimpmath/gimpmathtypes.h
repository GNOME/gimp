/* LIBGIMP - The GIMP Library 
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpmathtypes.h
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

#ifndef __GIMP_MATH_TYPES_H__
#define __GIMP_MATH_TYPES_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


typedef gdouble GimpMatrix3[3][3];
typedef gdouble GimpMatrix4[4][4];

typedef struct _GimpVector2 GimpVector2;
typedef struct _GimpVector3 GimpVector3;
typedef struct _GimpVector4 GimpVector4;

struct _GimpVector2
{
  gdouble x, y;
};

struct _GimpVector3
{
  gdouble x, y, z;
};

struct _GimpVector4
{
  gdouble x, y, z, w;
};


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GIMP_MATH_TYPES_H__ */
