/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimphistogram module Copyright (C) 1999 Jay Cox <jaycox@earthlink.net>
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

#include "config.h"

#include "gimphistogramP.h"
#include "gimphistogram.h"
#include "pixel_region.h"
#include "gimpdrawable.h"
#include "channel.h"
#include "gimpimage.h"
#include "gimprc.h"
#include <glib.h>
#include <math.h>

#ifdef ENABLE_MP

#include <pthread.h>

#endif /* ENABLE_MP */


GimpHistogram *
gimp_histogram_new()
{
  GimpHistogram *histogram;
  histogram = g_new(GimpHistogram, 1);
  histogram->values = NULL;
  histogram->nchannels = 0;
  return histogram;
}

void
gimp_histogram_free (GimpHistogram *histogram)
{
  int i;
  if (histogram->values)
  {
    for (i = 0; i < histogram->nchannels; i++)
      g_free(histogram->values[i]);
    g_free(histogram->values);
  }
  g_free(histogram);
}

void
gimp_histogram_calculate_sub_region(GimpHistogram *histogram,
				    PixelRegion *region, PixelRegion *mask)
{
  const unsigned char *src, *msrc;
  const unsigned char *m, *s;
  double **values;
  int h, w, max;
#ifdef ENABLE_MP
  int slot = 0;
  /* find an unused temporary slot to put our results in and lock it */
  pthread_mutex_lock(&histogram->mutex);
  {
    while (histogram->tmp_slots[slot])
      slot++;
    values = histogram->tmp_values[slot];
    histogram->tmp_slots[slot] = 1;
  }
  pthread_mutex_unlock(&histogram->mutex);
#else /* !ENABLE_MP */
  values = histogram->values;
#endif
  h = region->h;
  w = region->w;
  if (mask)
  {
    double masked;
    src = region->data;
    msrc = region->data;
    while (h--)
    {
      s = src;
      m = msrc;
      w = region->w;
      switch(region->bytes)
      {
       case 1:
	 while (w--)
	 {
	   masked = m[0] / 255.0;
	   values[0][s[0]] += masked;
	   s += 1;
	   m += 1;
	 } break;
       case 2:
	 while (w--)
	 {
	   masked = m[0] / 255.0;
	   values[0][s[0]] += masked;
	   values[1][s[1]] += masked;
	   s += 2;
	   m += 1;
	 } break;
       case 3: /* calculate seperate value values */ 
	 while (w--)
	 {
	   masked = m[0] / 255.0;
	   values[1][s[0]] += masked;
	   values[2][s[1]] += masked;
	   values[3][s[2]] += masked;
	   max = (s[0] > s[1]) ? s[0] : s[1];
	   if (s[2] > max)
	     values[0][s[2]] += masked;
	   else
	     values[0][max] += masked;
	   s += 3;
	   m += 1;
	 } break;
       case 4: /* calculate seperate value values */ 
	 while (w--)
	 {
	   masked = m[0] / 255.0;
	   values[1][s[0]] += masked;
	   values[2][s[1]] += masked;
	   values[3][s[2]] += masked;
	   values[4][s[3]] += masked;
	   max = (s[0] > s[1]) ? s[0] : s[1];
	   if (s[2] > max)
	     values[0][s[2]] += masked;
	   else
	     values[0][max] += masked;
	   s += 3;
	   m += 1;
	 } break;
      }
      src += region->rowstride;
      msrc += mask->rowstride;
    }
  }
  else /* no mask */
  {
    src = region->data;
    while (h--)
    {
      s = src;
      w = region->w;
      switch(region->bytes)
      {
       case 1:
	 while (w--)
	 {
	   values[0][s[0]] += 1.0;
	   s += 1;
	 } break;
       case 2:
	 while (w--)
	 {
	   values[0][s[0]] += 1.0;
	   values[1][s[1]] += 1.0;
	   s += 2;
	 } break;
       case 3: /* calculate seperate value values */ 
	 while (w--)
	 {
	   values[1][s[0]] += 1.0;
	   values[2][s[1]] += 1.0;
	   values[3][s[2]] += 1.0;
	   max = (s[0] > s[1]) ? s[0] : s[1];
	   if (s[2] > max)
	     values[0][s[2]] += 1.0;
	   else
	     values[0][max] += 1.0;
	   s += 3;
	 } break;
       case 4: /* calculate seperate value values */ 
	 while (w--)
	 {
	   values[1][s[0]] += 1.0;
	   values[2][s[1]] += 1.0;
	   values[3][s[2]] += 1.0;
	   values[4][s[3]] += 1.0;
	   max = (s[0] > s[1]) ? s[0] : s[1];
	   if (s[2] > max)
	     values[0][s[2]] += 1.0;
	   else
	     values[0][max] += 1.0;
	   s += 3;
	 } break;
      }
      src += region->rowstride;
    }
  }

#ifdef ENABLE_MP
  /* unlock this slot */
  /* we shouldn't have to use mutex locks here */
  pthread_mutex_lock(&histogram->mutex);
  histogram->tmp_slots[slot] = 0;
  pthread_mutex_unlock(&histogram->mutex);
#endif
}

static void gimp_histogram_alloc(GimpHistogram *histogram, int bytes)
{
  int i;
  if (bytes + 1 != histogram->nchannels)
  {
    if (histogram->values)
    {
      for (i = 0; i < histogram->nchannels; i++)
	g_free(histogram->values[i]);
      g_free(histogram->values);
    }
    histogram->nchannels = bytes + 1;
    histogram->values = g_new(double *, histogram->nchannels);
    for (i = 0; i < histogram->nchannels; i++)
      histogram->values[i] = g_new(double, 256);
  }
}

void
gimp_histogram_calculate (GimpHistogram *histogram, PixelRegion *region,
			  PixelRegion *mask)
{
  int i, j;
#ifdef ENABLE_MP
  int k;
#endif

  gimp_histogram_alloc(histogram, region->bytes);

#ifdef ENABLE_MP
  pthread_mutex_init(&histogram->mutex, NULL);
  histogram->tmp_slots = g_new(char, num_processors);
  histogram->tmp_values = g_new(double **, num_processors);
  for (i = 0; i < num_processors; i++)
  {
    histogram->tmp_values[i] = g_new(double *, histogram->nchannels);
    histogram->tmp_slots[i] = 0;
    for (j = 0; j < histogram->nchannels; j++)
    {
      histogram->tmp_values[i][j] = g_new(double, 256);
      for (k = 0; k < 256; k++)
	histogram->tmp_values[i][j][k] = 0.0;
    }
  }
#endif

  for (i = 0; i < histogram->nchannels; i++)
    for (j = 0; j < 256; j++)
      histogram->values[i][j] = 0.0;

  pixel_regions_process_parallel((p_func)gimp_histogram_calculate_sub_region,
				histogram, 2, region, mask);

#ifdef ENABLE_MP
  /* add up all the tmp buffers and free their memmory */
  for (i = 0; i < num_processors; i++)
  {
    for (j = 0; j < histogram->nchannels; j++)
    {
      for (k = 0; k < 256; k++)
	histogram->values[j][k] += histogram->tmp_values[i][j][k];
      g_free (histogram->tmp_values[i][j]);
    }
    g_free(histogram->tmp_values[i]);
  }
  g_free(histogram->tmp_values);
  g_free(histogram->tmp_slots);
#endif
}


void
gimp_histogram_calculate_drawable(GimpHistogram *histogram,
				  GimpDrawable *drawable)
{
  PixelRegion region;
  PixelRegion mask;
  int x1, y1, x2, y2;
  int off_x, off_y;
  int no_mask;

  no_mask = (drawable_mask_bounds (drawable, &x1, &y1, &x2, &y2) == FALSE);
  pixel_region_init (&region, gimp_drawable_data (drawable), x1, y1,
		     (x2 - x1), (y2 - y1), FALSE);
  if (!no_mask)
  {
    Channel *sel_mask;
    GimpImage *gimage;

    gimage = gimp_drawable_gimage(drawable);
    sel_mask = gimp_image_get_mask (gimage);

    drawable_offsets (drawable, &off_x, &off_y);
    pixel_region_init (&mask, gimp_drawable_data (GIMP_DRAWABLE (sel_mask)),
		       x1 + off_x, y1 + off_y, (x2 - x1), (y2 - y1), FALSE);
    gimp_histogram_calculate (histogram, &region, &mask);
  }
  else
    gimp_histogram_calculate (histogram, &region, NULL);
}

double
gimp_histogram_get_maximum (GimpHistogram *histogram, int channel)
{
  double max = 0.0;
  int x;
  for (x = 0; x < 256; x++)
    if (histogram->values[channel][x] > max)
      max = histogram->values[channel][x];
  return max;
}

double
gimp_histogram_get_value (GimpHistogram *histogram, int channel, int bin)
{
  if (channel < histogram->nchannels && bin >= 0 && bin < 256)
    return histogram->values[channel][bin];
  return 0.0;
}

double
gimp_histogram_get_channel (GimpHistogram *histogram, int channel, int bin)
{
  if (histogram->nchannels > 3)
    return gimp_histogram_get_value(histogram, channel + 1, bin);
  else
    return gimp_histogram_get_value(histogram, channel    , bin);
}

int
gimp_histogram_nchannels (GimpHistogram *histogram)
{
  return histogram->nchannels - 1;
}

double
gimp_histogram_get_count (GimpHistogram *histogram, int start, int end)
{
  int i;
  double count = 0.0;
  for (i = start; i <= end; i++)
    count += histogram->values[0][i];
  return count;
}

double
gimp_histogram_get_mean (GimpHistogram *histogram, int channel,
			 int start, int end)
{
  int i;
  double mean = 0.0;
  double count;
  for (i = start; i <= end; i++)
    mean += i * histogram->values[channel][i];
  count = gimp_histogram_get_count(histogram, start, end);
  if (count > 0.0)
    return mean / count;
  return mean;
}

int
gimp_histogram_get_median (GimpHistogram *histogram, int channel, int start, int end)
{
  int i;
  double sum = 0.0;
  double count;
  count = gimp_histogram_get_count(histogram, start, end);
  for (i = start; i <= end; i++)
  {
    sum += i * histogram->values[channel][i];
    if (sum * 2 > count)
      return i;
  }
  return -1;
}

double
gimp_histogram_get_std_dev (GimpHistogram *histogram, int channel,
			    int start, int end)
{
  int i;
  double dev = 0.0;
  double count;
  double mean;
  mean = gimp_histogram_get_mean(histogram, channel, start, end);
  count = gimp_histogram_get_count(histogram, start, end);
  if (count == 0.0)
    count = 1.0;
  for (i = start; i <= end; i++)
    dev += gimp_histogram_get_value(histogram, channel, i) *
      (i - mean) * (i - mean);
  return sqrt (dev / count);
}
