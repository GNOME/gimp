/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpmath.h
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

#ifndef __GIMP_MATH_H__
#define __GIMP_MATH_H__


#include <math.h>

#ifdef HAVE_IEEEFP_H
#include <ieeefp.h>
#endif

#ifdef G_OS_WIN32
#include <float.h>
#endif

#define __GIMP_MATH_H_INSIDE__

#include <libgimpmath/gimpmathtypes.h>

#include <libgimpmath/gimpmatrix.h>
#include <libgimpmath/gimpmd5.h>
#include <libgimpmath/gimpvector.h>

#undef __GIMP_MATH_H_INSIDE__


G_BEGIN_DECLS


/**
 * SECTION: gimpmath
 * @title: GimpMath
 * @short_description: Mathematical definitions and macros.
 *
 * Mathematical definitions and macros for use both by the GIMP
 * application and plug-ins. These macros should be used rather than
 * the ones from &lt;math.h&gt; for enhanced portability.
 **/


/**
 * RINT:
 * @x: the value to be rounded
 *
 * This macro rounds its argument @x to an integer value in floating
 * point format. Use RINT() instead of rint().
 **/
#if defined (HAVE_RINT) && 0
/* note:  rint() depends on the current floating-point rounding mode.  when the
 * rounding mode is FE_TONEAREST, it, in parctice, breaks ties to even.  this
 * is different from 'floor (x + 0.5)', which breaks ties up.  in other words
 * 'rint (2.5) == 2.0', while 'floor (2.5 + 0.5) == 3.0'.  this is asking for
 * trouble, so let's just use the latter.
 */
#define RINT(x) rint(x)
#else
#define RINT(x) floor ((x) + 0.5)
#endif

/**
 * ROUND:
 * @x: the value to be rounded.
 *
 * This macro rounds its positive argument @x to the nearest integer.
 **/
#define ROUND(x) ((int) ((x) + 0.5))

/**
 * SIGNED_ROUND:
 * @x: the value to be rounded.
 *
 * This macro rounds its argument @x to the nearest integer.
 **/
#define SIGNED_ROUND(x) ((int) RINT (x))

/**
 * SQR:
 * @x: the value to be squared.
 *
 * This macro squares its argument @x.
 **/
#define SQR(x) ((x) * (x))

/**
 * MAX255:
 * @a: the value to be limited.
 *
 * This macro limits it argument @a, an (0-511) int, to 255.
 **/
#define MAX255(a)  ((a) | (((a) & 256) - (((a) & 256) >> 8)))

/**
 * CLAMP0255:
 * @a: the value to be clamped.
 *
 * This macro clamps its argument @a, an int32-range int, between 0
 * and 255 inclusive.
 **/
#define CLAMP0255(a)  CLAMP(a,0,255)

/**
 * SAFE_CLAMP:
 * @x:    the value to be limited.
 * @low:  the lower limit.
 * @high: the upper limit.
 *
 * Ensures that @x is between the limits set by @low and @high,
 * even if @x is NaN. If @low is greater than @high, or if either
 * of them is NaN, the result is undefined.
 *
 * Since: 2.10
 **/
#define SAFE_CLAMP(x, low, high)  ((x) > (low) ? (x) < (high) ? (x) : (high) : (low))

/**
 * gimp_deg_to_rad:
 * @angle: the angle to be converted.
 *
 * This macro converts its argument @angle from degree to radian.
 **/
#define gimp_deg_to_rad(angle) ((angle) * (2.0 * G_PI) / 360.0)

/**
 * gimp_rad_to_deg:
 * @angle: the angle to be converted.
 *
 * This macro converts its argument @angle from radian to degree.
 **/
#define gimp_rad_to_deg(angle) ((angle) * 360.0 / (2.0 * G_PI))


G_END_DECLS

#endif /* __GIMP_MATH_H__ */
