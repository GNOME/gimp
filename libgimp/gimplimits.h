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
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef __GIMP_LIMITS_H__
#define __GIMP_LIMITS_H__

/*  pixel sizes
 */
#define GIMP_MIN_IMAGE_SIZE  1
#define GIMP_MAX_IMAGE_SIZE  16777216

/*  dots per inch
 */
#define GIMP_MIN_RESOLUTION  (1.0 / 65536.0)
#define GIMP_MAX_RESOLUTION  65536.0

/*  the size of the checks which indicate transparency...
 */
#define GIMP_CHECK_SIZE    8
#define GIMP_CHECK_SIZE_SM 4

/*  ...and their colors
 */
#define GIMP_CHECK_DARK  (1.0 / 3.0)
#define GIMP_CHECK_LIGHT (2.0 / 3.0)

#endif /* __GIMP_LIMITS_H__ */
