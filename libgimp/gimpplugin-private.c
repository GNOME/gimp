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

#include "gimp.h"
#include "gimpplugin-private.h"
#include "gimpprocedure-private.h"


/*  public functions  */

void
_gimp_plug_in_init (GimpPlugIn *plug_in)
{
  gchar **procedures;
  gint    n_procedures;
  gint    i;

  g_return_if_fail (GIMP_IS_PLUG_IN (plug_in));

  if (! GIMP_PLUG_IN_GET_CLASS (plug_in)->init_procedures)
    return;

  procedures = GIMP_PLUG_IN_GET_CLASS (plug_in)->init_procedures (plug_in,
                                                                  &n_procedures);

  for (i = 0; i < n_procedures; i++)
    {
      GimpProcedure *procedure;

      procedure = gimp_plug_in_create_procedure (plug_in, procedures[i]);
      _gimp_procedure_register (procedure);
      g_object_unref (procedure);
    }
}

void
_gimp_plug_in_query (GimpPlugIn *plug_in)
{
  gchar **procedures;
  gint    n_procedures;
  gint    i;

  g_return_if_fail (GIMP_IS_PLUG_IN (plug_in));

  if (! GIMP_PLUG_IN_GET_CLASS (plug_in)->query_procedures)
    return;

  procedures = GIMP_PLUG_IN_GET_CLASS (plug_in)->query_procedures (plug_in,
                                                                   &n_procedures);

  for (i = 0; i < n_procedures; i++)
    {
      GimpProcedure *procedure;

      procedure = gimp_plug_in_create_procedure (plug_in, procedures[i]);
      _gimp_procedure_register (procedure);
      g_object_unref (procedure);
    }
}

void
_gimp_plug_in_quit (GimpPlugIn *plug_in)
{
  g_return_if_fail (GIMP_IS_PLUG_IN (plug_in));

  if (GIMP_PLUG_IN_GET_CLASS (plug_in)->quit)
    GIMP_PLUG_IN_GET_CLASS (plug_in)->quit (plug_in);
}
