/* LIBGIMP - The GIMP Library
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

#ifndef __GIMP_CHECKS_H__
#define __GIMP_CHECKS_H__

G_BEGIN_DECLS


/*  the default size of the checks which indicate transparency ...
 */
#define GIMP_CHECK_SIZE      8
#define GIMP_CHECK_SIZE_SM   4

/*  ... and their default shades
 */
#define GIMP_CHECK_DARK      0.4
#define GIMP_CHECK_LIGHT     0.6


void  gimp_checks_get_shades (GimpCheckType  type,
                              guchar        *light,
                              guchar        *dark);


G_END_DECLS

#endif  /* __GIMP_CHECKS_H__ */
