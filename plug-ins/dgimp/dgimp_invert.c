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

void
dgimp_invert_proc (char    *name,
		   int      nparams,
		   GParam  *params,
		   int     *nreturn_vals,
		   GParam **return_vals)
{
  GParam *lgp_params;
  gint drawable_id;
  gint x1, y1, x2, y2;

  /*  Get drawable and LGP parameters  */
  drawable_id = params[1].data.d_drawable;
  /*lgp_params = dgimp_convert_params (nparams, params);*/

  /*  Get work area  */
  gimp_drawable_mask_bounds (drawable_id, &x1, &y1, &x2, &y2);

  /*  Distribute work  */
  /*  dgimp_auto_dist ("lgp_invert", nparams + 4, lgp_params, x1, y1, x2, y2);*/

  /*  Merge shadow and update  */
  gimp_drawable_merge_shadow (drawable_id, TRUE);
  gimp_drawable_update (drawable_id, x1, y1, (x2 - x1), (y2 - y1));

  /*gimp_destory_params (lgp_params, nparams + 4);*/

  *nreturn_vals = 1;
  *return_vals = success_value;
}
