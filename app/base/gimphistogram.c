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

#include <glib-object.h>

#include "libgimpmath/gimpmath.h"

#include "base-types.h"

#include "gimphistogram.h"
#include "pixel-processor.h"
#include "pixel-region.h"


#ifdef ENABLE_MP
#define NUM_SLOTS  GIMP_MAX_NUM_THREADS
#else
#define NUM_SLOTS  1
#endif


struct _GimpHistogram
{
  gint           n_channels;
#ifdef ENABLE_MP
  GStaticMutex   mutex;
  gchar          slots[NUM_SLOTS];
#endif
  gdouble      **values[NUM_SLOTS];
};


/*  local function prototypes  */

static void  gimp_histogram_alloc_values         (GimpHistogram *histogram,
                                                  gint           bytes);
static void  gimp_histogram_free_values          (GimpHistogram *histogram);
static void  gimp_histogram_calculate_sub_region (GimpHistogram *histogram,
                                                  PixelRegion   *region,
                                                  PixelRegion   *mask);


/*  public functions  */

GimpHistogram *
gimp_histogram_new (void)
{
  GimpHistogram *histogram = g_new0 (GimpHistogram, 1);

#ifdef ENABLE_MP
  g_static_mutex_init (&histogram->mutex);
#endif

  return histogram;
}

void
gimp_histogram_free (GimpHistogram *histogram)
{
  g_return_if_fail (histogram != NULL);

  gimp_histogram_free_values (histogram);
  g_free (histogram);
}

void
gimp_histogram_calculate (GimpHistogram *histogram,
			  PixelRegion   *region,
			  PixelRegion   *mask)
{
  gint i;

  g_return_if_fail (histogram != NULL);

  if (! region)
    {
      gimp_histogram_free_values (histogram);
      return;
    }

  gimp_histogram_alloc_values (histogram, region->bytes);

  for (i = 0; i < NUM_SLOTS; i++)
    if (histogram->values[i])
      {
        gint j, k;

        for (j = 0; j < histogram->n_channels; j++)
          for (k = 0; k < 256; k++)
            histogram->values[i][j][k] = 0.0;
      }

  pixel_regions_process_parallel ((PixelProcessorFunc)
                                  gimp_histogram_calculate_sub_region,
                                  histogram, 2, region, mask);

#ifdef ENABLE_MP
  /* add up all slots */
  for (i = 1; i < NUM_SLOTS; i++)
    if (histogram->values[i])
      {
        gint j, k;

        for (j = 0; j < histogram->n_channels; j++)
          for (k = 0; k < 256; k++)
            histogram->values[0][j][k] += histogram->values[i][j][k];
      }
#endif
}

gdouble
gimp_histogram_get_maximum (GimpHistogram        *histogram,
			    GimpHistogramChannel  channel)
{
  gdouble max = 0.0;
  gint    x;

  g_return_val_if_fail (histogram != NULL, 0.0);

  /*  the gray alpha channel is in slot 1  */
  if (histogram->n_channels == 3 && channel == GIMP_HISTOGRAM_ALPHA)
    channel = 1;

  if (! histogram->values ||
      (channel != GIMP_HISTOGRAM_RGB && channel >= histogram->n_channels))
    return 0.0;

  if (channel == GIMP_HISTOGRAM_RGB)
    for (x = 0; x < 256; x++)
      {
	max = MAX (max, histogram->values[0][GIMP_HISTOGRAM_RED][x]);
	max = MAX (max, histogram->values[0][GIMP_HISTOGRAM_GREEN][x]);
	max = MAX (max, histogram->values[0][GIMP_HISTOGRAM_BLUE][x]);
      }
  else
    for (x = 0; x < 256; x++)
      if (histogram->values[0][channel][x] > max)
	max = histogram->values[0][channel][x];

  return max;
}

gdouble
gimp_histogram_get_value (GimpHistogram        *histogram,
			  GimpHistogramChannel  channel,
			  gint                  bin)
{
  g_return_val_if_fail (histogram != NULL, 0.0);

  /*  the gray alpha channel is in slot 1  */
  if (histogram->n_channels == 3 && channel == GIMP_HISTOGRAM_ALPHA)
    channel = 1;

  if (! histogram->values ||
      bin < 0 || bin >= 256 ||
      (channel == GIMP_HISTOGRAM_RGB && histogram->n_channels < 4) ||
      (channel != GIMP_HISTOGRAM_RGB && channel >= histogram->n_channels))
    return 0.0;

  if (channel == GIMP_HISTOGRAM_RGB)
    {
      gdouble min = histogram->values[0][GIMP_HISTOGRAM_RED][bin];

      min = MIN (min, histogram->values[0][GIMP_HISTOGRAM_GREEN][bin]);

      return MIN (min, histogram->values[0][GIMP_HISTOGRAM_BLUE][bin]);
    }
  else
    {
      return histogram->values[0][channel][bin];
    }
}

gdouble
gimp_histogram_get_channel (GimpHistogram        *histogram,
			    GimpHistogramChannel  channel,
			    gint                  bin)
{
  g_return_val_if_fail (histogram != NULL, 0.0);

  if (histogram->n_channels > 3)
    return gimp_histogram_get_value (histogram, channel + 1, bin);
  else
    return gimp_histogram_get_value (histogram, channel,     bin);
}

gint
gimp_histogram_n_channels (GimpHistogram *histogram)
{
  g_return_val_if_fail (histogram != NULL, 0);

  return histogram->n_channels - 1;
}

gdouble
gimp_histogram_get_count (GimpHistogram        *histogram,
                          GimpHistogramChannel  channel,
			  gint                  start,
			  gint                  end)
{
  gint    i;
  gdouble count = 0.0;

  g_return_val_if_fail (histogram != NULL, 0.0);

  /*  the gray alpha channel is in slot 1  */
  if (histogram->n_channels == 3 && channel == GIMP_HISTOGRAM_ALPHA)
    channel = 1;

  if (channel == GIMP_HISTOGRAM_RGB)
    return (gimp_histogram_get_count (histogram,
                                      GIMP_HISTOGRAM_RED, start, end)   +
	    gimp_histogram_get_count (histogram,
                                      GIMP_HISTOGRAM_GREEN, start, end) +
	    gimp_histogram_get_count (histogram,
                                      GIMP_HISTOGRAM_BLUE, start, end));

  if (! histogram->values ||
      start > end ||
      channel >= histogram->n_channels)
    return 0.0;

  start = CLAMP (start, 0, 255);
  end   = CLAMP (end, 0, 255);

  for (i = start; i <= end; i++)
    count += histogram->values[0][channel][i];

  return count;
}

gdouble
gimp_histogram_get_mean (GimpHistogram        *histogram,
			 GimpHistogramChannel  channel,
			 gint                  start,
			 gint                  end)
{
  gint    i;
  gdouble mean = 0.0;
  gdouble count;

  g_return_val_if_fail (histogram != NULL, 0.0);

  /*  the gray alpha channel is in slot 1  */
  if (histogram->n_channels == 3 && channel == GIMP_HISTOGRAM_ALPHA)
    channel = 1;

  if (! histogram->values ||
      start > end ||
      (channel == GIMP_HISTOGRAM_RGB && histogram->n_channels < 4) ||
      (channel != GIMP_HISTOGRAM_RGB && channel >= histogram->n_channels))
    return 0.0;

  start = CLAMP (start, 0, 255);
  end = CLAMP (end, 0, 255);

  if (channel == GIMP_HISTOGRAM_RGB)
    {
      for (i = start; i <= end; i++)
	mean += (i * histogram->values[0][GIMP_HISTOGRAM_RED][i]   +
		 i * histogram->values[0][GIMP_HISTOGRAM_GREEN][i] +
		 i * histogram->values[0][GIMP_HISTOGRAM_BLUE][i]);
    }
  else
    {
      for (i = start; i <= end; i++)
	mean += i * histogram->values[0][channel][i];
    }

  count = gimp_histogram_get_count (histogram, channel, start, end);

  if (count > 0.0)
    return mean / count;

  return mean;
}

gint
gimp_histogram_get_median (GimpHistogram         *histogram,
			   GimpHistogramChannel   channel,
			   gint                   start,
			   gint                   end)
{
  gint    i;
  gdouble sum = 0.0;
  gdouble count;

  g_return_val_if_fail (histogram != NULL, -1);

  /*  the gray alpha channel is in slot 1  */
  if (histogram->n_channels == 3 && channel == GIMP_HISTOGRAM_ALPHA)
    channel = 1;

  if (! histogram->values ||
      start > end ||
      (channel == GIMP_HISTOGRAM_RGB && histogram->n_channels < 4) ||
      (channel != GIMP_HISTOGRAM_RGB && channel >= histogram->n_channels))
    return 0;

  start = CLAMP (start, 0, 255);
  end = CLAMP (end, 0, 255);

  count = gimp_histogram_get_count (histogram, channel, start, end);

  if (channel == GIMP_HISTOGRAM_RGB)
    for (i = start; i <= end; i++)
      {
	sum += (histogram->values[0][GIMP_HISTOGRAM_RED][i]   +
		histogram->values[0][GIMP_HISTOGRAM_GREEN][i] +
		histogram->values[0][GIMP_HISTOGRAM_BLUE][i]);

	if (sum * 2 > count)
	  return i;
      }
  else
    for (i = start; i <= end; i++)
      {
	sum += histogram->values[0][channel][i];

	if (sum * 2 > count)
	  return i;
      }

  return -1;
}

gdouble
gimp_histogram_get_std_dev (GimpHistogram        *histogram,
			    GimpHistogramChannel  channel,
			    gint                  start,
			    gint                  end)
{
  gint    i;
  gdouble dev = 0.0;
  gdouble count;
  gdouble mean;

  g_return_val_if_fail (histogram != NULL, 0.0);

  /*  the gray alpha channel is in slot 1  */
  if (histogram->n_channels == 3 && channel == GIMP_HISTOGRAM_ALPHA)
    channel = 1;

  if (! histogram->values ||
      start > end ||
      (channel == GIMP_HISTOGRAM_RGB && histogram->n_channels < 4) ||
      (channel != GIMP_HISTOGRAM_RGB && channel >= histogram->n_channels))
    return 0.0;

  mean  = gimp_histogram_get_mean  (histogram, channel, start, end);
  count = gimp_histogram_get_count (histogram, channel, start, end);

  if (count == 0.0)
    count = 1.0;

  for (i = start; i <= end; i++)
    dev += gimp_histogram_get_value (histogram, channel, i) *
      (i - mean) * (i - mean);

  return sqrt (dev / count);
}


/*  private functions  */

static void
gimp_histogram_alloc_values (GimpHistogram *histogram,
                             gint           bytes)
{
  gint i;

  if (bytes + 1 != histogram->n_channels)
    {
      gimp_histogram_free_values (histogram);

      histogram->n_channels = bytes + 1;

      histogram->values[0] = g_new0 (gdouble *, histogram->n_channels);

      for (i = 0; i < histogram->n_channels; i++)
	histogram->values[0][i] = g_new (gdouble, 256);
    }
}

static void
gimp_histogram_free_values (GimpHistogram *histogram)
{
  gint i, j;

  for (i = 0; i < NUM_SLOTS; i++)
    if (histogram->values[i])
      {
        for (j = 0; j < histogram->n_channels; j++)
          g_free (histogram->values[i][j]);

        g_free (histogram->values[i]);
        histogram->values[i] = NULL;
      }

  histogram->n_channels = 0;
}

static void
gimp_histogram_calculate_sub_region (GimpHistogram *histogram,
				     PixelRegion   *region,
				     PixelRegion   *mask)
{
  const guchar *src, *msrc;
  const guchar *m, *s;
  gdouble **values;
  gint h, w, max;

#ifdef ENABLE_MP
  gint slot = 0;

  /* find an unused temporary slot to put our results in and lock it */
  g_static_mutex_lock (&histogram->mutex);

  while (histogram->slots[slot])
    slot++;

  values = histogram->values[slot];
  histogram->slots[slot] = 1;

  g_static_mutex_unlock (&histogram->mutex);

  if (! values)
    {
      gint j;

      histogram->values[slot] = g_new0 (gdouble *, histogram->n_channels);

      for (j = 0; j < histogram->n_channels; j++)
        histogram->values[slot][j] = g_new0 (gdouble, 256);

      values = histogram->values[slot];
    }

#else
  values = histogram->values[0];
#endif

  h = region->h;
  w = region->w;

  if (mask)
    {
      gdouble masked;

      src  = region->data;
      msrc = mask->data;

      while (h--)
	{
	  s = src;
	  m = msrc;
	  w = region->w;

	  switch (region->bytes)
	    {
	    case 1:
	      while (w--)
		{
		  masked = m[0] / 255.0;
		  values[0][s[0]] += masked;
		  s += 1;
		  m += 1;
		}
	      break;

	    case 2:
	      while (w--)
		{
		  masked = m[0] / 255.0;
		  values[0][s[0]] += masked;
		  values[1][s[1]] += masked;
		  s += 2;
		  m += 1;
		}
	      break;

	    case 3: /* calculate separate value values */
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
		}
	      break;

	    case 4: /* calculate separate value values */
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

		  s += 4;
		  m += 1;
		}
	      break;
	    }

	  src  += region->rowstride;
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
		}
	      break;

	    case 2:
	      while (w--)
		{
		  values[0][s[0]] += 1.0;
		  values[1][s[1]] += 1.0;
		  s += 2;
		}
	      break;

	    case 3: /* calculate separate value values */
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
		}
	      break;

	    case 4: /* calculate separate value values */
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

		  s += 4;
		}
	      break;
	    }

	  src += region->rowstride;
	}
    }

#ifdef ENABLE_MP
  /* unlock this slot */
  g_static_mutex_lock (&histogram->mutex);

  histogram->slots[slot] = 0;

  g_static_mutex_unlock (&histogram->mutex);
#endif
}
