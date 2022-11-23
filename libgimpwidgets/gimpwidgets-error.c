/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmawidgets-error.c
 * Copyright (C) 2008 Martin Nordholts <martinn@svn.gnome.org>
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

#include "ligmawidgets-error.h"


/**
 * ligma_widgets_error_quark:
 *
 * This function is never called directly. Use LIGMA_WIDGETS_ERROR() instead.
 *
 * Returns: the #GQuark that defines the LIGMA widgets error domain.
 **/
GQuark
ligma_widgets_error_quark (void)
{
  return g_quark_from_static_string ("ligma-widgets-error-quark");
}
