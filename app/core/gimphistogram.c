/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimphistogram module Copyright (C) 1999 Jay Cox <jaycox@gimp.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gegl.h>

#include "libgimpmath/gimpmath.h"

#include "core-types.h"

#include "gegl/gimp-babl.h"

#include "gimphistogram.h"


struct _GimpHistogram
{
  gint     ref_count;
  gint     n_channels;
  gint     n_bins;
  gdouble *values;
};


/*  local function prototypes  */

static void   gimp_histogram_alloc_values (GimpHistogram *histogram,
                                           gint           n_components,
                                           gint           n_bins);


/*  public functions  */

GimpHistogram *
gimp_histogram_new (void)
{
  GimpHistogram *histogram = g_slice_new0 (GimpHistogram);

  histogram->ref_count = 1;
  histogram->n_bins    = 256;

  return histogram;
}

GimpHistogram *
gimp_histogram_ref (GimpHistogram *histogram)
{
  g_return_val_if_fail (histogram != NULL, NULL);

  histogram->ref_count++;

  return histogram;
}

void
gimp_histogram_unref (GimpHistogram *histogram)
{
  g_return_if_fail (histogram != NULL);

  histogram->ref_count--;

  if (histogram->ref_count == 0)
    {
      gimp_histogram_clear_values (histogram);
      g_slice_free (GimpHistogram, histogram);
    }
}

/**
 * gimp_histogram_duplicate:
 * @histogram: a %GimpHistogram
 *
 * Creates a duplicate of @histogram. The duplicate has a reference
 * count of 1 and contains the values from @histogram.
 *
 * Return value: a newly allocated %GimpHistogram
 **/
GimpHistogram *
gimp_histogram_duplicate (GimpHistogram *histogram)
{
  GimpHistogram *dup;

  g_return_val_if_fail (histogram != NULL, NULL);

  dup = gimp_histogram_new ();

  dup->n_channels = histogram->n_channels;
  dup->n_bins     = histogram->n_bins;
  dup->values     = g_memdup (histogram->values,
                              sizeof (gdouble) * dup->n_channels * dup->n_bins);

  return dup;
}

void
gimp_histogram_calculate (GimpHistogram       *histogram,
                          GeglBuffer          *buffer,
                          const GeglRectangle *buffer_rect,
                          GeglBuffer          *mask,
                          const GeglRectangle *mask_rect)
{
  GeglBufferIterator *iter;
  const Babl         *format;
  gint                n_components;
  gint                n_bins;

  g_return_if_fail (histogram != NULL);
  g_return_if_fail (GEGL_IS_BUFFER (buffer));
  g_return_if_fail (buffer_rect != NULL);

  format = gegl_buffer_get_format (buffer);

  if (babl_format_get_type (format, 0) == babl_type ("u8"))
    n_bins = 256;
  else
    n_bins = 1024;

  if (babl_format_is_palette (format))
    {
      if (babl_format_has_alpha (format))
        format = babl_format ("R'G'B'A float");
      else
        format = babl_format ("R'G'B' float");
    }
  else
    {
      const Babl *model = babl_format_get_model (format);

      if (model == babl_model ("Y"))
        {
          format = babl_format ("Y float");
        }
      else if (model == babl_model ("Y'"))
        {
          format = babl_format ("Y' float");
        }
      else if (model == babl_model ("YA"))
        {
          format = babl_format ("YA float");
        }
      else if (model == babl_model ("Y'A"))
        {
          format = babl_format ("Y'A float");
        }
      else if (model == babl_model ("RGB"))
        {
          format = babl_format ("RGB float");
        }
      else if (model == babl_model ("R'G'B'"))
        {
          format = babl_format ("R'G'B' float");
        }
      else if (model == babl_model ("RGBA"))
        {
          format = babl_format ("RGBA float");
        }
      else if (model == babl_model ("R'G'B'A"))
        {
          format = babl_format ("R'G'B'A float");
        }
      else
        {
          g_return_if_reached ();
        }
    }

  n_components = babl_format_get_n_components (format);

  gimp_histogram_alloc_values (histogram, n_components, n_bins);

  iter = gegl_buffer_iterator_new (buffer, buffer_rect, 0, format,
                                   GEGL_BUFFER_READ, GEGL_ABYSS_NONE);

  if (mask)
    gegl_buffer_iterator_add (iter, mask, mask_rect, 0,
                              babl_format ("Y float"),
                              GEGL_BUFFER_READ, GEGL_ABYSS_NONE);

#define VALUE(c,i) (histogram->values[(c) * histogram->n_bins + (gint) ((i) * (histogram->n_bins - 0.0001))])

  while (gegl_buffer_iterator_next (iter))
    {
      const gfloat *data = iter->data[0];
      gfloat        max;

      if (mask)
        {
          const gfloat *mask_data = iter->data[1];

          switch (n_components)
            {
            case 1:
              while (iter->length--)
                {
                  const gdouble masked = *mask_data;

                  VALUE (0, data[0]) += masked;

                  data += n_components;
                  mask_data += 1;
                }
              break;

            case 2:
              while (iter->length--)
                {
                  const gdouble masked = *mask_data;
                  const gdouble weight = data[1];

                  VALUE (0, data[0]) += weight * masked;
                  VALUE (1, data[1]) += masked;

                  data += n_components;
                  mask_data += 1;
                }
              break;

            case 3: /* calculate separate value values */
              while (iter->length--)
                {
                  const gdouble masked = *mask_data;

                  VALUE (1, data[0]) += masked;
                  VALUE (2, data[1]) += masked;
                  VALUE (3, data[2]) += masked;

                  max = MAX (data[0], data[1]);
                  max = MAX (data[2], max);

                  VALUE (0, max) += masked;

                  data += n_components;
                  mask_data += 1;
                }
              break;

            case 4: /* calculate separate value values */
              while (iter->length--)
                {
                  const gdouble masked = *mask_data;
                  const gdouble weight = data[3];

                  VALUE (1, data[0]) += weight * masked;
                  VALUE (2, data[1]) += weight * masked;
                  VALUE (3, data[2]) += weight * masked;
                  VALUE (4, data[3]) += masked;

                  max = MAX (data[0], data[1]);
                  max = MAX (data[2], max);

                  VALUE (0, max) += weight * masked;

                  data += n_components;
                  mask_data += 1;
                }
              break;
            }
        }
      else /* no mask */
        {
          switch (n_components)
            {
            case 1:
              while (iter->length--)
                {
                  VALUE (0, data[0]) += 1.0;

                  data += n_components;
                }
              break;

            case 2:
              while (iter->length--)
                {
                  const gdouble weight = data[1];

                  VALUE (0, data[0]) += weight;
                  VALUE (1, data[1]) += 1.0;

                  data += n_components;
                }
              break;

            case 3: /* calculate separate value values */
              while (iter->length--)
                {
                  VALUE (1, data[0]) += 1.0;
                  VALUE (2, data[1]) += 1.0;
                  VALUE (3, data[2]) += 1.0;

                  max = MAX (data[0], data[1]);
                  max = MAX (data[2], max);

                  VALUE (0, max) += 1.0;

                  data += n_components;
                }
              break;

            case 4: /* calculate separate value values */
              while (iter->length--)
                {
                  const gdouble weight = data[3];

                  VALUE (1, data[0]) += weight;
                  VALUE (2, data[1]) += weight;
                  VALUE (3, data[2]) += weight;
                  VALUE (4, data[3]) += 1.0;

                  max = MAX (data[0], data[1]);
                  max = MAX (data[2], max);

                  VALUE (0, max) += weight;

                  data += n_components;
                }
              break;
            }
        }
    }

#undef VALUE
}

void
gimp_histogram_clear_values (GimpHistogram *histogram)
{
  g_return_if_fail (histogram != NULL);

  if (histogram->values)
    {
      g_free (histogram->values);
      histogram->values = NULL;
    }

  histogram->n_channels = 0;
}


#define HISTOGRAM_VALUE(c,i) (histogram->values[(c) * histogram->n_bins + (i)])


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
    {
      for (x = 0; x < histogram->n_bins; x++)
        {
          max = MAX (max, HISTOGRAM_VALUE (GIMP_HISTOGRAM_RED,   x));
          max = MAX (max, HISTOGRAM_VALUE (GIMP_HISTOGRAM_GREEN, x));
          max = MAX (max, HISTOGRAM_VALUE (GIMP_HISTOGRAM_BLUE,  x));
        }
    }
  else
    {
      for (x = 0; x < histogram->n_bins; x++)
        {
          max = MAX (max, HISTOGRAM_VALUE (channel, x));
        }
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

  if (! histogram->values ||
      bin < 0 || bin >= histogram->n_bins ||
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

gint
gimp_histogram_n_bins (GimpHistogram *histogram)
{
  g_return_val_if_fail (histogram != NULL, 0);

  return histogram->n_bins;
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

  start = CLAMP (start, 0, histogram->n_bins - 1);
  end   = CLAMP (end,   0, histogram->n_bins - 1);

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

  if (! histogram->values ||
      start > end ||
      (channel == GIMP_HISTOGRAM_RGB && histogram->n_channels < 4) ||
      (channel != GIMP_HISTOGRAM_RGB && channel >= histogram->n_channels))
    return 0.0;

  start = CLAMP (start, 0, histogram->n_bins - 1);
  end   = CLAMP (end,   0, histogram->n_bins - 1);

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

  if (! histogram->values ||
      start > end ||
      (channel == GIMP_HISTOGRAM_RGB && histogram->n_channels < 4) ||
      (channel != GIMP_HISTOGRAM_RGB && channel >= histogram->n_channels))
    return 0;

  start = CLAMP (start, 0, histogram->n_bins - 1);
  end   = CLAMP (end,   0, histogram->n_bins - 1);

  count = gimp_histogram_get_count (histogram, channel, start, end);

  if (channel == GIMP_HISTOGRAM_RGB)
    {
      for (i = start; i <= end; i++)
        {
          sum += (HISTOGRAM_VALUE (GIMP_HISTOGRAM_RED,   i) +
                  HISTOGRAM_VALUE (GIMP_HISTOGRAM_GREEN, i) +
                  HISTOGRAM_VALUE (GIMP_HISTOGRAM_BLUE,  i));

          if (sum * 2 > count)
            return i;
        }
    }
  else
    {
      for (i = start; i <= end; i++)
        {
          sum += HISTOGRAM_VALUE (channel, i);

          if (sum * 2 > count)
            return i;
        }
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

  if (! histogram->values ||
      start > end ||
      (channel == GIMP_HISTOGRAM_RGB && histogram->n_channels < 4) ||
      (channel != GIMP_HISTOGRAM_RGB && channel >= histogram->n_channels))
    return 0;

  start = CLAMP (start, 0, histogram->n_bins - 1);
  end   = CLAMP (end,   0, histogram->n_bins - 1);

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
    {
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
                             gint           n_components,
                             gint           n_bins)
{
  if (n_components + 1 != histogram->n_channels ||
      n_bins != histogram->n_bins)
    {
      gimp_histogram_clear_values (histogram);

      histogram->n_channels = n_components + 1;
      histogram->n_bins     = n_bins;

      histogram->values = g_new0 (gdouble,
                                  histogram->n_channels * histogram->n_bins);
    }
  else
    {
      memset (histogram->values, 0,
              histogram->n_channels * histogram->n_bins * sizeof (gdouble));
    }
}
