/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1999 Peter Mattis and Spencer Kimball
 *
 * gimplimits.h
 * Copyright (C) 1999 Michael Natterer <mitschel@cs.tu-berlin.de>
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

#ifndef __GIMP_LIMITS_H__
#define __GIMP_LIMITS_H__

G_BEGIN_DECLS


/*  pixel sizes
 */
#define GIMP_MIN_IMAGE_SIZE  1
#define GIMP_MAX_IMAGE_SIZE  262144    /*  2^18  */

/*  dots per inch
 */
#define GIMP_MIN_RESOLUTION  5e-3      /*  shouldn't display as 0.000  */
#define GIMP_MAX_RESOLUTION  65536.0

/*  memory sizes
 */
#define GIMP_MAX_MEMSIZE     ((guint64) 1 << 42) /*  4 terabyte;
                                                  *  needs a 64bit variable
                                                  *  and must be < G_MAXDOUBLE
                                                  */


G_END_DECLS

#endif /* __GIMP_LIMITS_H__ */
