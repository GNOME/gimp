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
#include "boundary.h"
#include "tag.h"
#include "gimprc.h"
#include "paint.h"
#include "paint_funcs_row.h"
#include "paint_funcs_row_u8.h"
#include "paint_funcs_row_u16.h"
#include "paint_funcs_row_float.h"
#include "pixelrow.h"

#define RANDOM_SEED        314159265
#define RANDOM_TABLE_SIZE  4096
static int random_table [RANDOM_TABLE_SIZE];
static int random_table_initialized = FALSE;
static void random_table_initialize (void);


static void
random_table_initialize ( 
			)
{
  gint i;
  /*  generate a table of random seeds  */
  srand (RANDOM_SEED);
  for (i = 0; i < RANDOM_TABLE_SIZE; i++)
    random_table[i] = rand ();
  
  random_table_initialized = TRUE;
}

void
paint_funcs_randomize_row (int y)
{
  srand (random_table [y % RANDOM_TABLE_SIZE]);
}

void
color_row (
	      PixelRow *dest_row,
	      Paint   *col
	      )
{

  switch (tag_precision (pixelrow_tag (dest_row)))
    {
    case PRECISION_U8:
      color_row_u8 (dest_row, col);
      break;
    case PRECISION_U16:
      color_row_u16 (dest_row, col);
      break;
    case PRECISION_FLOAT:
      color_row_float (dest_row, col);
      break;
    case PRECISION_NONE:
      break;	
    }
}
void
blend_row (
              PixelRow *src1_row,
	      PixelRow *src2_row,
	      PixelRow *dest_row,
	      Paint    *blend
              )
{
  switch (tag_precision (pixelrow_tag (dest_row)))
    {
    case PRECISION_U8:
      blend_row_u8 (src1_row, src2_row, dest_row, blend);
      break;
    case PRECISION_U16:
      blend_row_u16 (src1_row, src2_row, dest_row, blend);
      break;
    case PRECISION_FLOAT:
      blend_row_float (src1_row, src2_row, dest_row, blend);
      break;	
    case PRECISION_NONE:
      break;	
    }
}
void
shade_row (
		 PixelRow *src_row,
	         PixelRow *dest_row,
	         Paint    *color,
		 Paint    *blend
	         )
{
  switch (tag_precision (pixelrow_tag (dest_row)))
    {
    case PRECISION_U8:
      shade_row_u8 (src_row, dest_row, color, blend);
      break;
    case PRECISION_U16:
      shade_row_u16 (src_row, dest_row, color, blend);
      break;
    case PRECISION_FLOAT:
      shade_row_float (src_row, dest_row, color, blend);
      break;	
    case PRECISION_NONE:
      break;	
    }
}
void 
copy_row  (
           PixelRow * src_row,
           PixelRow * dest_row
           )
{
  switch (tag_precision (pixelrow_tag (src_row)))
    {
    case PRECISION_U8:
      copy_row_u8 (src_row, dest_row);
      break;
    case PRECISION_U16:
      copy_row_u16 (src_row, dest_row);
      break;
    case PRECISION_FLOAT:
      copy_row_float (src_row, dest_row);
      break;
    case PRECISION_NONE:
      g_warning ("doh in copy_row()");
      break;	
    }
}
void
extract_alpha_row (
		      PixelRow *src_row,
		      PixelRow *mask_row,
		      PixelRow *dest_row
		      )

{
  switch (tag_precision (pixelrow_tag (dest_row)))
    {
    case PRECISION_U8:
      extract_alpha_row_u8 (src_row, mask_row, dest_row);
      break;
    case PRECISION_U16:
      extract_alpha_row_u16 (src_row, mask_row, dest_row);
      break;
    case PRECISION_FLOAT:
      extract_alpha_row_float (src_row, mask_row, dest_row);
      break;	
    case PRECISION_NONE:
      break;	
    }
}
void
darken_row (
	 	PixelRow *src1_row,
		PixelRow *src2_row,
		PixelRow *dest_row
	       )
{
  switch (tag_precision (pixelrow_tag (dest_row)))
    {
    case PRECISION_U8:
      darken_row_u8 (src1_row, src2_row, dest_row);
      break;
    case PRECISION_U16:
      darken_row_u16 (src1_row, src2_row, dest_row);
      break;
    case PRECISION_FLOAT:
      darken_row_float (src1_row, src2_row, dest_row);
      break;	
    case PRECISION_NONE:
      break;	
    }
}
void
lighten_row (
		   PixelRow *src1_row,
		   PixelRow *src2_row,
                   PixelRow *dest_row
		   )
{
  switch (tag_precision (pixelrow_tag (dest_row)))
    {
    case PRECISION_U8:
      lighten_row_u8 (src1_row, src2_row, dest_row);
      break;
    case PRECISION_U16:
      lighten_row_u16 (src1_row, src2_row, dest_row);
      break;
    case PRECISION_FLOAT:
      lighten_row_float (src1_row, src2_row, dest_row);
      break;	
    case PRECISION_NONE:
      break;	
    }
}
void
hsv_only_row (
		    PixelRow *src1_row,
		    PixelRow *src2_row,
		    PixelRow *dest_row,
		    gint       mode
		    )
{
  switch (tag_precision (pixelrow_tag (dest_row)))
    {
    case PRECISION_U8:
      hsv_only_row_u8 (src1_row, src2_row, dest_row, mode);
      break;
    case PRECISION_U16:
      hsv_only_row_u16 (src1_row, src2_row, dest_row, mode);
      break;
    case PRECISION_FLOAT:
      hsv_only_row_float (src1_row, src2_row, dest_row, mode);
      break;	
    case PRECISION_NONE:
      break;	
    }
}
void
color_only_row (
		      PixelRow *src1_row,
		      PixelRow *src2_row,
		      PixelRow *dest_row,
		      gint       mode
		     )
{
  switch (tag_precision (pixelrow_tag (dest_row)))
    {
    case PRECISION_U8:
      color_only_row_u8 (src1_row, src2_row, dest_row, mode);
      break;
    case PRECISION_U16:
      color_only_row_u16 (src1_row, src2_row, dest_row, mode);
      break;
    case PRECISION_FLOAT:
      color_only_row_float (src1_row, src2_row, dest_row, mode);
      break;	
    case PRECISION_NONE:
      break;	
    }
}
void
multiply_row (
		 PixelRow *src1_row,
		 PixelRow *src2_row,
		 PixelRow *dest_row
		 )
{
  switch (tag_precision (pixelrow_tag (dest_row)))
    {
    case PRECISION_U8:
      multiply_row_u8 (src1_row, src2_row, dest_row);
      break;
    case PRECISION_U16:
      multiply_row_u16 (src1_row, src2_row, dest_row);
      break;
    case PRECISION_FLOAT:
      multiply_row_float (src1_row, src2_row, dest_row);
      break;	
    case PRECISION_NONE:
      break;	
    }
}

void
screen_row (
		  PixelRow *src1_row,
		  PixelRow *src2_row,
		  PixelRow *dest_row
	          )
{
  switch (tag_precision (pixelrow_tag (dest_row)))
    {
    case PRECISION_U8:
      screen_row_u8 (src1_row, src2_row, dest_row);
      break;
    case PRECISION_U16:
      screen_row_u16 (src1_row, src2_row, dest_row);
      break;
    case PRECISION_FLOAT:
      screen_row_float (src1_row, src2_row, dest_row);
      break;	
    case PRECISION_NONE:
      break;	
    }
}

void
overlay_row (
		   PixelRow *src1_row,
		   PixelRow *src2_row,
		   PixelRow *dest_row
		   )
{
  switch (tag_precision (pixelrow_tag (dest_row)))
    {
    case PRECISION_U8:
      overlay_row_u8 (src1_row, src2_row, dest_row);
      break;
    case PRECISION_U16:
      overlay_row_u16 (src1_row, src2_row, dest_row);
      break;
    case PRECISION_FLOAT:
      overlay_row_float (src1_row, src2_row, dest_row);
      break;	
    case PRECISION_NONE:
      break;	
    }
}

void
add_row ( 
	       PixelRow *src1_row,
	       PixelRow *src2_row,
	       PixelRow *dest_row
	      )
{
  switch (tag_precision (pixelrow_tag (dest_row)))
    {
    case PRECISION_U8:
      add_row_u8 (src1_row, src2_row, dest_row);
      break;
    case PRECISION_U16:
      add_row_u16 (src1_row, src2_row, dest_row);
      break;
    case PRECISION_FLOAT:
      add_row_float (src1_row, src2_row, dest_row);
      break;	
    case PRECISION_NONE:
      break;	
    }
}

void
subtract_row (
		    PixelRow *src1_row,
		    PixelRow *src2_row,
		    PixelRow *dest_row
		    )
{
  switch (tag_precision (pixelrow_tag (dest_row)))
    {
    case PRECISION_U8:
      subtract_row_u8 (src1_row, src2_row, dest_row);
      break;
    case PRECISION_U16:
      subtract_row_u16 (src1_row, src2_row, dest_row);
      break;
    case PRECISION_FLOAT:
      subtract_row_float (src1_row, src2_row, dest_row);
      break;	
    case PRECISION_NONE:
      break;	
    }
}

void
difference_row (
		      PixelRow *src1_row,
		      PixelRow *src2_row,
		      PixelRow *dest_row
		      )
{
  switch (tag_precision (pixelrow_tag (dest_row)))
    {
    case PRECISION_U8:
      difference_row_u8 (src1_row, src2_row, dest_row);
      break;
    case PRECISION_U16:
      difference_row_u16 (src1_row, src2_row, dest_row);
      break;
    case PRECISION_FLOAT:
      difference_row_float (src1_row, src2_row, dest_row);
      break;	
    case PRECISION_NONE:
      break;	
    }
}

void
dissolve_row (
		    PixelRow *src_row,
		    PixelRow *dest_row,
		    gint      x,
		    gint      y,
		    Paint    *opac
		    )
{
  gint b;
  
  if (!random_table_initialized) 
    random_table_initialize();
  
  /*  Set up the random number generator  */
  srand (random_table [y % RANDOM_TABLE_SIZE]);
  for (b = 0; b < x; b++)
    rand ();
  
  switch (tag_precision (pixelrow_tag (dest_row)))
    {
    case PRECISION_U8:
      dissolve_row_u8 (src_row, dest_row, x, y, opac);
      break;
    case PRECISION_U16:
      dissolve_row_u16 (src_row, dest_row, x, y, opac);
      break;
    case PRECISION_FLOAT:
      dissolve_row_float (src_row, dest_row, x, y, opac);
      break;	
    case PRECISION_NONE:
      break;	
    }
}

void
replace_row (
		   PixelRow *src1_row,
		   PixelRow *src2_row,
		   PixelRow *dest_row,
		   PixelRow *mask_row,
		   Paint    *opac,
		   gint      *affect
		   )
{
  switch (tag_precision (pixelrow_tag (dest_row)))
    {
    case PRECISION_U8:
      replace_row_u8 (src1_row, src2_row, dest_row, mask_row, opac, affect);
      break;
    case PRECISION_U16:
      replace_row_u16 (src1_row, src2_row, dest_row, mask_row, opac, affect);
      break;
    case PRECISION_FLOAT:
      replace_row_float (src1_row, src2_row, dest_row, mask_row, opac, affect);
      break;	
    case PRECISION_NONE:
      break;	
    }
}

void
swap_row (
                PixelRow *src_row,
	        PixelRow *dest_row
	        )
{
#if 1
  guint8 *dest = (guint8*)pixelrow_data (dest_row);
  guint8 *src  = (guint8*)pixelrow_data (src_row);
  gint    width = pixelrow_width (dest_row) * tag_bytes (pixelrow_tag (dest_row));
  
  while (width--)
    {
      *src = *src ^ *dest;
      *dest = *dest ^ *src;
      *src = *src ^ *dest;
      src++;
      dest++;
    }
#else
  switch (tag_precision (pixelrow_tag (dest_row)))
    {
    case PRECISION_U8:
      swap_row_u8 (src_row, dest_row);
      break;
    case PRECISION_U16:
      swap_row_u16 (src_row, dest_row);
      break;
    case PRECISION_FLOAT:
      swap_row_float (src_row, dest_row);
      break;	
    case PRECISION_NONE:
      break;	
    }
#endif
}

void
scale_row (
		 PixelRow *src_row,
		 PixelRow *dest_row,
		 gfloat      scale
		  )
{
  switch (tag_precision (pixelrow_tag (dest_row)))
    {
    case PRECISION_U8:
      scale_row_u8 (src_row, dest_row, scale);
      break;
    case PRECISION_U16:
      scale_row_u16 (src_row, dest_row, scale);
      break;
    case PRECISION_FLOAT:
      scale_row_float (src_row, dest_row, scale);
      break;	
    case PRECISION_NONE:
      break;	
    }
}

void
add_alpha_row (
		  PixelRow *src_row,
		  PixelRow *dest_row
		  )
{
  switch (tag_precision (pixelrow_tag (dest_row)))
    {
    case PRECISION_U8:
      add_alpha_row_u8 (src_row, dest_row);
      break;
    case PRECISION_U16:
      add_alpha_row_u16 (src_row, dest_row);
      break;
    case PRECISION_FLOAT:
      add_alpha_row_float (src_row, dest_row);
      break;	
    case PRECISION_NONE:
      break;	
    }
}

void
flatten_row (
		   PixelRow *src_row,
		   PixelRow *dest_row,
 		   Paint    *background
		  )
{
  switch (tag_precision (pixelrow_tag (dest_row)))
    {
    case PRECISION_U8:
      flatten_row_u8 (src_row, dest_row, background);
      break;
    case PRECISION_U16:
      flatten_row_u16 (src_row, dest_row, background);
      break;
    case PRECISION_FLOAT:
      flatten_row_float (src_row, dest_row, background);
      break;	
    case PRECISION_NONE:
      break;	
    }
}

void
multiply_alpha_row( 
		      PixelRow * src_row
		     )
{
  switch (tag_precision (pixelrow_tag (src_row)))
    {
    case PRECISION_U8:
      multiply_alpha_row_u8 (src_row);
      break;
    case PRECISION_U16:
      multiply_alpha_row_u16 (src_row);
      break;
    case PRECISION_FLOAT:
      multiply_alpha_row_float (src_row);
      break;	
    case PRECISION_NONE:
      break;	
    }
}

void separate_alpha_row( 
			    PixelRow *src_row
                           )
{
  switch (tag_precision (pixelrow_tag (src_row)))
    {
    case PRECISION_U8:
      separate_alpha_row_u8 (src_row);
      break;
    case PRECISION_U16:
      separate_alpha_row_u16 (src_row);
      break;
    case PRECISION_FLOAT:
      separate_alpha_row_float (src_row);
      break;	
    case PRECISION_NONE:
      break;	
    }
}

void
gray_to_rgb_row (
		       PixelRow *src_row,
		       PixelRow *dest_row
		       )
{
  switch (tag_precision (pixelrow_tag (dest_row)))
    {
    case PRECISION_U8:
      gray_to_rgb_row_u8 (src_row, dest_row);
      break;
    case PRECISION_U16:
      gray_to_rgb_row_u16 (src_row, dest_row);
      break;
    case PRECISION_FLOAT:
      gray_to_rgb_row_float (src_row, dest_row);
      break;	
    case PRECISION_NONE:
      break;	
    }
}

/*  apply the mask data to the alpha channel of the pixel data  */
void
apply_mask_to_alpha_channel_row (
				PixelRow *src_row,
				PixelRow *mask_row,
				Paint    *opac
			       )
{
  switch (tag_precision (pixelrow_tag (src_row)))
    {
    case PRECISION_U8:
      apply_mask_to_alpha_channel_row_u8 (src_row, mask_row, opac);
      break;
    case PRECISION_U16:
      apply_mask_to_alpha_channel_row_u16 (src_row, mask_row, opac);
      break;
    case PRECISION_FLOAT:
      apply_mask_to_alpha_channel_row_float (src_row, mask_row, opac);
      break;	
    case PRECISION_NONE:
      break;	
    }
}

/*  combine the mask data with the alpha channel of the pixel data  */
void
combine_mask_and_alpha_channel_row (
				    PixelRow *src_row,
				    PixelRow *mask_row,
				    Paint        *opac
				    )
{
  switch (tag_precision (pixelrow_tag (src_row)))
    {
    case PRECISION_U8:
      combine_mask_and_alpha_channel_row_u8 (src_row, mask_row, opac);
      break;
    case PRECISION_U16:
      combine_mask_and_alpha_channel_row_u16 (src_row, mask_row, opac);
      break;
    case PRECISION_FLOAT:
      combine_mask_and_alpha_channel_row_float (src_row, mask_row, opac);
      break;	
    case PRECISION_NONE:
      break;	
    }
}

/*  copy gray pixels to intensity-alpha pixels.  This function
 *  essentially takes a source that is only a grayscale image and
 *  copies it to the destination, expanding to RGB if necessary and
 *  adding an alpha channel.  (OPAQUE)
 */
void
copy_gray_to_inten_a_row (
				PixelRow *src_row,
				PixelRow *dest_row
			        )
{
  switch (tag_precision (pixelrow_tag (dest_row)))
    {
    case PRECISION_U8:
      copy_gray_to_inten_a_row_u8 (src_row, dest_row);
      break;
    case PRECISION_U16:
      copy_gray_to_inten_a_row_u16 (src_row, dest_row);
      break;
    case PRECISION_FLOAT:
      copy_gray_to_inten_a_row_float (src_row, dest_row);
      break;	
    case PRECISION_NONE:
      break;	
    }
}

/*  lay down the initial pixels in the case of only one
 *  channel being visible and no layers...In this singular
 *  case, we want to display a grayscale image w/o transparency
 */
void
initial_channel_row (
			   PixelRow *src_row,
			   PixelRow *dest_row
			  )
{
  switch (tag_precision (pixelrow_tag (dest_row)))
    {
    case PRECISION_U8:
      initial_channel_row_u8 (src_row, dest_row);
      break;
    case PRECISION_U16:
      initial_channel_row_u16 (src_row, dest_row);
      break;
    case PRECISION_FLOAT:
      initial_channel_row_float (src_row, dest_row);
      break;	
    case PRECISION_NONE:
      break;	
    }
}

/*  lay down the initial pixels in the case of an indexed image.
 *  This process obviously requires no composition
 */
void
initial_indexed_row (
			    PixelRow *src_row,
			    PixelRow *dest_row,
			    unsigned char *cmap
			   )
{
  switch (tag_precision (pixelrow_tag (dest_row)))
    {
    case PRECISION_U8:
      initial_indexed_row_u8 (src_row, dest_row, cmap);
      break;
    case PRECISION_U16:
      initial_indexed_row_u16 (src_row, dest_row, cmap);
      break;
    case PRECISION_FLOAT:
    case PRECISION_NONE:
      break;	
    }
}

/*  lay down the initial pixels in the case of an indexed image.
 *  This process obviously requires no composition
 */
void
initial_indexed_a_row (
			     PixelRow *src_row,
			     PixelRow *dest_row,
			     PixelRow *mask_row,
			     unsigned char *cmap,
			     Paint    *opac
			     )
{
  switch (tag_precision (pixelrow_tag (dest_row)))
    {
    case PRECISION_U8:
      initial_indexed_a_row_u8 (src_row, dest_row, mask_row, cmap, opac);
      break;
    case PRECISION_U16:
      initial_indexed_a_row_u16 (src_row, dest_row, mask_row, cmap, opac);
      break;
    case PRECISION_FLOAT:
    case PRECISION_NONE:
      break;	
    }
}

/*  lay down the initial pixels for the base layer.
 *  This process obviously requires no composition.
 */
void
initial_inten_row (
			  PixelRow *src_row,
			  PixelRow *dest_row,
			  PixelRow *mask_row,
			  Paint    *opac,
			  gint      *affect
		         )
{
  switch (tag_precision (pixelrow_tag (dest_row)))
    {
    case PRECISION_U8:
      initial_inten_row_u8 (src_row, dest_row, mask_row, opac, affect);
      break;
    case PRECISION_U16:
      initial_inten_row_u16 (src_row, dest_row, mask_row, opac, affect);
      break;
    case PRECISION_FLOAT:
      initial_inten_row_float (src_row, dest_row, mask_row, opac, affect);
      break;	
    case PRECISION_NONE:
      break;	
    }
}

/*  lay down the initial pixels for the base layer.
 *  This process obviously requires no composition.
 */
void
initial_inten_a_row (
			    PixelRow *src_row,
			    PixelRow *dest_row,
			    PixelRow *mask_row,
			    Paint    *opac,
			    gint     *affect
			   )
{
  switch (tag_precision (pixelrow_tag (dest_row)))
    {
    case PRECISION_U8:
      initial_inten_a_row_u8 (src_row, dest_row, mask_row, opac, affect);
      break;
    case PRECISION_U16:
      initial_inten_a_row_u16 (src_row, dest_row, mask_row, opac, affect);
      break;
    case PRECISION_FLOAT:
      initial_inten_a_row_float (src_row, dest_row, mask_row, opac, affect);
      break;	
    case PRECISION_NONE:
      break;	
    }
}

/*  combine indexed images with an optional mask which
 *  is interpreted as binary...destination is indexed...
 */
void
combine_indexed_and_indexed_row (
					PixelRow *src1_row,
					PixelRow *src2_row,
					PixelRow *dest_row,
					PixelRow *mask_row,
					Paint    *opac,
				        gint     *affect
				       )
{
  switch (tag_precision (pixelrow_tag (dest_row)))
    {
    case PRECISION_U8:
      combine_indexed_and_indexed_row_u8 (src1_row, src2_row, dest_row, mask_row, opac, affect);
      break;
    case PRECISION_U16:
      combine_indexed_and_indexed_row_u16 (src1_row, src2_row, dest_row, mask_row, opac, affect);
      break;
    case PRECISION_FLOAT:
    case PRECISION_NONE:
      break;	
    }
}

/*  combine indexed images with indexed-alpha images
 *  result is an indexed image
 */
void
combine_indexed_and_indexed_a_row (
					PixelRow *src1_row,
					PixelRow *src2_row,
					PixelRow *dest_row,
					PixelRow *mask_row,
					Paint    *opac,
				        gint     *affect
				        ) 
{
  switch (tag_precision (pixelrow_tag (dest_row)))
    {
    case PRECISION_U8:
      combine_indexed_and_indexed_a_row_u8 (src1_row, src2_row, dest_row, mask_row, opac, affect);
      break;
    case PRECISION_U16:
      combine_indexed_and_indexed_a_row_u16 (src1_row, src2_row, dest_row, mask_row, opac, affect);
      break;
    case PRECISION_FLOAT:
    case PRECISION_NONE:
      break;	
    }
}

/*  combine indexed-alpha images with indexed-alpha images
 *  result is an indexed-alpha image.  use this for painting
 *  to an indexed floating sel
 */
void
combine_indexed_a_and_indexed_a_row (
					    PixelRow *src1_row,
					    PixelRow *src2_row,
					    PixelRow *dest_row,
					    PixelRow *mask_row,
					    Paint    *opac,
					    gint      *affect
					   )
{
  switch (tag_precision (pixelrow_tag (dest_row)))
    {
    case PRECISION_U8:
      combine_indexed_a_and_indexed_a_row_u8 (src1_row, src2_row, dest_row, mask_row, opac, affect);
      break;
    case PRECISION_U16:
      combine_indexed_a_and_indexed_a_row_u16 (src1_row, src2_row, dest_row, mask_row, opac, affect);
      break;
    case PRECISION_FLOAT:
    case PRECISION_NONE:
      break;	
    }
}

/*  combine intensity with indexed, destination is
 *  intensity-alpha...use this for an indexed floating sel
 */
void
combine_inten_a_and_indexed_a_row (
					  PixelRow *src1_row,
					  PixelRow *src2_row,
					  PixelRow *dest_row,
					  PixelRow *mask_row,
					  unsigned char *cmap,
					  Paint    *opac
					 )
{
  switch (tag_precision (pixelrow_tag (dest_row)))
    {
    case PRECISION_U8:
      combine_inten_a_and_indexed_a_row_u8 (src1_row, src2_row, dest_row, mask_row, cmap, opac);
      break;
    case PRECISION_U16:
      combine_inten_a_and_indexed_a_row_u16 (src1_row, src2_row, dest_row, mask_row, cmap, opac);
      break;
    case PRECISION_FLOAT:
    case PRECISION_NONE:
      break;	
    }
}

/*  combine RGB image with RGB or GRAY with GRAY
 *  destination is intensity-only...
 */
void
combine_inten_and_inten_row (
				    PixelRow *src1_row,
				    PixelRow *src2_row,
				    PixelRow *dest_row,
				    PixelRow *mask_row,
				    Paint    *opac,
				    gint      *affect
				   )
{
  switch (tag_precision (pixelrow_tag (dest_row)))
    {
    case PRECISION_U8:
      combine_inten_and_inten_row_u8 (src1_row, src2_row, dest_row, mask_row, opac, affect);
      break;
    case PRECISION_U16:
      combine_inten_and_inten_row_u16 (src1_row, src2_row, dest_row, mask_row, opac, affect);
      break;
    case PRECISION_FLOAT:
      combine_inten_and_inten_row_float (src1_row, src2_row, dest_row, mask_row, opac, affect);
      break;	
    case PRECISION_NONE:
      break;	
    }
}

/*  combine an RGBA or GRAYA image with an RGB or GRAY image
 *  destination is intensity-only...
 */
void
combine_inten_and_inten_a_row (
				      PixelRow *src1_row,
				      PixelRow *src2_row,
				      PixelRow *dest_row,
				      PixelRow *mask_row,
				      Paint    *opac,
				      gint      *affect
				      )
{
  switch (tag_precision (pixelrow_tag (dest_row)))
    {
    case PRECISION_U8:
      combine_inten_and_inten_a_row_u8 (src1_row, src2_row, dest_row, mask_row, opac, affect);
      break;
    case PRECISION_U16:
      combine_inten_and_inten_a_row_u16 (src1_row, src2_row, dest_row, mask_row, opac, affect);
      break;
    case PRECISION_FLOAT:
      combine_inten_and_inten_a_row_float (src1_row, src2_row, dest_row, mask_row, opac, affect);
      break;	
    case PRECISION_NONE:
      break;	
    }
}

/*  combine an RGB or GRAY image with an RGBA or GRAYA image
 *  destination is intensity-alpha...
 */
void
combine_inten_a_and_inten_row (
				      PixelRow *src1_row,
				      PixelRow *src2_row,
				      PixelRow *dest_row,
				      PixelRow *mask_row,
				      Paint    *opac,
				      gint      *affect,
				      gint       mode_affect /* how does the combination mode affect alpha?  */
				      )  
{
  switch (tag_precision (pixelrow_tag (dest_row)))
    {
    case PRECISION_U8:
      combine_inten_a_and_inten_row_u8 (src1_row, src2_row, dest_row, mask_row, opac, affect, mode_affect);
      break;
    case PRECISION_U16:
      combine_inten_a_and_inten_row_u16 (src1_row, src2_row, dest_row, mask_row, opac, affect, mode_affect);
      break;
    case PRECISION_FLOAT:
      combine_inten_a_and_inten_row_float (src1_row, src2_row, dest_row, mask_row, opac, affect, mode_affect);
      break;	
    case PRECISION_NONE:
      break;	
    }
}

/*  combine an RGBA or GRAYA image with an RGBA or GRAYA image
 *  destination is of course intensity-alpha...
 */
void
combine_inten_a_and_inten_a_row (
					PixelRow *src1_row,
					PixelRow *src2_row,
					PixelRow *dest_row,
					PixelRow *mask_row,
					Paint    *opac,
					gint      *affect,
					gint       mode_affect
					)
{
  switch (tag_precision (pixelrow_tag (dest_row)))
    {
    case PRECISION_U8:
      combine_inten_a_and_inten_a_row_u8 (src1_row, src2_row, dest_row, mask_row, opac, affect, mode_affect);
      break;
    case PRECISION_U16:
      combine_inten_a_and_inten_a_row_u16 (src1_row, src2_row, dest_row, mask_row, opac, affect, mode_affect);
      break;
    case PRECISION_FLOAT:
      combine_inten_a_and_inten_a_row_float (src1_row, src2_row, dest_row, mask_row, opac, affect, mode_affect);
      break;	
    case PRECISION_NONE:
      break;	
    }
}

/*  combine a channel with intensity-alpha pixels based
 *  on some opacity, and a channel color...
 *  destination is intensity-alpha
 */
void
combine_inten_a_and_channel_mask_row (
					    PixelRow *src_row,
					    PixelRow *channel_row,
					    PixelRow *dest_row,
					    Paint    *col,
					    Paint    *opac
					    )
{
  switch (tag_precision (pixelrow_tag (dest_row)))
    {
    case PRECISION_U8:
      combine_inten_a_and_channel_mask_row_u8 (src_row, channel_row, dest_row, col, opac);
      break;
    case PRECISION_U16:
      combine_inten_a_and_channel_mask_row_u16 (src_row, channel_row, dest_row, col, opac);
      break;
    case PRECISION_FLOAT:
      combine_inten_a_and_channel_mask_row_float (src_row, channel_row, dest_row, col, opac);
      break;	
    case PRECISION_NONE:
      break;	
    }
}

void
combine_inten_a_and_channel_selection_row (
						  PixelRow *src_row,
						  PixelRow *channel_row,
						  PixelRow *dest_row,
						  Paint    *col,
						  Paint    *opac
						 )
{
  switch (tag_precision (pixelrow_tag (dest_row)))
    {
    case PRECISION_U8:
      combine_inten_a_and_channel_selection_row_u8 (src_row, channel_row, dest_row, col, opac);
      break;
    case PRECISION_U16:
      combine_inten_a_and_channel_selection_row_u16 (src_row, channel_row, dest_row, col, opac);
      break;
    case PRECISION_FLOAT:
      combine_inten_a_and_channel_selection_row_float (src_row, channel_row, dest_row, col, opac);
      break;	
    case PRECISION_NONE:
      break;	
    }
}

/*  paint "behind" the existing pixel row.
 *  This is similar in appearance to painting on a layer below
 *  the existing pixels.
 */
void
behind_inten_row (
			PixelRow *src1_row,
			PixelRow *src2_row,
			PixelRow *dest_row,
			PixelRow *mask_row,
			Paint    *opac,
			gint      *affect
                        )
{
  switch (tag_precision (pixelrow_tag (dest_row)))
    {
    case PRECISION_U8:
      behind_inten_row_u8 (src1_row, src2_row, dest_row, mask_row, opac, affect);
      break;
    case PRECISION_U16:
      behind_inten_row_u16 (src1_row, src2_row, dest_row, mask_row, opac, affect);
      break;
    case PRECISION_FLOAT:
      behind_inten_row_float (src1_row, src2_row, dest_row,mask_row, opac, affect);
      break;	
    case PRECISION_NONE:
      break;	
    }
}

/*  paint "behind" the existing pixel row (for indexed images).
 *  This is similar in appearance to painting on a layer below
 *  the existing pixels.
 */
void
behind_indexed_row (
			  PixelRow *src1_row,
			  PixelRow *src2_row,
			  PixelRow *dest_row,
			  PixelRow *mask_row,
			  Paint    *opac,
			  gint     *affect
			  )
{
  switch (tag_precision (pixelrow_tag (dest_row)))
    {
    case PRECISION_U8:
      behind_indexed_row_u8 (src1_row, src2_row, dest_row, mask_row, opac, affect);
      break;
    case PRECISION_U16:
      behind_indexed_row_u16 (src1_row, src2_row, dest_row, mask_row, opac, affect);
      break;
    case PRECISION_FLOAT:
    case PRECISION_NONE:
      break;	
    }
}

/*  replace the contents of one pixel row with the other
 *  The operation is still bounded by mask/opacity constraints
 */
void
replace_inten_row (
			  PixelRow *src1_row,
			  PixelRow *src2_row,
			  PixelRow *dest_row,
			  PixelRow *mask_row,
			  Paint    *opac,
			  gint     *affect
			 )
{
  switch (tag_precision (pixelrow_tag (dest_row)))
    {
    case PRECISION_U8:
      replace_inten_row_u8 (src1_row, src2_row, dest_row, mask_row, opac, affect);
      break;
    case PRECISION_U16:
      replace_inten_row_u16 (src1_row, src2_row, dest_row, mask_row, opac, affect);
      break;
    case PRECISION_FLOAT:
      replace_inten_row_float (src1_row, src2_row, dest_row, mask_row, opac, affect);
      break;	
    case PRECISION_NONE:
      break;	
    }
}

/*  replace the contents of one pixel row with the other
 *  The operation is still bounded by mask/opacity constraints
 */
void
replace_indexed_row (
			    PixelRow *src1_row,
			    PixelRow *src2_row,
			    PixelRow *dest_row,
			    PixelRow *mask_row,
			    Paint    *opac,
			    gint     *affect
			   )
{
  switch (tag_precision (pixelrow_tag (dest_row)))
    {
    case PRECISION_U8:
      replace_indexed_row_u8 (src1_row, src2_row, dest_row, mask_row, opac, affect);
      break;
    case PRECISION_U16:
      replace_indexed_row_u16 (src1_row, src2_row, dest_row, mask_row, opac, affect);
      break;
    case PRECISION_FLOAT:
    case PRECISION_NONE:
      break;	
    }
}

/*  apply source 2 to source 1, but in a non-additive way,
 *  multiplying alpha channels  (works for intensity)
 */
void
erase_inten_row (
			PixelRow *src1_row,
			PixelRow *src2_row,
			PixelRow *dest_row,
			PixelRow *mask_row,
			Paint    *opac,
			gint     *affect
			)
{
  switch (tag_precision (pixelrow_tag (dest_row)))
    {
    case PRECISION_U8:
      erase_inten_row_u8 (src1_row, src2_row, dest_row, mask_row, opac, affect);
      break;
    case PRECISION_U16:
      erase_inten_row_u16 (src1_row, src2_row, dest_row, mask_row, opac, affect);
      break;
    case PRECISION_FLOAT:
      erase_inten_row_float (src1_row, src2_row, dest_row, mask_row, opac, affect);
      break;	
    case PRECISION_NONE:
      break;	
    }
}

/*  apply source 2 to source 1, but in a non-additive way,
 *  multiplying alpha channels  (works for indexed)
 */
void
erase_indexed_row (
			  PixelRow *src1_row,
			  PixelRow *src2_row,
			  PixelRow *dest_row,
			  PixelRow *mask_row,
			  Paint    *opac,
			  gint     *affect
			 )
{
  switch (tag_precision (pixelrow_tag (dest_row)))
    {
    case PRECISION_U8:
      erase_indexed_row_u8 (src1_row, src2_row, dest_row, mask_row, opac, affect);
      break;
    case PRECISION_U16:
      erase_indexed_row_u16 (src1_row, src2_row, dest_row, mask_row, opac, affect);
      break;
    case PRECISION_FLOAT:
    case PRECISION_NONE:
      break;	
    }
}

/*  extract information from intensity pixels based on
 *  a mask.
 */
void
extract_from_inten_row (
			      PixelRow *src_row,
			      PixelRow *dest_row,
			      PixelRow *mask_row,
			      Paint    *background,
			      gint      cut
			      )
{
  switch (tag_precision (pixelrow_tag (dest_row)))
    {
    case PRECISION_U8:
      extract_from_inten_row_u8 (src_row, dest_row, mask_row, background, cut); 
      break;
    case PRECISION_U16:
      extract_from_inten_row_u16 (src_row, dest_row, mask_row, background, cut); 
      break;
    case PRECISION_FLOAT:
      extract_from_inten_row_float (src_row, dest_row, mask_row, background, cut); 
      break;	
    case PRECISION_NONE:
      break;	
    }
}

/*  extract information from indexed pixels based on
 *  a mask.
 */
void
extract_from_indexed_row (
				PixelRow *src_row,
				PixelRow *dest_row,
				PixelRow *mask_row,
				unsigned char   *cmap,
				Paint *background,
				gint      cut
				)
{
  switch (tag_precision (pixelrow_tag (dest_row)))
    {
    case PRECISION_U8:
      extract_from_indexed_row_u8 (src_row, dest_row, mask_row,cmap, background, cut); 
      break;
    case PRECISION_U16:
      extract_from_indexed_row_u16 (src_row, dest_row, mask_row,cmap, background, cut); 
      break;
    case PRECISION_FLOAT:
    case PRECISION_NONE:
      break;	
    }
}
