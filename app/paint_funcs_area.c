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
#include "gimprc.h"
#include "paint.h"
#include "paint_funcs_area.h"
#include "paint_funcs_row.h"
#include "pixelarea.h"
#include "pixelrow.h"
#include "tag.h"


#define EPSILON            0.0001
#define STD_BUF_SIZE       1021

static unsigned char * tmp_buffer;  /* temporary buffer available upon request */
static int tmp_buffer_size;


typedef enum _ScaleType ScaleType;
enum _ScaleType
{
  MinifyX_MinifyY,
  MinifyX_MagnifyY,
  MagnifyX_MinifyY,
  MagnifyX_MagnifyY
};

/*  Layer modes information  */
typedef struct _LayerMode LayerMode;
struct _LayerMode
{
  int affect_alpha;   /*  does the layer mode affect the alpha channel  */
  char *name;         /*  layer mode specification  */
};

LayerMode layer_modes_16[] =
{
  { 1, "Normal" },
  { 1, "Dissolve" },
  { 1, "Behind" },
  { 0, "Multiply" },
  { 0, "Screen" },
  { 0, "Overlay" },
  { 0, "Difference" },
  { 0, "Addition" },
  { 0, "Subtraction" },
  { 0, "Darken Only" },
  { 0, "Lighten Only" },
  { 0, "Hue" },
  { 0, "Saturation" },
  { 0, "Color" },
  { 0, "Value" },
  { 1, "Erase" },
  { 1, "Replace" }
};

/* static functions */
static gdouble * make_curve ( gdouble, gint*, gdouble);
static void draw_segments ( PixelArea*, BoundSeg*, gint, gint, gint, Paint*);
static gdouble cubic ( gdouble, gdouble, gdouble, gdouble, gdouble, gdouble, gdouble);
static int apply_layer_mode ( PixelRow*, PixelRow*, PixelRow*, gint, gint, Paint*, gint, gint*);
static int apply_indexed_layer_mode ( PixelRow*, PixelRow*, PixelRow*, gint);
static void apply_layer_mode_replace  ( PixelRow*, PixelRow*, PixelRow*, PixelRow*, Paint* , gint*); 


/* The 8bit data static functions */
static void convolve_area_u8 (PixelArea*, PixelArea*, gint*, gint, gint, gint, gint);
static gdouble* gaussian_curve_u8 (gint, gint*); 
static void rle_row_u8 (PixelRow*, gint*, PixelRow*);
static void gaussian_blur_row_u8 (PixelRow*, PixelRow*, gint*, PixelRow*, gint, gfloat*, gfloat); 
static void border_set_opacity_u8 (gint, gint, Paint*);
static void scale_row_no_resample_u8 ( PixelRow*, PixelRow*, gint*);
static void scale_set_dest_row_u8 ( PixelRow*, gdouble*);
static void scale_row_u8 ( guchar*, guchar*, guchar*, guchar*, gdouble, gdouble*, gdouble, gdouble, gdouble*, gint, gint, gint, gint); 
static void subsample_row_u8 (PixelRow *, gdouble *, gdouble, gdouble, gdouble*);   
static gint thin_row_u8 ( PixelRow*, PixelRow*, PixelRow*, PixelRow*, gint);


/* The 16bit data static functions */
static void convolve_area_u16 (PixelArea*, PixelArea*, gint*, gint, gint, gint, gint);
static gdouble* gaussian_curve_u16 (gint, gint*); 
static void rle_row_u16 (PixelRow*, gint*, PixelRow*);
static void gaussian_blur_row_u16 (PixelRow*, PixelRow*, gint*, PixelRow*, gint, gfloat*, gfloat); 
static void border_set_opacity_u16 (gint, gint, Paint*);
static void scale_row_no_resample_u16 ( PixelRow*, PixelRow*, gint*);
static void scale_set_dest_row_u16 ( PixelRow*, gdouble*);
static void scale_row_u16 ( guchar*, guchar*, guchar*, guchar*, gdouble, gdouble*, gdouble, gdouble, gdouble*, gint, gint, gint, gint); 
static void subsample_row_u16(PixelRow *, gdouble *, gdouble, gdouble, gdouble*);   
static gint thin_row_u16 ( PixelRow*, PixelRow*, PixelRow*, PixelRow*, gint);


/* The float data static functions */
static void convolve_area_float (PixelArea*, PixelArea*, gint*, gint, gint, gint, gint);
static gdouble* gaussian_curve_float (gint, gint*); 
static void rle_row_float (PixelRow*, gint*, PixelRow*);
static void gaussian_blur_row_float (PixelRow*, PixelRow*, gint*, PixelRow*, gint, gfloat*, gfloat); 
static void border_set_opacity_float (gint, gint, Paint*);
static void scale_row_no_resample_float ( PixelRow*, PixelRow*, gint*);
static void scale_set_dest_row_float ( PixelRow*, gdouble*);
static void scale_row_float ( guchar*, guchar*, guchar*, guchar*, gdouble, gdouble*, gdouble, gdouble, gdouble*, gint, gint, gint, gint); 
static void subsample_row_float (PixelRow *, gdouble *, gdouble, gdouble, gdouble*);   
static gint thin_row_float ( PixelRow*, PixelRow*, PixelRow*, PixelRow*, gint);


void 
paint_funcs_area_setup  (
                         void
                         )
{
  /*  allocate the temporary buffer  */
  tmp_buffer = (unsigned char *) g_malloc (STD_BUF_SIZE);
  tmp_buffer_size = STD_BUF_SIZE;
}


void 
paint_funcs_area_free  (
                        void
                        )
{
  /*  free the temporary buffer  */
  g_free (tmp_buffer);
}


unsigned char * 
paint_funcs_area_get_buffer  (
                              int size
                              )
{
  if (size > tmp_buffer_size)
    {
      tmp_buffer_size = size;
      tmp_buffer = (unsigned char *) g_realloc (tmp_buffer, size);
    }

  return tmp_buffer;
}

void
paint_funcs_area_randomize (
                            int y
                            )
{
  paint_funcs_randomize_row ( y );
}


/**************************************************/
/*    AREA FUNCTIONS                            */
/**************************************************/

void 
color_area  (
             PixelArea * src_area,
             Paint * color
             )
{
  void *  pag;
  Tag src_tag = pixelarea_tag (src_area); 
  Tag color_tag = paint_tag (color); 
  
   /*put in tags check*/

  for (pag = pixelarea_register (1, src_area);
       pag != NULL;
       pag = pixelarea_process (pag))
    {
      PixelRow row;
      gint h = pixelarea_height (src_area);
      while (h--)
        {
          pixelarea_getdata (src_area, &row, h);
          color_row (&row, color);
        }
    }
}


void 
blend_area  (
             PixelArea * src1_area,
             PixelArea * src2_area,
             PixelArea * dest_area,
             Paint * blend
             )
{
  void *  pag;
  Tag src1_tag = pixelarea_tag (src1_area); 
  Tag src2_tag = pixelarea_tag (src2_area); 
  Tag dest_tag = pixelarea_tag (dest_area); 
  
  /* put in tags check */
  
  for (pag = pixelarea_register (3, src1_area, src2_area, dest_area);
       pag != NULL;
       pag = pixelarea_process (pag))
    {
      PixelRow src1_row;
      PixelRow src2_row;
      PixelRow dest_row;
 
      gint h = pixelarea_height (src1_area);
      while (h--)
        {
          pixelarea_getdata (src1_area, &src1_row, h);
          pixelarea_getdata (src2_area, &src2_row, h);
          pixelarea_getdata (dest_area, &dest_row, h);
            
         /*blend_pixels (s1, s2, d, blend, src1->w, src1->bytes, src1_has_alpha); */
           blend_row (&src1_row, &src2_row, &dest_row, blend);
        }
    }
}


void 
shade_area  (
             PixelArea * src_area,
             PixelArea * dest_area,
             Paint * col,
             Paint * blend
             )
{
  void *  pag;
  Tag src_tag = pixelarea_tag (src_area); 
  Tag dest_tag = pixelarea_tag (dest_area); 

  /*put in tags check*/
  
  for (pag = pixelarea_register (2, src_area, dest_area);
       pag != NULL;
       pag = pixelarea_process (pag))
    {
      PixelRow src_row;
      PixelRow dest_row;
 
      gint h = pixelarea_height (src_area);
      while (h--)
        {
          pixelarea_getdata (src_area, &src_row, h);
          pixelarea_getdata (dest_area, &dest_row, h);
            
          shade_row (&src_row, &dest_row, col, blend);
        }
    }
}


void 
copy_area  (
            PixelArea * src_area,
            PixelArea * dest_area
            )
{
  PixelRow srow;
  PixelRow drow;
  void * pag;
  
  for (pag = pixelarea_register (2, src_area, dest_area);
       pag != NULL;
       pag = pixelarea_process (pag))
    {
      int h = pixelarea_height (src_area);
      while (h--)
        {
          pixelarea_getdata (src_area, &srow, h);
          pixelarea_getdata (dest_area, &drow, h);
          copy_row (&srow, &drow);
        }
    }
}




void 
add_alpha_area  (
                 PixelArea * src_area,
                 PixelArea * dest_area
                 )
{
  void *  pag;
  Tag src_tag = pixelarea_tag (src_area); 
  Tag dest_tag = pixelarea_tag (dest_area); 

   /*put in tags check*/
  
  for (pag = pixelarea_register (2, src_area, dest_area);
       pag != NULL;
       pag = pixelarea_process (pag))
    {
      PixelRow src_row;
      PixelRow dest_row;
 
      gint h = pixelarea_height (src_area);
      while (h--)
        {
          pixelarea_getdata (src_area, &src_row, h);
          pixelarea_getdata (dest_area, &dest_row, h);
            
  	 /*add_alpha_pixels (s, d, src->w, src->bytes);*/
          add_alpha_row (&src_row, &dest_row);
        }
    }
}


void 
flatten_area  (
               PixelArea * src_area,
               PixelArea * dest_area,
               Paint * bg
               )
{
  void *  pag;
  PixelRow src_row;
  PixelRow dest_row;
  Tag src_tag = pixelarea_tag (src_area); 
  Tag dest_tag = pixelarea_tag (dest_area); 
  gint h = pixelarea_height (src_area);

   /*put in tags check*/
  
  while (h--)
    {
      pixelarea_getdata (src_area, &src_row, h);
      pixelarea_getdata (dest_area, &dest_row, h);
	
      /*flatten_pixels (s, d, bg, src->w, src->bytes);*/
        flatten_row (&src_row, &dest_row, bg);
    }
}


void 
extract_alpha_area  (
                     PixelArea * src_area,
                     PixelArea * mask_area,
                     PixelArea * dest_area
                     )
{
  void *  pag;
  Tag src_tag = pixelarea_tag (src_area); 
  Tag dest_tag = pixelarea_tag (dest_area); 
  Tag mask_tag = pixelarea_tag (mask_area); 

   /*put in tags check*/
  
  for (pag = pixelarea_register (3, src_area, dest_area, mask_area);
       pag != NULL;
       pag = pixelarea_process (pag))
    {
      PixelRow src_row;
      PixelRow dest_row;
      PixelRow mask_row;
 
      gint h = pixelarea_height (src_area);
      while (h--)
        {
          pixelarea_getdata (src_area, &src_row, h);
          pixelarea_getdata (dest_area, &dest_row, h);
          pixelarea_getdata (mask_area, &mask_row, h);
            
  	 /* extract_alpha_pixels (s, m, d, src->w, src->bytes); */
          extract_alpha_row (&src_row, &mask_row, &dest_row);
        }
    }
}


void 
extract_from_area  (
                    PixelArea * src_area,
                    PixelArea * dest_area,
                    PixelArea * mask_area,
                    Paint *bg,
                    unsigned char*cmap,
                    gint cut
                    )
{
  void *  pag;
  Tag src_tag = pixelarea_tag (src_area); 
  Tag dest_tag = pixelarea_tag (dest_area); 
  Tag mask_tag = pixelarea_tag (mask_area); 
  Format format = tag_format (src_tag);

   /*put in tags check*/
  
  for (pag = pixelarea_register (3, src_area, dest_area, mask_area);
       pag != NULL;
       pag = pixelarea_process (pag))
    {
      PixelRow src_row;
      PixelRow dest_row;
      PixelRow mask_row;
 
      gint h = pixelarea_height (src_area);
      while (h--)
        {
          pixelarea_getdata (src_area, &src_row, h);
          pixelarea_getdata (dest_area, &dest_row, h);
          pixelarea_getdata (mask_area, &mask_row, h);
            
	  switch (format)
	    {
            case FORMAT_NONE:
              break;
	    case FORMAT_RGB:
	    case FORMAT_GRAY:
  	      /*extract_from_inten_pixels (s, d, m, bg, cut, src->w, src->bytes,src_has_alpha);*/
              extract_from_inten_row (&src_row, &dest_row, &mask_row, bg, cut);
	      break;
	    case FORMAT_INDEXED:
  	      /*extract_from_indexed_pixels (s, d, m, cmap--wheres the cmap?, bg, cut, src->w, src  bytes, src_has_alpha);*/
              extract_from_indexed_row (&src_row, &dest_row, &mask_row, cmap, bg, cut);
	      break;
	    }
        }
    }
}


typedef void  (*ConvolveAreaFunc) (PixelArea*,PixelArea*,gint*,gint,gint,gint,gint);
static ConvolveAreaFunc convolve_area_funcs[3] =
{
  convolve_area_u8,
  convolve_area_u16,
  convolve_area_float
};

void
convolve_area (
	 	PixelArea   *src_area,
		PixelArea   *dest_area,
		gint         *matrix,
		gint          matrix_size,
		gint          divisor,
		gint          mode
		)
{
  gint offset;
  guint src_area_height = pixelarea_height (src_area);  
  guint src_area_width = pixelarea_width (src_area);  
  Tag src_tag = pixelarea_tag (src_area);
  Tag dest_tag = pixelarea_tag (dest_area);
  Precision prec = tag_precision (src_tag);

  if ( !tag_equal (src_tag, dest_tag) )
     return;

  /*  If the mode is NEGATIVE, the offset should be 128  */
  if (mode == NEGATIVE)
    {
      offset = 128;
      mode = 0;
    }
  else
    offset = 0;
  
  /*  check for the boundary cases  */
  if (src_area_width < (matrix_size - 1) || 
		src_area_height < (matrix_size - 1))
    return;

  (*convolve_area_funcs [prec-1]) (src_area, dest_area, matrix, 
				matrix_size, divisor, mode, offset);
}

static void
convolve_area_u8 (
		  PixelArea   *src_area,
		  PixelArea   *dest_area,
		  gint         *matrix,
		  gint          size,
		  gint          divisor,
		  gint          mode,
                  gint          offset
		  )
{
  guint8 *src, *s_row, * s;
  guint8 *dest, * d;
  gint * m;
  gint total [4];
  gint b, num_channels;
  gint wraparound;
  gint margin;      /*  margin imposed by size of conv. matrix  */
  gint i, j;
  gint x, y;
  gint src_rowstride;
  gint dest_rowstride;
  gint src_height;
  gint src_width;
  gint bytes;
  
  Tag src_tag = pixelarea_tag (src_area); 
  /*  Initialize some values  */
  src_width = pixelarea_width (src_area);
  src_height = pixelarea_height (src_area);
  bytes = tag_bytes (src_tag);  /* per pixel */ 
  num_channels = tag_num_channels (src_tag); /* per pixel */
  
  src_rowstride =  pixelarea_rowstride (src_area);
  dest_rowstride = pixelarea_rowstride (dest_area);
  
  margin = size / 2;
  src = (guint8 *) pixelarea_data (src_area);
  dest = (guint8 *) pixelarea_data (dest_area);

  /*  calculate the source wraparound value  */
  wraparound = src_rowstride  - size * num_channels;

  /* copy the first (size / 2) scanlines of the src image... */
  for (i = 0; i < margin; i++)
    {
      memcpy (dest, src, src_width * bytes);
      src += src_rowstride;
      dest += dest_rowstride;
    }

  src = (guint8 *) pixelarea_data (src_area);
  
  for (y = margin; y < src_height - margin; y++)
    {
      s_row = src;
      s = s_row + src_rowstride*margin;
      d = dest;

      /* handle the first margin pixels... */
      b = num_channels * margin;
      while (b --)
	*d++ = *s++;

      /* now, handle the center pixels */
      x = src_width - margin*2;
      while (x--)
	{
	  s = s_row;

	  m = matrix;
	  total [0] = total [1] = total [2] = total [3] = 0;
	  i = size;
	  while (i --)
	    {
	      j = size;
	      while (j --)
		{
		  for (b = 0; b < num_channels; b++)
		    total [b] += *m * *s++;
		  m ++;
		}

	      s += wraparound;
	    }

	  for (b = 0; b < num_channels; b++)
	    {
	      total [b] = total [b] / divisor + offset;

	      /*  only if mode was ABSOLUTE will mode by non-zero here  */
	      if (total [b] < 0 && mode)
		total [b] = - total [b];

	      if (total [b] < 0)
		*d++ = 0;
	      else
		*d++ = (total [b] > 255) ? 255 : (unsigned char) total [b];
	    }

	  s_row += num_channels;

	}

      /* handle the last pixel... */
      s = s_row + (src_rowstride + num_channels) * margin;
      b = num_channels * margin;
      while (b --)
	*d++ = *s++;

      /* set the memory pointers */
      src += src_rowstride;
      dest += dest_rowstride;
    }

  src += src_rowstride*margin;

  /* copy the last (margin) scanlines of the src image... */
  for (i = 0; i < margin; i++)
    {
      memcpy (dest, src, src_width * bytes) ;
      src += src_rowstride;
      dest += dest_rowstride;
    }
}

static void
convolve_area_u16 (
		   PixelArea   *src_area,
		   PixelArea   *dest_area,
		   gint         *matrix,
		   gint          size,
		   gint          divisor,
		   gint          mode,
                   gint          offset
		   )
{
  guint16 *src, *s_row, *s;
  guint16 *dest, *d;
  gint * m;
  gint total [4];
  gint b, num_channels;
  gint wraparound;
  gint margin;      /*  margin imposed by size of conv. matrix  */
  gint i, j;
  gint x, y;
  gint src_rowstride;
  gint dest_rowstride;
  gint src_height;
  gint src_width;
  gint bytes; 
  Tag src_tag = pixelarea_tag (src_area); 
  /*  Initialize some values  */
  src_width = pixelarea_width (src_area);
  src_height = pixelarea_height (src_area);
  bytes = tag_bytes (src_tag);  /* per pixel */ 
  num_channels = tag_num_channels (src_tag); /* per pixel */
  
  /* divide rowstride by bytes per channel */ 
  src_rowstride = pixelarea_rowstride (src_area) / sizeof (guint16);
  dest_rowstride = pixelarea_rowstride (dest_area) / sizeof(guint16);
  
  margin = size / 2;
  src = (guint16 *) pixelarea_data (src_area);
  dest = (guint16 *) pixelarea_data (dest_area);

  /*  calculate the source wraparound value  */
  wraparound = src_rowstride  - size * num_channels;

  /* copy the first (size / 2) scanlines of the src image... */
  for (i = 0; i < margin; i++)
    {
      memcpy (dest, src, src_width * bytes);
      src += src_rowstride;
      dest += dest_rowstride;
    }

  src = (guint16 *) pixelarea_data (src_area);
  
  for (y = margin; y < src_height - margin; y++)
    {
      s_row = src;
      s = s_row + src_rowstride*margin;
      d = dest;

      /* handle the first margin pixels... */
      b = num_channels * margin;
      while (b --)
	*d++ = *s++;

      /* now, handle the center pixels */
      x = src_width - margin*2;
      while (x--)
	{
	  s = s_row;

	  m = matrix;
	  total [0] = total [1] = total [2] = total [3] = 0;
	  i = size;
	  while (i --)
	    {
	      j = size;
	      while (j --)
		{
		  for (b = 0; b < num_channels; b++)
		    total [b] += *m * *s++;
		  m ++;
		}

	      s += wraparound;
	    }

	  for (b = 0; b < num_channels; b++)
	    {
	      total [b] = total [b] / divisor + offset;

	      /*  only if mode was ABSOLUTE will mode by non-zero here  */
	      if (total [b] < 0 && mode)
		total [b] = - total [b];

	      if (total [b] < 0)
		*d++ = 0;
	      else
		*d++ = (total [b] > 65535) ? 65535 : (guint16) total [b];
	    }

	  s_row += num_channels;

	}

      /* handle the last pixel... */
      s = s_row + (src_rowstride + num_channels) * margin;
      b = num_channels * margin;
      while (b --)
	*d++ = *s++;

      /* set the memory pointers */
      src += src_rowstride;
      dest += dest_rowstride;
    }

  src += src_rowstride*margin;

  /* copy the last (margin) scanlines of the src image... */
  for (i = 0; i < margin; i++)
    {
      memcpy (dest, src, src_width * bytes);
      src += src_rowstride;
      dest += dest_rowstride;
    }
}

static void
convolve_area_float (PixelArea *src_area,
		 PixelArea *dest_area,
		 gint         *matrix,
		 gint          size,
		 gint          divisor,
		 gint          mode,
                 gint          offset)
{
  gfloat *src, *s_row, *s;
  gfloat *dest, *d;
  gint * m;
  gdouble total [4];
  gint b, num_channels;
  gint wraparound;
  gint margin;      /*  margin imposed by size of conv. matrix  */
  gint i, j;
  gint x, y;
  gint src_rowstride;
  gint dest_rowstride;
  gint src_height;
  gint src_width;
  gint bytes;
  Tag src_tag = pixelarea_tag (src_area); 
 
  /*  Initialize some values  */
  src_width = pixelarea_width (src_area);
  src_height = pixelarea_height (src_area);
  bytes = tag_bytes (src_tag);  /* per pixel */ 
  num_channels = tag_num_channels (src_tag); /* per pixel */
  
  /* divide rowstride by bytes per channel */ 
  src_rowstride = pixelarea_rowstride (src_area) / sizeof (gfloat);
  dest_rowstride = pixelarea_rowstride (dest_area) / sizeof(gfloat);
  
  margin = size / 2;
  src = (gfloat *) pixelarea_data (src_area);
  dest = (gfloat *) pixelarea_data (dest_area);

  /*  calculate the source wraparound value  */
  wraparound = src_rowstride  - size * num_channels;

  /* copy the first (size / 2) scanlines of the src image... */
  for (i = 0; i < margin; i++)
    {
      memcpy (dest, src, src_width * bytes);
      src += src_rowstride;
      dest += dest_rowstride;
    }

  src = (gfloat *) pixelarea_data (src_area);
  
  for (y = margin; y < src_height - margin; y++)
    {
      s_row = src;
      s = s_row + src_rowstride*margin;
      d = dest;

      /* handle the first margin pixels... */
      b = num_channels * margin;
      while (b --)
	*d++ = *s++;

      /* now, handle the center pixels */
      x = src_width - margin*2;
      while (x--)
	{
	  s = s_row;

	  m = matrix;
	  total [0] = total [1] = total [2] = total [3] = 0.0;
	  i = size;
	  while (i --)
	    {
	      j = size;
	      while (j --)
		{
		  for (b = 0; b < num_channels; b++)
		    total [b] += *m * *s++;
		  m ++;
		}

	      s += wraparound;
	    }

	  for (b = 0; b < num_channels; b++)
	    {
	      total [b] = total [b] /(gdouble)divisor + offset;

	      /*  only if mode was ABSOLUTE will mode by non-zero here  */
	      if (total [b] < 0 && mode)
		total [b] = - total [b];

	      if (total [b] < 0)
		*d++ = 0;
	      else
		*d++ = (total [b] > 1.0) ? 1.0 : total [b];
	    }

	  s_row += num_channels;

	}

      /* handle the last pixel... */
      s = s_row + (src_rowstride + num_channels) * margin;
      b = num_channels * margin;
      while (b --)
	*d++ = *s++;

      /* set the memory pointers */
      src += src_rowstride;
      dest += dest_rowstride;
    }

  src += src_rowstride*margin;

  /* copy the last (margin) scanlines of the src image... */
  for (i = 0; i < margin; i++)
    {
      memcpy (dest, src, src_width * bytes);
      src += src_rowstride;
      dest += dest_rowstride;
    }
}


/* Convert from unmultiplied to premultiplied alpha. */ 
void 
multiply_alpha_area  (
                      PixelArea * src_area
                      )
{
  void *  pag;
  Tag src_tag = pixelarea_tag (src_area); 

   /*put in tags check*/
  
  for (pag = pixelarea_register (1, src_area);
       pag != NULL;
       pag = pixelarea_process (pag))
    {
      PixelRow src_row;
 
      gint h = pixelarea_height (src_area);
      while (h--)
        {
          pixelarea_getdata (src_area, &src_row, h);
            
          multiply_alpha_row (&src_row);
        }
    }
}

/* Convert from premultiplied alpha to unpremultiplied */
void 
separate_alpha_area  (
                      PixelArea * src_area
                      )
{
  void *  pag;
  Tag src_tag = pixelarea_tag (src_area); 

   /*put in tags check*/
  
  for (pag = pixelarea_register (1, src_area);
       pag != NULL;
       pag = pixelarea_process (pag))
    {
      PixelRow src_row;
 
      gint h = pixelarea_height (src_area);
      while (h--)
        {
          pixelarea_getdata (src_area, &src_row, h);
            
          separate_alpha_row (&src_row);
        }
    }
}

typedef gdouble*  (*GaussianCurveFunc) (gint,gint*);
static GaussianCurveFunc gaussian_curve_funcs[] =
{
  gaussian_curve_u8,
  gaussian_curve_u16,
  gaussian_curve_float
};

typedef void  (*RleRowFunc) (PixelRow*,gint*,PixelRow*);
static RleRowFunc rle_row_funcs[] =
{
  rle_row_u8,
  rle_row_u16,
  rle_row_float
};

typedef void  (*GaussianBlurRowFunc) (PixelRow*,PixelRow*,gint*,
					PixelRow*,gint,gfloat*,gfloat);
static GaussianBlurRowFunc gaussian_blur_row_funcs[] =
{
  gaussian_blur_row_u8,
  gaussian_blur_row_u16,
  gaussian_blur_row_float
};

void
gaussian_blur_area (
		    PixelArea    *src_area,
		    gdouble       radius
		   )
{
  gdouble *curve;
  gfloat *sum, total;
  gint length, i, col, row;
  PixelRow src_row, src_col, dest_row, rle_values;
  guchar *src_row_data, *src_col_data, *dest_row_data, *rle_values_data;
  gint *rle_count;  
  Tag rle_values_tag;
  Tag src_tag = pixelarea_tag (src_area);
  Precision prec = tag_precision (src_tag);
  guint width = pixelarea_width (src_area);
  guint height = pixelarea_height (src_area);
  guint bytes_per_pixel = tag_bytes (src_tag);
  guint num_channels = tag_num_channels (src_tag);  /*per pixel*/
  guint bytes_per_channel = bytes_per_pixel / num_channels;
  gint src_x = pixelarea_x (src_area);
  gint src_y = pixelarea_y (src_area);

  /* add a tag check here to look for an alpha channel */
 
  if (radius == 0.0) return;		
  
  /* get a gaussian  */
  curve = (*gaussian_curve_funcs[prec-1])(radius, &length ); 
  
  /* compute a sum for the gaussian */ 
  sum = g_malloc (sizeof (gfloat) * (2 * length + 1));
  sum[0] = 0.0;
  for (i = 1; i <= length*2; i++)
    sum[i] = curve[i-length-1] + sum[i-1];
  sum += length;
  total = sum[length] - sum[-length];
  
  /* create src column and row pixel rows */
  src_col_data = (guchar *) g_malloc (height * bytes_per_pixel);
  src_row_data = (guchar *) g_malloc (width * bytes_per_pixel);
  dest_row_data = (guchar *) g_malloc (width * bytes_per_pixel); 
  pixelrow_init (&src_col, src_tag, src_col_data, height);
  pixelrow_init (&src_row, src_tag, src_row_data, width);
  pixelrow_init (&dest_row, src_tag, dest_row_data, width);

  /* create rle arrays */
  rle_count = g_malloc (sizeof (int) * MAXIMUM (width, height)); 
  
  rle_values_tag = tag_new (tag_precision (src_tag), FORMAT_GRAY, ALPHA_NO); 
  rle_values_data = (guchar *) g_malloc (MAXIMUM (width, height) * bytes_per_channel); 
  pixelrow_init (&rle_values, rle_values_tag , rle_values_data, MAXIMUM(width, height));
  
  /* first do the columns */
  for (col = 0; col < width; col++)
    {
      /* get a copy of the column from src */
      /*pixel_region_get_col (srcR, col + srcR->x, srcR->y, height, src, 1);*/
      pixelarea_copy_col (src_area, &src_col, src_x + col, src_y, height, 1);
      
      /* get an "rle" of the columns alpha */
      (*rle_row_funcs [prec-1]) (&src_col, rle_count, &rle_values);
      
      /* run a 1d gaussian on the column */
      (*gaussian_blur_row_funcs [prec-1]) (&src_col, &src_col, rle_count, 
					 &rle_values, length, sum, total); 

      /* copy the result back into the src */
      /*pixel_region_set_col (srcR, col + srcR->x, srcR->y, height, src);*/
      pixelarea_write_col (src_area, &src_col, src_x + col, src_y, height);
    }
  
  /* do the rows next */
  for (row = 0; row < height; row++)
    {
      /*pixel_region_get_row (srcR, srcR->x, row + srcR->y, width, src, 1);*/
      
      /* make copies of the row for src and dest */
 
      pixelarea_copy_row (src_area, &src_row, src_x, src_y + row, width, 1);
      pixelarea_copy_row (src_area, &dest_row, src_x, src_y + row, width, 1); 
      
      /* get an "rle" of the rows alpha */
      (*rle_row_funcs [prec-1]) (&src_row, rle_count, &rle_values);
      
      /* run a 1d gaussian on the row */
      (*gaussian_blur_row_funcs [prec-1]) (&src_row, &dest_row, rle_count, 
					 &rle_values, length, sum, total); 

      /* copy the result into src */ 	
      /*pixel_region_set_row (srcR, srcR->x, row + srcR->y, width, dest);*/
      pixelarea_write_row (src_area, &dest_row, src_x, src_y + row, width);
    }
    g_free (sum - length);
    g_free (curve - length);
    g_free (rle_values_data);
    g_free (src_row_data);
    g_free (src_col_data);
    g_free (dest_row_data);
}

static gdouble *
gaussian_curve_u8 ( gint radius, gint *length ) 
{
  gdouble std_dev;
  std_dev = sqrt (-(radius * radius) / (2 * log (1.0/255.0)));
  return make_curve (std_dev, length, 1.0/255.0);
}

static gdouble *
gaussian_curve_u16 ( gint radius, gint *length ) 
{
  gdouble std_dev;
  std_dev = sqrt (-(radius * radius) / (2 * log (1.0/65535.0)));
  return make_curve (std_dev, length, 1.0/65535.0);
}

static gdouble *
gaussian_curve_float ( gint radius, gint *length ) 
{
  gdouble std_dev;
  std_dev = sqrt (-(radius * radius) / (2 * log (.000001)));
  return make_curve (std_dev, length, .000001 );
}

static void
rle_row_u8
                       (     
		       PixelRow *src_row,    /*in --encode this row*/
		       gint     *how_many,   /*out --how many of each code*/
		       PixelRow *values_row /*out --the codes*/
		       )
{
  gint 	  start;
  gint    i;
  gint    j;
  gint    pixels_equal;
  gint    b;
  gint    num_channels = tag_num_channels (pixelrow_tag (src_row));
  gint    width        = pixelrow_width (src_row);
  guint8 *src          = (guint8*)pixelrow_data (src_row);
  guint8 *values       = (guint8*)pixelrow_data (values_row);
  guint8 *last;
  
  last = src;
  src += num_channels;
  start = 0;
  for (i = 1; i < width; i++)
    {
      b = num_channels;
      pixels_equal = TRUE;
      for( b = 0; b < num_channels; b++)	
	{
	  if( src[b] != last[b] ) 
	  { 
	    pixels_equal = FALSE;
	    break;
	  } 
	}
      if (!pixels_equal)
	{
	  for (j = start; j < i; j++)
	    {
	      *how_many++ = (i - j);
               for( b = 0; b < num_channels; b++)	
	          values[b]  = last[b];
               values += num_channels;
	    }
	  start = i;
	  last = src;
	}
      src += num_channels;
    }

  for (j = start; j < i; j++)
    {
      *how_many++ = (i - j);
      for( b = 0; b < num_channels; b++)	
	values[b]  = last[b];
      values += num_channels;
    }
}

static void
rle_row_u16
                       (     
		       PixelRow *src_row,    /*in --encode this row*/
		       gint      *how_many,   /*out --how many of each code*/
		       PixelRow *values_row /*out --the codes*/
		       )
{
  gint start;
  gint i;
  gint j;
  gint pixels_equal;
  gint b;
  gint num_channels = tag_num_channels (pixelrow_tag (src_row));
  gint    width        = pixelrow_width (src_row);
  guint16 *src = (guint16*)pixelrow_data (src_row);
  guint16 *values = (guint16*)pixelrow_data (values_row);
  guint16 *last;
  
  last = src;
  src += num_channels;
  start = 0;
  for (i = 1; i < width; i++)
    {
      b = num_channels;
      pixels_equal = TRUE;
      for( b = 0; b < num_channels; b++)	
	{
	  if( src[b] != last[b] ) 
	  { 
	    pixels_equal = FALSE;
	    break;
	  } 
	}
      if (!pixels_equal)
	{
	  for (j = start; j < i; j++)
	    {
	      *how_many++ = (i - j);
               for( b = 0; b < num_channels; b++)	
	          values[b]  = last[b];
               values += num_channels;
	    }
	  start = i;
	  last = src;
	}
      src += num_channels;
    }

  for (j = start; j < i; j++)
    {
      *how_many++ = (i - j);
      for( b = 0; b < num_channels; b++)	
	values[b]  = last[b];
      values += num_channels;
    }
}

static void
rle_row_float
                       (     
		       PixelRow *src_row,    /*in --encode this row*/
		       gint      *how_many,   /*out --how many of each code*/
		       PixelRow *values_row   /*out --the codes*/
		       )
{
  int start;
  int i;
  int j;
  int pixels_equal;
  int b;
  int num_channels = tag_num_channels (pixelrow_tag (src_row));
  gint    width        = pixelrow_width (src_row);
  gfloat *src = (gfloat*)pixelrow_data (src_row);
  gfloat *values = (gfloat*)pixelrow_data (values_row);
  gfloat *last;
  
  last = src;
  src += num_channels;
  start = 0;
  for (i = 1; i < width; i++)
    {
      b = num_channels;
      pixels_equal = TRUE;
      for( b = 0; b < num_channels; b++)	
	{
	  if( src[b] != last[b] ) 
	  { 
	    pixels_equal = FALSE;
	    break;
	  } 
	}
      
	if (!pixels_equal)
	{
	  for (j = start; j < i; j++)
	    {
	      *how_many++ = (i - j);
               for( b = 0; b < num_channels; b++)	
	          values[b]  = last[b];
               values += num_channels;
	    }
	  start = i;
	  last = src;
	}
      src += num_channels;
    }

  for (j = start; j < i; j++)
    {
      *how_many++ = (i - j);
      for( b = 0; b < num_channels; b++)	
	values[b]  = last[b];
      values += num_channels;
    }
}

static void gaussian_blur_row_u8 (
                        PixelRow *src, 
			PixelRow *dest, 
			gint* rle_count, 
			PixelRow *rle_values, 
			gint length, 
			gfloat* sum,
                        gfloat  total
			) 
{  
  gint val, x, i, start, end, pixels;
  guint8 *rle_values_ptr;
  gint   *rle_count_ptr;
  guint8 *src_data = (guint8*)pixelrow_data (src);
  guint8 *dest_data = (guint8*)pixelrow_data (dest);
  guint8 *rle_values_data = (guint8*)pixelrow_data (rle_values);
  Tag src_tag = pixelrow_tag (src);
  gint num_channels = tag_num_channels (src_tag);
  gint width = pixelrow_width (src);
  gint alpha = num_channels - 1;  /*same for both */
  guint8 *sp = src_data + alpha;
  guint8 *dp = dest_data + alpha;
  gint initial_p = sp[0];
  gint initial_m = sp[(width -1) * num_channels];
  
  for (x = 0; x < width; x++)
    {
      start = (x < length) ? -x : -length;
      end = (width <= (x + length)) ? (width - x - 1) : length;
      start = -x;
      val = 0;
      i = start;
      rle_count_ptr = rle_count + (x + i);
      rle_values_ptr = (guint8*)rle_values_data + (x+i); 

      if (start != -length)
	val += initial_p * (sum[start] - sum[-length]);

      while (i < end)
	{
	  pixels = *rle_count_ptr;
	  i += pixels;
	  if (i > end)
	    i = end;
	  val += *rle_values_ptr * (sum[i] - sum[start]);
	  rle_values_ptr += pixels;
	  rle_count_ptr += pixels;
	  start = i;
	}

      if (end != length)
	val += initial_m * (sum[length] - sum[end]);

      dp[x * num_channels] = val / total;
    }
}

static void
gaussian_blur_row_u16 (
                        PixelRow *src, 
			PixelRow *dest, 
			gint* rle_count, 
			PixelRow *rle_values, 
			gint length, 
			gfloat *sum,
                        gfloat total
			) 
{  
  gint val, x, i, start, end, pixels;
  guint16 *rle_values_ptr;
  gint   *rle_count_ptr;
  guint16 *src_data = (guint16*)pixelrow_data (src);
  guint16 *dest_data = (guint16*)pixelrow_data (dest);
  guint16 *rle_values_data = (guint16*)pixelrow_data (rle_values);
  Tag src_tag = pixelrow_tag (src);
  gint num_channels = tag_num_channels (src_tag);
  gint width = pixelrow_width (src);
  gint alpha = num_channels - 1;  /*same for both */
  guint16 *sp = src_data + alpha;
  guint16 *dp = dest_data + alpha;
  gint initial_p = sp[0];
  gint initial_m = sp[(width -1) * num_channels];
  
  for (x = 0; x < width; x++)
    {
      start = (x < length) ? -x : -length;
      end = (width <= (x + length)) ? (width - x - 1) : length;
      start = -x;
      val = 0;
      i = start;
      rle_count_ptr = rle_count + (x + i);
      rle_values_ptr = (guint16*)rle_values_data + (x+i); 

      if (start != -length)
	val += initial_p * (sum[start] - sum[-length]);

      while (i < end)
	{
	  pixels = *rle_count_ptr;
	  i += pixels;
	  if (i > end)
	    i = end;
	  val += *rle_values_ptr * (sum[i] - sum[start]);
	  rle_values_ptr += pixels;
	  rle_count_ptr += pixels;
	  start = i;
	}

      if (end != length)
	val += initial_m * (sum[length] - sum[end]);

      dp[x * num_channels] = val / total;
    }
}

static void 
gaussian_blur_row_float (
                        PixelRow *src, 
			PixelRow *dest, 
			gint* rle_count, 
			PixelRow *rle_values, 
			gint length, 
			gfloat *sum,
                        gfloat total
			) 
{  
  gint val, x, i, start, end, pixels;
  gfloat *rle_values_ptr;
  gint   *rle_count_ptr;
  gfloat *src_data = (gfloat *)pixelrow_data (src);
  gfloat *dest_data = (gfloat *)pixelrow_data (dest);
  gfloat *rle_values_data = (gfloat*)pixelrow_data (rle_values);
  Tag src_tag = pixelrow_tag (src);
  gint num_channels = tag_num_channels (src_tag);
  gint width = pixelrow_width (src);
  gint alpha = num_channels - 1;  /*same for both */
  gfloat *sp = src_data + alpha;
  gfloat *dp = dest_data + alpha;
  gint initial_p = sp[0];
  gint initial_m = sp[(width -1) * num_channels];
  
  for (x = 0; x < width; x++)
    {
      start = (x < length) ? -x : -length;
      end = (width <= (x + length)) ? (width - x - 1) : length;
      start = -x;
      val = 0;
      i = start;
      rle_count_ptr = rle_count + (x + i);
      rle_values_ptr = (gfloat*)rle_values_data + (x+i); 

      if (start != -length)
	val += initial_p * (sum[start] - sum[-length]);

      while (i < end)
	{
	  pixels = *rle_count_ptr;
	  i += pixels;
	  if (i > end)
	    i = end;
	  val += *rle_values_ptr * (sum[i] - sum[start]);
	  rle_values_ptr += pixels;
	  rle_count_ptr += pixels;
	  start = i;
	}

      if (end != length)
	val += initial_m * (sum[length] - sum[end]);

      dp[x * num_channels] = val / total;
    }
}


typedef void (*BorderSetOpacityFunc) (gint,gint,Paint*);
static BorderSetOpacityFunc border_set_opacity_funcs[] =
{
  border_set_opacity_u8,
  border_set_opacity_u16,
  border_set_opacity_float
};


void
border_area (
	     PixelArea     *dest_area,
	     void          *bs_ptr,
	     gint          bs_segs,
	     gint          radius
	    )
{
  BoundSeg * bs;
  gint r, i, j;
  Tag dest_tag = pixelarea_tag (dest_area);
  Precision prec = tag_precision (dest_tag);
  Paint *opacity = paint_new ( dest_tag, NULL) ;
  
  /* should do a check on the dest_tag to make sure grayscale */
 
  bs = (BoundSeg *) bs_ptr;

  /*  draw the border  */
  for (r = 0; r <= radius; r++)
    {
      (*border_set_opacity_funcs[ prec-1 ]) (r, radius, opacity);      

      j = radius - r;

      for (i = 0; i <= j; i++)
	{
	  /*  northwest  */
	  draw_segments (dest_area, bs, bs_segs, -i, -(j - i), opacity);

	  /*  only draw the rest if they are different  */
	  if (j)
	    {
	      /*  northeast  */
	      draw_segments (dest_area, bs, bs_segs, i, -(j - i), opacity);
	      /*  southeast  */
	      draw_segments (dest_area, bs, bs_segs, i, (j - i), opacity);
	      /*  southwest  */
	      draw_segments (dest_area, bs, bs_segs, -i, (j - i), opacity);
	    }
	}
    }
    paint_delete (opacity);
}

static void
border_set_opacity_u8(
		     gint r,
		     gint radius,
                     Paint *opacity
		    )
{
 
  guint8 *data =(guint8*) paint_data (opacity);
  *data = 255 * (r + 1) / (radius + 1);
}


static void
border_set_opacity_u16(
		     gint r,
		     gint radius,
                     Paint *opacity
		    )
{
  guint16 *data =(guint16*) paint_data (opacity);
  *data = 65535 * (r + 1) / (radius + 1);
}


static void
border_set_opacity_float(
		     gint r,
		     gint radius,
                     Paint *opacity
		    )
{
  gfloat *data = (gfloat*)paint_data (opacity);
  *data = (r + 1) / (gfloat)(radius + 1);
}


/* non-interpolating scale_region.  [adam]
 */

typedef void (*ScaleRowNoResampleFunc) (PixelRow*,PixelRow*,gint*);
static ScaleRowNoResampleFunc scale_row_no_resample_funcs[] =
{
  scale_row_no_resample_u8,
  scale_row_no_resample_u16,
  scale_row_no_resample_float
};

void
scale_area_no_resample (
			PixelArea *src_area,
			PixelArea *dest_area
		       )
{
  gint *x_src_offsets, *y_src_offsets;
  gint last_src_y;
  gint x,y,b;
  PixelRow src_row, dest_row;
  guchar *dest_row_data,  *src_row_data;
  Tag src_tag = pixelarea_tag (src_area);
  Tag dest_tag = pixelarea_tag (dest_area);
  gint orig_width = pixelarea_width (src_area);
  gint orig_height = pixelarea_height (src_area); 
  gint width = pixelarea_width (dest_area);
  gint height = pixelarea_height (dest_area); 
  gint bytes_per_pixel = tag_bytes (src_tag);
  gint num_channels = tag_num_channels (src_tag);
  gint row_num_channels;
  Precision prec = tag_precision (src_tag);

  /* get a dest row to copy src pixels to */ 
  dest_row_data = (guchar *) g_malloc (width * bytes_per_pixel); 
  pixelrow_init (&dest_row, dest_tag,(guchar *)dest_row_data, width);
  
  /* get a src row */
  src_row_data = (guchar *) g_malloc (orig_width * bytes_per_pixel); 
  pixelrow_init (&src_row, src_tag,(guchar *)src_row_data, orig_width);
  
  /* x and y scale tables  */
  x_src_offsets = (int *) g_malloc (width * num_channels * sizeof(int));
  y_src_offsets = (int *) g_malloc (height * sizeof(int));
  
  /*  pre-calc the scale tables  */
  for (b = 0; b < num_channels; b++)
    {
      for (x = 0; x < width; x++)
	{
	  x_src_offsets [b + x * num_channels] = b + num_channels * ((x * orig_width + orig_width / 2) / width);
	}
    }
  for (y = 0; y < height; y++)
    {
      y_src_offsets [y] = (y * orig_height + orig_height / 2) / height;
    }
  
  /*  do the scaling  */
  row_num_channels = width * num_channels;
  last_src_y = -1;
  for (y = 0; y < height; y++)
    {
      /* if the source of this line was the same as the source
       *  of the last line, there's no point in re-rescaling.
       */
      if (y_src_offsets[y] != last_src_y)
	{
          /* pixel_region_get_row (srcPR, 0, y_src_offsets[y], orig_width, src, 1); */
          pixelarea_copy_row (src_area, &src_row, 0, y_src_offsets[y], orig_width,  1);
          (*scale_row_no_resample_funcs [prec-1]) (&src_row, &dest_row, x_src_offsets);
	  last_src_y = y_src_offsets[y];
	}
      /* pixel_region_set_row (destPR, 0 , y, width, dest); */
      pixelarea_write_row (dest_area, &dest_row, 0, y, width );
    }
  
  g_free (x_src_offsets);
  g_free (y_src_offsets);
  g_free (dest_row_data);
  g_free (src_row_data);
}

static void
scale_row_no_resample_u8 (
			  PixelRow *src_row,
			  PixelRow *dest_row,
			  gint     *x_src_offsets
			 )
{
  gint x;
  guint8 *dest = (guint8*) pixelrow_data (dest_row);
  guint8 *src = (guint8*) pixelrow_data (src_row);		
  gint row_num_channels = tag_num_channels (pixelrow_tag (dest_row));
  for (x = 0; x < row_num_channels ; x++)
    dest[x] = src[x_src_offsets[x]];
}
			
static void
scale_row_no_resample_u16 (
			  PixelRow *src_row,
			  PixelRow *dest_row,
			  gint     *x_src_offsets
			 )
{
  gint x;
  guint16 *dest = (guint16*) pixelrow_data (dest_row);
  guint16 *src = (guint16*) pixelrow_data (src_row);		
  gint row_num_channels = tag_num_channels (pixelrow_tag (dest_row));
  for (x = 0; x < row_num_channels ; x++)
    dest[x] = src[x_src_offsets[x]];
}
			
static void
scale_row_no_resample_float (
			  PixelRow *src_row,
			  PixelRow *dest_row,
			  gint     *x_src_offsets
			 )
{
  gint x;
  gfloat *dest = (gfloat*) pixelrow_data (dest_row);
  gfloat *src = (gfloat*) pixelrow_data (src_row);		
  gint row_num_channels = tag_num_channels (pixelrow_tag (dest_row));
  for (x = 0; x < row_num_channels ; x++)
    dest[x] = src[x_src_offsets[x]];
}


typedef void (*ScaleRowFunc) ( guchar*, guchar*, guchar*, guchar*, 
		    gdouble, gdouble*, gdouble, gdouble, gdouble*, 
		    gint, gint, gint, gint); 
static ScaleRowFunc scale_row_funcs[] =
{
  scale_row_u8,
  scale_row_u16,
  scale_row_float
};

typedef void (*ScaleSetDestRowFunc) (PixelRow*, gdouble*);
static ScaleSetDestRowFunc scale_set_dest_row_funcs[] =
{
  scale_set_dest_row_u8,
  scale_set_dest_row_u16,
  scale_set_dest_row_float
};

void
scale_area (
	    PixelArea *src_area,
	    PixelArea *dest_area
           )
{
  gdouble *row_accum;
  gint src_row_num, dest_row_num, src_col_num;
  gdouble x_rat, y_rat;
  gdouble x_cum, y_cum;
  gdouble x_last, y_last;
  gdouble * x_frac, y_frac, tot_frac;
  gfloat  dy;
  gint i, x;
  gint advance_dest_y;
  ScaleType scale_type;
  guchar *src_row_data, *src_m1_row_data, *src_p1_row_data, *src_p2_row_data;
  PixelRow src_row, src_m1_row, src_p1_row, src_p2_row;
  guchar *dest_row_data;
  PixelRow dest_row;
  guchar *src, *src_m1, *src_p1, *src_p2;
  gint orig_width = pixelarea_width (src_area);
  gint orig_height = pixelarea_height (src_area);
  gint width = pixelarea_width (dest_area);
  gint height = pixelarea_height (dest_area);
  Tag src_tag = pixelarea_tag (src_area);
  Tag dest_tag = pixelarea_tag (dest_area);
  gint num_channels = tag_num_channels (dest_tag);
  gint bytes_per_pixel = tag_bytes (dest_tag);
  Precision prec = tag_precision (src_tag); 

  /* set up the src rows and data  */
  src_m1_row_data = (guchar *) g_malloc (orig_width * bytes_per_pixel);
  src_row_data    = (guchar *) g_malloc (orig_width * bytes_per_pixel);
  src_p1_row_data = (guchar *) g_malloc (orig_width * bytes_per_pixel);
  src_p2_row_data = (guchar *) g_malloc (orig_width * bytes_per_pixel);
  
  pixelrow_init (&src_m1_row, src_tag,(guchar *)src_m1_row_data, orig_width);
  pixelrow_init (&src_row, src_tag,(guchar *)src_row_data, orig_width);
  pixelrow_init (&src_p1_row, src_tag,(guchar *)src_p1_row_data, orig_width);
  pixelrow_init (&src_p2_row, src_tag,(guchar *)src_p2_row_data, orig_width);
  
  /*  find the ratios of old x to new x and old y to new y  */
  x_rat = (double) orig_width / (double) width;
  y_rat = (double) orig_height / (double) height;

  /*  determine the scale type  */
  if (x_rat < 1.0 && y_rat < 1.0)
    scale_type = MagnifyX_MagnifyY;
  else if (x_rat < 1.0 && y_rat >= 1.0)
    scale_type = MagnifyX_MinifyY;
  else if (x_rat >= 1.0 && y_rat < 1.0)
    scale_type = MinifyX_MagnifyY;
  else
    scale_type = MinifyX_MinifyY;

  /*  allocate an array to help with the calculations  */
  row_accum  = (double *) g_malloc (sizeof (double) * width * num_channels);
  x_frac = (double *) g_malloc (sizeof (double) * (width + orig_width));

  /*  initialize the pre-calculated pixel fraction array  */
  src_col_num = 0;
  x_cum = 0.0;
  x_last = 0.0;

  for (i = 0; i < width + orig_width; i++)
    {
      if (x_cum + x_rat <= (src_col_num + 1 + EPSILON))
	{
	  x_cum += x_rat;
	  x_frac[i] = x_cum - x_last;
	}
      else
	{
	  src_col_num ++;
	  x_frac[i] = src_col_num - x_last;
	}
      x_last += x_frac[i];
    }

  /*  clear the "row" array  */
  memset (row_accum, 0, sizeof (double) * width * num_channels);

  /*  counters zero...  */
  src_row_num = 0;
  dest_row_num = 0;
  y_cum = 0.0;
  y_last = 0.0;

  /*  Get the first src row  */
  /*pixel_region_get_row (srcPR, 0, src_row_num, orig_width, src, 1);*/
  pixelarea_copy_row (src_area, &src_row, 0, src_row_num, orig_width, 1); 

  /*  Get the next two if possible  */
  if (src_row_num < (orig_height - 1))
    /*pixel_region_get_row (srcPR, 0, (src_row_num + 1), orig_width, src_p1, 1);*/
    pixelarea_copy_row (src_area, &src_p1_row, 0, src_row_num + 1, orig_width, 1); 
  if ((src_row_num + 1) < (orig_height - 1))
    /*pixel_region_get_row (srcPR, 0, (src_row_num + 2), orig_width, src_p2, 1);*/
    pixelarea_copy_row (src_area, &src_p2_row, 0, src_row_num + 2, orig_width, 1); 

  /*  Scale the selected region  */
  i = height;
  while (i)
    {

      /* determine the fraction of the src pixel we are using for y */
      if (y_cum + y_rat <= (src_row_num + 1 + EPSILON))
	{
	  y_cum += y_rat;
	  dy = y_cum - src_row_num;
	  y_frac = y_cum - y_last;
	  advance_dest_y = TRUE;
	}
      else
	{
	  y_frac = (src_row_num + 1) - y_last;
	  dy = 1.0;
	  advance_dest_y = FALSE;
	}

      y_last += y_frac;

      /* set up generic guchar pointers to correct data */ 
      src = pixelrow_data (&src_row);
      
      if (src_row_num > 0)
	src_m1 = pixelrow_data (&src_m1_row);
      else
	src_m1 = pixelrow_data (&src_row);
      
      if (src_row_num < (orig_height - 1))
	src_p1 = pixelrow_data (&src_p1_row);
      else
	src_p1 = pixelrow_data (&src_row);

      if ((src_row_num + 1) < (orig_height - 1))
	src_p2 = pixelrow_data (&src_p2_row);
      else
	src_p2 = pixelrow_data (&src_p1_row);
       
      /* scale and accumulate into row_accum array */ 
      (*scale_row_funcs [prec-1])(src, src_m1, src_p1, src_p2, dy, x_frac, y_frac, x_rat, 
			row_accum, num_channels, orig_width, width, scale_type); 

      if (advance_dest_y)
	{
	  tot_frac = 1.0 / (x_rat * y_rat);
	  pixelarea_getdata (dest_area, &dest_row, dest_row_num);
	  
          /*  copy row_accum to "dest"  */
          for( x= 0; x < width; x++)
            row_accum[x] *= tot_frac; 
         
          /* this sets dest row from array row_accum */
          (*scale_set_dest_row_funcs [prec-1])( &dest_row, row_accum ); 
	  
          /*  clear row_accum  */
	  memset (row_accum, 0, sizeof (double) * width * num_channels);
          dest_row_num++;
	  i--;
	}
      else
	{
	  /*  Shuffle data in the rows  */
          pixelrow_init (&src_m1_row, src_tag, src_row_data, orig_width);
	  pixelrow_init (&src_row, src_tag, src_p1_row_data, orig_width);
	  pixelrow_init (&src_p1_row, src_tag, src_p2_row_data, orig_width);
	  pixelrow_init (&src_p2_row, src_tag, src_m1_row_data, orig_width);
	  
	  src_row_num++;
  	  if ((src_row_num + 1) < (orig_height - 1))
	    /*pixel_region_get_row (srcPR, 0, (src_row_num + 2), orig_width, src_p2, 1);*/
            pixelarea_copy_row (src_area, &src_p2_row, 0, src_row_num + 2, orig_width, 1);
	}
    }

  /*  free up temporary arrays  */
  g_free (row_accum);
  g_free (x_frac);
  g_free (src_m1_row_data);
  g_free (src_row_data);
  g_free (src_p1_row_data);
  g_free (src_p2_row_data);
  g_free (dest_row_data);
}


static void
scale_set_dest_row_u8 (
			PixelRow *dest_row,
			gdouble *row_accum
		   )
{
  gint x;
  guint8 *dest = (guint8*)pixelrow_data (dest_row);
  gint width = pixelrow_width (dest_row);
  for (x = 0; x < width; x++)
    {
      dest[x] = (guint8) row_accum[x];
    }
}


static void
scale_set_dest_row_u16 (
			PixelRow *dest_row,
			gdouble *row_accum
		   )
{
  gint x;
  guint16 *dest = (guint16*)pixelrow_data (dest_row);
  gint width = pixelrow_width (dest_row);
  for (x = 0; x < width; x++)
    {
      dest[x] = (guint16) row_accum[x];
    }
}


static void
scale_set_dest_row_float (
			PixelRow *dest_row,
			gdouble *row_accum
		   )
{
  gint x;
  gfloat *dest = (gfloat*)pixelrow_data (dest_row);
  gint width = pixelrow_width (dest_row);
  for (x = 0; x < width; x++)
    {
      dest[x] = (gfloat) row_accum[x];
    }
}


static void 
scale_row_u8( 
	      guchar *src, 
	      guchar *src_m1, 
	      guchar *src_p1, 
	      guchar *src_p2, 
	      gdouble dy, 
	      gdouble *x_frac, 
	      gdouble y_frac, 
	      gdouble x_rat, 
	      gdouble *row_accum, 
	      gint    num_channels, 
	      gint    orig_width, 
	      gint    width, 
	      gint    scale_type
             ) 
{
  gdouble dx;
  gint b;
  gint advance_dest_x;
  gint minus_x, plus_x, plus2_x;
  gdouble tot_frac;
  
  /* cast the data to the right type */
  guint8 *s    = (guint8*)src;
  guint8 *s_m1 = (guint8*)src_m1;
  guint8 *s_p1 = (guint8*)src_p1;
  guint8 *s_p2 = (guint8*)src_p2;
  gint src_col_num = 0;
  gdouble x_cum = 0.0;
  gdouble *r = row_accum;   /*the acumulated array so far*/ 
  gint frac = 0;
  gint j = width;
  
  /* loop through the x values in dest */
 
  while (j)
    {
      if (x_cum + x_rat <= (src_col_num + 1 + EPSILON))
	{
	  x_cum += x_rat;
	  dx = x_cum - src_col_num;
	  advance_dest_x = TRUE;
	}
      else
	{
	  dx = 1.0;
	  advance_dest_x = FALSE;
	}

      tot_frac = x_frac[frac++] * y_frac;

      minus_x = (src_col_num > 0) ? -num_channels : 0;
      plus_x = (src_col_num < (orig_width - 1)) ? num_channels : 0;
      plus2_x = ((src_col_num + 1) < (orig_width - 1)) ? num_channels * 2 : plus_x;

      if (cubic_interpolation)
	switch (scale_type)
	  {
	  case MagnifyX_MagnifyY:
	    for (b = 0; b < num_channels; b++)
	      r[b] += cubic (dy, 
	           cubic (dx, s_m1[b+minus_x], s_m1[b], s_m1[b+plus_x], s_m1[b+plus2_x], 255.0, 0.0),
	           cubic (dx, s[b+minus_x], s[b], s[b+plus_x], s[b+plus2_x], 255.0, 0.0),
		   cubic (dx, s_p1[b+minus_x], s_p1[b], s_p1[b+plus_x], s_p1[b+plus2_x], 255.0, 0.0),
		   cubic (dx, s_p2[b+minus_x], s_p2[b], s_p2[b+plus_x], s_p2[b+plus2_x], 255.0, 0.0), 
		   255.0, 
		   0.0) * tot_frac;
	    break;
	  case MagnifyX_MinifyY:
	    for (b = 0; b < num_channels; b++)
	      r[b] += cubic (dx, s[b+minus_x], s[b], s[b+plus_x], s[b+plus2_x], 255.0, 0.0) * tot_frac;
	    break;
	  case MinifyX_MagnifyY:
	    for (b = 0; b < num_channels; b++)
	      r[b] += cubic (dy, s_m1[b], s[b], s_p1[b], s_p2[b], 255.0, 0.0) * tot_frac;
	    break;
	  case MinifyX_MinifyY:
	    for (b = 0; b < num_channels; b++)
	      r[b] += s[b] * tot_frac;
	    break;
	  }
      else
	switch (scale_type)
	  {
	  case MagnifyX_MagnifyY:
	    for (b = 0; b < num_channels; b++)
	      r[b] += ((1 - dy) * ((1 - dx) * s[b] + dx * s[b+plus_x]) +
		       dy  * ((1 - dx) * s_p1[b] + dx * s_p1[b+plus_x])) * tot_frac;
	    break;
	  case MagnifyX_MinifyY:
	    for (b = 0; b < num_channels; b++)
	      r[b] += (s[b] * (1 - dx) + s[b+plus_x] * dx) * tot_frac;
	    break;
	  case MinifyX_MagnifyY:
	    for (b = 0; b < num_channels; b++)
	      r[b] += (s[b] * (1 - dy) + s_p1[b] * dy) * tot_frac;
	    break;
	  case MinifyX_MinifyY:
	    for (b = 0; b < num_channels; b++)
	      r[b] += s[b] * tot_frac;
	    break;
	  }

      if (advance_dest_x)
	{
	  r += num_channels;
	  j--;
	}
      else
	{
	  s_m1 += num_channels;
	  s    += num_channels;
	  s_p1 += num_channels;
	  s_p2 += num_channels;
	  src_col_num++;
	}
    }
}

static void 
scale_row_u16( 
	      guchar *src, 
	      guchar *src_m1, 
	      guchar *src_p1, 
	      guchar *src_p2, 
	      gdouble dy, 
	      gdouble *x_frac, 
	      gdouble y_frac, 
	      gdouble x_rat, 
	      gdouble *row_accum, 
	      gint    num_channels, 
	      gint    orig_width, 
	      gint    width, 
	      gint    scale_type
             ) 
{
  gdouble dx;
  gint b;
  gint advance_dest_x;
  gint minus_x, plus_x, plus2_x;
  gdouble tot_frac;
  
  /* cast the data to the right type */
  guint16 *s    = (guint16*)src;
  guint16 *s_m1 = (guint16*)src_m1;
  guint16 *s_p1 = (guint16*)src_p1;
  guint16 *s_p2 = (guint16*)src_p2;
  gint src_col_num = 0;
  gdouble x_cum = 0.0;
  gdouble *r = row_accum;   /*the acumulated array so far*/ 
  gint frac = 0;
  gint j = width;
  
  /* loop through the x values in dest */
 
  while (j)
    {
      if (x_cum + x_rat <= (src_col_num + 1 + EPSILON))
	{
	  x_cum += x_rat;
	  dx = x_cum - src_col_num;
	  advance_dest_x = TRUE;
	}
      else
	{
	  dx = 1.0;
	  advance_dest_x = FALSE;
	}

      tot_frac = x_frac[frac++] * y_frac;

      minus_x = (src_col_num > 0) ? -num_channels : 0;
      plus_x = (src_col_num < (orig_width - 1)) ? num_channels : 0;
      plus2_x = ((src_col_num + 1) < (orig_width - 1)) ? num_channels * 2 : plus_x;

      if (cubic_interpolation)
	switch (scale_type)
	  {
	  case MagnifyX_MagnifyY:
	    for (b = 0; b < num_channels; b++)
	      r[b] += cubic ( dy, 
                   cubic (dx, s_m1[b+minus_x], s_m1[b], s_m1[b+plus_x], s_m1[b+plus2_x], 65535.0, 0.0),
		   cubic (dx, s[b+minus_x], s[b],	s[b+plus_x], s[b+plus2_x], 65535.0, 0.0),
		   cubic (dx, s_p1[b+minus_x], s_p1[b], s_p1[b+plus_x], s_p1[b+plus2_x], 65535.0, 0.0),
		   cubic (dx, s_p2[b+minus_x], s_p2[b], s_p2[b+plus_x], s_p2[b+plus2_x], 65535.0, 0.0), 
		   65535.0, 
		   0.0) * tot_frac;
	    break;
	  case MagnifyX_MinifyY:
	    for (b = 0; b < num_channels; b++)
	      r[b] += cubic (dx, s[b+minus_x], s[b], s[b+plus_x], s[b+plus2_x], 65535.0, 0.0) * tot_frac;
	    break;
	  case MinifyX_MagnifyY:
	    for (b = 0; b < num_channels; b++)
	      r[b] += cubic (dy, s_m1[b], s[b], s_p1[b], s_p2[b], 65535.0, 0.0) * tot_frac;
	    break;
	  case MinifyX_MinifyY:
	    for (b = 0; b < num_channels; b++)
	      r[b] += s[b] * tot_frac;
	    break;
	  }
      else
	switch (scale_type)
	  {
	  case MagnifyX_MagnifyY:
	    for (b = 0; b < num_channels; b++)
	      r[b] += ((1 - dy) * ((1 - dx) * s[b] + dx * s[b+plus_x]) +
		       dy  * ((1 - dx) * s_p1[b] + dx * s_p1[b+plus_x])) * tot_frac;
	    break;
	  case MagnifyX_MinifyY:
	    for (b = 0; b < num_channels; b++)
	      r[b] += (s[b] * (1 - dx) + s[b+plus_x] * dx) * tot_frac;
	    break;
	  case MinifyX_MagnifyY:
	    for (b = 0; b < num_channels; b++)
	      r[b] += (s[b] * (1 - dy) + s_p1[b] * dy) * tot_frac;
	    break;
	  case MinifyX_MinifyY:
	    for (b = 0; b < num_channels; b++)
	      r[b] += s[b] * tot_frac;
	    break;
	  }

      if (advance_dest_x)
	{
	  r += num_channels;
	  j--;
	}
      else
	{
	  s_m1 += num_channels;
	  s    += num_channels;
	  s_p1 += num_channels;
	  s_p2 += num_channels;
	  src_col_num++;
	}
    }
}

static void 
scale_row_float( 
	      guchar *src, 
	      guchar *src_m1, 
	      guchar *src_p1, 
	      guchar *src_p2, 
	      gdouble dy, 
	      gdouble *x_frac, 
	      gdouble y_frac, 
	      gdouble x_rat, 
	      gdouble *row_accum, 
	      gint    num_channels, 
	      gint    orig_width, 
	      gint    width, 
	      gint    scale_type
             ) 
{
  gdouble dx;
  gint b;
  gint advance_dest_x;
  gint minus_x, plus_x, plus2_x;
  gdouble tot_frac;
  
  /* cast the data to the right type */
  gfloat *s    = (gfloat*)src;
  gfloat *s_m1 = (gfloat*)src_m1;
  gfloat *s_p1 = (gfloat*)src_p1;
  gfloat *s_p2 = (gfloat*)src_p2;
  gint src_col_num = 0;
  gdouble x_cum = 0.0;
  gdouble *r = row_accum;   /*the acumulated array so far*/ 
  gint frac = 0;
  gint j = width;
  
  /* loop through the x values in dest */
 
  while (j)
    {
      if (x_cum + x_rat <= (src_col_num + 1 + EPSILON))
	{
	  x_cum += x_rat;
	  dx = x_cum - src_col_num;
	  advance_dest_x = TRUE;
	}
      else
	{
	  dx = 1.0;
	  advance_dest_x = FALSE;
	}

      tot_frac = x_frac[frac++] * y_frac;

      minus_x = (src_col_num > 0) ? -num_channels : 0;
      plus_x = (src_col_num < (orig_width - 1)) ? num_channels : 0;
      plus2_x = ((src_col_num + 1) < (orig_width - 1)) ? num_channels * 2 : plus_x;

      if (cubic_interpolation)
	switch (scale_type)
	  {
	  case MagnifyX_MagnifyY:
	    for (b = 0; b < num_channels; b++)
	      r[b] += cubic ( dy, 
                   cubic (dx, s_m1[b+minus_x], s_m1[b], s_m1[b+plus_x], s_m1[b+plus2_x], G_MAXFLOAT, G_MINFLOAT),
		   cubic (dx, s[b+minus_x], s[b],	s[b+plus_x], s[b+plus2_x], G_MAXFLOAT, G_MINFLOAT),
		   cubic (dx, s_p1[b+minus_x], s_p1[b], s_p1[b+plus_x], s_p1[b+plus2_x], G_MAXFLOAT, G_MINFLOAT),
		   cubic (dx, s_p2[b+minus_x], s_p2[b], s_p2[b+plus_x], s_p2[b+plus2_x], G_MAXFLOAT, G_MINFLOAT), 
		   G_MAXFLOAT, 
		   G_MINFLOAT) * tot_frac;
	    break;
	  case MagnifyX_MinifyY:
	    for (b = 0; b < num_channels; b++)
	      r[b] += cubic (dx, s[b+minus_x], s[b], s[b+plus_x], s[b+plus2_x], G_MAXFLOAT ,G_MINFLOAT) * tot_frac;
	    break;
	  case MinifyX_MagnifyY:
	    for (b = 0; b < num_channels; b++)
	      r[b] += cubic (dy, s_m1[b], s[b], s_p1[b], s_p2[b], G_MAXFLOAT, G_MINFLOAT) * tot_frac;
	    break;
	  case MinifyX_MinifyY:
	    for (b = 0; b < num_channels; b++)
	      r[b] += s[b] * tot_frac;
	    break;
	  }
      else
	switch (scale_type)
	  {
	  case MagnifyX_MagnifyY:
	    for (b = 0; b < num_channels; b++)
	      r[b] += ((1 - dy) * ((1 - dx) * s[b] + dx * s[b+plus_x]) +
		       dy  * ((1 - dx) * s_p1[b] + dx * s_p1[b+plus_x])) * tot_frac;
	    break;
	  case MagnifyX_MinifyY:
	    for (b = 0; b < num_channels; b++)
	      r[b] += (s[b] * (1 - dx) + s[b+plus_x] * dx) * tot_frac;
	    break;
	  case MinifyX_MagnifyY:
	    for (b = 0; b < num_channels; b++)
	      r[b] += (s[b] * (1 - dy) + s_p1[b] * dy) * tot_frac;
	    break;
	  case MinifyX_MinifyY:
	    for (b = 0; b < num_channels; b++)
	      r[b] += s[b] * tot_frac;
	    break;
	  }

      if (advance_dest_x)
	{
	  r += num_channels;
	  j--;
	}
      else
	{
	  s_m1 += num_channels;
	  s    += num_channels;
	  s_p1 += num_channels;
	  s_p2 += num_channels;
	  src_col_num++;
	}
    }
}


typedef void (*SubsampleRowFunc) (PixelRow*, gdouble*, gdouble, gdouble, gdouble*);
static SubsampleRowFunc subsample_row_funcs[] =
{
  subsample_row_u8,
  subsample_row_u16,
  subsample_row_float
};

void
subsample_area (
		PixelArea *src_area,
		PixelArea *dest_area,
		gint        subsample
	       )
{
  gdouble * row_accum;
  gint src_row_num, src_col_num, dest_row_num;
  gdouble x_rat, y_rat;
  gdouble x_cum, y_cum;
  gdouble x_last, y_last;
  gdouble * x_frac, y_frac, tot_frac;
  gint i;
  gint x;
  gint advance_dest;
  PixelRow src_row, dest_row;
  guchar *src_row_data;
  Tag src_tag = pixelarea_tag (src_area);
  Tag dest_tag = pixelarea_tag (dest_area);
  Precision prec = tag_precision (src_tag); 
  
  /* src and dest width and height */
  gint orig_width = pixelarea_width (src_area) / subsample;
  gint orig_height = pixelarea_height (src_area) / subsample;
  gint width = pixelarea_width (dest_area);
  gint height = pixelarea_height (dest_area);
  
  /*  get bytes per pixel and num_channels per pixel */
  gint bytes_per_pixel = tag_bytes (dest_tag);
  gint num_channels = tag_num_channels (dest_tag); 

  /*  set up the src pixel row */
  src_row_data = (guchar *) g_malloc (orig_width * bytes_per_pixel);
  pixelrow_init (&src_row, src_tag, src_row_data, orig_width);

  /*  find the ratios of old x to new x and old y to new y  */
  x_rat = (double) orig_width / (double) width;
  y_rat = (double) orig_height / (double) height;

  /*  allocate an array to help with the calculations  */
  row_accum    = (double *) g_malloc (sizeof (double) * width * num_channels);
  x_frac = (double *) g_malloc (sizeof (double) * (width + orig_width));

  /*  initialize the pre-calculated pixel fraction array  */
  src_col_num = 0;
  x_cum = 0.0;
  x_last = 0.0;

  for (i = 0; i < width + orig_width; i++)
    {
      if (x_cum + x_rat <= (src_col_num + 1 + EPSILON))
	{
	  x_cum += x_rat;
	  x_frac[i] = x_cum - x_last;
	}
      else
	{
	  src_col_num ++;
	  x_frac[i] = src_col_num - x_last;
	}
      x_last += x_frac[i];
    }

  /*  clear the row_accum array  */
  memset (row_accum, 0, sizeof (double) * width * num_channels);

  /*  counters...  */
  src_row_num = 0;
  dest_row_num = 0;
  y_cum = 0.0;
  y_last = 0.0;

  /* pixel_region_get_row (srcPR, 0, src_row_num * subsample, orig_width * subsample, src, subsample); */
  pixelarea_copy_row (src_area, &src_row, 0, src_row_num * subsample,
  			 orig_width * subsample, subsample);

  /*  Scale the selected region  */
  for (i = 0; i < height; )
    {

      /* determine the fraction of the src pixel we are using for y */
      if (y_cum + y_rat <= (src_row_num + 1 + EPSILON))
	{
	  y_cum += y_rat;
	  y_frac = y_cum - y_last;
	  advance_dest = TRUE;
	}
      else
	{
	  src_row_num ++;
	  y_frac = src_row_num - y_last;
	  advance_dest = FALSE;
	}

      y_last += y_frac;
       
      (*subsample_row_funcs[prec-1]) ( &src_row, x_frac, y_frac, x_rat, row_accum);   

      if (advance_dest)
	{
	  tot_frac = 1.0 / (x_rat * y_rat);
  	  pixelarea_getdata (dest_area, &dest_row, dest_row_num);
          
          for( x= 0; x < width; x++)
            row_accum[x] *= tot_frac; 
         
          /*  set dest row from row_accum */
          (*scale_set_dest_row_funcs [prec-1])( &dest_row, row_accum ); 
          
          dest_row_num++;

	  /*  clear the row_accum array  */
	  memset (row_accum, 0, sizeof (double) * width * num_channels);

	  i++;
	}
      else
 	 /*pixel_region_get_row (srcPR, 0, src_row_num * subsample, orig_width * subsample, src, subsample);*/
        pixelarea_copy_row (src_area, &src_row, 0, src_row_num * subsample, 
  				orig_width * subsample, subsample);
    }

  /*  free up temporary arrays  */
  g_free (row_accum);
  g_free (x_frac);
  g_free (src_row_data);
}


static void
subsample_row_u8 ( 
		PixelRow *src_row, 
		gdouble *x_frac, 
		gdouble y_frac, 
		gdouble x_rat, 
		gdouble *row_accum
		)   
{
  gdouble tot_frac;
  gint b;
  guint8 * s = (guint8 *)pixelrow_data (src_row);
  gint num_channels = tag_num_channels (pixelrow_tag (src_row));
  gint frac = 0;
  gdouble *r = row_accum;
  gint src_col_num = 0;
  gdouble x_cum = 0.0;
  gint j = pixelrow_width (src_row);
   
  while (j)
    {
      tot_frac = x_frac[frac++] * y_frac;

      for (b = 0; b < num_channels; b++)
	r[b] += s[b] * tot_frac;

      /*  increment the destination  */
      if (x_cum + x_rat <= (src_col_num + 1 + EPSILON))
	{
	  r += num_channels;
	  x_cum += x_rat;
	  j--;
	}
      else  /* increment the source */
	{
	  s += num_channels;
	  src_col_num++;
	}
    }
}


static void
subsample_row_u16 ( 
		PixelRow *src_row, 
		gdouble *x_frac, 
		gdouble y_frac, 
		gdouble x_rat, 
		gdouble *row_accum
		)   
{
  gdouble tot_frac;
  gint b;
  guint16 * s = (guint16 *)pixelrow_data (src_row);
  gint num_channels = tag_num_channels (pixelrow_tag (src_row));
  gint frac = 0;
  gdouble *r = row_accum;
  gint src_col_num = 0;
  gdouble x_cum = 0.0;
  gint j = pixelrow_width (src_row);
   
  while (j)
    {
      tot_frac = x_frac[frac++] * y_frac;

      for (b = 0; b < num_channels; b++)
	r[b] += s[b] * tot_frac;

      /*  increment the destination  */
      if (x_cum + x_rat <= (src_col_num + 1 + EPSILON))
	{
	  r += num_channels;
	  x_cum += x_rat;
	  j--;
	}
      else  /* increment the source */
	{
	  s += num_channels;
	  src_col_num++;
	}
    }
}


static void
subsample_row_float ( 
		PixelRow *src_row, 
		gdouble *x_frac, 
		gdouble y_frac, 
		gdouble x_rat, 
		gdouble *row_accum
		)   
{
  gdouble tot_frac;
  gint b;
  gfloat *s = (gfloat *)pixelrow_data (src_row);
  gint num_channels = tag_num_channels (pixelrow_tag (src_row));
  gint frac = 0;
  gdouble *r = row_accum;
  gint src_col_num = 0;
  gdouble x_cum = 0.0;
  gint j = pixelrow_width (src_row);
   
  while (j)
    {
      tot_frac = x_frac[frac++] * y_frac;

      for (b = 0; b < num_channels; b++)
	r[b] += s[b] * tot_frac;

      /*  increment the destination  */
      if (x_cum + x_rat <= (src_col_num + 1 + EPSILON))
	{
	  r += num_channels;
	  x_cum += x_rat;
	  j--;
	}
      else  /* increment the source */
	{
	  s += num_channels;
	  src_col_num++;
	}
    }
}

float 
shapeburst_area  (
                  PixelArea * srcPR,
                  PixelArea * distPR
                  )
{
#if 0
  Tile *tile;
  unsigned char *tile_data;
  float max_iterations;
  float *distp_cur;
  float *distp_prev;
  float *tmp;
  float min_prev;
  int min;
  int min_left;
  int length;
  int i, j, k;
  int src;
  int fraction;
  int prev_frac;
  int x, y;
  int end;
  int boundary;
  int inc;

  src = 0;

  max_iterations = 0.0;

  length = distPR->w + 1;
  distp_prev = (float *) paint_funcs_get_buffer (sizeof (float) * length * 2);
  for (i = 0; i < length; i++)
    distp_prev[i] = 0.0;

  distp_prev += 1;
  distp_cur = distp_prev + length;

  for (i = 0; i < srcPR->h; i++)
    {
      /*  set the current dist row to 0's  */
      for (j = 0; j < length; j++)
	distp_cur[j - 1] = 0.0;

      for (j = 0; j < srcPR->w; j++)
	{
	  min_prev = MIN (distp_cur[j-1], distp_prev[j]);
	  min_left = MIN ((srcPR->w - j - 1), (srcPR->h - i - 1));
	  min = (int) MIN (min_left, min_prev);
	  fraction = 255;

	  /*  This might need to be changed to 0 instead of k = (min) ? (min - 1) : 0  */
	  for (k = (min) ? (min - 1) : 0; k <= min; k++)
	    {
	      x = j;
	      y = i + k;
	      end = y - k;

	      while (y >= end)
		{
		  tile = tile_manager_get_tile (srcPR->tiles, x, y, 0);
		  tile_ref (tile);
		  tile_data = tile->data + (tile->ewidth * (y % TILE_HEIGHT) + (x % TILE_WIDTH));
		  boundary = MIN ((y % TILE_HEIGHT), (tile->ewidth - (x % TILE_WIDTH) - 1));
		  boundary = MIN (boundary, (y - end)) + 1;
		  inc = 1 - tile->ewidth;

		  while (boundary--)
		    {
		      src = *tile_data;
		      if (src == 0)
			{
			  min = k;
			  y = -1;
			  break;
			}
		      if (src < fraction)
			fraction = src;

		      x++;
		      y--;
		      tile_data += inc;
		    }

		  tile_unref (tile, FALSE);
		}
	    }

	  if (src != 0)
	    {
	      /*  If min_left != min_prev use the previous fraction
	       *   if it is less than the one found
	       */
	      if (min_left != min)
		{
		  prev_frac = (int) (255 * (min_prev - min));
		  if (prev_frac == 255)
		    prev_frac = 0;
		  fraction = MIN (fraction, prev_frac);
		}
	      min++;
	    }

	  distp_cur[j] = min + fraction / 256.0;

	  if (distp_cur[j] > max_iterations)
	    max_iterations = distp_cur[j];
	}

      /*  set the dist row  */
      pixel_region_set_row (distPR, distPR->x, distPR->y + i, distPR->w, (unsigned char *) distp_cur);

      /*  swap pointers around  */
      tmp = distp_prev;
      distp_prev = distp_cur;
      distp_cur = tmp;
    }

  return max_iterations;
#endif
  return 0.0;
}

typedef gint (*ThinRowFunc) (PixelRow*,PixelRow*,PixelRow*,PixelRow*,gint);
static ThinRowFunc thin_row_funcs[] =
{
  thin_row_u8,
  thin_row_u16,
  thin_row_float
};


gint
thin_area (
	   PixelArea *src_area,
	   gint          type
          )
{
  int i;
  int found_one;

  PixelRow prev_row;
  PixelRow cur_row;
  PixelRow next_row;
  PixelRow dest_row;
  
  Tag src_tag = pixelarea_tag (src_area);
  Precision prec = tag_precision (src_tag);
  gint bytes_per_pixel = tag_bytes (src_tag);
  guint width = pixelarea_width (src_area);
  guint height = pixelarea_height (src_area);
  gint src_x = pixelarea_x (src_area); 
  gint src_y = pixelarea_y (src_area); 

  /* get buffers for data for prev, cur and next rows , 2 extra pixels for ends */
  guchar *prev_row_data = (guchar *) g_malloc ((width + 2) * bytes_per_pixel); 
  guchar *next_row_data = (guchar *) g_malloc ((width + 2) * bytes_per_pixel); 
  guchar *cur_row_data = (guchar *) g_malloc ((width + 2) * bytes_per_pixel); 
  
  /* get buffer for the dest row */
  guchar *dest_row_data = (guchar *) g_malloc (width  * bytes_per_pixel); 
  
  /* clear buffers to 0 */
  memset (prev_row_data, 0, (width + 2) * bytes_per_pixel);
  memset (next_row_data, 0, (width + 2) * bytes_per_pixel);
  memset (cur_row_data, 0, (width + 2) * bytes_per_pixel);
  
  /* set the PixelRow buffers, inset one pixel */ 
  pixelrow_init (&prev_row, src_tag, prev_row_data+1, width);
  pixelrow_init (&cur_row, src_tag, cur_row_data+1, width);
  pixelrow_init (&next_row, src_tag, next_row_data+1, width);
  
  /* load the first row of src_area into cur_row */
/*pixel_region_get_row (src, src->x, src->y, src->w, cur_row, 1); */
  pixelarea_copy_row (src_area, &cur_row, src_x, src_y, width, 1);      
  
  found_one = FALSE;
  for (i = 0; i < height; i++)
    {
      if (i < (height - 1))
        /*pixel_region_get_row (src,src->x,src->y + i + 1, src->w, cur_row, 1);*/
        pixelarea_copy_row (src_area, &next_row, src_x, src_y + i + 1, width,1);
      else
        memset (next_row_data, 0, width  * bytes_per_pixel);
       
      found_one = (*thin_row_funcs [prec-1]) (&cur_row, &next_row, &prev_row, &dest_row, type);

      pixelarea_write_row (src_area, &dest_row, src_x, src_y + i, width);
      
      /* shuffle prev, cur, next buffers around */
      pixelrow_init (&prev_row, src_tag, cur_row_data+1, width);
      pixelrow_init (&cur_row, src_tag, next_row_data+1, width);
      pixelrow_init (&next_row, src_tag, prev_row_data+1, width);
    }
  g_free (prev_row_data);
  g_free (next_row_data);
  g_free (cur_row_data);
  g_free (dest_row_data);
  return (found_one == FALSE);  /*  is the area empty yet?  */
}


static gint  
thin_row_u8 (
		PixelRow *cur_row,
		PixelRow *next_row,
		PixelRow *prev_row,
		PixelRow *dest_row,
		gint type
	     )
{
  gint j;
  gint found_one;
  guint8 *cur = (guint8*) pixelrow_data (cur_row);
  guint8 *next =(guint8*) pixelrow_data (next_row);
  guint8 *prev =(guint8*) pixelrow_data (prev_row);
  guint8 *dest =(guint8*) pixelrow_data (dest_row);
  gint width = pixelrow_width (cur_row);
  for (j = 0; j < width; j++)
    {
      if (type == SHRINK_REGION)
	{
	  if (cur[j] != 0)
	    {
	      found_one = TRUE;
	      dest[j] = cur[j];

	      if (cur[j - 1] < dest[j])
		dest[j] = cur[j - 1];
	      if (cur[j + 1] < dest[j])
		dest[j] = cur[j + 1];
	      if (prev[j] < dest[j])
		dest[j] = prev[j];
	      if (next[j] < dest[j])
		dest[j] = next[j];
	    }
	  else
	    dest[j] = cur[j];
	}
      else
	{
	  if (cur[j] != 255)
	    {
	      found_one = TRUE;
	      dest[j] = cur[j];

	      if (cur[j - 1] > dest[j])
		dest[j] = cur[j - 1];
	      if (cur[j + 1] > dest[j])
		dest[j] = cur[j + 1];
	      if (prev[j] > dest[j])
		dest[j] = prev[j];
	      if (next[j] > dest[j])
		dest[j] = next[j];
	    }
	  else
	    dest[j] = cur[j];
	}
    }
   return found_one;
}


static gint 
thin_row_u16 (
		PixelRow *cur_row,
		PixelRow *next_row,
		PixelRow *prev_row,
		PixelRow *dest_row,
		gint type
	     )
{
  gint j;
  gint found_one;
  guint16 *cur = (guint16*) pixelrow_data (cur_row);
  guint16 *next =(guint16*) pixelrow_data (next_row);
  guint16 *prev =(guint16*) pixelrow_data (prev_row);
  guint16 *dest =(guint16*) pixelrow_data (dest_row);
  gint width = pixelrow_width (cur_row);

  for (j = 0; j < width; j++)
    {
      if (type == SHRINK_REGION)
	{
	  if (cur[j] != 0)
	    {
	      found_one = TRUE;
	      dest[j] = cur[j];

	      if (cur[j - 1] < dest[j])
		dest[j] = cur[j - 1];
	      if (cur[j + 1] < dest[j])
		dest[j] = cur[j + 1];
	      if (prev[j] < dest[j])
		dest[j] = prev[j];
	      if (next[j] < dest[j])
		dest[j] = next[j];
	    }
	  else
	    dest[j] = cur[j];
	}
      else
	{
	  if (cur[j] != 65535)
	    {
	      found_one = TRUE;
	      dest[j] = cur[j];

	      if (cur[j - 1] > dest[j])
		dest[j] = cur[j - 1];
	      if (cur[j + 1] > dest[j])
		dest[j] = cur[j + 1];
	      if (prev[j] > dest[j])
		dest[j] = prev[j];
	      if (next[j] > dest[j])
		dest[j] = next[j];
	    }
	  else
	    dest[j] = cur[j];
	}
    }
   return found_one;
}

static gint 
thin_row_float (
		PixelRow *cur_row,
		PixelRow *next_row,
		PixelRow *prev_row,
		PixelRow *dest_row,
		gint type
	     )
{
  gint j;
  gint found_one;
  gfloat *cur = (gfloat*) pixelrow_data (cur_row);
  gfloat *next =(gfloat*) pixelrow_data (next_row);
  gfloat *prev =(gfloat*) pixelrow_data (prev_row);
  gfloat *dest =(gfloat*) pixelrow_data (dest_row);
  gint width = pixelrow_width (cur_row);

  for (j = 0; j < width; j++)
    {
      if (type == SHRINK_REGION)
	{
	  if (cur[j] != 0.0)
	    {
	      found_one = TRUE;
	      dest[j] = cur[j];

	      if (cur[j - 1] < dest[j])
		dest[j] = cur[j - 1];
	      if (cur[j + 1] < dest[j])
		dest[j] = cur[j + 1];
	      if (prev[j] < dest[j])
		dest[j] = prev[j];
	      if (next[j] < dest[j])
		dest[j] = next[j];
	    }
	  else
	    dest[j] = cur[j];
	}
      else
	{
	  if (cur[j] != 1.0)
	    {
	      found_one = TRUE;
	      dest[j] = cur[j];

	      if (cur[j - 1] > dest[j])
		dest[j] = cur[j - 1];
	      if (cur[j + 1] > dest[j])
		dest[j] = cur[j + 1];
	      if (prev[j] > dest[j])
		dest[j] = prev[j];
	      if (next[j] > dest[j])
		dest[j] = next[j];
	    }
	  else
	    dest[j] = cur[j];
	}
    }
   return found_one;
}


void 
swap_area  (
            PixelArea * src_area,
            PixelArea * dest_area
            )
{
  void *  pag;
  Tag src_tag = pixelarea_tag (src_area); 
  Tag dest_tag = pixelarea_tag (dest_area); 

   /* put in tags check */
  
  for (pag = pixelarea_register (2, src_area, dest_area);
       pag != NULL;
       pag = pixelarea_process (pag))
    {
      PixelRow src_row;
      PixelRow dest_row;
 
      gint h = pixelarea_height (src_area);
      while (h--)
        {
          pixelarea_getdata (src_area, &src_row, h);
          pixelarea_getdata (dest_area, &dest_row, h);
            
          /* swap_pixels (s, d, src->w * bytes); */
          swap_row (&src_row, &dest_row);
        }
    }
}


void 
apply_mask_to_area  (
                     PixelArea * src_area,
                     PixelArea * mask_area,
                     Paint * opacity
                     )
{
  void *  pag;
  Tag src_tag = pixelarea_tag (src_area); 
  Tag mask_tag = pixelarea_tag (mask_area); 

   /* put in tags check */
  
  for (pag = pixelarea_register (2, src_area, mask_area);
       pag != NULL;
       pag = pixelarea_process (pag))
    {
      PixelRow srcrow;
      PixelRow maskrow;
      int h;
      
      h = pixelarea_height (src_area);
      while (h--)
	{
	  pixelarea_getdata (src_area, &srcrow, h);
	  pixelarea_getdata (mask_area, &maskrow, h);
  	    /*apply_mask_to_alpha_channel(s, m, opacity, src-> w, srcPR->bytes);*/
	    apply_mask_to_alpha_channel_row (&srcrow, &maskrow, opacity);
	}
    }
}


void 
combine_mask_and_area  (
                        PixelArea * src_area,
                        PixelArea * mask_area,
                        Paint * opacity
                        )
{
  void *  pag;
  Tag src_tag = pixelarea_tag (src_area); 
  Tag mask_tag = pixelarea_tag (mask_area); 

   /*put in tags check */
  
  for (pag = pixelarea_register (2, src_area, mask_area);
       pag != NULL;
       pag = pixelarea_process (pag))
    {
      PixelRow srcrow;
      PixelRow maskrow;
      int h;
      
      h = pixelarea_height (src_area);
      while (h--)
	{
	  pixelarea_getdata (src_area, &srcrow, h);
	  pixelarea_getdata (mask_area, &maskrow, h);
  	  /*combine_mask_and_alpha_channel (s, m, opacity, src->w,src->bytes);*/
	  combine_mask_and_alpha_channel_row (&srcrow, &maskrow, opacity);
	}
    }
}


void 
copy_gray_to_area  (
                    PixelArea * src_area,
                    PixelArea * dest_area
                    )
{
  void *  pag;
  Tag src_tag = pixelarea_tag (src_area); 
  Tag dest_tag = pixelarea_tag (dest_area); 

   /*put in tags check*/
  
  for (pag = pixelarea_register (2, src_area, dest_area);
       pag != NULL;
       pag = pixelarea_process (pag))
    {
      PixelRow src_row;
      PixelRow dest_row;
 
      gint h = pixelarea_height (src_area);
      while (h--)
        {
          pixelarea_getdata (src_area, &src_row, h);
          pixelarea_getdata (dest_area, &dest_row, h);
            
  	  /*copy_gray_to_inten_a_pixels (s, d, src->w, dest->bytes);*/
          copy_gray_to_inten_a_row (&src_row, &dest_row);
        }
    }
}


void 
initial_area  (
               PixelArea * src_area,
               PixelArea * dest_area,
               PixelArea * mask_area,
               unsigned char * data,    /*data is a cmap or color if needed*/
               Paint * opacity,
               gint mode,
               gint * affect,
               gint type
               )
{
  void *  pag;
  gint buf_size;
  PixelRow buf_row;
  guchar *buf_row_data = 0; 
  Tag src_tag = pixelarea_tag (src_area); 
  Tag dest_tag = pixelarea_tag (dest_area); 
  Tag mask_tag = pixelarea_tag (mask_area); 
  gint src_width = pixelarea_width (src_area);
  gint src_num_channels = tag_num_channels (src_tag);
  gint src_bytes = tag_bytes (src_tag); /* per pixel */
  gint src_bytes_per_channel = src_bytes / src_num_channels;
  Precision prec = tag_precision (src_tag); 
   
  /*put in tags check*/
  
  /* get a buffer for dissolve if needed */
  
  if ( (type == INITIAL_INTENSITY && mode == DISSOLVE_MODE) ||
       (type == INITIAL_INTENSITY_ALPHA && mode == DISSOLVE_MODE) ) 
  {	 
    Tag buf_tag = tag_new (prec, tag_format (src_tag), ALPHA_YES);
    
    if ( tag_alpha(src_tag) == ALPHA_YES )
      buf_size = src_width * src_bytes;
    else
      buf_size = src_width * (src_bytes + src_bytes_per_channel);
    
    /* allocate the buf_rows data */  
    buf_row_data = (guchar *) g_malloc (buf_size);
    pixelrow_init (&buf_row, buf_tag , buf_row_data, src_width); 
  } 
  
  for (pag = pixelarea_register (3, src_area, dest_area, mask_area);
       pag != NULL;
       pag = pixelarea_process (pag))
    {
      PixelRow src_row;
      PixelRow dest_row;
      PixelRow mask_row;
 
      gint h = pixelarea_height (src_area);
      gint src_x = pixelarea_x (src_area);
      gint src_y = pixelarea_y (src_area);
      while (h--)
        {
          pixelarea_getdata (src_area, &src_row, h);
          pixelarea_getdata (dest_area, &dest_row, h);
          pixelarea_getdata (mask_area, &mask_row, h);
            
	  switch (type)
	    {
	    case INITIAL_CHANNEL_MASK:
	    case INITIAL_CHANNEL_SELECTION:
  	      /*initial_channel_pixels (s, d, src->w, dest->bytes);*/
              initial_channel_row (&src_row, &dest_row);
	      break;

	    case INITIAL_INDEXED:
  	      /*initial_indexed_pixels (s, d, data, src->w);*/
              initial_indexed_row (&src_row, &dest_row, data);
	      break;

	    case INITIAL_INDEXED_ALPHA:
  	      /*initial_indexed_a_pixels (s, d, m, data, opacity, src->w);*/
              initial_indexed_a_row (&src_row, &dest_row, &mask_row, data, opacity);
	      break;

	    case INITIAL_INTENSITY:
	      if (mode == DISSOLVE_MODE)
		{
  		  /*dissolve_pixels (s, buf, src->x, src->y + h, opacity, src->w, src->bytes, src->bytes + 1, 0);*/
  		  /*initial_inten_pixels (buf, d, m, opacity, affect, src->w,  src->bytes);*/
 		  dissolve_row (&src_row, &buf_row, src_x, src_y + h, opacity);
                  initial_inten_row (&buf_row, &dest_row, &mask_row, opacity, affect);
		}
	      else
		/*initial_inten_pixels (s, d, m, opacity, affect, src->w, pixel_region_bytes (src));*/
                initial_inten_row (&src_row, &dest_row, &mask_row, opacity, affect);
	      break;

	    case INITIAL_INTENSITY_ALPHA:
	      if (mode == DISSOLVE_MODE)
		{
		  /* dissolve_pixels (s, buf, src->x, src->y + h, opacity, src->w, src->bytes, src->bytes, 1);*/
		  /* initial_inten_a_pixels (buf, d, m, opacity, affect, src->w, src->bytes);*/
                  dissolve_row (&src_row, &buf_row, src_x, src_y + h, opacity);
                  initial_inten_a_row (&buf_row, &dest_row, &mask_row, opacity, affect);
		}
	      else
		/*initial_inten_a_pixels (s, d, m, opacity, affect, src->w, src->bytes);*/
                initial_inten_a_row (&src_row, &dest_row, &mask_row, opacity, affect);
	      break;
	    }
        }
    } 
    if (buf_row_data)
      g_free (buf_row_data);
}


void 
combine_areas  (
                PixelArea * src1_area,
                PixelArea * src2_area,
                PixelArea * dest_area,
                PixelArea * mask_area,
                unsigned char * data,   /* a colormap or a color --if needed */
                Paint * opacity,
                gint mode,
                gint * affect,
                gint type
                )
{
  void *  pag;
  gint combine;
  gint mode_affect;
  PixelRow buf_row;
  gint buf_size;
  guchar *buf_row_data = 0; 
  Tag src1_tag = pixelarea_tag (src1_area); 
  Tag src2_tag = pixelarea_tag (src2_area); 
  Tag dest_tag = pixelarea_tag (dest_area); 
  Tag mask_tag = pixelarea_tag (mask_area); 
  gint src1_width = pixelarea_width (src1_area);
  gint src1_num_channels = tag_num_channels (src1_tag);
  gint src1_bytes = tag_bytes (src1_tag);
  gint src1_bytes_per_channel = src1_bytes / src1_num_channels;
  Precision prec = tag_precision (src1_tag); 

  
  /*put in tags check*/
  
  Tag buf_tag = tag_new (prec, tag_format (src1_tag), ALPHA_YES);
  
  if ( tag_alpha(src1_tag) == ALPHA_YES )
    buf_size = src1_width * src1_bytes;
  else
    buf_size = src1_width * (src1_bytes + src1_bytes_per_channel);
  
  /* allocate the buf_rows data */  
  buf_row_data = (guchar *) g_malloc (buf_size);
  pixelrow_init (&buf_row, buf_tag , buf_row_data, src1_width); 
  
  for (pag = pixelarea_register (4, src1_area, src2_area, dest_area, mask_area);
       pag != NULL;
       pag = pixelarea_process (pag))
    {
      PixelRow src1_row;
      PixelRow src2_row;
      PixelRow dest_row;
      PixelRow mask_row;
 
      gint h = pixelarea_height (src1_area);
      gint src1_x = pixelarea_x (src1_area);
      gint src1_y = pixelarea_y (src1_area);
      while (h--)
        {
          pixelarea_getdata (src1_area, &src1_row, h);
          pixelarea_getdata (src2_area, &src2_row, h);
          pixelarea_getdata (dest_area, &dest_row, h);
          pixelarea_getdata (mask_area, &mask_row, h);
           
	  /*  apply the paint mode based on the combination type & mode  */
	  switch (type)
	    {
	    case COMBINE_INTEN_A_INDEXED_A:
	    case COMBINE_INTEN_A_CHANNEL_MASK:
	    case COMBINE_INTEN_A_CHANNEL_SELECTION:
	      combine = type;
	      break;

	    case COMBINE_INDEXED_INDEXED:
	    case COMBINE_INDEXED_INDEXED_A:
	    case COMBINE_INDEXED_A_INDEXED_A:
	      /*  Now, apply the paint mode--for indexed images  */
	      /*combine = apply_indexed_layer_mode (s1, s2, &s, mode, ha1, ha2);*/
	      combine = apply_indexed_layer_mode (&src1_row, &src2_row, &buf_row, mode);
	      break;

	    case COMBINE_INTEN_INTEN_A:
	    case COMBINE_INTEN_A_INTEN:
	    case COMBINE_INTEN_INTEN:
	    case COMBINE_INTEN_A_INTEN_A:
	      /*  Now, apply the paint mode  */
	      /*combine = apply_layer_mode (s1, s2, &s, src1->x, src1->y + h, opacity, src1->w, mode, src1->bytes, src2->bytes, &mode_affect);*/
	      combine = apply_layer_mode (&src1_row, &src2_row, &buf_row, src1_x, src1_y+ h, opacity, mode, &mode_affect);
	      break;

	    default:
	      break;
	    }
	  
          /*  based on the type of the initial image...  */
	  switch (combine)
	    {
	    case COMBINE_INDEXED_INDEXED:
	      /*combine_indexed_and_indexed_pixels (s1, s2, d, m, opacity, affect, src1->w, src1->bytes);*/
              combine_indexed_and_indexed_row (&src1_row, &src2_row, &dest_row, &mask_row, opacity, affect);
	      break;

	    case COMBINE_INDEXED_INDEXED_A:
	      /*combine_indexed_and_indexed_a_pixels (s1, s2, d, m, opacity, affect, src1->w, src1->bytes);*/
              combine_indexed_and_indexed_a_row (&src1_row, &src2_row, &dest_row, &mask_row, opacity, affect);
	      break;

	    case COMBINE_INDEXED_A_INDEXED_A:
	      /*combine_indexed_a_and_indexed_a_pixels (s1, s2, d, m, opacity, affect, src1->w, src1->bytes);*/
              combine_indexed_a_and_indexed_a_row (&src1_row, &src2_row, &dest_row, &mask_row, opacity, affect);
	      break;

	    case COMBINE_INTEN_A_INDEXED_A:
	      /*  assume the data passed to this procedure is the
	       *  indexed layer's colormap
	       */
	      /*combine_inten_a_and_indexed_a_pixels (s1, s2, d, m, data, opacity, src1->w, dest->bytes);*/
              combine_inten_a_and_indexed_a_row (&src1_row, &src2_row, &dest_row, &mask_row, data, opacity );
	      break;

	    case COMBINE_INTEN_A_CHANNEL_MASK:
	      /*  assume the data passed to this procedure is the channels color
	       * 
	       */
              /*combine_inten_a_and_channel_mask_pixels (s1, s2, d, data, opacity, src1->w, dest->bytes);*/
/* FIXME -- the data argument should be a Paint to pass to this routine */                
              combine_inten_a_and_channel_mask_row (&src1_row, &src2_row, &dest_row, NULL, opacity );
	      break;

	    case COMBINE_INTEN_A_CHANNEL_SELECTION:
	      /*combine_inten_a_and_channel_selection_pixels (s1, s2, d, data, opacity, src1->w, src1->bytes);*/
/* FIXME -- the data argument should be a Paint to pass to this routine */                
              combine_inten_a_and_channel_selection_row (&src1_row, &src2_row, &dest_row, NULL, opacity );
	      break;

	    case COMBINE_INTEN_INTEN:
	      /*combine_inten_and_inten_pixels (s1, s, d, m, opacity, affect, src1->w, src1->bytes);*/
              combine_inten_and_inten_row (&src1_row, &buf_row, &dest_row, &mask_row, opacity, affect);
	      break;

	    case COMBINE_INTEN_INTEN_A:
	      /*combine_inten_and_inten_a_pixels (s1, s, d, m, opacity, affect, src1->w, src1->bytes);*/
              combine_inten_and_inten_a_row (&src1_row, &buf_row, &dest_row, &mask_row, opacity, affect);
	      break;

	    case COMBINE_INTEN_A_INTEN:
              /*combine_inten_a_and_inten_pixels (s1, s, d, m, opacity, affect, mode_affect, src1->w, src1->bytes);*/
              combine_inten_a_and_inten_row (&src1_row, &buf_row, &dest_row, &mask_row, opacity, affect, mode_affect);
	      break;

	    case COMBINE_INTEN_A_INTEN_A:
	      /*combine_inten_a_and_inten_a_pixels (s1, s, d, m, opacity, affect, mode_affect, src1->w, src1->bytes);*/
              combine_inten_a_and_inten_a_row (&src1_row, &buf_row, &dest_row, &mask_row, opacity, affect, mode_affect);
	      break;

	    case BEHIND_INTEN:
	      /*behind_inten_pixels (s1, s, d, m, opacity, affect, src1->w, src1->bytes, src2->bytes, ha1, ha2);*/
              behind_inten_row (&src1_row, &buf_row, &dest_row, &mask_row, opacity, affect);
	      break;

	    case BEHIND_INDEXED:
	      /*behind_indexed_pixels (s1, s, d, m, opacity, affect, src1->w, src1->bytes, src2->bytes, ha1, ha2);*/
              behind_inten_row (&src1_row, &buf_row, &dest_row, &mask_row, opacity, affect);
	      break;

	    case REPLACE_INTEN:
              /*replace_inten_pixels (s1, s, d, m, opacity, affect, src1->w, src1->bytes, src2->bytes, ha1, ha2);*/
              replace_inten_row (&src1_row, &buf_row, &dest_row, &mask_row, opacity, affect);
	      break;

	    case REPLACE_INDEXED:
              /*replace_indexed_pixels (s1, s, d, m, opacity, affect, src1->w, src1->bytes, src2->bytes, ha1, ha2);*/
              replace_indexed_row (&src1_row, &buf_row, &dest_row, &mask_row, opacity, affect);
	      break;

	    case ERASE_INTEN:
	      /*erase_inten_pixels (s1, s, d, m, opacity, affect, src1->w, src1->bytes);*/
              erase_inten_row (&src1_row, &buf_row, &dest_row, &mask_row, opacity, affect);
	      break;

	    case ERASE_INDEXED:
              /*erase_indexed_pixels (s1, s, d, m, opacity, affect, src1->w, src1->bytes);*/
              erase_indexed_row (&src1_row, &buf_row, &dest_row, &mask_row, opacity, affect);
	      break;

	    case NO_COMBINATION:
	      break;

	    default:
	      break;
	    }
        }
    } 
    if (buf_row_data)
      g_free (buf_row_data);
}


void 
combine_areas_replace  (
                        PixelArea *src1_area,
                        PixelArea *src2_area,
                        PixelArea *dest_area,
                        PixelArea *mask_area,
                        guchar    *data,
                        Paint     *opacity,
                        gint       *affect,
                        gint        type
                        )
{
  void *  pag;
  Tag src1_tag = pixelarea_tag (src1_area); 
  Tag src2_tag = pixelarea_tag (src2_area); 
  Tag dest_tag = pixelarea_tag (dest_area); 
  Tag mask_tag = pixelarea_tag (mask_area); 
  
   /*put in tags check*/
  
  for (pag = pixelarea_register (4, src1_area, src2_area, dest_area, mask_area);
       pag != NULL;
       pag = pixelarea_process (pag))
    {
      PixelRow src1_row;
      PixelRow src2_row;
      PixelRow dest_row;
      PixelRow mask_row;
 
      gint h = pixelarea_height (src1_area);
      while (h--)
        {
         /*apply_layer_mode_replace (s1, s2, d, m, src1->x, src1->y + h, opacity, src1->w, src1->bytes, src2->bytes, affect);*/
         apply_layer_mode_replace (&src1_row, &src2_row, &dest_row, &mask_row, opacity, affect);
	}
    }
}


/*========================================================================*/

/*
 * The equations: g(r) = exp (- r^2 / (2 * sigma^2))
 *                   r = sqrt (x^2 + y ^2)
 *    return curve is mapping of [-length,length]-->[0.0,1.0]
 */

static gdouble *
make_curve (
	    gdouble  sigma,
	    gint      *length,
	    gdouble  precision_cutoff
	    )
{
  gdouble *curve;
  gdouble sigma2;
  gdouble l;
  gdouble temp;
  int i, n;

  sigma2 = 2 * sigma * sigma;
  l = sqrt (-sigma2 * log (precision_cutoff));

  n = ceil (l) * 2;
  if ((n % 2) == 0)
    n += 1;

  curve = g_malloc (sizeof (gdouble) * n);

  *length = n / 2;
  curve += *length;
  curve[0] = 1.0;

  for (i = 1; i <= *length; i++)
    {
      temp = (gdouble) exp (- (i * i) / sigma2); 
      curve[-i] = temp;
      curve[i] = temp;
    }
  return curve;
}

static void
draw_segments (
	       PixelArea   *dest_area,
	       BoundSeg    *bs,
	       gint          num_segs,
	       gint          off_x,
	       gint          off_y,
	       Paint       *opacity
               )
{
  gint x1, y1, x2, y2;
  gint tmp, i, length;
  PixelRow line;
  guchar *line_data;
  Tag dest_tag = pixelarea_tag (dest_area);
  gint width = pixelarea_width (dest_area);
  gint height = pixelarea_height (dest_area);
  gint bytes_per_pixel = tag_bytes (dest_tag); 
  
  length = MAXIMUM (width , height);

  /* get a row with same tag type as dest */
  line_data = (guchar *) g_malloc (length * bytes_per_pixel); 
  pixelrow_init (&line, dest_tag, line_data, length); 
  
  /* initialize line with opacity */
  color_row(&line, opacity); 

  for (i = 0; i < num_segs; i++)
    {
      x1 = bs[i].x1 + off_x;
      y1 = bs[i].y1 + off_y;
      x2 = bs[i].x2 + off_x;
      y2 = bs[i].y2 + off_y;

      if (bs[i].open == 0)
	{
	  /*  If it is vertical  */
	  if (x1 == x2)
	    {
	      x1 -= 1;
	      x2 -= 1;
	    }
	  else
	    {
	      y1 -= 1;
	      y2 -= 1;
	    }
	}

      /*  render segment  */
      x1 = BOUNDS (x1, 0, width - 1);
      y1 = BOUNDS (y1, 0, height - 1);
      x2 = BOUNDS (x2, 0, width - 1);
      y2 = BOUNDS (y2, 0, height - 1);

      if (x1 == x2)
	{
	  if (y2 < y1)
	    {
	      tmp = y1;
	      y1 = y2;
	      y2 = tmp;
	    }
	  /*pixel_region_set_col (destPR, x1, y1, (y2 - y1), line);*/
          pixelarea_write_col (dest_area, &line, x1, y1, (y2 - y1)); 
	}
      else
	{
	  if (x2 < x1)
	    {
	      tmp = x1;
	      x1 = x2;
	      x2 = tmp;
	    }
	  /*pixel_region_set_row (destPR, x1, y1, (x2 - x1), line);*/
          pixelarea_write_row (dest_area, &line, x1, y1, (y2 - y1)); 
	}
    }
}

static gdouble
cubic (
       gdouble  dx,
       gdouble  jm1,
       gdouble  j,
       gdouble  jp1,
       gdouble  jp2,
       gdouble  hi,    /* the hi code for data type */
       gdouble  lo     /* the lo code for data type */
       )
{
  double dx1, dx2, dx3;
  double h1, h2, h3, h4;
  double result;

  /*  constraint parameter = -1  */
  dx1 = fabs (dx);
  dx2 = dx1 * dx1;
  dx3 = dx2 * dx1;
  h1 = dx3 - 2 * dx2 + 1;
  result = h1 * j;

  dx1 = fabs (dx - 1.0);
  dx2 = dx1 * dx1;
  dx3 = dx2 * dx1;
  h2 = dx3 - 2 * dx2 + 1;
  result += h2 * jp1;

  dx1 = fabs (dx - 2.0);
  dx2 = dx1 * dx1;
  dx3 = dx2 * dx1;
  h3 = -dx3 + 5 * dx2 - 8 * dx1 + 4;
  result += h3 * jp2;

  dx1 = fabs (dx + 1.0);
  dx2 = dx1 * dx1;
  dx3 = dx2 * dx1;
  h4 = -dx3 + 5 * dx2 - 8 * dx1 + 4;
  result += h4 * jm1;

  if (result < lo)
    result = lo;
  if (result > hi)
    result = hi;

  return result;
}


/************************************/
/*       apply layer modes          */
/************************************/

static int 
apply_layer_mode  (
                   PixelRow *src1_row,
                   PixelRow *src2_row,
                   PixelRow *dest_row,
                   gint x,
                   gint y,
                   Paint *opacity,
                   gint mode,
                   gint * mode_affect
                   )
{
  gint combine;
  Tag src1_tag = pixelrow_tag (src1_row);
  Tag src2_tag = pixelrow_tag (src2_row);
  Tag dest_tag = pixelrow_tag (dest_row);
  gint ha1  = (tag_alpha (src1_tag)==ALPHA_YES)? TRUE: FALSE;
  gint ha2  = (tag_alpha (src2_tag)==ALPHA_YES)? TRUE: FALSE;
  guchar *src2_data = pixelrow_data (src2_row); 
  gint width = pixelrow_width (dest_row); 
  Format src1_format = tag_format (src1_tag);

  if (!ha1 && !ha2)
    combine = COMBINE_INTEN_INTEN;
  else if (!ha1 && ha2)
    combine = COMBINE_INTEN_INTEN_A;
  else if (ha1 && !ha2)
    combine = COMBINE_INTEN_A_INTEN;
  else
    combine = COMBINE_INTEN_A_INTEN_A;

  /*  assumes we're applying src2 TO src1  */
  switch (mode)
    {
    case NORMAL_MODE:
      /* *dest = src2; */
      pixelrow_init (dest_row, dest_tag, src2_data, width);
      break;

    case DISSOLVE_MODE:
      /*  Since dissolve requires an alpha channels...  */
      if (! ha2)
	/*add_alpha_pixels (src2, *dest, length, b2);*/
      add_alpha_row (src2_row, dest_row);

      /*dissolve_pixels (src2, *dest, x, y, opacity, length, b2, ((ha2) ? b2 : b2 + 1), ha2);*/
      dissolve_row (src2_row, dest_row, x, y, opacity);
      combine = (ha1) ? COMBINE_INTEN_A_INTEN_A : COMBINE_INTEN_INTEN_A;
      break;

    case MULTIPLY_MODE:
      /*multiply_pixels (src1, src2, *dest, length, b1, b2, ha1, ha2);*/
      multiply_row (src1_row, src2_row, dest_row);
      break;

    case SCREEN_MODE:
      /*screen_pixels (src1, src2, *dest, length, b1, b2, ha1, ha2);*/
      screen_row (src1_row, src2_row, dest_row);
      break;

    case OVERLAY_MODE:
      /*overlay_pixels (src1, src2, *dest, length, b1, b2, ha1, ha2);*/
      overlay_row (src1_row, src2_row, dest_row);
      break;

    case DIFFERENCE_MODE:
      /*difference_pixels (src1, src2, *dest, length, b1, b2, ha1, ha2);*/
      difference_row (src1_row, src2_row, dest_row);
      break;

    case ADDITION_MODE:
      /*add_pixels (src1, src2, *dest, length, b1, b2, ha1, ha2);*/
      add_row (src1_row, src2_row, dest_row);
      break;

    case SUBTRACT_MODE:
      /*subtract_pixels (src1, src2, *dest, length, b1, b2, ha1, ha2);*/
      subtract_row (src1_row, src2_row, dest_row);
      break;

    case DARKEN_ONLY_MODE:
      /*darken_pixels (src1, src2, *dest, length, b1, b2, ha1, ha2);*/
      darken_row (src1_row, src2_row, dest_row);
      break;

    case LIGHTEN_ONLY_MODE:
      /*lighten_pixels (src1, src2, *dest, length, b1, b2, ha1, ha2);*/
      lighten_row (src1_row, src2_row, dest_row);
      break;

    case HUE_MODE: case SATURATION_MODE: case VALUE_MODE:
      /*  only works on RGB color images  */
      /*if (b1 > 2) */
	/*hsv_only_pixels (src1, src2, *dest, mode, length, b1, b2, ha1, ha2);*/
      if( src1_format == FORMAT_RGB )
        hsv_only_row (src1_row, src2_row, dest_row, mode);
      else
	/* *dest = src2; */
         pixelrow_init (dest_row, dest_tag, src2_data, width);
      break;

    case COLOR_MODE:
      /*  only works on RGB color images  */
      /* if (b1 > 2) */
	 /* color_only_pixels (src1, src2, *dest, mode, length, b1, b2, ha1, ha2); */
      if( src1_format == FORMAT_RGB )
        color_only_row (src1_row, src2_row, dest_row, mode);
      else
         /* *dest = src2; */
         pixelrow_init (dest_row, dest_tag, src2_data, width);
      break;

    case BEHIND_MODE:
      /* *dest = src2; */
      pixelrow_init (dest_row, dest_tag, src2_data, width);
      if (ha1)
	combine = BEHIND_INTEN;
      else
	combine = NO_COMBINATION;
      break;

    case REPLACE_MODE:
      /* *dest = src2; */
      pixelrow_init (dest_row, dest_tag, src2_data, width);
      combine = REPLACE_INTEN;
      break;

    case ERASE_MODE:
      /* *dest = src2; */
      pixelrow_init (dest_row, dest_tag, src2_data, width);
      /*  If both sources have alpha channels, call erase function.
       *  Otherwise, just combine in the normal manner
       */
      combine = (ha1 && ha2) ? ERASE_INTEN : combine;
      break;

    default :
      break;
    }

  /*  Determine whether the alpha channel of the destination can be affected
   *  by the specified mode--This keeps consistency with varying opacities
   */
  *mode_affect = layer_modes_16[mode].affect_alpha;

  return combine;
}


static int 
apply_indexed_layer_mode  (
                           PixelRow * src1_row,
                           PixelRow * src2_row,
                           PixelRow * dest_row,
                           gint mode
                           )
{

  int combine;
  Tag src1_tag = pixelrow_tag (src1_row);
  Tag src2_tag = pixelrow_tag (src2_row);
  Tag dest_tag = pixelrow_tag (dest_row);
  gint ha1  = (tag_alpha (src1_tag)==ALPHA_YES)? TRUE: FALSE;
  gint ha2  = (tag_alpha (src2_tag)==ALPHA_YES)? TRUE: FALSE;
  guchar *src2_data = pixelrow_data (src2_row); 
  gint width = pixelrow_width (dest_row); 

  if (!ha1 && !ha2)
    combine = COMBINE_INDEXED_INDEXED;
  else if (!ha1 && ha2)
    combine = COMBINE_INDEXED_INDEXED_A;
  else if (ha1 && ha2)
    combine = COMBINE_INDEXED_A_INDEXED_A;
  else
    combine = NO_COMBINATION;

  /*  assumes we're applying src2 TO src1  */
  switch (mode)
    {
    case REPLACE_MODE:
      /* make the data of dest point to src2 */
      /* *dest = src2; */
      pixelrow_init (dest_row, dest_tag, src2_data, width);
      combine = REPLACE_INDEXED;
      break;

    case BEHIND_MODE:
      /* make the data of dest point to src2 */
      /* *dest = src2; */
      pixelrow_init (dest_row, dest_tag, src2_data, width);
      if (ha1)
	combine = BEHIND_INDEXED;
      else
	combine = NO_COMBINATION;
      break;

    case ERASE_MODE:
      /* make the data of dest point to src2 */
      /* *dest = src2; */
      pixelrow_init (dest_row, dest_tag, src2_data, width);
      /*  If both sources have alpha channels, call erase function.
       *  Otherwise, just combine in the normal manner
       */
      combine = (ha1 && ha2) ? ERASE_INDEXED : combine;
      break;

    default:
      break;
    }

  return combine;
}

static void 
apply_layer_mode_replace  (
                           PixelRow * src1_row,
                           PixelRow * src2_row,
                           PixelRow * dest_row,
                           PixelRow * mask_row,
                           Paint *opacity,
                           gint * affect
                           )
{
  /*replace_pixels (src1, src2, dest, mask, length, opacity, affect, b1, b2);*/
  replace_row (src1_row, src2_row, dest_row, mask_row, opacity, affect);
}
