/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include "libgimp/gimp.h"

#include "dgimp_procs.h"

GParam *
dgimp_convert_params (int      nparams,
		      GParam  *params)
{
  GParam *new_params;
  int i;

  new_params = g_new (GParam, nparams + 4);
  memcpy (new_params, params, nparams * sizeof (GParam));

  for (i = 0; i < 4; i++)
    {
      new_params[i].type = PARAM_INT32;
      new_params[i].data.d_int32 = 0;
    }

  return new_params;
}

gint
dgimp_auto_dist (gchar   *proc_name,
		 int      nparams,
		 GParam  *params,
		 int      x1,
		 int      y1,
		 int      x2,
		 int      y2)
{

}
