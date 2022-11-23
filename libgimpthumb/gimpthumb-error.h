/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * Thumbnail handling according to the Thumbnail Managing Standard.
 * https://specifications.freedesktop.org/thumbnail-spec/
 *
 * Copyright (C) 2001-2003  Sven Neumann <sven@ligma.org>
 *                          Michael Natterer <mitch@ligma.org>
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

#if !defined (__LIGMA_THUMB_H_INSIDE__) && !defined (LIGMA_THUMB_COMPILATION)
#error "Only <libligmathumb/ligmathumb.h> can be included directly."
#endif

#ifndef __LIGMA_THUMB_ERROR_H__
#define __LIGMA_THUMB_ERROR_H__

G_BEGIN_DECLS


/**
 * LigmaThumbError:
 * @LIGMA_THUMB_ERROR_OPEN:         there was a problem opening the file
 * @LIGMA_THUMB_ERROR_OPEN_ENOENT:  the file doesn't exist
 * @LIGMA_THUMB_ERROR_MKDIR:        there was a problem creating a directory
 *
 * These are the possible error codes used when a #GError is set by
 * libligmathumb.
 **/
typedef enum
{
  LIGMA_THUMB_ERROR_OPEN,
  LIGMA_THUMB_ERROR_OPEN_ENOENT,
  LIGMA_THUMB_ERROR_MKDIR
} LigmaThumbError;


/**
 * LIGMA_THUMB_ERROR:
 *
 * Identifier for the libligmathumb error domain.
 **/
#define LIGMA_THUMB_ERROR (ligma_thumb_error_quark ())

GQuark  ligma_thumb_error_quark (void) G_GNUC_CONST;


G_END_DECLS

#endif  /* __LIGMA_THUMB_ERROR_H__ */
