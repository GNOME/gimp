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


static void   gimp_plug_in_add_procedures (GimpPlugIn *plug_in);


/*  public functions  */

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

  gimp_plug_in_add_procedures (plug_in);
}

void
_gimp_plug_in_run (GimpPlugIn       *plug_in,
                   const gchar      *name,
                   gint              n_params,
                   const GimpParam  *params,
                   gint             *n_return_vals,
                   GimpParam       **return_vals)
{
  GimpProcedure *procedure;

  g_return_if_fail (GIMP_IS_PLUG_IN (plug_in));
  g_return_if_fail (name != NULL);

  gimp_plug_in_add_procedures (plug_in);

  procedure = gimp_plug_in_get_procedure (plug_in, name);

  if (procedure)
    {
      gimp_procedure_run_legacy (procedure,
                                 n_params,      params,
                                 n_return_vals, return_vals);
    }
}


/*  private functions  */

static void
gimp_plug_in_add_procedures (GimpPlugIn *plug_in)
{
  if (GIMP_PLUG_IN_GET_CLASS (plug_in)->list_procedures &&
      GIMP_PLUG_IN_GET_CLASS (plug_in)->create_procedure)
    {
      gchar **procedures;
      gint    n_procedures;
      gint    i;

      procedures =
        GIMP_PLUG_IN_GET_CLASS (plug_in)->list_procedures (plug_in,
                                                           &n_procedures);

      for (i = 0; i < n_procedures; i++)
        {
          GimpProcedure *procedure;

          procedure =
            GIMP_PLUG_IN_GET_CLASS (plug_in)->create_procedure (plug_in,
                                                                procedures[i]);

          gimp_plug_in_add_procedure (plug_in, procedure);
          g_object_unref (procedure);
        }

      g_strfreev (procedures);
    }
}
