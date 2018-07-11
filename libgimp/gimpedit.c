/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2003 Peter Mattis and Spencer Kimball
 *
 * gimpedit.c
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

#include "config.h"

#include "gimp.h"


/**
 * gimp_edit_paste_as_new:
 *
 * Paste buffer to a new image.
 *
 * This procedure pastes a copy of the internal GIMP edit buffer to a
 * new image. The GIMP edit buffer will be empty unless a call was
 * previously made to either gimp_edit_cut() or gimp_edit_copy(). This
 * procedure returns the new image or -1 if the edit buffer was empty.
 *
 * Deprecated: Use gimp_edit_paste_as_new_image() instead.
 *
 * Returns: The new image.
 *
 * Since: 2.4
 **/
gint32
gimp_edit_paste_as_new (void)
{
  return gimp_edit_paste_as_new_image ();
}

/**
 * gimp_edit_named_paste_as_new:
 * @buffer_name: The name of the buffer to paste.
 *
 * Paste named buffer to a new image.
 *
 * This procedure works like gimp_edit_paste_as_new_image() but pastes a
 * named buffer instead of the global buffer.
 *
 * Deprecated: Use gimp_edit_named_paste_as_new_image() instead.
 *
 * Returns: The new image.
 *
 * Since: 2.4
 **/
gint32
gimp_edit_named_paste_as_new (const gchar *buffer_name)
{
  return gimp_edit_named_paste_as_new_image (buffer_name);
}
