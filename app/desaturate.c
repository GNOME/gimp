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
#include "desaturate.h"
#include "float16.h"
#include "interface.h"
#include "gimage.h"
#include "pixelarea.h"
#include "pixelrow.h"

static void       desaturate (GimpDrawable *);
static Argument * desaturate_invoker (Argument *);

typedef void (*DesaturateRowFunc)(PixelRow *, PixelRow *);
static DesaturateRowFunc desaturate_row_func (Tag row_tag);
static void desaturate_row_u8 (PixelRow *, PixelRow *);
static void desaturate_row_u16 (PixelRow *, PixelRow *);
static void desaturate_row_float (PixelRow *, PixelRow *);
static void desaturate_row_float16 (PixelRow *, PixelRow *);


/*  Inverter  */
static
DesaturateRowFunc
desaturate_row_func (Tag row_tag)
{
  switch (tag_precision (row_tag))
  {
  case PRECISION_U8:
    return desaturate_row_u8; 
  case PRECISION_U16:
    return desaturate_row_u16; 
  case PRECISION_FLOAT:
    return desaturate_row_float; 
  case PRECISION_FLOAT16:
    return desaturate_row_float16; 
  default:
    return NULL;
  } 
}

void
image_desaturate (gimage_ptr)
     void *gimage_ptr;
{
  GImage *gimage;
  GimpDrawable *drawable;

  gimage = (GImage *) gimage_ptr;
  drawable = gimage_active_drawable (gimage);

  if (! drawable_color (drawable))
    {
      g_message ("Desaturate operates only on RGB color drawables.");
      return;
    }
  desaturate (drawable);
}


/*  Desaturateer  */

static void
desaturate (GimpDrawable *drawable)
{
  PixelArea src_area, dest_area;
  PixelRow src_row, dest_row;
  gint h;
  void *pr;
  int x1, y1, x2, y2;
  DesaturateRowFunc desaturate_row = desaturate_row_func(drawable_tag (drawable));

  if (!drawable) 
    return;

  drawable_mask_bounds (drawable, &x1, &y1, &x2, &y2);
  pixelarea_init (&src_area, drawable_data (drawable), 
		x1, y1, (x2 - x1), (y2 - y1), FALSE);
  pixelarea_init (&dest_area, drawable_shadow (drawable), 
		x1, y1, (x2 - x1), (y2 - y1), TRUE);

  for (pr = pixelarea_register (2, &src_area, &dest_area); 
	pr != NULL; 
	pr = pixelarea_process (pr))
    {
      h = pixelarea_height (&src_area);
      while (h--)
	{
          pixelarea_getdata (&src_area, &src_row, h);
          pixelarea_getdata (&dest_area, &dest_row, h);
          (*desaturate_row)(&src_row, &dest_row);
	}
    }

  drawable_merge_shadow (drawable, TRUE);
  drawable_update (drawable, x1, y1, (x2 - x1), (y2 - y1));
}


static void 
desaturate_row_u8( 
		   PixelRow *src_row,
		   PixelRow *dest_row
		) 
{
  Tag src_tag = pixelrow_tag (src_row);
  Tag dest_tag = pixelrow_tag (dest_row);
  gint src_num_channels = tag_num_channels (src_tag);
  gint dest_num_channels = tag_num_channels (dest_tag);
  gint has_alpha = (tag_alpha (src_tag) == ALPHA_YES)? TRUE: FALSE; 
  guint8 *s = (guint8*)pixelrow_data (src_row);
  guint8 *d = (guint8*)pixelrow_data (dest_row);
  int w;
  int lightness, min, max;
  
  w = pixelrow_width (src_row);
  while (w--)
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

      d += dest_num_channels;
      s += src_num_channels;
    }
}

static void
desaturate_row_u16( 
		   PixelRow *src_row,
		   PixelRow *dest_row
		) 
{
  Tag src_tag = pixelrow_tag (src_row);
  Tag dest_tag = pixelrow_tag (dest_row);
  gint src_num_channels = tag_num_channels (src_tag);
  gint dest_num_channels = tag_num_channels (dest_tag);
  gint has_alpha = (tag_alpha (src_tag) == ALPHA_YES)? TRUE: FALSE; 
  guint16 *s = (guint16*)pixelrow_data (src_row);
  guint16 *d = (guint16*)pixelrow_data (dest_row);
  int w;
  int lightness, min, max;
  
  w = pixelrow_width (src_row);
  while (w--)
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

      d += dest_num_channels;
      s += src_num_channels;
    }
}

static void 
desaturate_row_float16( 
		   PixelRow *src_row,
		   PixelRow *dest_row
		) 
{
  Tag src_tag = pixelrow_tag (src_row);
  Tag dest_tag = pixelrow_tag (dest_row);
  gint src_num_channels = tag_num_channels (src_tag);
  gint dest_num_channels = tag_num_channels (dest_tag);
  gint has_alpha = (tag_alpha (src_tag) == ALPHA_YES)? TRUE: FALSE; 
  guint16 *s = (guint16*)pixelrow_data (src_row);
  guint16 *d = (guint16*)pixelrow_data (dest_row);
  int w;
  int lightness, min, max;
  gfloat r, g, b, l;
  ShortsFloat u;
  
  w = pixelrow_width (src_row);
  while (w--)
    {
      r = FLT (s[RED_PIX], u);
      g = FLT (s[GREEN_PIX], u);
      b = FLT (s[BLUE_PIX], u);
      max = MAXIMUM (r, g);
      max = MAXIMUM (max, b);
      min = MINIMUM (r, g);
      min = MINIMUM (min, b);

      lightness = (max + min) / 2;

      l = FLT16 (lightness, u);
      d[RED_PIX] = l;
      d[GREEN_PIX] = l;
      d[BLUE_PIX] = l;

      if (has_alpha)
	d[ALPHA_PIX] = s[ALPHA_PIX];

      d += dest_num_channels;
      s += src_num_channels;
    }
}

static void 
desaturate_row_float( 
		   PixelRow *src_row,
		   PixelRow *dest_row
		) 
{
  Tag src_tag = pixelrow_tag (src_row);
  Tag dest_tag = pixelrow_tag (dest_row);
  gint src_num_channels = tag_num_channels (src_tag);
  gint dest_num_channels = tag_num_channels (dest_tag);
  gint has_alpha = (tag_alpha (src_tag) == ALPHA_YES)? TRUE: FALSE; 
  gfloat *s = (gfloat*)pixelrow_data (src_row);
  gfloat *d = (gfloat*)pixelrow_data (dest_row);
  int w;
  int lightness, min, max;
  
  w = pixelrow_width (src_row);
  while (w--)
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

      d += dest_num_channels;
      s += src_num_channels;
    }
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

  /*  check to make sure the drawable is color  */
  if (success)
    success = drawable_color (drawable);

  if (success)
    desaturate (drawable);

  return procedural_db_return_args (&desaturate_proc, success);
}
