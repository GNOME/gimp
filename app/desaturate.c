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
#include "desaturate.h"
#include "interface.h"
#include "paint_funcs.h"

static void       desaturate (int);
static Argument * desaturate_invoker (Argument *);


void
image_desaturate (gimage_ptr)
     void *gimage_ptr;
{
  GImage *gimage;
  int drawable_id;

  gimage = (GImage *) gimage_ptr;
  drawable_id = gimage_active_drawable (gimage);

  if (! drawable_color (drawable_id))
    {
      message_box ("Desaturate operates only on RGB color drawables.", NULL, NULL);
      return;
    }
  desaturate (drawable_id);
}


/*  Desaturateer  */

static void
desaturate (drawable_id)
     int drawable_id;
{
  PixelRegion srcPR, destPR;
  unsigned char *src, *s;
  unsigned char *dest, *d;
  int h, j;
  int lightness, min, max;
  int has_alpha;
  void *pr;
  int x1, y1, x2, y2;

  has_alpha = drawable_has_alpha (drawable_id);
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
	      max = MAXIMUM (s[RED_PIX], s[GREEN_PIX]);
	      max = MAXIMUM (max, s[BLUE_PIX]);
	      min = MINIMUM (s[RED_PIX], s[GREEN_PIX]);
	      min = MINIMUM (min, s[BLUE_PIX]);

	      lightness = (max + min) / 2;

	      d[RED_PIX] = lightness;
	      d[GREEN_PIX] = lightness;
	      d[BLUE_PIX] = lightness;

	      if (has_alpha)
		d[ALPHA_PIX] = s[ALPHA_PIX];

	      d += destPR.bytes;
	      s += srcPR.bytes;
	    }

	  src += srcPR.rowstride;
	  dest += destPR.rowstride;
	}
    }

  drawable_merge_shadow (drawable_id, TRUE);
  drawable_update (drawable_id, x1, y1, (x2 - x1), (y2 - y1));
}


/*  The desaturate procedure definition  */
ProcArg desaturate_args[] =
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

ProcRecord desaturate_proc =
{
  "gimp_desaturate",
  "Desaturate the contents of the specified drawable",
  "This procedure desaturates the contents of the specified drawable.  This procedure only works on drawables of type RGB color.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  2,
  desaturate_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { desaturate_invoker } },
};


static Argument *
desaturate_invoker (args)
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

  /*  check to make sure the drawable is color  */
  if (success)
    success = drawable_color (drawable_id);

  if (success)
    desaturate (drawable_id);

  return procedural_db_return_args (&desaturate_proc, success);
}
