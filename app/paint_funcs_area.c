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
#define RANDOM_SEED        314159265
#define RANDOM_TABLE_SIZE  4096

static int random_table [RANDOM_TABLE_SIZE];
static unsigned char * tmp_buffer;  /* temporary buffer available upon request */
static int tmp_buffer_size;

#define pixel_region_bytes(src) 0

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

static int *  make_curve         (double, int *);
static void   run_length_encode  (unsigned char *, int *, int, int);
static void   draw_segments      (PixelArea *, BoundSeg *, int, int, int, int);
static double cubic              (double, int, int, int, int);


static int    apply_layer_mode         (unsigned char *, unsigned char *,
                                        unsigned char **,
                                        int, int, int, int, int, int,
                                        int, int, int, int *);

static int    apply_indexed_layer_mode (unsigned char *, unsigned char *,
                                        unsigned char **, int, int, int);

static void   apply_layer_mode_replace (unsigned char *, unsigned char *,
                                        unsigned char *, unsigned char *,
                                        int, int, int,
                                        int, int, int, int *);



void 
paint_funcs_area_setup  (
                         void
                         )
{

  int i;

  /*  allocate the temporary buffer  */
  tmp_buffer = (unsigned char *) g_malloc (STD_BUF_SIZE);
  tmp_buffer_size = STD_BUF_SIZE;

  /*  generate a table of random seeds  */
  srand (RANDOM_SEED);
  for (i = 0; i < RANDOM_TABLE_SIZE; i++)
    random_table[i] = rand ();
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
  srand (random_table [y % RANDOM_TABLE_SIZE]);
}



/**************************************************/
/*    REGION FUNCTIONS                            */
/**************************************************/

void 
color_area  (
             PixelArea * src,
             Paint * color
             )
{
  void *  pag;

  for (pag = pixelarea_register (1, src);
       pag != NULL;
       pag = pixelarea_process (pag))
    {
      PixelRow row;
      int h = pixelarea_height (src);
      while (h--)
        {
          pixelarea_getdata (src, &row, h);
          color_row (&row, color);
        }
    }
}


void 
blend_area  (
             PixelArea * src1,
             PixelArea * src2,
             PixelArea * dest,
             gfloat blend
             )
{
#if 0
  int h;
  unsigned char * s1, * s2, * d;

  s1 = src1->data;
  s2 = src2->data;
  d = dest->data;
  h = src1->h;

  while (h --)
    {
/*      blend_pixels (s1, s2, d, blend, src1->w, src1->bytes);*/
      s1 += src1->rowstride;
      s2 += src2->rowstride;
      d += dest->rowstride;
    }
#endif
}


void 
shade_area  (
             PixelArea * src,
             PixelArea * dest,
             Paint * col,
             gfloat blend
             )
{
#if 0
  int h;
  unsigned char * s, * d;

  s = src->data;
  d = dest->data;
  h = src->h;

  while (h --)
    {
/*      blend_pixels (s, d, col, blend, src->w, src->bytes);*/
      s += src->rowstride;
      d += dest->rowstride;
    }
#endif
}


void 
copy_area  (
            PixelArea * src,
            PixelArea * dest
            )
{
#if 0
  int h;
  int pixelwidth;
  unsigned char * s, * d;
  void * pr;

  for (pr = pixel_regions_register (2, src, dest); pr != NULL; pr = pixel_regions_process (pr))
    {
      pixelwidth = src->w * pixel_region_bytes (src);
      s = src->data;
      d = dest->data;
      h = src->h;

      while (h --)
	{
	  memcpy (d, s, pixelwidth);
	  s += src->rowstride;
	  d += dest->rowstride;
	}
    }
#endif
}


void 
add_alpha_area  (
                 PixelArea * src,
                 PixelArea * dest
                 )
{
#if 0
  int h;
  unsigned char * s, * d;
  void * pr;
  int bytes;

  bytes = pixel_region_bytes (src);
  for (pr = pixel_regions_register (2, src, dest); pr != NULL; pr = pixel_regions_process (pr))
    {
      s = src->data;
      d = dest->data;
      h = src->h;

      while (h --)
	{
	  add_alpha_pixels (s, d, src->w, bytes);
	  s += src->rowstride;
	  d += dest->rowstride;
	}
    }
#endif
}


void 
flatten_area  (
               PixelArea * src,
               PixelArea * dest,
               Paint * bg
               )
{
#if 0
  int h;
  unsigned char * s, * d;
  int bytes;

  bytes = pixel_region_bytes (src);

  s = src->data;
  d = dest->data;
  h = src->h;

  while (h --)
    {
      flatten_pixels (s, d, bg, src->w, bytes);
      s += src->rowstride;
      d += dest->rowstride;
    }
#endif
}


void 
extract_alpha_area  (
                     PixelArea * src,
                     PixelArea * mask,
                     PixelArea * dest
                     )
{
#if 0
  int h;
  unsigned char * s, * m, * d;
  void * pr;
  int bytes;

  bytes = pixel_region_bytes (src);

  for (pr = pixel_regions_register (3, src, mask, dest); pr != NULL; pr = pixel_regions_process (pr))
    {
      s = src->data;
      d = dest->data;
      if (mask)
	m = mask->data;
      else
	m = NULL;

      h = src->h;
      while (h --)
	{
	  extract_alpha_pixels (s, m, d, src->w, bytes);
	  s += src->rowstride;
	  d += dest->rowstride;
	  if (mask)
	    m += mask->rowstride;
	}
    }
#endif
}


void 
extract_from_area  (
                    PixelArea * src,
                    PixelArea * dest,
                    PixelArea * mask,
                    Paint *bg,
                    int cut
                    )
{
#if 0
  int h;
  unsigned char * s, * d, * m;
  void * pr;
  int bytes;

  bytes = pixel_region_bytes (src);

  for (pr = pixel_regions_register (3, src, dest, mask); pr != NULL; pr = pixel_regions_process (pr))
    {
      s = src->data;
      d = dest->data;
      m = (mask) ? mask->data : NULL;
      h = src->h;

      while (h --)
	{
	  switch (format)
	    {
            case FORMAT_NONE:
              break;
	    case FORMAT_RGB:
	    case FORMAT_GRAY:
	      extract_from_inten_pixels (s, d, m, bg, cut, src->w,
					 bytes, has_alpha);
	      break;
	    case FORMAT_INDEXED:
	      extract_from_indexed_pixels (s, d, m, cmap, bg, cut, src->w,
					   bytes, has_alpha);
	      break;
	    }

	  s += src->rowstride;
	  d += dest->rowstride;
	  if (mask)
	    m += mask->rowstride;
	}
    }
#endif
}


void 
convolve_area  (
                PixelArea * srcR,
                PixelArea * destR,
                int * matrix,
                int size,
                int divisor,
                int mode
                )
{
#if 0
  /*  Convolve the src image using the convolution matrix, writing to dest  */
  /*  Convolve is not tile-enabled--use accordingly  */
  unsigned char *src, *s_row, * s;
  unsigned char *dest, * d;
  int * m;
  int total [4];
  int b, bytes;
  int length;
  int wraparound;
  int margin;      /*  margin imposed by size of conv. matrix  */
  int i, j;
  int x, y;
  int offset;

  /*  If the mode is NEGATIVE, the offset should be 128  */
  if (mode == NEGATIVE)
    {
      offset = 128;
      mode = 0;
    }
  else
    offset = 0;

  /*  check for the boundary cases  */
  if (srcR->w < (size - 1) || srcR->h < (size - 1))
    return;

  /*  Initialize some values  */
  bytes = pixel_region_bytes (srcR);
  length = bytes * srcR->w;
  margin = size / 2;
  src = srcR->data;
  dest = destR->data;

  /*  calculate the source wraparound value  */
  wraparound = srcR->rowstride - size * bytes;

  /* copy the first (size / 2) scanlines of the src image... */
  for (i = 0; i < margin; i++)
    {
      memcpy (dest, src, length);
      src += srcR->rowstride;
      dest += destR->rowstride;
    }

  src = srcR->data;

  for (y = margin; y < srcR->h - margin; y++)
    {
      s_row = src;
      s = s_row + srcR->rowstride*margin;
      d = dest;

      /* handle the first margin pixels... */
      b = bytes * margin;
      while (b --)
	*d++ = *s++;

      /* now, handle the center pixels */
      x = srcR->w - margin*2;
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
		  for (b = 0; b < bytes; b++)
		    total [b] += *m * *s++;
		  m ++;
		}

	      s += wraparound;
	    }

	  for (b = 0; b < bytes; b++)
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

	  s_row += bytes;

	}

      /* handle the last pixel... */
      s = s_row + (srcR->rowstride + bytes) * margin;
      b = bytes * margin;
      while (b --)
	*d++ = *s++;

      /* set the memory pointers */
      src += srcR->rowstride;
      dest += destR->rowstride;
    }

  src += srcR->rowstride*margin;

  /* copy the last (margin) scanlines of the src image... */
  for (i = 0; i < margin; i++)
    {
      memcpy (dest, src, length);
      src += srcR->rowstride;
      dest += destR->rowstride;
    }
#endif
}

/* Convert from separated alpha to premultiplied alpha. Only works on
   non-tiled regions! */
void 
multiply_alpha_area  (
                      PixelArea * srcR
                      )
{
#if 0
  unsigned char *src, *s;
  int x, y;
  int width, height;
  int b, bytes;
  double alpha_val;

  width = srcR->w;
  height = srcR->h;
  bytes = pixel_region_bytes (srcR);

  src = srcR->data;

  for (y = 0; y < height; y++)
    {
      s = src;
      for (x = 0; x < width; x++)
	{
	  alpha_val = s[bytes - 1] * (1.0 / 255.0);
	  for (b = 0; b < bytes - 1; b++)
	    s[b] = 0.5 + s[b] * alpha_val;
	  s += bytes;
	}
      src += srcR->rowstride;
    }
#endif
}

/* Convert from premultiplied alpha to separated alpha. Only works on
   non-tiled regions! */
void 
separate_alpha_area  (
                      PixelArea * srcR
                      )
{
#if 0
  unsigned char *src, *s;
  int x, y;
  int width, height;
  int b, bytes;
  double alpha_recip;
  int new_val;

  width = srcR->w;
  height = srcR->h;
  bytes = pixel_region_bytes (srcR);

  src = srcR->data;

  for (y = 0; y < height; y++)
    {
      s = src;
      for (x = 0; x < width; x++)
	{
	  /* predicate is equivalent to:
	     (((s[bytes - 1] - 1) & 255) + 2) & 256
	     */
	  if (s[bytes - 1] != 0 && s[bytes - 1] != 255)
	    {
	      alpha_recip = 255.0 / s[bytes - 1];
	      for (b = 0; b < bytes - 1; b++)
		{
		  new_val = 0.5 + s[b] * alpha_recip;
		  new_val = MIN (new_val, 255);
		  s[b] = new_val;
		}
	    }
	  s += bytes;
	}
      src += srcR->rowstride;
    }
#endif
}

void 
gaussian_blur_area  (
                     PixelArea * srcR,
                     double radius
                     )
{
#if 0
  double std_dev;
  long width, height;
  int bytes;
  unsigned char *src, *sp;
  unsigned char *dest, *dp;
  unsigned char *data;
  int *buf, *b;
  int pixels;
  int total;
  int i, row, col;
  int start, end;
  int *curve;
  int *sum;
  int val;
  int length;
  int alpha;
  int initial_p, initial_m;

  if (radius == 0.0) return;		/* zero blur is a no-op */

  /*  allocate the result buffer  */
  length = MAX (srcR->w, srcR->h) * pixel_region_bytes (srcR);
  data = paint_funcs_get_buffer (length * 2);
  src = data;
  dest = data + length;

  std_dev = sqrt (-(radius * radius) / (2 * log (1.0 / 255.0)));

  curve = make_curve (std_dev, &length);
  sum = g_malloc (sizeof (int) * (2 * length + 1));

  sum[0] = 0;

  for (i = 1; i <= length*2; i++)
    sum[i] = curve[i-length-1] + sum[i-1];
  sum += length;

  width = srcR->w;
  height = srcR->h;
  bytes = pixel_region_bytes (srcR);
  alpha = bytes - 1;

  buf = g_malloc (sizeof (int) * MAX (width, height) * 2);

  total = sum[length] - sum[-length];

  for (col = 0; col < width; col++)
    {
      pixel_region_get_col (srcR, col + srcR->x, srcR->y, height, src, 1);
      sp = src + alpha;

      initial_p = sp[0];
      initial_m = sp[(height-1) * bytes];

      /*  Determine a run-length encoded version of the column  */
      run_length_encode (sp, buf, height, bytes);

      for (row = 0; row < height; row++)
	{
	  start = (row < length) ? -row : -length;
	  end = (height <= (row + length)) ? (height - row - 1) : length;

	  val = 0;
	  i = start;
	  b = buf + (row + i) * 2;

	  if (start != -length)
	    val += initial_p * (sum[start] - sum[-length]);

	  while (i < end)
	    {
	      pixels = b[0];
	      i += pixels;
	      if (i > end)
		i = end;
	      val += b[1] * (sum[i] - sum[start]);
	      b += (pixels * 2);
	      start = i;
	    }

	  if (end != length)
	    val += initial_m * (sum[length] - sum[end]);

	  sp[row * bytes] = val / total;
	}

      pixel_region_set_col (srcR, col + srcR->x, srcR->y, height, src);
    }

  for (row = 0; row < height; row++)
    {
      pixel_region_get_row (srcR, srcR->x, row + srcR->y, width, src, 1);
      sp = src + alpha;
      dp = dest + alpha;

      initial_p = sp[0];
      initial_m = sp[(width-1) * bytes];

      /*  Determine a run-length encoded version of the row  */
      run_length_encode (sp, buf, width, bytes);

      for (col = 0; col < width; col++)
	{
	  start = (col < length) ? -col : -length;
	  end = (width <= (col + length)) ? (width - col - 1) : length;

	  val = 0;
	  i = start;
	  b = buf + (col + i) * 2;

	  if (start != -length)
	    val += initial_p * (sum[start] - sum[-length]);

	  while (i < end)
	    {
	      pixels = b[0];
	      i += pixels;
	      if (i > end)
		i = end;
	      val += b[1] * (sum[i] - sum[start]);
	      b += (pixels * 2);
	      start = i;
	    }

	  if (end != length)
	    val += initial_m * (sum[length] - sum[end]);

	  val = val / total;

	  dp[col * bytes] = val;
	}
      
      pixel_region_set_row (srcR, srcR->x, row + srcR->y, width, dest);
    }

  g_free (buf);
  g_free (sum - length);
  g_free (curve - length);
#endif
}


void 
border_area  (
              PixelArea * destPR,
              void * bs_ptr,
              int bs_segs,
              int radius
              )
{
#if 0
  BoundSeg * bs;
  unsigned char opacity;
  int r, i, j;

  bs = (BoundSeg *) bs_ptr;

  /*  draw the border  */
  for (r = 0; r <= radius; r++)
    {
      opacity = 255 * (r + 1) / (radius + 1);
      j = radius - r;

      for (i = 0; i <= j; i++)
	{
	  /*  northwest  */
	  draw_segments (destPR, bs, bs_segs, -i, -(j - i), opacity);

	  /*  only draw the rest if they are different  */
	  if (j)
	    {
	      /*  northeast  */
	      draw_segments (destPR, bs, bs_segs, i, -(j - i), opacity);
	      /*  southeast  */
	      draw_segments (destPR, bs, bs_segs, i, (j - i), opacity);
	      /*  southwest  */
	      draw_segments (destPR, bs, bs_segs, -i, (j - i), opacity);
	    }
	}
    }
#endif
}


/* non-interpolating scale_region.  [adam]
 */
void 
scale_area_no_resample  (
                         PixelArea * srcPR,
                         PixelArea * destPR
                         )
{
#if 0
  int * x_src_offsets;
  int * y_src_offsets;
  unsigned char * src;
  unsigned char * dest;
  int width, height, orig_width, orig_height;
  int last_src_y;
  int row_bytes;
  int x,y,b;
  char bytes;

  orig_width = srcPR->w;
  orig_height = srcPR->h;

  width = destPR->w;
  height = destPR->h;

  bytes = pixel_region_bytes (srcPR);

  /*  the data pointers...  */
  x_src_offsets = (int *) g_malloc (width * bytes * sizeof(int));
  y_src_offsets = (int *) g_malloc (height * sizeof(int));
  src  = (unsigned char *) g_malloc (orig_width * bytes);
  dest = (unsigned char *) g_malloc (width * bytes);
  
  /*  pre-calc the scale tables  */
  for (b = 0; b < bytes; b++)
    {
      for (x = 0; x < width; x++)
	{
	  x_src_offsets [b + x * bytes] = b + bytes * ((x * orig_width + orig_width / 2) / width);
	}
    }
  for (y = 0; y < height; y++)
    {
      y_src_offsets [y] = (y * orig_height + orig_height / 2) / height;
    }
  
  /*  do the scaling  */
  row_bytes = width * bytes;
  last_src_y = -1;
  for (y = 0; y < height; y++)
    {
      /* if the source of this line was the same as the source
       *  of the last line, there's no point in re-rescaling.
       */
      if (y_src_offsets[y] != last_src_y)
	{
	  pixel_region_get_row (srcPR, 0, y_src_offsets[y], orig_width, src, 1);
	  for (x = 0; x < row_bytes ; x++)
	    {
	      dest[x] = src[x_src_offsets[x]];
	    }
	  last_src_y = y_src_offsets[y];
	}

      pixel_region_set_row (destPR, 0, y, width, dest);
    }
  
  g_free (x_src_offsets);
  g_free (y_src_offsets);
  g_free (src);
  g_free (dest);
#endif
}


void 
scale_area  (
             PixelArea * srcPR,
             PixelArea * destPR
             )
{
#if 0
  unsigned char * src_m1, * src, * src_p1, * src_p2;
  unsigned char * s_m1, * s, * s_p1, * s_p2;
  unsigned char * dest, * d;
  double * row, * r;
  int src_row, src_col;
  int bytes, b;
  int width, height;
  int orig_width, orig_height;
  double x_rat, y_rat;
  double x_cum, y_cum;
  double x_last, y_last;
  double * x_frac, y_frac, tot_frac;
  float dx, dy;
  int i, j;
  int frac;
  int advance_dest_x, advance_dest_y;
  int minus_x, plus_x, plus2_x;
  ScaleType scale_type;

  orig_width = srcPR->w;
  orig_height = srcPR->h;

  width = destPR->w;
  height = destPR->h;

  /*  Some calculations...  */
  bytes = pixel_region_bytes (destPR);

  /*  the data pointers...  */
  src_m1 = (unsigned char *) g_malloc (orig_width * bytes);
  src    = (unsigned char *) g_malloc (orig_width * bytes);
  src_p1 = (unsigned char *) g_malloc (orig_width * bytes);
  src_p2 = (unsigned char *) g_malloc (orig_width * bytes);
  dest   = (unsigned char *) g_malloc (width * bytes);

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
  row    = (double *) g_malloc (sizeof (double) * width * bytes);
  x_frac = (double *) g_malloc (sizeof (double) * (width + orig_width));

  /*  initialize the pre-calculated pixel fraction array  */
  src_col = 0;
  x_cum = (double) src_col;
  x_last = x_cum;

  for (i = 0; i < width + orig_width; i++)
    {
      if (x_cum + x_rat <= (src_col + 1 + EPSILON))
	{
	  x_cum += x_rat;
	  x_frac[i] = x_cum - x_last;
	}
      else
	{
	  src_col ++;
	  x_frac[i] = src_col - x_last;
	}
      x_last += x_frac[i];
    }

  /*  clear the "row" array  */
  memset (row, 0, sizeof (double) * width * bytes);

  /*  counters...  */
  src_row = 0;
  y_cum = (double) src_row;
  y_last = y_cum;

  /*  Get the first src row  */
  pixel_region_get_row (srcPR, 0, src_row, orig_width, src, 1);
  /*  Get the next two if possible  */
  if (src_row < (orig_height - 1))
    pixel_region_get_row (srcPR, 0, (src_row + 1), orig_width, src_p1, 1);
  if ((src_row + 1) < (orig_height - 1))
    pixel_region_get_row (srcPR, 0, (src_row + 2), orig_width, src_p2, 1);

  /*  Scale the selected region  */
  i = height;
  while (i)
    {
      src_col = 0;
      x_cum = (double) src_col;

      /* determine the fraction of the src pixel we are using for y */
      if (y_cum + y_rat <= (src_row + 1 + EPSILON))
	{
	  y_cum += y_rat;
	  dy = y_cum - src_row;
	  y_frac = y_cum - y_last;
	  advance_dest_y = TRUE;
	}
      else
	{
	  y_frac = (src_row + 1) - y_last;
	  dy = 1.0;
	  advance_dest_y = FALSE;
	}

      y_last += y_frac;

      s = src;
      s_m1 = (src_row > 0) ? src_m1 : src;
      s_p1 = (src_row < (orig_height - 1)) ? src_p1 : src;
      s_p2 = ((src_row + 1) < (orig_height - 1)) ? src_p2 : s_p1;

      r = row;

      frac = 0;
      j = width;

      while (j)
	{
	  if (x_cum + x_rat <= (src_col + 1 + EPSILON))
	    {
	      x_cum += x_rat;
	      dx = x_cum - src_col;
	      advance_dest_x = TRUE;
	    }
	  else
	    {
	      dx = 1.0;
	      advance_dest_x = FALSE;
	    }

	  tot_frac = x_frac[frac++] * y_frac;

	  minus_x = (src_col > 0) ? -bytes : 0;
	  plus_x = (src_col < (orig_width - 1)) ? bytes : 0;
	  plus2_x = ((src_col + 1) < (orig_width - 1)) ? bytes * 2 : plus_x;

	  if (cubic_interpolation)
	    switch (scale_type)
	      {
	      case MagnifyX_MagnifyY:
		for (b = 0; b < bytes; b++)
		  r[b] += cubic (dy, cubic (dx, s_m1[b+minus_x], s_m1[b], s_m1[b+plus_x], s_m1[b+plus2_x]),
				 cubic (dx, s[b+minus_x], s[b],	s[b+plus_x], s[b+plus2_x]),
				 cubic (dx, s_p1[b+minus_x], s_p1[b], s_p1[b+plus_x], s_p1[b+plus2_x]),
				 cubic (dx, s_p2[b+minus_x], s_p2[b], s_p2[b+plus_x], s_p2[b+plus2_x])) * tot_frac;
		break;
	      case MagnifyX_MinifyY:
		for (b = 0; b < bytes; b++)
		  r[b] += cubic (dx, s[b+minus_x], s[b], s[b+plus_x], s[b+plus2_x]) * tot_frac;
		break;
	      case MinifyX_MagnifyY:
		for (b = 0; b < bytes; b++)
		  r[b] += cubic (dy, s_m1[b], s[b], s_p1[b], s_p2[b]) * tot_frac;
		break;
	      case MinifyX_MinifyY:
		for (b = 0; b < bytes; b++)
		  r[b] += s[b] * tot_frac;
		break;
	      }
	  else
	    switch (scale_type)
	      {
	      case MagnifyX_MagnifyY:
		for (b = 0; b < bytes; b++)
		  r[b] += ((1 - dy) * ((1 - dx) * s[b] + dx * s[b+plus_x]) +
			   dy  * ((1 - dx) * s_p1[b] + dx * s_p1[b+plus_x])) * tot_frac;
		break;
	      case MagnifyX_MinifyY:
		for (b = 0; b < bytes; b++)
		  r[b] += (s[b] * (1 - dx) + s[b+plus_x] * dx) * tot_frac;
		break;
	      case MinifyX_MagnifyY:
		for (b = 0; b < bytes; b++)
		  r[b] += (s[b] * (1 - dy) + s_p1[b] * dy) * tot_frac;
		break;
	      case MinifyX_MinifyY:
		for (b = 0; b < bytes; b++)
		  r[b] += s[b] * tot_frac;
		break;
	      }

	  if (advance_dest_x)
	    {
	      r += bytes;
	      j--;
	    }
	  else
	    {
	      s_m1 += bytes;
	      s    += bytes;
	      s_p1 += bytes;
	      s_p2 += bytes;
	      src_col++;
	    }
	}

      if (advance_dest_y)
	{
	  tot_frac = 1.0 / (x_rat * y_rat);

	  /*  copy "row" to "dest"  */
	  d = dest;
	  r = row;

	  j = width;
	  while (j--)
	    {
	      b = bytes;
	      while (b--)
		*d++ = (unsigned char) (*r++ * tot_frac);
	    }

	  /*  set the pixel region span  */
	  pixel_region_set_row (destPR, 0, (height - i), width, dest);

	  /*  clear the "row" array  */
	  memset (row, 0, sizeof (double) * width * bytes);

	  i--;
	}
      else
	{
	  /*  Shuffle pointers  */
	  s = src_m1;
	  src_m1 = src;
	  src = src_p1;
	  src_p1 = src_p2;
	  src_p2 = s;

	  src_row++;
	  if ((src_row + 1) < (orig_height - 1))
	    pixel_region_get_row (srcPR, 0, (src_row + 2), orig_width, src_p2, 1);
	}
    }

  /*  free up temporary arrays  */
  g_free (row);
  g_free (x_frac);
  g_free (src_m1);
  g_free (src);
  g_free (src_p1);
  g_free (src_p2);
  g_free (dest);
#endif
}


void 
subsample_area  (
                 PixelArea * srcPR,
                 PixelArea * destPR,
                 int subsample
                 )
{
#if 0
  unsigned char * src, * s;
  unsigned char * dest, * d;
  double * row, * r;
  int destwidth;
  int src_row, src_col;
  int bytes, b;
  int width, height;
  int orig_width, orig_height;
  double x_rat, y_rat;
  double x_cum, y_cum;
  double x_last, y_last;
  double * x_frac, y_frac, tot_frac;
  int i, j;
  int frac;
  int advance_dest;

  orig_width = srcPR->w / subsample;
  orig_height = srcPR->h / subsample;
  width = destPR->w;
  height = destPR->h;

  /*  Some calculations...  */
  bytes = pixel_region_bytes (destPR);
  destwidth = destPR->rowstride;

  /*  the data pointers...  */
  src = (unsigned char *) g_malloc (orig_width * bytes);
  dest = destPR->data;

  /*  find the ratios of old x to new x and old y to new y  */
  x_rat = (double) orig_width / (double) width;
  y_rat = (double) orig_height / (double) height;

  /*  allocate an array to help with the calculations  */
  row    = (double *) g_malloc (sizeof (double) * width * bytes);
  x_frac = (double *) g_malloc (sizeof (double) * (width + orig_width));

  /*  initialize the pre-calculated pixel fraction array  */
  src_col = 0;
  x_cum = (double) src_col;
  x_last = x_cum;

  for (i = 0; i < width + orig_width; i++)
    {
      if (x_cum + x_rat <= (src_col + 1 + EPSILON))
	{
	  x_cum += x_rat;
	  x_frac[i] = x_cum - x_last;
	}
      else
	{
	  src_col ++;
	  x_frac[i] = src_col - x_last;
	}
      x_last += x_frac[i];
    }

  /*  clear the "row" array  */
  memset (row, 0, sizeof (double) * width * bytes);

  /*  counters...  */
  src_row = 0;
  y_cum = (double) src_row;
  y_last = y_cum;

  pixel_region_get_row (srcPR, 0, src_row * subsample, orig_width * subsample, src, subsample);

  /*  Scale the selected region  */
  for (i = 0; i < height; )
    {
      src_col = 0;
      x_cum = (double) src_col;

      /* determine the fraction of the src pixel we are using for y */
      if (y_cum + y_rat <= (src_row + 1 + EPSILON))
	{
	  y_cum += y_rat;
	  y_frac = y_cum - y_last;
	  advance_dest = TRUE;
	}
      else
	{
	  src_row ++;
	  y_frac = src_row - y_last;
	  advance_dest = FALSE;
	}

      y_last += y_frac;

      s = src;
      r = row;

      frac = 0;
      j = width;

      while (j)
	{
	  tot_frac = x_frac[frac++] * y_frac;

	  for (b = 0; b < bytes; b++)
	    r[b] += s[b] * tot_frac;

	  /*  increment the destination  */
	  if (x_cum + x_rat <= (src_col + 1 + EPSILON))
	    {
	      r += bytes;
	      x_cum += x_rat;
	      j--;
	    }

	  /* increment the source */
	  else
	    {
	      s += bytes;
	      src_col++;
	    }
	}

      if (advance_dest)
	{
	  tot_frac = 1.0 / (x_rat * y_rat);

	  /*  copy "row" to "dest"  */
	  d = dest;
	  r = row;

	  j = width;
	  while (j--)
	    {
	      b = bytes;
	      while (b--)
		*d++ = (unsigned char) (*r++ * tot_frac);
	    }

	  dest += destwidth;

	  /*  clear the "row" array  */
	  memset (row, 0, sizeof (double) * destwidth);

	  i++;
	}
      else
	pixel_region_get_row (srcPR, 0, src_row * subsample, orig_width * subsample, src, subsample);
    }

  /*  free up temporary arrays  */
  g_free (row);
  g_free (x_frac);
  g_free (src);
#endif
}


float
shapeburst_area (
                 PixelArea *srcPR,
                 PixelArea *distPR
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


int
thin_area (
           PixelArea *src,
           int type
           )
{
#if 0
  int i, j;
  int found_one;
  unsigned char *destp;
  unsigned char *prev_row;
  unsigned char *cur_row;
  unsigned char *next_row;
  unsigned char *temp_row;

  /*  feed both the prev and cur row buffers from the paint func buffer  */
  prev_row = paint_funcs_get_buffer ((src->w + 2) * 3 + src->w);

  /*  set all values to 0  */
  memset (prev_row, 0, (src->w + 2) * 3);

  prev_row += 1;
  cur_row = prev_row + src->w + 2;
  next_row = cur_row + src->w + 2;
  destp = next_row + src->w + 1;

  pixel_region_get_row (src, src->x, src->y, src->w, cur_row, 1);
  found_one = FALSE;

  for (i = 0; i < src->h; i++)
    {
      if (i < (src->h - 1))
	pixel_region_get_row (src, src->x, src->y + i + 1, src->w, next_row, 1);
      else
	memset (next_row, 0, src->w);

      for (j = 0; j < src->w; j++)
	{
	  if (type == SHRINK_REGION)
	    {
	      if (cur_row[j] != 0)
		{
		  found_one = TRUE;
		  destp[j] = cur_row[j];

		  if (cur_row[j - 1] < destp[j])
		    destp[j] = cur_row[j - 1];
		  if (cur_row[j + 1] < destp[j])
		    destp[j] = cur_row[j + 1];
		  if (prev_row[j] < destp[j])
		    destp[j] = prev_row[j];
		  if (next_row[j] < destp[j])
		    destp[j] = next_row[j];
		}
	      else
		destp[j] = cur_row[j];
	    }
	  else
	    {
	      if (cur_row[j] != 255)
		{
		  found_one = TRUE;
		  destp[j] = cur_row[j];

		  if (cur_row[j - 1] > destp[j])
		    destp[j] = cur_row[j - 1];
		  if (cur_row[j + 1] > destp[j])
		    destp[j] = cur_row[j + 1];
		  if (prev_row[j] > destp[j])
		    destp[j] = prev_row[j];
		  if (next_row[j] > destp[j])
		    destp[j] = next_row[j];
		}
	      else
		destp[j] = cur_row[j];
	    }
	}

      pixel_region_set_row (src, src->x, src->y + i, src->w, destp);

      temp_row = prev_row;
      prev_row = cur_row;
      cur_row = next_row;
      next_row = temp_row;
    }

  return (found_one == FALSE);  /*  is the area empty yet?  */
#endif
  return 0;
}


void 
swap_area  (
            PixelArea * src,
            PixelArea * dest
            )
{
#if 0
  int h;
  int length;
  unsigned char * s, * d;
  void * pr;
  int bytes;

  bytes = pixel_region_bytes (src);

  for (pr = pixel_regions_register (2, src, dest); pr != NULL; pr = pixel_regions_process (pr))
    {
      s = src->data;
      h = src->h;
      d = dest->data;
      length = src->w * bytes;

      while (h --)
	{
	  swap_pixels (s, d, length);
	  s += src->rowstride;
	  d += dest->rowstride;
	}
    }
#endif
}


void 
apply_mask_to_area  (
                     PixelArea * src,
                     PixelArea * mask,
                     Paint * opacity
                     )
{
  if (tag_equal (pixelarea_tag (src), pixelarea_tag (mask)))
    {
      void *  pag;
      
      for (pag = pixelarea_register (2, src, mask);
           pag != NULL;
           pag = pixelarea_process (pag))
        {
          PixelRow srcrow;
          PixelRow maskrow;
          int h;
          
          h = pixelarea_height (src);
          while (h--)
            {
              pixelarea_getdata (src, &srcrow, h);
              pixelarea_getdata (mask, &maskrow, h);
              apply_mask_to_alpha_row (&srcrow, &maskrow, opacity);
            }
        }
    }
}


void 
combine_mask_and_area  (
                        PixelArea * src,
                        PixelArea * mask,
                        Paint * opacity
                        )
{
#if 0
  int h;
  unsigned char * s, * m;
  void * pr;
  int bytes;

  bytes = pixel_region_bytes (src);

  for (pr = pixel_regions_register (2, src, mask); pr != NULL; pr = pixel_regions_process (pr))
    {
      s = src->data;
      m = mask->data;
      h = src->h;

      while (h --)
	{
	  combine_mask_and_alpha_channel (s, m, opacity, src->w, bytes);
	  s += src->rowstride;
	  m += mask->rowstride;
	}
    }
#endif
}


void 
copy_gray_to_area  (
                    PixelArea * src,
                    PixelArea * dest
                    )
{
#if 0
  int h;
  unsigned char * s, * d;
  void * pr;
  int bytes;

  bytes = pixel_region_bytes (dest);

  for (pr = pixel_regions_register (2, src, dest); pr != NULL; pr = pixel_regions_process (pr))
    {
      s = src->data;
      d = dest->data;
      h = src->h;

      while (h --)
	{
	  copy_gray_to_inten_a_pixels (s, d, src->w, bytes);
	  s += src->rowstride;
	  d += dest->rowstride;
	}
    }
#endif
}


void 
initial_area  (
               PixelArea * src,
               PixelArea * dest,
               PixelArea * mask,
               unsigned char * data,
               int opacity,
               int mode,
               int * affect,
               int type
               )
{
#if 0
  int h;
  unsigned char * s, * d, * m;
  unsigned char * buf;
  void * pr;

  buf = paint_funcs_get_buffer (src->w * (pixel_region_bytes (src) + 1));

  for (pr = pixel_regions_register (3, src, dest, mask); pr != NULL; pr = pixel_regions_process (pr))
    {
      s = src->data;
      d = dest->data;
      m = (mask) ? mask->data : NULL;

      for (h = 0; h < src->h; h++)
	{
	  /*  based on the type of the initial image...  */
	  switch (type)
	    {
	    case INITIAL_CHANNEL_MASK:
	    case INITIAL_CHANNEL_SELECTION:
	      initial_channel_pixels (s, d, src->w, pixel_region_bytes (dest));
	      break;

	    case INITIAL_INDEXED:
	      initial_indexed_pixels (s, d, data, src->w);
	      break;

	    case INITIAL_INDEXED_ALPHA:
	      initial_indexed_a_pixels (s, d, m, data, opacity, src->w);
	      break;

	    case INITIAL_INTENSITY:
	      if (mode == DISSOLVE_MODE)
		{
		  dissolve_pixels (s, buf, src->x, src->y + h, opacity, src->w,
                                   pixel_region_bytes (src),
                                   pixel_region_bytes (src) + 1, 0);
		  initial_inten_pixels (buf, d, m, opacity, affect,
					src->w,  pixel_region_bytes (src));
		}
	      else
		initial_inten_pixels (s, d, m, opacity, affect, src->w, pixel_region_bytes (src));
	      break;

	    case INITIAL_INTENSITY_ALPHA:
	      if (mode == DISSOLVE_MODE)
		{
		  dissolve_pixels (s, buf, src->x, src->y + h, opacity, src->w, pixel_region_bytes (src),
				   pixel_region_bytes (src), 1);
		  initial_inten_a_pixels (buf, d, m, opacity, affect,
					  src->w, pixel_region_bytes (src));
		}
	      else
		initial_inten_a_pixels (s, d, m, opacity, affect, src->w, pixel_region_bytes (src));
	      break;
	    }

	  s += src->rowstride;
	  d += dest->rowstride;
	  if (mask)
	    m += mask->rowstride;
	}
    }
#endif
}



void 
combine_areas  (
                PixelArea * src1,
                PixelArea * src2,
                PixelArea * dest,
                PixelArea * mask,
                unsigned char * data,
                int opacity,
                int mode,
                int * affect,
                int type
                )
{
#if 0
  int h;
  int ha1, ha2;
  int combine;
  int mode_affect;
  unsigned char * s, * s1, * s2;
  unsigned char * d, * m;
  unsigned char * buf;
  void * pr;

  combine = 0;

  /*  Determine which sources have alpha channels  */
  switch (type)
    {
    case COMBINE_INTEN_INTEN:
    case COMBINE_INDEXED_INDEXED:
      ha1 = ha2 = 0;
      break;
    case COMBINE_INTEN_A_INTEN:
      ha1 = 1;
      ha2 = 0;
      break;
    case COMBINE_INTEN_INTEN_A:
    case COMBINE_INDEXED_INDEXED_A:
      ha1 = 0;
      ha2 = 1;
      break;
    case COMBINE_INTEN_A_INTEN_A:
    case COMBINE_INDEXED_A_INDEXED_A:
      ha1 = ha2 = 1;
      break;
    default:
      ha1 = ha2 = 0;
    }

  buf = paint_funcs_get_buffer (src1->w * (pixel_region_bytes (src1) + 1));

  for (pr = pixel_regions_register (4, src1, src2, dest, mask); pr != NULL; pr = pixel_regions_process (pr))
    {
      s1 = src1->data;
      s2 = src2->data;
      d = dest->data;
      m = (mask) ? mask->data : NULL;

      for (h = 0; h < src1->h; h++)
	{
	  s = buf;

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
	      combine = apply_indexed_layer_mode (s1, s2, &s, mode, ha1, ha2);
	      break;

	    case COMBINE_INTEN_INTEN_A:
	    case COMBINE_INTEN_A_INTEN:
	    case COMBINE_INTEN_INTEN:
	    case COMBINE_INTEN_A_INTEN_A:
	      /*  Now, apply the paint mode  */
	      combine = apply_layer_mode (s1, s2, &s, src1->x, src1->y + h, opacity, src1->w, mode,
					  pixel_region_bytes (src1), pixel_region_bytes (src2), ha1, ha2, &mode_affect);
	      break;

	    default:
	      break;
	    }

	  /*  based on the type of the initial image...  */
	  switch (combine)
	    {
	    case COMBINE_INDEXED_INDEXED:
	      combine_indexed_and_indexed_pixels (s1, s2, d, m, opacity,
						  affect, src1->w, pixel_region_bytes (src1));
	      break;

	    case COMBINE_INDEXED_INDEXED_A:
	      combine_indexed_and_indexed_a_pixels (s1, s2, d, m, opacity,
						    affect, src1->w, pixel_region_bytes (src1));
	      break;

	    case COMBINE_INDEXED_A_INDEXED_A:
	      combine_indexed_a_and_indexed_a_pixels (s1, s2, d, m, opacity,
						      affect, src1->w, pixel_region_bytes (src1));
	      break;

	    case COMBINE_INTEN_A_INDEXED_A:
	      /*  assume the data passed to this procedure is the
	       *  indexed layer's colormap
	       */
	      combine_inten_a_and_indexed_a_pixels (s1, s2, d, m, data, opacity,
						    src1->w, pixel_region_bytes (dest));
	      break;

	    case COMBINE_INTEN_A_CHANNEL_MASK:
	      /*  assume the data passed to this procedure is the
	       *  indexed layer's colormap
	       */
	      combine_inten_a_and_channel_mask_pixels (s1, s2, d, data, opacity,
						       src1->w, pixel_region_bytes (dest));
	      break;

	    case COMBINE_INTEN_A_CHANNEL_SELECTION:
	      combine_inten_a_and_channel_selection_pixels (s1, s2, d, data, opacity,
							    src1->w, pixel_region_bytes (src1));
	      break;

	    case COMBINE_INTEN_INTEN:
	      combine_inten_and_inten_pixels (s1, s, d, m, opacity,
					      affect, src1->w, pixel_region_bytes (src1));
	      break;

	    case COMBINE_INTEN_INTEN_A:
	      combine_inten_and_inten_a_pixels (s1, s, d, m, opacity,
						affect, src1->w, pixel_region_bytes (src1));
	      break;

	    case COMBINE_INTEN_A_INTEN:
	      combine_inten_a_and_inten_pixels (s1, s, d, m, opacity,
						affect, mode_affect, src1->w, pixel_region_bytes (src1));
	      break;

	    case COMBINE_INTEN_A_INTEN_A:
	      combine_inten_a_and_inten_a_pixels (s1, s, d, m, opacity,
						  affect, mode_affect, src1->w, pixel_region_bytes (src1));
	      break;

	    case BEHIND_INTEN:
	      behind_inten_pixels (s1, s, d, m, opacity,
				   affect, src1->w, pixel_region_bytes (src1),
				   pixel_region_bytes (src2), ha1, ha2);
	      break;

	    case BEHIND_INDEXED:
	      behind_indexed_pixels (s1, s, d, m, opacity,
				     affect, src1->w, pixel_region_bytes (src1),
				     pixel_region_bytes (src2), ha1, ha2);
	      break;

	    case REPLACE_INTEN:
	      replace_inten_pixels (s1, s, d, m, opacity,
				    affect, src1->w, pixel_region_bytes (src1),
				    pixel_region_bytes (src2), ha1, ha2);
	      break;

	    case REPLACE_INDEXED:
	      replace_indexed_pixels (s1, s, d, m, opacity,
				      affect, src1->w, pixel_region_bytes (src1),
				      pixel_region_bytes (src2), ha1, ha2);
	      break;

	    case ERASE_INTEN:
	      erase_inten_pixels (s1, s, d, m, opacity,
				  affect, src1->w, pixel_region_bytes (src1));
	      break;

	    case ERASE_INDEXED:
	      erase_indexed_pixels (s1, s, d, m, opacity,
				    affect, src1->w, pixel_region_bytes (src1));
	      break;

	    case NO_COMBINATION:
	      break;

	    default:
	      break;
	    }

	  s1 += src1->rowstride;
	  s2 += src2->rowstride;
	  d += dest->rowstride;
	  if (mask)
	    m += mask->rowstride;
	}
    }
#endif
}

void 
combine_areas_replace  (
                        PixelArea * src1,
                        PixelArea * src2,
                        PixelArea * dest,
                        PixelArea * mask,
                        unsigned char * data,
                        int opacity,
                        int * affect,
                        int type
                        )
{
#if 0
  int h;
  unsigned char * s1, * s2;
  unsigned char * d, * m;
  void * pr;

  for (pr = pixel_regions_register (4, src1, src2, dest, mask); pr != NULL; pr = pixel_regions_process (pr))
    {
      s1 = src1->data;
      s2 = src2->data;
      d = dest->data;
      m = mask->data;

      for (h = 0; h < src1->h; h++)
	{

	  /*  Now, apply the paint mode  */
	  apply_layer_mode_replace (s1, s2, d, m, src1->x, src1->y + h, opacity, src1->w,
                                    pixel_region_bytes (src1), pixel_region_bytes (src2),
                                    affect);

	  s1 += src1->rowstride;
	  s2 += src2->rowstride;
	  d += dest->rowstride;
	  m += mask->rowstride;
	}
    }
#endif
}



/*========================================================================*/

/*
 * The equations: g(r) = exp (- r^2 / (2 * sigma^2))
 *                   r = sqrt (x^2 + y ^2)
 */
static int * 
make_curve  (
             double sigma,
             int * length
             )
{
  int *curve;
  double sigma2;
  double l;
  int temp;
  int i, n;

  sigma2 = 2 * sigma * sigma;
  l = sqrt (-sigma2 * log (1.0 / 255.0));

  n = ceil (l) * 2;
  if ((n % 2) == 0)
    n += 1;

  curve = g_malloc (sizeof (int) * n);

  *length = n / 2;
  curve += *length;
  curve[0] = 255;

  for (i = 1; i <= *length; i++)
    {
      temp = (int) (exp (- (i * i) / sigma2) * 255);
      curve[-i] = temp;
      curve[i] = temp;
    }

  return curve;
}


static void 
run_length_encode  (
                    unsigned char * src,
                    int * dest,
                    int w,
                    int bytes
                    )
{
  int start;
  int i;
  int j;
  unsigned char last;

  last = *src;
  src += bytes;
  start = 0;

  for (i = 1; i < w; i++)
    {
      if (*src != last)
	{
	  for (j = start; j < i; j++)
	    {
	      *dest++ = (i - j);
	      *dest++ = last;
	    }
	  start = i;
	  last = *src;
	}
      src += bytes;
    }

  for (j = start; j < i; j++)
    {
      *dest++ = (i - j);
      *dest++ = last;
    }
}


static void 
draw_segments  (
                PixelArea * destPR,
                BoundSeg * bs,
                int num_segs,
                int off_x,
                int off_y,
                int opacity
                )
{
#if 0
  int x1, y1, x2, y2;
  int tmp, i, length;
  unsigned char *line;

  length = MAX (destPR->w, destPR->h);
  line = paint_funcs_get_buffer (length);
  memset (line, opacity, length);

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
      x1 = CLAMP (x1, 0, destPR->w - 1);
      y1 = CLAMP (y1, 0, destPR->h - 1);
      x2 = CLAMP (x2, 0, destPR->w - 1);
      y2 = CLAMP (y2, 0, destPR->h - 1);

      if (x1 == x2)
	{
	  if (y2 < y1)
	    {
	      tmp = y1;
	      y1 = y2;
	      y2 = tmp;
	    }
	  pixel_region_set_col (destPR, x1, y1, (y2 - y1), line);
	}
      else
	{
	  if (x2 < x1)
	    {
	      tmp = x1;
	      x1 = x2;
	      x2 = tmp;
	    }
	  pixel_region_set_row (destPR, x1, y1, (x2 - x1), line);
	}
    }
#endif
}


static double 
cubic  (
        double dx,
        int jm1,
        int j,
        int jp1,
        int jp2
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

  if (result < 0.0)
    result = 0.0;
  if (result > 255.0)
    result = 255.0;

  return result;
}


/************************************/
/*       apply layer modes          */
/************************************/

static int 
apply_layer_mode  (
                   unsigned char * src1,
                   unsigned char * src2,
                   unsigned char ** dest,
                   int x,
                   int y,
                   int opacity,
                   int length,
                   int mode,
                   int b1,
                   int b2,
                   int ha1,
                   int ha2,
                   int * mode_affect
                   )
{
#if 0
  int combine;

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
      *dest = src2;
      break;

    case DISSOLVE_MODE:
      /*  Since dissolve requires an alpha channels...  */
      if (! ha2)
	add_alpha_pixels (src2, *dest, length, b2);

      dissolve_pixels (src2, *dest, x, y, opacity, length, b2,
		       ((ha2) ? b2 : b2 + 1), ha2);
      combine = (ha1) ? COMBINE_INTEN_A_INTEN_A : COMBINE_INTEN_INTEN_A;
      break;

    case MULTIPLY_MODE:
      multiply_pixels (src1, src2, *dest, length, b1, b2, ha1, ha2);
      break;

    case SCREEN_MODE:
      screen_pixels (src1, src2, *dest, length, b1, b2, ha1, ha2);
      break;

    case OVERLAY_MODE:
      overlay_pixels (src1, src2, *dest, length, b1, b2, ha1, ha2);
      break;

    case DIFFERENCE_MODE:
      difference_pixels (src1, src2, *dest, length, b1, b2, ha1, ha2);
      break;

    case ADDITION_MODE:
      add_pixels (src1, src2, *dest, length, b1, b2, ha1, ha2);
      break;

    case SUBTRACT_MODE:
      subtract_pixels (src1, src2, *dest, length, b1, b2, ha1, ha2);
      break;

    case DARKEN_ONLY_MODE:
      darken_pixels (src1, src2, *dest, length, b1, b2, ha1, ha2);
      break;

    case LIGHTEN_ONLY_MODE:
      lighten_pixels (src1, src2, *dest, length, b1, b2, ha1, ha2);
      break;

    case HUE_MODE: case SATURATION_MODE: case VALUE_MODE:
      /*  only works on RGB color images  */
      if (b1 > 2)
	hsv_only_pixels (src1, src2, *dest, mode, length, b1, b2, ha1, ha2);
      else
	*dest = src2;
      break;

    case COLOR_MODE:
      /*  only works on RGB color images  */
      if (b1 > 2)
	color_only_pixels (src1, src2, *dest, mode, length, b1, b2, ha1, ha2);
      else
	*dest = src2;
      break;

    case BEHIND_MODE:
      *dest = src2;
      if (ha1)
	combine = BEHIND_INTEN;
      else
	combine = NO_COMBINATION;
      break;

    case REPLACE_MODE:
      *dest = src2;
      combine = REPLACE_INTEN;
      break;

    case ERASE_MODE:
      *dest = src2;
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
#endif
  return 0;
}


static int 
apply_indexed_layer_mode  (
                           unsigned char * src1,
                           unsigned char * src2,
                           unsigned char ** dest,
                           int mode,
                           int ha1,
                           int ha2
                           )
{
#if 0
  int combine;

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
      *dest = src2;
      combine = REPLACE_INDEXED;
      break;

    case BEHIND_MODE:
      *dest = src2;
      if (ha1)
	combine = BEHIND_INDEXED;
      else
	combine = NO_COMBINATION;
      break;

    case ERASE_MODE:
      *dest = src2;
      /*  If both sources have alpha channels, call erase function.
       *  Otherwise, just combine in the normal manner
       */
      combine = (ha1 && ha2) ? ERASE_INDEXED : combine;
      break;

    default:
      break;
    }

  return combine;
#endif
  return 0;
}

static void 
apply_layer_mode_replace  (
                           unsigned char * src1,
                           unsigned char * src2,
                           unsigned char * dest,
                           unsigned char * mask,
                           int x,
                           int y,
                           int opacity,
                           int length,
                           int b1,
                           /* bytes */ int b2,
                           /* bytes */ int * affect
                           )
{
#if 0
  replace_pixels (src1, src2, dest, mask, length, opacity, affect, b1, b2);
#endif
}
