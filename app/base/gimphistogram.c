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

#ifdef ENABLE_MP
#include <pthread.h>
#endif

#include <glib-object.h>

#include "libgimpmath/gimpmath.h"

#include "base-types.h"

#include "config/gimpbaseconfig.h"

#include "gimphistogram.h"
#include "pixel-processor.h"
#include "pixel-region.h"


struct _GimpHistogram
{
  gint               bins;
  gdouble          **values;
  gint               n_channels;

#ifdef ENABLE_MP
  pthread_mutex_t    mutex;
  gint               num_slots;
  gdouble         ***tmp_values;
  gchar             *tmp_slots;
#endif
};


/*  local function prototypes  */

static void   gimp_histogram_alloc_values         (GimpHistogram *histogram,
                                                   gint           bytes);
static void   gimp_histogram_free_values          (GimpHistogram *histogram);
static void   gimp_histogram_calculate_sub_region (GimpHistogram *histogram,
                                                   PixelRegion   *region,
                                                   PixelRegion   *mask);


/*  public functions  */

GimpHistogram *
gimp_histogram_new (GimpBaseConfig *config)
{
  GimpHistogram *histogram;

  g_return_val_if_fail (GIMP_IS_BASE_CONFIG (config), NULL);

  histogram = g_new0 (GimpHistogram, 1);

  histogram->bins       = 0;
  histogram->values     = NULL;
  histogram->n_channels = 0;

#ifdef ENABLE_MP
  pthread_mutex_init (&histogram->mutex, NULL);

  histogram->num_slots  = config->num_processors;
  histogram->tmp_slots  = g_new0 (gchar,      histogram->num_slots);
  histogram->tmp_values = g_new0 (gdouble **, histogram->num_slots);
#endif /* ENABLE_MP */

  return histogram;
}

void
gimp_histogram_free (GimpHistogram *histogram)
{
  g_return_if_fail (histogram != NULL);

#ifdef ENABLE_MP
  pthread_mutex_destroy (&histogram->mutex);

  g_free (histogram->tmp_values);
  g_free (histogram->tmp_slots);
#endif

  gimp_histogram_free_values (histogram);
  g_free (histogram);
}

void
gimp_histogram_calculate (GimpHistogram *histogram,
			  PixelRegion   *region,
			  PixelRegion   *mask)
{
  gint i, j;

  g_return_if_fail (histogram != NULL);

  if (! region)
    {
      gimp_histogram_free_values (histogram);
      return;
    }

  gimp_histogram_alloc_values (histogram, region->bytes);

#ifdef ENABLE_MP
  for (i = 0; i < histogram->num_slots; i++)
    {
      histogram->tmp_values[i] = g_new0 (gdouble *, histogram->n_channels);
      histogram->tmp_slots[i]  = 0;

      for (j = 0; j < histogram->n_channels; j++)
        {
          gint k;

          histogram->tmp_values[i][j] = g_new0 (gdouble, 256);

          for (k = 0; k < 256; k++)
            histogram->tmp_values[i][j][k] = 0.0;
        }
    }
#endif

  for (i = 0; i < histogram->n_channels; i++)
    for (j = 0; j < 256; j++)
      histogram->values[i][j] = 0.0;

  pixel_regions_process_parallel ((p_func) gimp_histogram_calculate_sub_region,
                                  histogram, 2, region, mask);

#ifdef ENABLE_MP
  /* add up all the tmp buffers and free their memmory */
  for (i = 0; i < histogram->num_slots; i++)
    {
      for (j = 0; j < histogram->n_channels; j++)
        {
          gint k;

          for (k = 0; k < 256; k++)
            histogram->values[j][k] += histogram->tmp_values[i][j][k];

          g_free (histogram->tmp_values[i][j]);
        }

      g_free (histogram->tmp_values[i]);
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
	max = MAX (max, histogram->values[GIMP_HISTOGRAM_RED][x]);
	max = MAX (max, histogram->values[GIMP_HISTOGRAM_GREEN][x]);
	max = MAX (max, histogram->values[GIMP_HISTOGRAM_BLUE][x]);
      }
  else
    for (x = 0; x < 256; x++)
      if (histogram->values[channel][x] > max)
	max = histogram->values[channel][x];

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
      gdouble min = histogram->values[GIMP_HISTOGRAM_RED][bin];

      min = MIN (min, histogram->values[GIMP_HISTOGRAM_GREEN][bin]);

      return MIN (min, histogram->values[GIMP_HISTOGRAM_BLUE][bin]);
    }
  else
    return histogram->values[channel][bin];
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
    count += histogram->values[channel][i];

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
	mean += (i * histogram->values[GIMP_HISTOGRAM_RED][i]   +
		 i * histogram->values[GIMP_HISTOGRAM_GREEN][i] +
		 i * histogram->values[GIMP_HISTOGRAM_BLUE][i]);
    }
  else
    {
      for (i = start; i <= end; i++)
	mean += i * histogram->values[channel][i];
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
	sum += (histogram->values[GIMP_HISTOGRAM_RED][i]   +
		histogram->values[GIMP_HISTOGRAM_GREEN][i] +
		histogram->values[GIMP_HISTOGRAM_BLUE][i]);

	if (sum * 2 > count)
	  return i;
      }
  else
    for (i = start; i <= end; i++)
      {
	sum += histogram->values[channel][i];

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
      histogram->values     = g_new0 (gdouble *, histogram->n_channels);

      for (i = 0; i < histogram->n_channels; i++)
	histogram->values[i] = g_new (gdouble, 256);
    }
}

static void
gimp_histogram_free_values (GimpHistogram *histogram)
{
  gint i;

  if (histogram->values)
    {
      for (i = 0; i < histogram->n_channels; i++)
        g_free (histogram->values[i]);

      g_free (histogram->values);

      histogram->values     = NULL;
      histogram->n_channels = 0;
    }
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
  pthread_mutex_lock (&histogram->mutex);

  while (histogram->tmp_slots[slot])
    slot++;

  values = histogram->tmp_values[slot];
  histogram->tmp_slots[slot] = 1;

  pthread_mutex_unlock (&histogram->mutex);

#else
  values = histogram->values;
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
  /* we shouldn't have to use mutex locks here */
  pthread_mutex_lock (&histogram->mutex);
  histogram->tmp_slots[slot] = 0;
  pthread_mutex_unlock (&histogram->mutex);
#endif
}
