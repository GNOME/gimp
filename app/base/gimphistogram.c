/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimphistogram module Copyright (C) 1999 Jay Cox <jaycox@gimp.org>
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

#include <string.h>

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
  gdouble       *values[NUM_SLOTS];
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
  GimpHistogram *histogram = g_slice_new0 (GimpHistogram);

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
  g_slice_free (GimpHistogram, histogram);
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
      memset (histogram->values[i],
              0, histogram->n_channels * 256 * sizeof (gdouble));

  pixel_regions_process_parallel ((PixelProcessorFunc)
                                  gimp_histogram_calculate_sub_region,
                                  histogram, 2, region, mask);

#ifdef ENABLE_MP
  /* add up all slots */
  for (i = 1; i < NUM_SLOTS; i++)
    if (histogram->values[i])
      {
        gint j;

        for (j = 0; j < histogram->n_channels * 256; j++)
          histogram->values[0][j] += histogram->values[i][j];
      }
#endif
}


#define HISTOGRAM_VALUE(c,i) (histogram->values[0][(c) * 256 + (i)])


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

  if (! histogram->values[0] ||
      (channel != GIMP_HISTOGRAM_RGB && channel >= histogram->n_channels))
    return 0.0;

  if (channel == GIMP_HISTOGRAM_RGB)
    for (x = 0; x < 256; x++)
      {
        max = MAX (max, HISTOGRAM_VALUE (GIMP_HISTOGRAM_RED,   x));
        max = MAX (max, HISTOGRAM_VALUE (GIMP_HISTOGRAM_GREEN, x));
        max = MAX (max, HISTOGRAM_VALUE (GIMP_HISTOGRAM_BLUE,  x));
      }
  else
    for (x = 0; x < 256; x++)
      {
        max = MAX (max, HISTOGRAM_VALUE (channel, x));
      }

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

  if (! histogram->values[0] ||
      bin < 0 || bin >= 256 ||
      (channel == GIMP_HISTOGRAM_RGB && histogram->n_channels < 4) ||
      (channel != GIMP_HISTOGRAM_RGB && channel >= histogram->n_channels))
    return 0.0;

  if (channel == GIMP_HISTOGRAM_RGB)
    {
      gdouble min = HISTOGRAM_VALUE (GIMP_HISTOGRAM_RED, bin);

      min = MIN (min, HISTOGRAM_VALUE (GIMP_HISTOGRAM_GREEN, bin));

      return MIN (min, HISTOGRAM_VALUE (GIMP_HISTOGRAM_BLUE, bin));
    }
  else
    {
      return HISTOGRAM_VALUE (channel, bin);
    }
}

gdouble
gimp_histogram_get_channel (GimpHistogram        *histogram,
                            GimpHistogramChannel  channel,
                            gint                  bin)
{
  g_return_val_if_fail (histogram != NULL, 0.0);

  if (histogram->n_channels > 3)
    channel++;

  return gimp_histogram_get_value (histogram, channel, bin);
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

  if (! histogram->values[0] ||
      start > end ||
      channel >= histogram->n_channels)
    return 0.0;

  start = CLAMP (start, 0, 255);
  end   = CLAMP (end, 0, 255);

  for (i = start; i <= end; i++)
    count += HISTOGRAM_VALUE (channel, i);

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

  if (! histogram->values[0] ||
      start > end ||
      (channel == GIMP_HISTOGRAM_RGB && histogram->n_channels < 4) ||
      (channel != GIMP_HISTOGRAM_RGB && channel >= histogram->n_channels))
    return 0.0;

  start = CLAMP (start, 0, 255);
  end = CLAMP (end, 0, 255);

  if (channel == GIMP_HISTOGRAM_RGB)
    {
      for (i = start; i <= end; i++)
        mean += (i * HISTOGRAM_VALUE (GIMP_HISTOGRAM_RED,   i) +
                 i * HISTOGRAM_VALUE (GIMP_HISTOGRAM_GREEN, i) +
                 i * HISTOGRAM_VALUE (GIMP_HISTOGRAM_BLUE,  i));
    }
  else
    {
      for (i = start; i <= end; i++)
        mean += i * HISTOGRAM_VALUE (channel, i);
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

  if (! histogram->values[0] ||
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
        sum += (HISTOGRAM_VALUE (GIMP_HISTOGRAM_RED,   i) +
                HISTOGRAM_VALUE (GIMP_HISTOGRAM_GREEN, i) +
                HISTOGRAM_VALUE (GIMP_HISTOGRAM_BLUE,  i));

        if (sum * 2 > count)
          return i;
      }
  else
    for (i = start; i <= end; i++)
      {
        sum += HISTOGRAM_VALUE (channel, i);

        if (sum * 2 > count)
          return i;
      }

  return -1;
}

/*
 * adapted from GNU ocrad 0.14 : page_image_io.cc : otsu_th
 *
 *  N. Otsu, "A threshold selection method from gray-level histograms,"
 *  IEEE Trans. Systems, Man, and Cybernetics, vol. 9, no. 1, pp. 62-66, 1979.
 */
gdouble
gimp_histogram_get_threshold (GimpHistogram        *histogram,
                              GimpHistogramChannel  channel,
                              gint                  start,
                              gint                  end)
{
  gint     i;
  gint     maxval;
  gdouble *hist      = NULL;
  gdouble *chist     = NULL;
  gdouble *cmom      = NULL;
  gdouble  hist_max  = 0.0;
  gdouble  chist_max = 0.0;
  gdouble  cmom_max  = 0.0;
  gdouble  bvar_max  = 0.0;
  gint     threshold = 127;

  g_return_val_if_fail (histogram != NULL, -1);

  /*  the gray alpha channel is in slot 1  */
  if (histogram->n_channels == 3 && channel == GIMP_HISTOGRAM_ALPHA)
    channel = 1;

  if (! histogram->values[0] ||
      start > end ||
      (channel == GIMP_HISTOGRAM_RGB && histogram->n_channels < 4) ||
      (channel != GIMP_HISTOGRAM_RGB && channel >= histogram->n_channels))
    return 0;

  start = CLAMP (start, 0, 255);
  end = CLAMP (end, 0, 255);

  maxval = end - start;

  hist  = g_newa (gdouble, maxval + 1);
  chist = g_newa (gdouble, maxval + 1);
  cmom  = g_newa (gdouble, maxval + 1);

  if (channel == GIMP_HISTOGRAM_RGB)
    {
      for (i = start; i <= end; i++)
        hist[i - start] = (HISTOGRAM_VALUE (GIMP_HISTOGRAM_RED,   i) +
                           HISTOGRAM_VALUE (GIMP_HISTOGRAM_GREEN, i) +
                           HISTOGRAM_VALUE (GIMP_HISTOGRAM_BLUE,  i));
    }
  else
    {
      for (i = start; i <= end; i++)
        hist[i - start] = HISTOGRAM_VALUE (channel, i);
    }

  hist_max = hist[0];
  chist[0] = hist[0];
  cmom[0] = 0;

  for (i = 1; i <= maxval; i++)
    {
      if (hist[i] > hist_max)
	hist_max = hist[i];

      chist[i] = chist[i-1] + hist[i];
      cmom[i] = cmom[i-1] + i * hist[i];
    }

  chist_max = chist[maxval];
  cmom_max = cmom[maxval];
  bvar_max = 0;

  for (i = 0; i < maxval; ++i)
    if (chist[i] > 0 && chist[i] < chist_max)
      {
	gdouble bvar;

	bvar = (gdouble) cmom[i] / chist[i];
	bvar -= (cmom_max - cmom[i]) / (chist_max - chist[i]);
	bvar *= bvar;
	bvar *= chist[i];
	bvar *= chist_max - chist[i];

	if (bvar > bvar_max)
	  {
	    bvar_max = bvar;
	    threshold = start + i;
	  }
      }

  return threshold;
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

  if (! histogram->values[0] ||
      start > end ||
      (channel == GIMP_HISTOGRAM_RGB && histogram->n_channels < 4) ||
      (channel != GIMP_HISTOGRAM_RGB && channel >= histogram->n_channels))
    return 0.0;

  mean  = gimp_histogram_get_mean  (histogram, channel, start, end);
  count = gimp_histogram_get_count (histogram, channel, start, end);

  if (count == 0.0)
    count = 1.0;

  for (i = start; i <= end; i++)
    {
      gdouble value;

      if (channel == GIMP_HISTOGRAM_RGB)
        {
          value = (HISTOGRAM_VALUE (GIMP_HISTOGRAM_RED,   i) +
                   HISTOGRAM_VALUE (GIMP_HISTOGRAM_GREEN, i) +
                   HISTOGRAM_VALUE (GIMP_HISTOGRAM_BLUE,  i));
        }
      else
        {
          value = gimp_histogram_get_value (histogram, channel, i);
        }

      dev += value * SQR (i - mean);
    }

  return sqrt (dev / count);
}


/*  private functions  */

static void
gimp_histogram_alloc_values (GimpHistogram *histogram,
                             gint           bytes)
{
  if (bytes + 1 != histogram->n_channels)
    {
      gimp_histogram_free_values (histogram);

      histogram->n_channels = bytes + 1;

      histogram->values[0] = g_new (gdouble, histogram->n_channels * 256);
    }
}

static void
gimp_histogram_free_values (GimpHistogram *histogram)
{
  gint i;

  for (i = 0; i < NUM_SLOTS; i++)
    if (histogram->values[i])
      {
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
  gdouble      *values;
  gint          h, w, max;

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
      histogram->values[slot] = g_new0 (gdouble, histogram->n_channels * 256);
      values = histogram->values[slot];
    }

#else
  values = histogram->values[0];
#endif

#define VALUE(c,i) (values[(c) * 256 + (i)])

  h = region->h;
  w = region->w;

  if (mask)
    {
      src  = region->data;
      msrc = mask->data;

      switch (region->bytes)
        {
        case 1:
          while (h--)
            {
              s = src;
              m = msrc;
              w = region->w;

              while (w--)
                {
                  const gdouble masked = m[0] / 255.0;

                  VALUE (0, s[0]) += masked;

                  s += 1;
                  m += 1;
                }

              src  += region->rowstride;
              msrc += mask->rowstride;
            }
          break;

        case 2:
          while (h--)
            {
              s = src;
              m = msrc;
              w = region->w;

              while (w--)
                {
                  const gdouble masked = m[0] / 255.0;
                  const gdouble weight = s[1] / 255.0;

                  VALUE (0, s[0]) += weight * masked;
                  VALUE (1, s[1]) += masked;

                  s += 2;
                  m += 1;
                }

              src  += region->rowstride;
              msrc += mask->rowstride;
            }
          break;

        case 3: /* calculate separate value values */
          while (h--)
            {
              s = src;
              m = msrc;
              w = region->w;

              while (w--)
                {
                  const gdouble masked = m[0] / 255.0;

                  VALUE (1, s[0]) += masked;
                  VALUE (2, s[1]) += masked;
                  VALUE (3, s[2]) += masked;

                  max = (s[0] > s[1]) ? s[0] : s[1];

                  if (s[2] > max)
                    VALUE (0, s[2]) += masked;
                  else
                    VALUE (0, max) += masked;

                  s += 3;
                  m += 1;
                }

              src  += region->rowstride;
              msrc += mask->rowstride;
            }
          break;

        case 4: /* calculate separate value values */
          while (h--)
            {
              s = src;
              m = msrc;
              w = region->w;

              while (w--)
                {
                  const gdouble masked = m[0] / 255.0;
                  const gdouble weight = s[3] / 255.0;

                  VALUE (1, s[0]) += weight * masked;
                  VALUE (2, s[1]) += weight * masked;
                  VALUE (3, s[2]) += weight * masked;
                  VALUE (4, s[3]) += masked;

                  max = (s[0] > s[1]) ? s[0] : s[1];

                  if (s[2] > max)
                   VALUE (0, s[2]) += weight * masked;
                  else
                    VALUE (0, max) += weight * masked;

                  s += 4;
                  m += 1;
                }

              src  += region->rowstride;
              msrc += mask->rowstride;
            }
          break;
        }
    }
  else /* no mask */
    {
      src = region->data;

      switch (region->bytes)
        {
        case 1:
          while (h--)
            {
              s = src;
              w = region->w;

              while (w--)
                {
                  VALUE (0, s[0]) += 1.0;

                  s += 1;
                }

              src += region->rowstride;
            }
          break;

        case 2:
          while (h--)
            {
              s = src;
              w = region->w;

              while (w--)
                {
                  const gdouble weight = s[1] / 255;

                  VALUE (0, s[0]) += weight;
                  VALUE (1, s[1]) += 1.0;

                  s += 2;
                }

              src += region->rowstride;
            }
          break;

        case 3: /* calculate separate value values */
          while (h--)
            {
              s = src;
              w = region->w;

              while (w--)
                {
                  VALUE (1, s[0]) += 1.0;
                  VALUE (2, s[1]) += 1.0;
                  VALUE (3, s[2]) += 1.0;

                  max = (s[0] > s[1]) ? s[0] : s[1];

                  if (s[2] > max)
                    VALUE (0, s[2]) += 1.0;
                  else
                    VALUE (0, max) += 1.0;

                  s += 3;
                }

              src += region->rowstride;
            }
          break;

        case 4: /* calculate separate value values */
          while (h--)
            {
              s = src;
              w = region->w;

              while (w--)
                {
                  const gdouble weight = s[3] / 255;

                  VALUE (1, s[0]) += weight;
                  VALUE (2, s[1]) += weight;
                  VALUE (3, s[2]) += weight;
                  VALUE (4, s[3]) += 1.0;

                  max = (s[0] > s[1]) ? s[0] : s[1];

                  if (s[2] > max)
                    VALUE (0, s[2]) += weight;
                  else
                    VALUE (0, max) += weight;

                  s += 4;
                }

              src += region->rowstride;
            }
          break;
        }
    }

#ifdef ENABLE_MP
  /* unlock this slot */
  g_static_mutex_lock (&histogram->mutex);

  histogram->slots[slot] = 0;

  g_static_mutex_unlock (&histogram->mutex);
#endif
}
