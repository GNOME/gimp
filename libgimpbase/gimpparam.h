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

#ifndef __GIMP_PARAM_H__
#define __GIMP_PARAM_H__


/**
 * SECTION: gimpparam
 * @title: gimpparam
 * @short_description: Definitions of useful #GParamFlags.
 *
 * Definitions of useful #GParamFlags.
 **/


/**
 * GIMP_PARAM_STATIC_STRINGS:
 *
 * Since: 2.4
 **/
#define GIMP_PARAM_STATIC_STRINGS (G_PARAM_STATIC_NAME | \
                                   G_PARAM_STATIC_NICK | \
                                   G_PARAM_STATIC_BLURB)

/**
 * GIMP_PARAM_READABLE:
 *
 * Since: 2.4
 **/
#define GIMP_PARAM_READABLE       (G_PARAM_READABLE    | \
                                   GIMP_PARAM_STATIC_STRINGS)

/**
 * GIMP_PARAM_WRITABLE:
 *
 * Since: 2.4
 **/
#define GIMP_PARAM_WRITABLE       (G_PARAM_WRITABLE    | \
                                   GIMP_PARAM_STATIC_STRINGS)

/**
 * GIMP_PARAM_READWRITE:
 *
 * Since: 2.4
 **/
#define GIMP_PARAM_READWRITE      (G_PARAM_READWRITE   | \
                                   GIMP_PARAM_STATIC_STRINGS)


#endif  /*  __GIMP_PARAM_H__  */
