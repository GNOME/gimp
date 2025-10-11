/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpparasiteio.c
 * Copyright (C) 1999 Tor Lillqvist <tml@iki.fi>
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

/*
 * Functions for building and parsing string representations of
 * various standard parasite types.
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include "gimpparasiteio.h"
#include "gimpversion-private.h"


GIMP_WARNING_API_BREAK("Undeprecate this whole API and make it private instead")


/**
 * SECTION: gimpparasiteio
 * @title: gimpparasiteio
 * @short_description: Utility functions to (de)serialize certain C
 *                     structures to/from #GimpParasite's.
 * @see_also: #GimpParasite
 *
 * Utility functions to (de)serialize certain C structures to/from*
 * #GimpParasite's.
 **/


/**
 * gimp_pixpipe_params_init:
 * @params:
 *
 * Do not use this function. It is deprecated and will be removed
 * eventually.
 *
 * Deprecated: 3.2: Only for core use.
 */
void
gimp_pixpipe_params_init (GimpPixPipeParams *params)
{
  gint i;

  g_return_if_fail (params != NULL);

  params->step       = 100;
  params->ncells     = 1;
  params->cellwidth  = 1;
  params->cellheight = 1;
  params->dim        = 1;
  params->cols       = 1;
  params->rows       = 1;
  params->placement  = g_strdup ("constant");

  for (i = 0; i < GIMP_PIXPIPE_MAXDIM; i++)
    params->selection[i] = g_strdup ("random");

  params->rank[0] = 1;
  for (i = 1; i < GIMP_PIXPIPE_MAXDIM; i++)
    params->rank[i] = 0;
}

/**
 * gimp_pixpipe_params_parse:
 * @parameters:
 * @params:
 *
 * Do not use this function. It is deprecated and will be removed
 * eventually.
 *
 * Deprecated: 3.2: Only for core use.
 */
void
gimp_pixpipe_params_parse (const gchar       *parameters,
                           GimpPixPipeParams *params)
{
  gchar *copy;
  gchar *p, *q, *r;
  gint   i;

  g_return_if_fail (parameters != NULL);
  g_return_if_fail (params != NULL);

  copy = g_strdup (parameters);

  q = copy;
  while ((p = strtok (q, " \r\n")) != NULL)
    {
      q = NULL;
      r = strchr (p, ':');
      if (r)
        *r = 0;

      if (strcmp (p, "ncells") == 0)
        {
          if (r)
            params->ncells = atoi (r + 1);
        }
      else if (strcmp (p, "step") == 0)
        {
          if (r)
            params->step = atoi (r + 1);
        }
      else if (strcmp (p, "dim") == 0)
        {
          if (r)
            {
              params->dim = atoi (r + 1);
              params->dim = CLAMP (params->dim, 1, GIMP_PIXPIPE_MAXDIM);
            }
        }
      else if (strcmp (p, "cols") == 0)
        {
          if (r)
            params->cols = atoi (r + 1);
        }
      else if (strcmp (p, "rows") == 0)
        {
          if (r)
            params->rows = atoi (r + 1);
        }
      else if (strcmp (p, "cellwidth") == 0)
        {
          if (r)
            params->cellwidth = atoi (r + 1);
        }
      else if (strcmp (p, "cellheight") == 0)
        {
          if (r)
            params->cellheight = atoi (r + 1);
        }
      else if (strcmp (p, "placement") == 0)
        {
          if (r)
            {
              g_free (params->placement);
              params->placement = g_strdup (r + 1);
            }
        }
      else if (strncmp (p, "rank", strlen ("rank")) == 0 && r)
        {
          if (r)
            {
              i = atoi (p + strlen ("rank"));
              if (i >= 0 && i < params->dim)
                params->rank[i] = atoi (r + 1);
            }
        }
      else if (strncmp (p, "sel", strlen ("sel")) == 0 && r)
        {
          if (r)
            {
              i = atoi (p + strlen ("sel"));
              if (i >= 0 && i < params->dim)
                {
                  g_free (params->selection[i]);
                  params->selection[i] = g_strdup (r + 1);
                }
            }
        }
      if (r)
        *r = ':';
    }

  g_free (copy);
}

/**
 * gimp_pixpipe_params_build:
 * @params:
 *
 * Do not use this function. It is deprecated and will be removed
 * eventually.
 *
 * Deprecated: 3.2: Only for core use.
 */
gchar *
gimp_pixpipe_params_build (GimpPixPipeParams *params)
{
  GString *str;
  gint     i;

  g_return_val_if_fail (params != NULL, NULL);

  str = g_string_new (NULL);

  g_string_printf (str, "ncells:%d cellwidth:%d cellheight:%d "
                   "step:%d dim:%d cols:%d rows:%d placement:%s",
                   params->ncells, params->cellwidth, params->cellheight,
                   params->step, params->dim,
                   params->cols, params->rows,
                   params->placement);

  for (i = 0; i < params->dim; i++)
    {
      g_string_append_printf (str, " rank%d:%d", i, params->rank[i]);
      g_string_append_printf (str, " sel%d:%s", i, params->selection[i]);
    }

  return g_string_free (str, FALSE);
}

/**
 * gimp_pixpipe_params_free:
 * @params:
 *
 * Do not use this function. It is deprecated and will be removed
 * eventually.
 *
 * Deprecated: 3.2: Only for core use.
 */
void
gimp_pixpipe_params_free (GimpPixPipeParams *params)
{
  gint i;

  g_free (params->placement);

  for (i = 0; i < GIMP_PIXPIPE_MAXDIM; i++)
    g_free (params->selection[i]);
}
