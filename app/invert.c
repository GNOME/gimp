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
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "appenv.h"
#include "drawable.h"
#include "interface.h"
#include "invert.h"

static void       invert (int);
static Argument * invert_invoker (Argument *);


void
image_invert (gimage_ptr)
     void *gimage_ptr;
{
  GImage *gimage;
  int drawable_id;
  Argument *return_vals;
  int nreturn_vals;

  gimage = (GImage *) gimage_ptr;
  drawable_id = gimage_active_drawable (gimage);

  if (drawable_indexed (drawable_id))
    {
      message_box ("Invert does not operate on indexed drawables.", NULL, NULL);
      return;
    }

  return_vals = procedural_db_run_proc ("gimp_invert",
					&nreturn_vals,
					PDB_IMAGE, gimage->ID,
					PDB_DRAWABLE, drawable_id,
					PDB_END);

  if (return_vals[0].value.pdb_int != PDB_SUCCESS)
    message_box ("Invert operation failed.", NULL, NULL);

  procedural_db_destroy_args (return_vals, nreturn_vals);
}


/*  Inverter  */

static void
invert (drawable_id)
     int drawable_id;
{
  PixelRegion srcPR, destPR;
  unsigned char *src, *s;
  unsigned char *dest, *d;
  int h, j, b;
  int has_alpha;
  int alpha, bytes;
  void *pr;
  int x1, y1, x2, y2;

  bytes = drawable_bytes (drawable_id);
  has_alpha = drawable_has_alpha (drawable_id);
  alpha = has_alpha ? (bytes - 1) : bytes;
  drawable_mask_bounds (drawable_id, &x1, &y1, &x2, &y2);
  pixel_region_init (&srcPR, drawable_data (drawable_id), x1, y1, (x2 - x1), (y2 - y1), FALSE);
  pixel_region_init (&destPR, drawable_shadow (drawable_id), x1, y1, (x2 - x1), (y2 - y1), TRUE);

  for (pr = pixel_regions_register (2, &srcPR, &destPR); pr != NULL; pr = pixel_regions_process (pr))
    {
      src = srcPR.data;
      dest = destPR.data;
      h = srcPR.h;

      while (h--)
	{
	  s = src;
	  d = dest;

	  for (j = 0; j < srcPR.w; j++)
	    {
	      for (b = 0; b < alpha; b++)
		d[b] = 255 - s[b];

	      if (has_alpha)
		d[alpha] = s[alpha];

	      d += bytes;
	      s += bytes;
	    }

	  src += srcPR.rowstride;
	  dest += destPR.rowstride;
	}
    }

  drawable_merge_shadow (drawable_id, TRUE);
  drawable_update (drawable_id, x1, y1, (x2 - x1), (y2 - y1));
}


/*  The invert procedure definition  */
ProcArg invert_args[] =
{
  { PDB_IMAGE,
    "image",
    "the image"
  },
  { PDB_DRAWABLE,
    "drawable",
    "the drawable"
  }
};

ProcRecord invert_proc =
{
  "gimp_invert",
  "Invert the contents of the specified drawable",
  "This procedure inverts the contents of the specified drawable.  Each intensity channel is inverted independently.  The inverted intensity is given as inten' = (255 - inten).  Indexed color drawables are not valid for this operation.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  2,
  invert_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { invert_invoker } },
};


static Argument *
invert_invoker (args)
     Argument *args;
{
  int success = TRUE;
  int int_value;
  GImage *gimage;
  int drawable_id;

  drawable_id = -1;

  /*  the gimage  */
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if (! (gimage = gimage_get_ID (int_value)))
	success = FALSE;
    }
  /*  the drawable  */
  if (success)
    {
      int_value = args[1].value.pdb_int;
      if (gimage != drawable_gimage (int_value))
	success = FALSE;
      else
	drawable_id = int_value;
    }
  /*  make sure the drawable is not indexed color  */
  if (success)
    success = ! drawable_indexed (drawable_id);

  if (success)
    invert (drawable_id);

  return procedural_db_return_args (&invert_proc, success);
}
