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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>
#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpcolor/gimpcolor.h"

#include "core-types.h"

#include "gegl/gimp-babl.h"
#include "gegl/gimp-gegl-loops.h"

#include "gimp-atomic.h"
#include "gimp-parallel.h"
#include "gimpasync.h"
#include "gimphistogram.h"
#include "gimpwaitable.h"


#define MAX_N_COMPONENTS   4
#define N_DERIVED_CHANNELS 2

#define PIXELS_PER_THREAD \
  (/* each thread costs as much as */ 64.0 * 64.0 /* pixels */)


enum
{
  PROP_0,
  PROP_N_COMPONENTS,
  PROP_N_BINS,
  PROP_VALUES
};

struct _GimpHistogramPrivate
{
  GimpTRCType  trc;
  gint         n_channels;
  gint         n_bins;
  gdouble     *values;
  GimpAsync   *calculate_async;
};

typedef struct
{
  /*  input  */
  GimpHistogram *histogram;
  GeglBuffer    *buffer;
  GeglRectangle  buffer_rect;
  GeglBuffer    *mask;
  GeglRectangle  mask_rect;

  /*  output  */
  gint           n_components;
  gint           n_bins;
  gdouble       *values;
} CalculateContext;

typedef struct
{
  GimpAsync        *async;
  CalculateContext *context;

  const Babl       *format;
  GSList           *values_list;
} CalculateData;


/*  local function prototypes  */

static void       gimp_histogram_finalize                 (GObject              *object);
static void       gimp_histogram_set_property             (GObject              *object,
                                                           guint                 property_id,
                                                           const GValue         *value,
                                                           GParamSpec           *pspec);
static void       gimp_histogram_get_property             (GObject              *object,
                                                           guint                 property_id,
                                                           GValue               *value,
                                                           GParamSpec           *pspec);

static gint64     gimp_histogram_get_memsize              (GimpObject           *object,
                                                           gint64               *gui_size);

static gboolean   gimp_histogram_map_channel              (GimpHistogram        *histogram,
                                                           GimpHistogramChannel *channel);

static void       gimp_histogram_set_values               (GimpHistogram        *histogram,
                                                           gint                  n_components,
                                                           gint                  n_bins,
                                                           gdouble              *values);

static void       gimp_histogram_calculate_internal       (GimpAsync            *async,
                                                           CalculateContext     *context);
static void       gimp_histogram_calculate_area           (const GeglRectangle  *area,
                                                           CalculateData        *data);
static void       gimp_histogram_calculate_async_callback (GimpAsync            *async,
                                                           CalculateContext     *context);


G_DEFINE_TYPE_WITH_PRIVATE (GimpHistogram, gimp_histogram, GIMP_TYPE_OBJECT)

#define parent_class gimp_histogram_parent_class


static void
gimp_histogram_class_init (GimpHistogramClass *klass)
{
  GObjectClass      *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass   *gimp_object_class = GIMP_OBJECT_CLASS (klass);

  object_class->finalize         = gimp_histogram_finalize;
  object_class->set_property     = gimp_histogram_set_property;
  object_class->get_property     = gimp_histogram_get_property;

  gimp_object_class->get_memsize = gimp_histogram_get_memsize;

  g_object_class_install_property (object_class, PROP_N_COMPONENTS,
                                   g_param_spec_int ("n-components", NULL, NULL,
                                                     0, MAX_N_COMPONENTS, 0,
                                                     GIMP_PARAM_READABLE));

  g_object_class_install_property (object_class, PROP_N_BINS,
                                   g_param_spec_int ("n-bins", NULL, NULL,
                                                     256, 1024, 1024,
                                                     GIMP_PARAM_READABLE));

  /* this is just for notifications */
  g_object_class_install_property (object_class, PROP_VALUES,
                                   g_param_spec_boolean ("values", NULL, NULL,
                                                         FALSE,
                                                         G_PARAM_READABLE));
}

static void
gimp_histogram_init (GimpHistogram *histogram)
{
  histogram->priv = gimp_histogram_get_instance_private (histogram);
}

static void
gimp_histogram_finalize (GObject *object)
{
  GimpHistogram *histogram = GIMP_HISTOGRAM (object);

  gimp_histogram_clear_values (histogram, 0);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_histogram_set_property (GObject      *object,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_histogram_get_property (GObject    *object,
                             guint       property_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  GimpHistogram *histogram = GIMP_HISTOGRAM (object);

  switch (property_id)
    {
    case PROP_N_COMPONENTS:
      g_value_set_int (value, gimp_histogram_n_components (histogram));
      break;

    case PROP_N_BINS:
      g_value_set_int (value, histogram->priv->n_bins);
      break;

    case PROP_VALUES:
      /* return a silly boolean */
      g_value_set_boolean (value, histogram->priv->values != NULL);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gint64
gimp_histogram_get_memsize (GimpObject *object,
                            gint64     *gui_size)
{
  GimpHistogram *histogram = GIMP_HISTOGRAM (object);
  gint64         memsize   = 0;

  if (histogram->priv->values)
    memsize += (histogram->priv->n_channels *
                histogram->priv->n_bins * sizeof (gdouble));

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

/*  public functions  */

GimpHistogram *
gimp_histogram_new (GimpTRCType trc)
{
  GimpHistogram *histogram = g_object_new (GIMP_TYPE_HISTOGRAM, NULL);

  histogram->priv->trc = trc;

  return histogram;
}

/**
 * gimp_histogram_duplicate:
 * @histogram: a %GimpHistogram
 *
 * Creates a duplicate of @histogram. The duplicate has a reference
 * count of 1 and contains the values from @histogram.
 *
 * Returns: a newly allocated %GimpHistogram
 **/
GimpHistogram *
gimp_histogram_duplicate (GimpHistogram *histogram)
{
  GimpHistogram *dup;

  g_return_val_if_fail (GIMP_IS_HISTOGRAM (histogram), NULL);

  if (histogram->priv->calculate_async)
    gimp_waitable_wait (GIMP_WAITABLE (histogram->priv->calculate_async));

  dup = gimp_histogram_new (histogram->priv->trc);

  dup->priv->n_channels = histogram->priv->n_channels;
  dup->priv->n_bins     = histogram->priv->n_bins;
  dup->priv->values     = g_memdup2 (histogram->priv->values,
                                     sizeof (gdouble) *
                                     dup->priv->n_channels *
                                     dup->priv->n_bins);

  return dup;
}

void
gimp_histogram_calculate (GimpHistogram       *histogram,
                          GeglBuffer          *buffer,
                          const GeglRectangle *buffer_rect,
                          GeglBuffer          *mask,
                          const GeglRectangle *mask_rect)
{
  CalculateContext context = {};

  g_return_if_fail (GIMP_IS_HISTOGRAM (histogram));
  g_return_if_fail (GEGL_IS_BUFFER (buffer));
  g_return_if_fail (buffer_rect != NULL);

  if (histogram->priv->calculate_async)
    gimp_async_cancel_and_wait (histogram->priv->calculate_async);

  context.histogram   = histogram;
  context.buffer      = buffer;
  context.buffer_rect = *buffer_rect;

  if (mask)
    {
      context.mask = mask;

      if (mask_rect)
        context.mask_rect = *mask_rect;
      else
        context.mask_rect = *gegl_buffer_get_extent (mask);
    }

  gimp_histogram_calculate_internal (NULL, &context);

  gimp_histogram_set_values (histogram,
                             context.n_components, context.n_bins,
                             context.values);
}

GimpAsync *
gimp_histogram_calculate_async (GimpHistogram       *histogram,
                                GeglBuffer          *buffer,
                                const GeglRectangle *buffer_rect,
                                GeglBuffer          *mask,
                                const GeglRectangle *mask_rect)
{
  CalculateContext *context;
  GeglRectangle     rect;

  g_return_val_if_fail (GIMP_IS_HISTOGRAM (histogram), NULL);
  g_return_val_if_fail (GEGL_IS_BUFFER (buffer), NULL);
  g_return_val_if_fail (buffer_rect != NULL, NULL);

  if (histogram->priv->calculate_async)
    gimp_async_cancel_and_wait (histogram->priv->calculate_async);

  gegl_rectangle_align_to_buffer (&rect, buffer_rect, buffer,
                                  GEGL_RECTANGLE_ALIGNMENT_SUPERSET);

  context = g_slice_new0 (CalculateContext);

  context->histogram   = histogram;
  context->buffer      = gegl_buffer_new (&rect,
                                          gegl_buffer_get_format (buffer));
  context->buffer_rect = *buffer_rect;

  gimp_gegl_buffer_copy (buffer, &rect, GEGL_ABYSS_NONE,
                         context->buffer, NULL);

  if (mask)
    {
      if (mask_rect)
        context->mask_rect = *mask_rect;
      else
        context->mask_rect = *gegl_buffer_get_extent (mask);

      gegl_rectangle_align_to_buffer (&rect, &context->mask_rect, mask,
                                      GEGL_RECTANGLE_ALIGNMENT_SUPERSET);

      context->mask = gegl_buffer_new (&rect, gegl_buffer_get_format (mask));

      gimp_gegl_buffer_copy (mask, &rect, GEGL_ABYSS_NONE,
                             context->mask, NULL);
    }

  histogram->priv->calculate_async = gimp_parallel_run_async (
    (GimpRunAsyncFunc) gimp_histogram_calculate_internal,
    context);

  gimp_async_add_callback (
    histogram->priv->calculate_async,
    (GimpAsyncCallback) gimp_histogram_calculate_async_callback,
    context);

  return histogram->priv->calculate_async;
}

void
gimp_histogram_clear_values (GimpHistogram *histogram,
                             gint           n_components)
{
  g_return_if_fail (GIMP_IS_HISTOGRAM (histogram));

  if (histogram->priv->calculate_async)
    gimp_async_cancel_and_wait (histogram->priv->calculate_async);

  gimp_histogram_set_values (histogram, n_components, 0, NULL);
}


#define HISTOGRAM_VALUE(c,i) (priv->values[(c) * priv->n_bins + (i)])


gdouble
gimp_histogram_get_maximum (GimpHistogram        *histogram,
                            GimpHistogramChannel  channel)
{
  GimpHistogramPrivate *priv;
  gdouble               max = 0.0;
  gint                  x;

  g_return_val_if_fail (GIMP_IS_HISTOGRAM (histogram), 0.0);

  priv = histogram->priv;

  if (! priv->values ||
      ! gimp_histogram_map_channel (histogram, &channel))
    {
      return 0.0;
    }

  if (channel == GIMP_HISTOGRAM_RGB)
    {
      for (x = 0; x < priv->n_bins; x++)
        {
          max = MAX (max, HISTOGRAM_VALUE (GIMP_HISTOGRAM_RED,   x));
          max = MAX (max, HISTOGRAM_VALUE (GIMP_HISTOGRAM_GREEN, x));
          max = MAX (max, HISTOGRAM_VALUE (GIMP_HISTOGRAM_BLUE,  x));
        }
    }
  else
    {
      for (x = 0; x < priv->n_bins; x++)
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
  GimpHistogramPrivate *priv;

  g_return_val_if_fail (GIMP_IS_HISTOGRAM (histogram), 0.0);

  priv = histogram->priv;

  if (! priv->values                   ||
      (bin < 0 || bin >= priv->n_bins) ||
      ! gimp_histogram_map_channel (histogram, &channel))
    {
      return 0.0;
    }

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
gimp_histogram_get_component (GimpHistogram *histogram,
                              gint           component,
                              gint           bin)
{
  g_return_val_if_fail (GIMP_IS_HISTOGRAM (histogram), 0.0);

  if (gimp_histogram_n_components (histogram) > 2)
    component++;

  return gimp_histogram_get_value (histogram, component, bin);
}

gint
gimp_histogram_n_components (GimpHistogram *histogram)
{
  g_return_val_if_fail (GIMP_IS_HISTOGRAM (histogram), 0);

  if (histogram->priv->n_channels > 0)
    return histogram->priv->n_channels - N_DERIVED_CHANNELS;
  else
    return 0;
}

gint
gimp_histogram_n_bins (GimpHistogram *histogram)
{
  g_return_val_if_fail (GIMP_IS_HISTOGRAM (histogram), 0);

  return histogram->priv->n_bins;
}

gboolean
gimp_histogram_has_channel (GimpHistogram        *histogram,
                            GimpHistogramChannel  channel)
{
  g_return_val_if_fail (GIMP_IS_HISTOGRAM (histogram), FALSE);

  switch (channel)
    {
    case GIMP_HISTOGRAM_VALUE:
      return TRUE;

    case GIMP_HISTOGRAM_RED:
    case GIMP_HISTOGRAM_GREEN:
    case GIMP_HISTOGRAM_BLUE:
    case GIMP_HISTOGRAM_LUMINANCE:
    case GIMP_HISTOGRAM_RGB:
      return gimp_histogram_n_components (histogram) >= 3;

    case GIMP_HISTOGRAM_ALPHA:
      return gimp_histogram_n_components (histogram) == 2 ||
             gimp_histogram_n_components (histogram) == 4;
    }

  g_return_val_if_reached (FALSE);
}

gdouble
gimp_histogram_get_count (GimpHistogram        *histogram,
                          GimpHistogramChannel  channel,
                          gint                  start,
                          gint                  end)
{
  GimpHistogramPrivate *priv;
  gint                  i;
  gdouble               count = 0.0;

  g_return_val_if_fail (GIMP_IS_HISTOGRAM (histogram), 0.0);

  priv = histogram->priv;

  if (! priv->values ||
      start > end    ||
      ! gimp_histogram_map_channel (histogram, &channel))
    {
      return 0.0;
    }

  if (channel == GIMP_HISTOGRAM_RGB)
    return (gimp_histogram_get_count (histogram,
                                      GIMP_HISTOGRAM_RED, start, end)   +
            gimp_histogram_get_count (histogram,
                                      GIMP_HISTOGRAM_GREEN, start, end) +
            gimp_histogram_get_count (histogram,
                                      GIMP_HISTOGRAM_BLUE, start, end));

  start = CLAMP (start, 0, priv->n_bins - 1);
  end   = CLAMP (end,   0, priv->n_bins - 1);

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
  GimpHistogramPrivate *priv;
  gint                  i;
  gdouble               mean = 0.0;
  gdouble               count;

  g_return_val_if_fail (GIMP_IS_HISTOGRAM (histogram), 0.0);

  priv = histogram->priv;

  if (! priv->values ||
      start > end    ||
      ! gimp_histogram_map_channel (histogram, &channel))
    {
      return 0.0;
    }

  start = CLAMP (start, 0, priv->n_bins - 1);
  end   = CLAMP (end,   0, priv->n_bins - 1);

  if (channel == GIMP_HISTOGRAM_RGB)
    {
      for (i = start; i <= end; i++)
        {
          gdouble factor = (gdouble) i / (gdouble)  (priv->n_bins - 1);

          mean += (factor * HISTOGRAM_VALUE (GIMP_HISTOGRAM_RED,   i) +
                   factor * HISTOGRAM_VALUE (GIMP_HISTOGRAM_GREEN, i) +
                   factor * HISTOGRAM_VALUE (GIMP_HISTOGRAM_BLUE,  i));
        }
    }
  else
    {
      for (i = start; i <= end; i++)
        {
          gdouble factor = (gdouble) i / (gdouble)  (priv->n_bins - 1);

          mean += factor * HISTOGRAM_VALUE (channel, i);
        }
    }

  count = gimp_histogram_get_count (histogram, channel, start, end);

  if (count > 0.0)
    return mean / count;

  return mean;
}

gdouble
gimp_histogram_get_median (GimpHistogram         *histogram,
                           GimpHistogramChannel   channel,
                           gint                   start,
                           gint                   end)
{
  GimpHistogramPrivate *priv;
  gint                  i;
  gdouble               sum = 0.0;
  gdouble               count;

  g_return_val_if_fail (GIMP_IS_HISTOGRAM (histogram), -1.0);

  priv = histogram->priv;

  if (! priv->values ||
      start > end    ||
      ! gimp_histogram_map_channel (histogram, &channel))
    {
      return 0.0;
    }

  start = CLAMP (start, 0, priv->n_bins - 1);
  end   = CLAMP (end,   0, priv->n_bins - 1);

  count = gimp_histogram_get_count (histogram, channel, start, end);

  if (channel == GIMP_HISTOGRAM_RGB)
    {
      for (i = start; i <= end; i++)
        {
          sum += (HISTOGRAM_VALUE (GIMP_HISTOGRAM_RED,   i) +
                  HISTOGRAM_VALUE (GIMP_HISTOGRAM_GREEN, i) +
                  HISTOGRAM_VALUE (GIMP_HISTOGRAM_BLUE,  i));

          if (sum * 2 > count)
            return ((gdouble) i / (gdouble)  (priv->n_bins - 1));
        }
    }
  else
    {
      for (i = start; i <= end; i++)
        {
          sum += HISTOGRAM_VALUE (channel, i);

          if (sum * 2 > count)
            return ((gdouble) i / (gdouble)  (priv->n_bins - 1));
        }
    }

  return -1.0;
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
  GimpHistogramPrivate *priv;
  gint                 i;
  gint                 maxval;
  gdouble             *hist      = NULL;
  gdouble             *chist     = NULL;
  gdouble             *cmom      = NULL;
  gdouble              hist_max  = 0.0;
  gdouble              chist_max = 0.0;
  gdouble              cmom_max  = 0.0;
  gdouble              bvar_max  = 0.0;
  gint                 threshold = 127;

  g_return_val_if_fail (GIMP_IS_HISTOGRAM (histogram), -1);

  priv = histogram->priv;

  if (! priv->values ||
      start > end    ||
      ! gimp_histogram_map_channel (histogram, &channel))
    {
      return 0;
    }

  start = CLAMP (start, 0, priv->n_bins - 1);
  end   = CLAMP (end,   0, priv->n_bins - 1);

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
  GimpHistogramPrivate *priv;
  gint                  i;
  gdouble               dev = 0.0;
  gdouble               count;
  gdouble               mean;

  g_return_val_if_fail (GIMP_IS_HISTOGRAM (histogram), 0.0);

  priv = histogram->priv;

  if (! priv->values ||
      start > end    ||
      ! gimp_histogram_map_channel (histogram, &channel))
    {
      return 0.0;
    }

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

      dev += value * SQR (((gdouble) i / (gdouble)  (priv->n_bins - 1)) - mean);
    }

  return sqrt (dev / count);
}


/*  private functions  */

static gboolean
gimp_histogram_map_channel (GimpHistogram        *histogram,
                            GimpHistogramChannel *channel)
{
  GimpHistogramPrivate *priv = histogram->priv;

  if (*channel == GIMP_HISTOGRAM_RGB)
    return gimp_histogram_n_components (histogram) >= 3;

  switch (*channel)
    {
    case GIMP_HISTOGRAM_ALPHA:
      if (gimp_histogram_n_components (histogram) == 2)
        *channel = 1;
      break;

    case GIMP_HISTOGRAM_LUMINANCE:
      *channel = gimp_histogram_n_components (histogram) + 1;
      break;

    default:
      break;
    }

  return *channel < priv->n_channels;
}

static void
gimp_histogram_set_values (GimpHistogram *histogram,
                           gint           n_components,
                           gint           n_bins,
                           gdouble       *values)
{
  GimpHistogramPrivate *priv                = histogram->priv;
  gint                  n_channels          = n_components;
  gboolean              notify_n_components = FALSE;
  gboolean              notify_n_bins       = FALSE;

  if (n_channels > 0)
    n_channels += N_DERIVED_CHANNELS;

  if (n_channels != priv->n_channels)
    {
      priv->n_channels = n_channels;

      notify_n_components = TRUE;
    }

  if (n_bins != priv->n_bins)
    {
      priv->n_bins = n_bins;

      notify_n_bins = TRUE;
    }

  if (values != priv->values)
    {
      if (priv->values)
        g_free (priv->values);

      priv->values = values;
    }

  if (notify_n_components)
    g_object_notify (G_OBJECT (histogram), "n-components");

  if (notify_n_bins)
    g_object_notify (G_OBJECT (histogram), "n-bins");

  g_object_notify (G_OBJECT (histogram), "values");
}

static void
gimp_histogram_calculate_internal (GimpAsync        *async,
                                   CalculateContext *context)
{
  CalculateData         data;
  GimpHistogramPrivate *priv;
  const Babl           *format;
  const Babl           *space;

  priv = context->histogram->priv;

  format = gegl_buffer_get_format (context->buffer);
  space  = babl_format_get_space (format);

  if (babl_format_get_type (format, 0) == babl_type ("u8"))
    context->n_bins = 256;
  else
    context->n_bins = 1024;

  switch (gimp_babl_format_get_base_type (format))
    {
    case GIMP_RGB:
    case GIMP_INDEXED:
      format = gimp_babl_format (GIMP_RGB,
                                 gimp_babl_precision (GIMP_COMPONENT_TYPE_FLOAT,
                                                      priv->trc),
                                 babl_format_has_alpha (format),
                                 space);
      break;

    case GIMP_GRAY:
      format = gimp_babl_format (GIMP_GRAY,
                                 gimp_babl_precision (GIMP_COMPONENT_TYPE_FLOAT,
                                                      priv->trc),
                                 babl_format_has_alpha (format),
                                 space);
      break;

    default:
      if (async)
        gimp_async_abort (async);

      g_return_if_reached ();
    }

  context->n_components = babl_format_get_n_components (format);

  data.async       = async;
  data.context     = context;
  data.format      = format;
  data.values_list = NULL;

  gegl_parallel_distribute_area (
    &context->buffer_rect, PIXELS_PER_THREAD, GEGL_SPLIT_STRATEGY_AUTO,
    (GeglParallelDistributeAreaFunc) gimp_histogram_calculate_area,
    &data);

  if (! async || ! gimp_async_is_canceled (async))
    {
      gdouble *total_values = NULL;
      gint     n_values     = (context->n_components + N_DERIVED_CHANNELS) *
                              context->n_bins;
      GSList  *iter;

      for (iter = data.values_list; iter; iter = g_slist_next (iter))
        {
          gdouble *values = iter->data;

          if (! total_values)
            {
              total_values = values;
            }
          else
            {
              gint i;

              for (i = 0; i < n_values; i++)
                total_values[i] += values[i];

              g_free (values);
            }
        }

      g_slist_free (data.values_list);

      context->values = total_values;

      if (async)
        gimp_async_finish (async, NULL);
    }
  else
    {
      g_slist_free_full (data.values_list, g_free);

      if (async)
        gimp_async_abort (async);
    }
}

static void
gimp_histogram_calculate_area (const GeglRectangle *area,
                               CalculateData       *data)
{
  GimpAsync            *async;
  CalculateContext     *context;
  GeglBufferIterator   *iter;
  gdouble              *values;
  gint                  n_components;
  gint                  n_bins;
  gfloat                n_bins_1f;
  gfloat                temp;

  async   = data->async;
  context = data->context;

  n_bins       = context->n_bins;
  n_components = context->n_components;

  values = g_new0 (gdouble, (n_components + N_DERIVED_CHANNELS) * n_bins);
  gimp_atomic_slist_push_head (&data->values_list, values);

  iter = gegl_buffer_iterator_new (context->buffer, area, 0,
                                   data->format,
                                   GEGL_ACCESS_READ, GEGL_ABYSS_NONE, 2);

  if (context->mask)
    {
      GeglRectangle mask_area = *area;

      mask_area.x += context->mask_rect.x - context->buffer_rect.x;
      mask_area.y += context->mask_rect.y - context->buffer_rect.y;

      gegl_buffer_iterator_add (iter, context->mask, &mask_area, 0,
                                babl_format ("Y float"),
                                GEGL_ACCESS_READ, GEGL_ABYSS_NONE);
    }

  n_bins_1f = n_bins - 1;

#define VALUE(c,i) (*(temp = (i) * n_bins_1f,                                  \
                      &values[(c) * n_bins +                                   \
                              SIGNED_ROUND (SAFE_CLAMP (temp,                  \
                                                        0.0f,                  \
                                                        n_bins_1f))]))

#define CHECK_CANCELED(length)                                                 \
  G_STMT_START                                                                 \
    {                                                                          \
      if ((length) % 128 == 0 && async && gimp_async_is_canceled (async))      \
        {                                                                      \
          gegl_buffer_iterator_stop (iter);                                    \
                                                                               \
          return;                                                              \
        }                                                                      \
    }                                                                          \
  G_STMT_END

  while (gegl_buffer_iterator_next (iter))
    {
      const gfloat *data   = iter->items[0].data;
      gint          length = iter->length;
      gfloat        max;
      gfloat        luminance;

      CHECK_CANCELED (0);

      if (context->mask)
        {
          const gfloat *mask_data = iter->items[1].data;

          switch (n_components)
            {
            case 1:
              while (length--)
                {
                  const gdouble masked = *mask_data;

                  VALUE (0, data[0]) += masked;

                  data += n_components;
                  mask_data += 1;

                  CHECK_CANCELED (length);
                }
              break;

            case 2:
              while (length--)
                {
                  const gdouble masked = *mask_data;
                  const gdouble weight = data[1];

                  VALUE (0, data[0]) += weight * masked;
                  VALUE (1, data[1]) += masked;

                  data += n_components;
                  mask_data += 1;

                  CHECK_CANCELED (length);
                }
              break;

            case 3: /* calculate separate value values */
              while (length--)
                {
                  const gdouble masked = *mask_data;

                  VALUE (1, data[0]) += masked;
                  VALUE (2, data[1]) += masked;
                  VALUE (3, data[2]) += masked;

                  max = MAX (data[0], data[1]);
                  max = MAX (data[2], max);
                  VALUE (0, max) += masked;

                  luminance = GIMP_RGB_LUMINANCE (data[0], data[1], data[2]);
                  VALUE (4, luminance) += masked;

                  data += n_components;
                  mask_data += 1;

                  CHECK_CANCELED (length);
                }
              break;

            case 4: /* calculate separate value values */
              while (length--)
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

                  luminance = GIMP_RGB_LUMINANCE (data[0], data[1], data[2]);
                  VALUE (5, luminance) += weight * masked;

                  data += n_components;
                  mask_data += 1;

                  CHECK_CANCELED (length);
                }
              break;
            }
        }
      else /* no mask */
        {
          switch (n_components)
            {
            case 1:
              while (length--)
                {
                  VALUE (0, data[0]) += 1.0;

                  data += n_components;

                  CHECK_CANCELED (length);
                }
              break;

            case 2:
              while (length--)
                {
                  const gdouble weight = data[1];

                  VALUE (0, data[0]) += weight;
                  VALUE (1, data[1]) += 1.0;

                  data += n_components;

                  CHECK_CANCELED (length);
                }
              break;

            case 3: /* calculate separate value values */
              while (length--)
                {
                  VALUE (1, data[0]) += 1.0;
                  VALUE (2, data[1]) += 1.0;
                  VALUE (3, data[2]) += 1.0;

                  max = MAX (data[0], data[1]);
                  max = MAX (data[2], max);
                  VALUE (0, max) += 1.0;

                  luminance = GIMP_RGB_LUMINANCE (data[0], data[1], data[2]);
                  VALUE (4, luminance) += 1.0;

                  data += n_components;

                  CHECK_CANCELED (length);
                }
              break;

            case 4: /* calculate separate value values */
              while (length--)
                {
                  const gdouble weight = data[3];

                  VALUE (1, data[0]) += weight;
                  VALUE (2, data[1]) += weight;
                  VALUE (3, data[2]) += weight;
                  VALUE (4, data[3]) += 1.0;

                  max = MAX (data[0], data[1]);
                  max = MAX (data[2], max);
                  VALUE (0, max) += weight;

                  luminance = GIMP_RGB_LUMINANCE (data[0], data[1], data[2]);
                  VALUE (5, luminance) += weight;

                  data += n_components;

                  CHECK_CANCELED (length);
                }
              break;
            }
        }
    }

#undef VALUE
#undef CHECK_CANCELED
}

static void
gimp_histogram_calculate_async_callback (GimpAsync        *async,
                                         CalculateContext *context)
{
  context->histogram->priv->calculate_async = NULL;

  if (gimp_async_is_finished (async))
    {
      gimp_histogram_set_values (context->histogram,
                                 context->n_components, context->n_bins,
                                 context->values);
    }

  g_object_unref (context->buffer);
  if (context->mask)
    g_object_unref (context->mask);

  g_slice_free (CalculateContext, context);
}
