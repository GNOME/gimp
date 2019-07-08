/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * Thumbnail handling according to the Thumbnail Managing Standard.
 * https://specifications.freedesktop.org/thumbnail-spec/
 *
 * Copyright (C) 2001-2003  Sven Neumann <sven@gimp.org>
 *                          Michael Natterer <mitch@gimp.org>
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

#if !defined (__GIMP_THUMB_H_INSIDE__) && !defined (GIMP_THUMB_COMPILATION)
#error "Only <libgimpthumb/gimpthumb.h> can be included directly."
#endif

#ifndef __GIMP_THUMB_ERROR_H__
#define __GIMP_THUMB_ERROR_H__

G_BEGIN_DECLS


/**
 * GimpThumbError:
 * @GIMP_THUMB_ERROR_OPEN:         there was a problem opening the file
 * @GIMP_THUMB_ERROR_OPEN_ENOENT:  the file doesn't exist
 * @GIMP_THUMB_ERROR_MKDIR:        there was a problem creating a directory
 *
 * These are the possible error codes used when a #GError is set by
 * libgimpthumb.
 **/
typedef enum
{
  GIMP_THUMB_ERROR_OPEN,
  GIMP_THUMB_ERROR_OPEN_ENOENT,
  GIMP_THUMB_ERROR_MKDIR
} GimpThumbError;


/**
 * GIMP_THUMB_ERROR:
 *
 * Identifier for the libgimpthumb error domain.
 **/
#define GIMP_THUMB_ERROR (gimp_thumb_error_quark ())

GQuark  gimp_thumb_error_quark (void) G_GNUC_CONST;


G_END_DECLS

#endif  /* __GIMP_THUMB_ERROR_H__ */
