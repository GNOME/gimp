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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "appenv.h"
#include "canvas.h"
#include "drawable.h"
#include "interface.h"
#include "invert.h"
#include "gimage.h"
#include "pixelarea.h"
#include "pixelrow.h"

static void       invert (GimpDrawable *);
static Argument * invert_invoker (Argument *);

typedef void (*InvertRowFunc)(PixelRow *, PixelRow *);
static InvertRowFunc invert_row_func (Tag row_tag);
static void invert_row_u8 (PixelRow *, PixelRow *);
static void invert_row_u16 (PixelRow *, PixelRow *);
static void invert_row_float (PixelRow *, PixelRow *);


/*  Inverter  */
static
InvertRowFunc
invert_row_func (Tag row_tag)
{
  switch (tag_precision (row_tag))
  {
  case PRECISION_U8:
    return invert_row_u8; 
  case PRECISION_U16:
    return invert_row_u16; 
  case PRECISION_FLOAT:
    return invert_row_float; 
  default:
    return NULL;
  } 
}

static void
invert (drawable)
     GimpDrawable *drawable;
{
  PixelArea src_area, dest_area;
  PixelRow src_row, dest_row;
  int h; 
  void *pag;
  int x1, y1, x2, y2;
  InvertRowFunc invert_row;
  invert_row = invert_row_func (drawable_tag (drawable));
  drawable_mask_bounds (drawable, &x1, &y1, &x2, &y2);

  pixelarea_init (&src_area, drawable_data (drawable),
				x1, y1, (x2 - x1), (y2 - y1), FALSE);
  pixelarea_init (&dest_area, drawable_shadow (drawable),
				x1, y1, (x2 - x1), (y2 - y1), TRUE);

  for (pag = pixelarea_register (2, &src_area, &dest_area);
       pag != NULL;
       pag = pixelarea_process (pag))
    {
      h = pixelarea_height (&src_area);
      while (h--)
        {
          pixelarea_getdata (&src_area, &src_row, h);
          pixelarea_getdata (&dest_area, &dest_row, h);
          (*invert_row) (&src_row, &dest_row);
        }
    }
  
  drawable_merge_shadow (drawable, TRUE);
  drawable_update (drawable, x1, y1, (x2 - x1), (y2 - y1));
}


void
invert_row_u8(
		PixelRow *src_row,
		PixelRow *dest_row
		)

{
  gint    alpha, b;
  Tag     src_tag      = pixelrow_tag (src_row); 
  gint 	  has_alpha    = tag_alpha (src_tag) == ALPHA_YES ? TRUE: FALSE;
  guint8 *dest         = (guint8*)pixelrow_data (dest_row);
  guint8 *src          = (guint8*)pixelrow_data (src_row);
  gint    width        = pixelrow_width (src_row);
  gint    num_channels = tag_num_channels (src_tag);
  
  alpha = has_alpha ? (num_channels - 1) : num_channels;
  
  while (width --)
    {
      for (b = 0; b < alpha; b++)
	dest[b] = 255 - src[b];

      if (has_alpha)
	dest[alpha] = src[alpha];

      dest += num_channels;
      src += num_channels;
    }
}


void
invert_row_u16(
		PixelRow *src_row,
		PixelRow *dest_row
		)

{
  gint    alpha, b;
  Tag     src_tag      = pixelrow_tag (src_row); 
  gint 	  has_alpha    = tag_alpha (src_tag) == ALPHA_YES ? TRUE: FALSE;
  guint16 *dest         = (guint16*)pixelrow_data (dest_row);
  guint16 *src          = (guint16*)pixelrow_data (src_row);
  gint    width        = pixelrow_width (src_row);
  gint    num_channels = tag_num_channels (src_tag);
  
  alpha = has_alpha ? (num_channels - 1) : num_channels;
  
  while (width --)
    {
      for (b = 0; b < alpha; b++)
	dest[b] = 65535 - src[b];

      if (has_alpha)
	dest[alpha] = src[alpha];

      dest += num_channels;
      src += num_channels;
    }
}


void
invert_row_float(
		PixelRow *src_row,
		PixelRow *dest_row
		)

{
  gint    alpha, b;
  Tag     src_tag      = pixelrow_tag (src_row); 
  gint 	  has_alpha    = tag_alpha (src_tag) == ALPHA_YES ? TRUE: FALSE;
  gfloat *dest         = (gfloat*)pixelrow_data (dest_row);
  gfloat *src          = (gfloat*)pixelrow_data (src_row);
  gint    width        = pixelrow_width (src_row);
  gint    num_channels = tag_num_channels (src_tag);
  
  alpha = has_alpha ? (num_channels - 1) : num_channels;
  
  while (width --)
    {
      for (b = 0; b < alpha; b++)
	dest[b] = 1.0 - src[b];

      if (has_alpha)
	dest[alpha] = src[alpha];

      dest += num_channels;
      src += num_channels;
    }
}


void
image_invert (gimage_ptr)
     void *gimage_ptr;
{
  GImage *gimage;
  GimpDrawable *drawable;
  Argument *return_vals;
  int nreturn_vals;

  gimage = (GImage *) gimage_ptr;
  drawable = gimage_active_drawable (gimage);

  if (drawable_indexed (drawable))
    {
      g_message ("Invert does not operate on indexed drawables.");
      return;
    }

  return_vals = procedural_db_run_proc ("gimp_invert",
					&nreturn_vals,
					PDB_IMAGE, gimage->ID,
					PDB_DRAWABLE, drawable_ID (drawable),
					PDB_END);

  if (return_vals[0].value.pdb_int != PDB_SUCCESS)
    g_message ("Invert operation failed.");

  procedural_db_destroy_args (return_vals, nreturn_vals);
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
  GimpDrawable *drawable;

  drawable = NULL;

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
      drawable = drawable_get_ID (int_value);
      if (drawable == NULL || gimage != drawable_gimage (drawable))
	success = FALSE;
    }
  /*  make sure the drawable is not indexed color  */
  if (success)
    success = ! drawable_indexed (drawable);

  if (success)
    invert (drawable);

  return procedural_db_return_args (&invert_proc, success);
}
