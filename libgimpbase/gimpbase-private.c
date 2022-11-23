/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmabase-private.c
 * Copyright (C) 2003 Sven Neumann <sven@ligma.org>
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

#include <gio/gio.h>

#include "ligmabasetypes.h"

#include "ligmabase-private.h"
#include "ligmacompatenums.h"


LigmaUnitVtable _ligma_unit_vtable = { NULL, };


void
ligma_base_init (LigmaUnitVtable *vtable)
{
  static gboolean ligma_base_initialized = FALSE;

  g_return_if_fail (vtable != NULL);

  if (ligma_base_initialized)
    g_error ("ligma_base_init() must only be called once!");

  _ligma_unit_vtable = *vtable;

  ligma_base_compat_enums_init ();

  ligma_base_initialized = TRUE;
}

void
ligma_base_compat_enums_init (void)
{
#if 0
  static gboolean ligma_base_compat_initialized = FALSE;
  GQuark          quark;

  if (ligma_base_compat_initialized)
    return;

  quark = g_quark_from_static_string ("ligma-compat-enum");

  /*  This is how a compat enum is registered, leave one here for
   *  documentation purposes, remove it as soon as we get a real
   *  compat enum again
   */
  g_type_set_qdata (LIGMA_TYPE_ADD_MASK_TYPE, quark,
                    (gpointer) LIGMA_TYPE_ADD_MASK_TYPE_COMPAT);

  ligma_base_compat_initialized = TRUE;
#endif
}
