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
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gio/gio.h>

#include "gimpbasetypes.h"

#include "gimpbase-private.h"
#include "gimpcompatenums.h"


GimpUnitVtable _gimp_unit_vtable = { NULL, };


void
gimp_base_init (GimpUnitVtable *vtable)
{
  static gboolean gimp_base_initialized = FALSE;

  g_return_if_fail (vtable != NULL);

  if (gimp_base_initialized)
    g_error ("gimp_base_init() must only be called once!");

  _gimp_unit_vtable = *vtable;

  gimp_base_compat_enums_init ();

  gimp_base_initialized = TRUE;
}

void
gimp_base_compat_enums_init (void)
{
  static gboolean gimp_base_compat_initialized = FALSE;
  GQuark          quark;

  if (gimp_base_compat_initialized)
    return;

  quark = g_quark_from_static_string ("gimp-compat-enum");

  g_type_set_qdata (GIMP_TYPE_ADD_MASK_TYPE, quark,
                    (gpointer) GIMP_TYPE_ADD_MASK_TYPE_COMPAT);
  g_type_set_qdata (GIMP_TYPE_BLEND_MODE, quark,
                    (gpointer) GIMP_TYPE_BLEND_MODE_COMPAT);
  g_type_set_qdata (GIMP_TYPE_BUCKET_FILL_MODE, quark,
                    (gpointer) GIMP_TYPE_BUCKET_FILL_MODE_COMPAT);
  g_type_set_qdata (GIMP_TYPE_CHANNEL_TYPE, quark,
                    (gpointer) GIMP_TYPE_CHANNEL_TYPE_COMPAT);
  g_type_set_qdata (GIMP_TYPE_CLONE_TYPE, quark,
                    (gpointer) GIMP_TYPE_CLONE_TYPE_COMPAT);
  g_type_set_qdata (GIMP_TYPE_CONVERT_PALETTE_TYPE, quark,
                    (gpointer) GIMP_TYPE_CONVERT_PALETTE_TYPE_COMPAT);
  g_type_set_qdata (GIMP_TYPE_CONVOLVE_TYPE, quark,
                    (gpointer) GIMP_TYPE_CONVOLVE_TYPE_COMPAT);
  g_type_set_qdata (GIMP_TYPE_DESATURATE_MODE, quark,
                    (gpointer) GIMP_TYPE_DESATURATE_MODE_COMPAT);
  g_type_set_qdata (GIMP_TYPE_DODGE_BURN_TYPE, quark,
                    (gpointer) GIMP_TYPE_DODGE_BURN_TYPE_COMPAT);
  g_type_set_qdata (GIMP_TYPE_FILL_TYPE, quark,
                    (gpointer) GIMP_TYPE_FILL_TYPE_COMPAT);
  g_type_set_qdata (GIMP_TYPE_HUE_RANGE, quark,
                    (gpointer) GIMP_TYPE_HUE_RANGE_COMPAT);
  g_type_set_qdata (GIMP_TYPE_ICON_TYPE, quark,
                    (gpointer) GIMP_TYPE_ICON_TYPE_COMPAT);
  g_type_set_qdata (GIMP_TYPE_INTERPOLATION_TYPE, quark,
                    (gpointer) GIMP_TYPE_INTERPOLATION_TYPE_COMPAT);
  g_type_set_qdata (GIMP_TYPE_TRANSFER_MODE, quark,
                    (gpointer) GIMP_TYPE_TRANSFER_MODE_COMPAT);

  gimp_base_compat_initialized = TRUE;
}
