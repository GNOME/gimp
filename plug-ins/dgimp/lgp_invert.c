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

#include "lgp_procs.h"

static void
lgp_invert_proc (char    *name,
		 int      nparams,
		 GParam  *param,
		 int     *nreturn_vals,
		 GParam **return_vals)
{
  static GParam values[1];

  g_print ("LGP: processing invert\n");
  fflush (stdout);

  *nreturn_vals = 1;
  *return_vals = values;

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = STATUS_SUCCESS;
}

void
lgp_invert_install ()
{
  static GParamDef args[] =
  {
    { PARAM_IMAGE, "image", "the image" },
    { PARAM_DRAWABLE, "drawable", "the drawable" },
    { PARAM_INT32, "x1", "upper-left x coord" },
    { PARAM_INT32, "y1", "upper-left y coord" },
    { PARAM_INT32, "x2", "lower-right x coord" },
    { PARAM_INT32, "y2", "lower-right y coord" },
  };
  static GParamDef *return_vals = NULL;
  static int nargs = sizeof (args) / sizeof (args[0]);
  static int nreturn_vals = 0;

  gimp_install_temp_proc ("lgp_invert",
			  "Invert a portion of an image",
			  "NONE",
			  "Spencer Kimball",
			  "Spencer Kimball",
			  "1997",
			  "", NULL, PROC_TEMPORARY,
			  nargs, nreturn_vals,
			  args, return_vals,
			  lgp_invert_proc);
}
