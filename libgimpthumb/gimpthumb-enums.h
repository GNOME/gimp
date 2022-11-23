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

#ifndef __LIGMA_THUMB_ENUMS_H__
#define __LIGMA_THUMB_ENUMS_H__

G_BEGIN_DECLS


/**
 * SECTION: ligmathumb-enums
 * @title: LigmaThumb-enums
 * @short_description: Enumerations used by libligmathumb
 *
 * Enumerations used by libligmathumb
 **/


/**
 * LigmaThumbFileType:
 * @LIGMA_THUMB_FILE_TYPE_NONE:    file does not exist
 * @LIGMA_THUMB_FILE_TYPE_REGULAR: a regular file
 * @LIGMA_THUMB_FILE_TYPE_FOLDER:  a directory
 * @LIGMA_THUMB_FILE_TYPE_SPECIAL: a special file (device node, fifo, socket, ...)
 *
 * File types as returned by ligma_thumb_file_test().
 **/
#define LIGMA_TYPE_THUMB_FILE_TYPE (ligma_thumb_file_type_get_type ())

GType ligma_thumb_file_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_THUMB_FILE_TYPE_NONE,
  LIGMA_THUMB_FILE_TYPE_REGULAR,
  LIGMA_THUMB_FILE_TYPE_FOLDER,
  LIGMA_THUMB_FILE_TYPE_SPECIAL
} LigmaThumbFileType;


/**
 * LigmaThumbSize:
 * @LIGMA_THUMB_SIZE_FAIL:   special size used to indicate a thumbnail
 *                          creation failure
 * @LIGMA_THUMB_SIZE_NORMAL: normal thumbnail size (128 pixels)
 * @LIGMA_THUMB_SIZE_LARGE:  large thumbnail size (256 pixels)
 *
 * Possible thumbnail sizes as defined by the Thumbnail Managing
 * Standard.
 **/
#define LIGMA_TYPE_THUMB_SIZE (ligma_thumb_size_get_type ())

GType ligma_thumb_size_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_THUMB_SIZE_FAIL   = 0,
  LIGMA_THUMB_SIZE_NORMAL = 128,
  LIGMA_THUMB_SIZE_LARGE  = 256
} LigmaThumbSize;


/**
 * LigmaThumbState:
 * @LIGMA_THUMB_STATE_UNKNOWN:   nothing is known about the file/thumbnail
 * @LIGMA_THUMB_STATE_REMOTE:    the file is on a remote file system
 * @LIGMA_THUMB_STATE_FOLDER:    the file is a directory
 * @LIGMA_THUMB_STATE_SPECIAL:   the file is a special file
 * @LIGMA_THUMB_STATE_NOT_FOUND: the file/thumbnail doesn't exist
 * @LIGMA_THUMB_STATE_EXISTS:    the file/thumbnail exists
 * @LIGMA_THUMB_STATE_OLD:       the thumbnail may be outdated
 * @LIGMA_THUMB_STATE_FAILED:    the thumbnail couldn't be created
 * @LIGMA_THUMB_STATE_OK:        the thumbnail exists and matches the image
 *
 * Possible image and thumbnail file states used by libligmathumb.
 **/
#define LIGMA_TYPE_THUMB_STATE (ligma_thumb_state_get_type ())

GType ligma_thumb_state_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_THUMB_STATE_UNKNOWN,
  LIGMA_THUMB_STATE_REMOTE,
  LIGMA_THUMB_STATE_FOLDER,
  LIGMA_THUMB_STATE_SPECIAL,
  LIGMA_THUMB_STATE_NOT_FOUND,
  LIGMA_THUMB_STATE_EXISTS,
  LIGMA_THUMB_STATE_OLD,
  LIGMA_THUMB_STATE_FAILED,
  LIGMA_THUMB_STATE_OK
} LigmaThumbState;


G_END_DECLS

#endif  /* __LIGMA_THUMB_ENUMS_H__ */
