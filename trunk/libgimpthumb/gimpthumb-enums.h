/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * Thumbnail handling according to the Thumbnail Managing Standard.
 * http://triq.net/~pearl/thumbnail-spec/
 *
 * Copyright (C) 2001-2003  Sven Neumann <sven@gimp.org>
 *                          Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_THUMB_ENUMS_H__
#define __GIMP_THUMB_ENUMS_H__

G_BEGIN_DECLS


#define GIMP_TYPE_THUMB_FILE_TYPE (gimp_thumb_file_type_get_type ())

GType gimp_thumb_file_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_THUMB_FILE_TYPE_NONE,
  GIMP_THUMB_FILE_TYPE_REGULAR,
  GIMP_THUMB_FILE_TYPE_FOLDER,
  GIMP_THUMB_FILE_TYPE_SPECIAL
} GimpThumbFileType;


#define GIMP_TYPE_THUMB_SIZE (gimp_thumb_size_get_type ())

GType gimp_thumb_size_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_THUMB_SIZE_FAIL   = 0,
  GIMP_THUMB_SIZE_NORMAL = 128,
  GIMP_THUMB_SIZE_LARGE  = 256
} GimpThumbSize;


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
