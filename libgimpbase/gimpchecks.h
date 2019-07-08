/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#if !defined (__GIMP_BASE_H_INSIDE__) && !defined (GIMP_BASE_COMPILATION)
#error "Only <libgimpbase/gimpbase.h> can be included directly."
#endif

#ifndef __GIMP_CHECKS_H__
#define __GIMP_CHECKS_H__

G_BEGIN_DECLS


/**
 * GIMP_CHECK_SIZE:
 *
 * The default checkerboard size in pixels. This is configurable in
 * the core but GIMP plug-ins can't access the user preference and
 * should use this constant instead.
 **/
#define GIMP_CHECK_SIZE     8

/**
 * GIMP_CHECK_SIZE_SM:
 *
 * The default small checkerboard size in pixels.
 **/
#define GIMP_CHECK_SIZE_SM  4


/**
 * GIMP_CHECK_DARK:
 *
 * The dark gray value for the default checkerboard pattern.
 **/
#define GIMP_CHECK_DARK   0.4

/**
 * GIMP_CHECK_LIGHT:
 *
 * The dark light value for the default checkerboard pattern.
 **/
#define GIMP_CHECK_LIGHT  0.6


void  gimp_checks_get_shades (GimpCheckType  type,
                              guchar        *light,
                              guchar        *dark);


G_END_DECLS

#endif  /* __GIMP_CHECKS_H__ */
