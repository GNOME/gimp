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

#ifndef __GIMP_THUMB_ENUMS_H__
#define __GIMP_THUMB_ENUMS_H__

G_BEGIN_DECLS


/**
 * SECTION: gimpthumb-enums
 * @title: GimpThumb-enums
 * @short_description: Enumerations used by libgimpthumb
 *
 * Enumerations used by libgimpthumb
 **/


/**
 * GimpThumbFileType:
 * @GIMP_THUMB_FILE_TYPE_NONE:    file does not exist
 * @GIMP_THUMB_FILE_TYPE_REGULAR: a regular file
 * @GIMP_THUMB_FILE_TYPE_FOLDER:  a directory
 * @GIMP_THUMB_FILE_TYPE_SPECIAL: a special file (device node, fifo, socket, ...)
 *
 * File types as returned by gimp_thumb_file_test().
 **/
#define GIMP_TYPE_THUMB_FILE_TYPE (gimp_thumb_file_type_get_type ())

GType gimp_thumb_file_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_THUMB_FILE_TYPE_NONE,
  GIMP_THUMB_FILE_TYPE_REGULAR,
  GIMP_THUMB_FILE_TYPE_FOLDER,
  GIMP_THUMB_FILE_TYPE_SPECIAL
} GimpThumbFileType;


/**
 * GimpThumbSize:
 * @GIMP_THUMB_SIZE_FAIL:   special size used to indicate a thumbnail
 *                          creation failure
 * @GIMP_THUMB_SIZE_NORMAL: normal thumbnail size (128 pixels)
 * @GIMP_THUMB_SIZE_LARGE:  large thumbnail size (256 pixels)
 *
 * Possible thumbnail sizes as defined by the Thumbnail Managing
 * Standard.
 **/
#define GIMP_TYPE_THUMB_SIZE (gimp_thumb_size_get_type ())

GType gimp_thumb_size_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_THUMB_SIZE_FAIL   = 0,
  GIMP_THUMB_SIZE_NORMAL = 128,
  GIMP_THUMB_SIZE_LARGE  = 256
} GimpThumbSize;


/**
 * GimpThumbState:
 * @GIMP_THUMB_STATE_UNKNOWN:   nothing is known about the file/thumbnail
 * @GIMP_THUMB_STATE_REMOTE:    the file is on a remote file system
 * @GIMP_THUMB_STATE_FOLDER:    the file is a directory
 * @GIMP_THUMB_STATE_SPECIAL:   the file is a special file
 * @GIMP_THUMB_STATE_NOT_FOUND: the file/thumbnail doesn't exist
 * @GIMP_THUMB_STATE_EXISTS:    the file/thumbnail exists
 * @GIMP_THUMB_STATE_OLD:       the thumbnail may be outdated
 * @GIMP_THUMB_STATE_FAILED:    the thumbnail couldn't be created
 * @GIMP_THUMB_STATE_OK:        the thumbnail exists and matches the image
 *
 * Possible image and thumbnail file states used by libgimpthumb.
 **/
#define GIMP_TYPE_THUMB_STATE (gimp_thumb_state_get_type ())

GType gimp_thumb_state_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_THUMB_STATE_UNKNOWN,
  GIMP_THUMB_STATE_REMOTE,
  GIMP_THUMB_STATE_FOLDER,
  GIMP_THUMB_STATE_SPECIAL,
  GIMP_THUMB_STATE_NOT_FOUND,
  GIMP_THUMB_STATE_EXISTS,
  GIMP_THUMB_STATE_OLD,
  GIMP_THUMB_STATE_FAILED,
  GIMP_THUMB_STATE_OK
} GimpThumbState;


G_END_DECLS

#endif  /* __GIMP_THUMB_ENUMS_H__ */
