/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-2001 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __GIMP_CHECKS_H__
#define __GIMP_CHECKS_H__


/*  Transparency representation  */

typedef enum
{
  LIGHT_CHECKS = 0,
  GRAY_CHECKS  = 1,
  DARK_CHECKS  = 2,
  WHITE_ONLY   = 3,
  GRAY_ONLY    = 4,
  BLACK_ONLY   = 5
} GimpCheckType;

typedef enum
{
  SMALL_CHECKS  = 0,
  MEDIUM_CHECKS = 1,
  LARGE_CHECKS  = 2
} GimpCheckSize;

#endif  /*  __GIMP_CHECKS_H__  */

