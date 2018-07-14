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

#include "config.h"

#include <glib.h>

#include "gimpthumb-error.h"


/**
 * SECTION: gimpthumb-error
 * @title: GimpThumb-error
 * @short_description: Error codes used by libgimpthumb
 *
 * Error codes used by libgimpthumb
 **/


/**
 * gimp_thumb_error_quark:
 *
 * This function is never called directly. Use GIMP_THUMB_ERROR() instead.
 *
 * Return value: the #GQuark that defines the GimpThumb error domain.
 **/
GQuark
gimp_thumb_error_quark (void)
{
  return g_quark_from_static_string ("gimp-thumb-error-quark");
}
