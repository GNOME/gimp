/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <glib-object.h>

#include "gimppluginerror.h"


/**
 * gimp_plug_in_error_quark:
 *
 * This function is never called directly. Use GIMP_PLUG_IN_ERROR() instead.
 *
 * Returns: the #GQuark that defines the GimpPlugIn error domain.
 **/
GQuark
gimp_plug_in_error_quark (void)
{
  return g_quark_from_static_string ("gimp-plug-in-error-quark");
}
