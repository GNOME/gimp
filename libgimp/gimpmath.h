/* gimpmath.h
 *
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball                
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

#ifndef __GIMPMATH_H__
#define __GIMPMATH_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <math.h>

/* Some portability enhancing stuff. For use both by the gimp app
 * as well as plug-ins and modules.
 *
 * Include this instead of just <math.h>.
 */

#ifndef G_PI			/* G_PI will be in GLib eventually */
#define G_PI    3.14159265358979323846
#endif
#ifndef G_PI_2			/* As will G_PI_2 */
#define G_PI_2  1.57079632679489661923
#endif
#ifndef G_PI_4			/* As will G_PI_4 */
#define G_PI_4  0.78539816339744830962
#endif
#ifndef G_SQRT2			/* As will G_SQRT2 */
#define G_SQRT2 1.4142135623730951
#endif

/* Use RINT() instead of rint() */
#ifdef HAVE_RINT
#define RINT(x) rint(x)
#else
#define RINT(x) floor ((x) + 0.5)
#endif

#define ROUND(x) ((int) ((x) + 0.5))

/* Square */
#define SQR(x) ((x) * (x))

/* limit a (0->511) int to 255 */
#define MAX255(a)  ((a) | (((a) & 256) - (((a) & 256) >> 8)))

/* clamp a >>int32<<-range int between 0 and 255 inclusive */
/* broken! -> #define CLAMP0255(a)  ((a & 0xFFFFFF00)? (~(a>>31)) : a) */
#define CLAMP0255(a)  CLAMP(a,0,255)

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /*  __GIMPMATH_H__  */
