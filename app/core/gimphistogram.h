/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmahistogram module Copyright (C) 1999 Jay Cox <jaycox@ligma.org>
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

#ifndef __LIGMA_HISTOGRAM_H__
#define __LIGMA_HISTOGRAM_H__


#include "ligmaobject.h"


#define LIGMA_TYPE_HISTOGRAM            (ligma_histogram_get_type ())
#define LIGMA_HISTOGRAM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_HISTOGRAM, LigmaHistogram))
#define LIGMA_HISTOGRAM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_HISTOGRAM, LigmaHistogramClass))
#define LIGMA_IS_HISTOGRAM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_HISTOGRAM))
#define LIGMA_IS_HISTOGRAM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_HISTOGRAM))
#define LIGMA_HISTOGRAM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_HISTOGRAM, LigmaHistogramClass))


typedef struct _LigmaHistogramPrivate LigmaHistogramPrivate;
typedef struct _LigmaHistogramClass   LigmaHistogramClass;

struct _LigmaHistogram
{
  LigmaObject            parent_instance;

  LigmaHistogramPrivate *priv;
};

struct _LigmaHistogramClass
{
  LigmaObjectClass  parent_class;
};


GType           ligma_histogram_get_type        (void) G_GNUC_CONST;

LigmaHistogram * ligma_histogram_new             (LigmaTRCType           trc);

LigmaHistogram * ligma_histogram_duplicate       (LigmaHistogram        *histogram);

void            ligma_histogram_calculate       (LigmaHistogram        *histogram,
                                                GeglBuffer           *buffer,
                                                const GeglRectangle  *buffer_rect,
                                                GeglBuffer           *mask,
                                                const GeglRectangle  *mask_rect);
LigmaAsync     * ligma_histogram_calculate_async (LigmaHistogram        *histogram,
                                                GeglBuffer           *buffer,
                                                const GeglRectangle  *buffer_rect,
                                                GeglBuffer           *mask,
                                                const GeglRectangle  *mask_rect);

void            ligma_histogram_clear_values    (LigmaHistogram        *histogram,
                                                gint                  n_components);

gdouble         ligma_histogram_get_maximum     (LigmaHistogram        *histogram,
                                                LigmaHistogramChannel  channel);
gdouble         ligma_histogram_get_count       (LigmaHistogram        *histogram,
                                                LigmaHistogramChannel  channel,
                                                gint                  start,
                                                gint                  end);
gdouble         ligma_histogram_get_mean        (LigmaHistogram        *histogram,
                                                LigmaHistogramChannel  channel,
                                                gint                  start,
                                                gint                  end);
gdouble         ligma_histogram_get_median      (LigmaHistogram        *histogram,
                                                LigmaHistogramChannel  channel,
                                                gint                  start,
                                                gint                  end);
gdouble         ligma_histogram_get_std_dev     (LigmaHistogram        *histogram,
                                                LigmaHistogramChannel  channel,
                                                gint                  start,
                                                gint                  end);
gdouble         ligma_histogram_get_threshold   (LigmaHistogram        *histogram,
                                                LigmaHistogramChannel  channel,
                                                gint                  start,
                                                gint                  end);
gdouble         ligma_histogram_get_value       (LigmaHistogram        *histogram,
                                                LigmaHistogramChannel  channel,
                                                gint                  bin);
gdouble         ligma_histogram_get_component   (LigmaHistogram        *histogram,
                                                gint                  component,
                                                gint                  bin);
gint            ligma_histogram_n_components    (LigmaHistogram        *histogram);
gint            ligma_histogram_n_bins          (LigmaHistogram        *histogram);
gboolean        ligma_histogram_has_channel     (LigmaHistogram        *histogram,
                                                LigmaHistogramChannel  channel);


#endif /* __LIGMA_HISTOGRAM_H__ */
