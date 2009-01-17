/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpbase-private.c
 * Copyright (C) 2003 Sven Neumann <sven@gimp.org>
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
 * <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <glib-object.h>

#include "libgimpbase/gimpbase.h"

#include "gimpbasetypes.h"

#include "gimpbase-private.h"


GimpUnitVtable _gimp_unit_vtable = { NULL, };


void
gimp_base_init (GimpUnitVtable *vtable)
{
  static gboolean gimp_base_initialized = FALSE;

  g_return_if_fail (vtable != NULL);

  if (gimp_base_initialized)
    g_error ("gimp_base_init() must only be called once!");

  _gimp_unit_vtable = *vtable;

  gimp_base_initialized = TRUE;
}
