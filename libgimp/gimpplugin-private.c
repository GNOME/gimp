/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2003 Peter Mattis and Spencer Kimball
 *
 * gimpplugin-private.c
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

#include <string.h>

#include "gimp.h"
#include "gimpplugin-private.h"


void
_gimp_plug_in_init (GimpPlugIn *plug_in)
{
  g_return_if_fail (GIMP_IS_PLUG_IN (plug_in));

  if (GIMP_PLUG_IN_GET_CLASS (plug_in)->init)
    GIMP_PLUG_IN_GET_CLASS (plug_in)->init (plug_in);
}

void
_gimp_plug_in_quit (GimpPlugIn *plug_in)
{
  g_return_if_fail (GIMP_IS_PLUG_IN (plug_in));

  if (GIMP_PLUG_IN_GET_CLASS (plug_in)->quit)
    GIMP_PLUG_IN_GET_CLASS (plug_in)->quit (plug_in);
}

void
_gimp_plug_in_query (GimpPlugIn *plug_in)
{
  g_return_if_fail (GIMP_IS_PLUG_IN (plug_in));

  if (GIMP_PLUG_IN_GET_CLASS (plug_in)->query)
    GIMP_PLUG_IN_GET_CLASS (plug_in)->query (plug_in);
}

void
_gimp_plug_in_run (GimpPlugIn       *plug_in,
                   const gchar      *name,
                   gint              n_params,
                   const GimpParam  *param,
                   gint             *n_return_vals,
                   GimpParam       **return_vals)
{
  g_return_if_fail (GIMP_IS_PLUG_IN (plug_in));

  if (GIMP_PLUG_IN_GET_CLASS (plug_in)->run)
    GIMP_PLUG_IN_GET_CLASS (plug_in)->run (plug_in,
                                           name,
                                           n_params, param,
                                           n_return_vals, return_vals);
}
