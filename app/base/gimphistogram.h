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

#ifndef __GIMP_HISTOGRAM_H__
#define __GIMP_HISTOGRAM_H__


GimpHistogram * gimp_histogram_new           (void);
void            gimp_histogram_free          (GimpHistogram        *histogram);

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
