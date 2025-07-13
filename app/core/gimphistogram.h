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

#pragma once

#include "gimpobject.h"


#define GIMP_TYPE_HISTOGRAM            (gimp_histogram_get_type ())
#define GIMP_HISTOGRAM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_HISTOGRAM, GimpHistogram))
#define GIMP_HISTOGRAM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_HISTOGRAM, GimpHistogramClass))
#define GIMP_IS_HISTOGRAM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_HISTOGRAM))
#define GIMP_IS_HISTOGRAM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_HISTOGRAM))
#define GIMP_HISTOGRAM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_HISTOGRAM, GimpHistogramClass))


typedef struct _GimpHistogramPrivate GimpHistogramPrivate;
typedef struct _GimpHistogramClass   GimpHistogramClass;

struct _GimpHistogram
{
  GimpObject            parent_instance;

  GimpHistogramPrivate *priv;
};

struct _GimpHistogramClass
{
  GimpObjectClass  parent_class;
};


GType           gimp_histogram_get_type        (void) G_GNUC_CONST;

GimpHistogram * gimp_histogram_new             (GimpTRCType           trc);

GimpHistogram * gimp_histogram_duplicate       (GimpHistogram        *histogram);

void            gimp_histogram_calculate       (GimpHistogram        *histogram,
                                                GeglBuffer           *buffer,
                                                const GeglRectangle  *buffer_rect,
                                                GeglBuffer           *mask,
                                                const GeglRectangle  *mask_rect);
GimpAsync     * gimp_histogram_calculate_async (GimpHistogram        *histogram,
                                                GeglBuffer           *buffer,
                                                const GeglRectangle  *buffer_rect,
                                                GeglBuffer           *mask,
                                                const GeglRectangle  *mask_rect);

void            gimp_histogram_clear_values    (GimpHistogram        *histogram,
                                                gint                  n_components);

gdouble         gimp_histogram_get_maximum     (GimpHistogram        *histogram,
                                                GimpHistogramChannel  channel);
gdouble         gimp_histogram_get_count       (GimpHistogram        *histogram,
                                                GimpHistogramChannel  channel,
                                                gint                  start,
                                                gint                  end);
gdouble         gimp_histogram_get_mean        (GimpHistogram        *histogram,
                                                GimpHistogramChannel  channel,
                                                gint                  start,
                                                gint                  end);
gdouble         gimp_histogram_get_median      (GimpHistogram        *histogram,
                                                GimpHistogramChannel  channel,
                                                gint                  start,
                                                gint                  end);
gdouble         gimp_histogram_get_std_dev     (GimpHistogram        *histogram,
                                                GimpHistogramChannel  channel,
                                                gint                  start,
                                                gint                  end);
gdouble         gimp_histogram_get_threshold   (GimpHistogram        *histogram,
                                                GimpHistogramChannel  channel,
                                                gint                  start,
                                                gint                  end);
gdouble         gimp_histogram_get_value       (GimpHistogram        *histogram,
                                                GimpHistogramChannel  channel,
                                                gint                  bin);
gdouble         gimp_histogram_get_component   (GimpHistogram        *histogram,
                                                gint                  component,
                                                gint                  bin);
gint            gimp_histogram_n_components    (GimpHistogram        *histogram);
gint            gimp_histogram_n_bins          (GimpHistogram        *histogram);
gboolean        gimp_histogram_has_channel     (GimpHistogram        *histogram,
                                                GimpHistogramChannel  channel);
guint           gimp_histogram_unique_colors   (GimpDrawable         *drawable);
