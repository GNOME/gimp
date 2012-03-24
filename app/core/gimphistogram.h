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

#ifndef __GIMP_HISTOGRAM_H__
#define __GIMP_HISTOGRAM_H__


GimpHistogram * gimp_histogram_new           (void);

GimpHistogram * gimp_histogram_ref           (GimpHistogram        *histogram);
void            gimp_histogram_unref         (GimpHistogram        *histogram);

GimpHistogram * gimp_histogram_duplicate     (GimpHistogram        *histogram);

void            gimp_histogram_calculate     (GimpHistogram        *histogram,
                                              PixelRegion          *region,
                                              PixelRegion          *mask);

gdouble         gimp_histogram_get_maximum   (GimpHistogram        *histogram,
                                              GimpHistogramChannel  channel);
gdouble         gimp_histogram_get_count     (GimpHistogram        *histogram,
                                              GimpHistogramChannel  channel,
                                              gint                  start,
                                              gint                  end);
gdouble         gimp_histogram_get_mean      (GimpHistogram        *histogram,
                                              GimpHistogramChannel  channel,
                                              gint                  start,
                                              gint                  end);
gint            gimp_histogram_get_median    (GimpHistogram        *histogram,
                                              GimpHistogramChannel  channel,
                                              gint                  start,
                                              gint                  end);
gdouble         gimp_histogram_get_std_dev   (GimpHistogram        *histogram,
                                              GimpHistogramChannel  channel,
                                              gint                  start,
                                              gint                  end);
gdouble         gimp_histogram_get_threshold (GimpHistogram        *histogram,
                                              GimpHistogramChannel  channel,
                                              gint                  start,
                                              gint                  end);
gdouble         gimp_histogram_get_value     (GimpHistogram        *histogram,
                                              GimpHistogramChannel  channel,
                                              gint                  bin);
gdouble         gimp_histogram_get_channel   (GimpHistogram        *histogram,
                                              GimpHistogramChannel  channel,
                                              gint                  bin);
gint            gimp_histogram_n_channels    (GimpHistogram        *histogram);


#endif /* __GIMP_HISTOGRAM_H__ */
