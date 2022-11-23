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

#include "config.h"

#include <glib.h>

#include "ligmathumb-error.h"


/**
 * SECTION: ligmathumb-error
 * @title: LigmaThumb-error
 * @short_description: Error codes used by libligmathumb
 *
 * Error codes used by libligmathumb
 **/


/**
 * ligma_thumb_error_quark:
 *
 * This function is never called directly. Use LIGMA_THUMB_ERROR() instead.
 *
 * Returns: the #GQuark that defines the LigmaThumb error domain.
 **/
GQuark
ligma_thumb_error_quark (void)
{
  return g_quark_from_static_string ("ligma-thumb-error-quark");
}
