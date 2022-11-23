/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#ifndef __LIGMA_HISTOGRAM_VIEW_H__
#define __LIGMA_HISTOGRAM_VIEW_H__


#define LIGMA_TYPE_HISTOGRAM_VIEW            (ligma_histogram_view_get_type ())
#define LIGMA_HISTOGRAM_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_HISTOGRAM_VIEW, LigmaHistogramView))
#define LIGMA_HISTOGRAM_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_HISTOGRAM_VIEW, LigmaHistogramViewClass))
#define LIGMA_IS_HISTOGRAM_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_HISTOGRAM_VIEW))
#define LIGMA_IS_HISTOGRAM_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_HISTOGRAM_VIEW))
#define LIGMA_HISTOGRAM_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_HISTOGRAM_VIEW, LigmaHistogramViewClass))


typedef struct _LigmaHistogramViewClass  LigmaHistogramViewClass;

struct _LigmaHistogramView
{
  GtkDrawingArea         parent_instance;

  LigmaHistogram         *histogram;
  LigmaHistogram         *bg_histogram;
  LigmaHistogramChannel   channel;
  LigmaHistogramScale     scale;
  gint                   n_bins;
  gint                   start;
  gint                   end;

  gint                   border_width;
  gint                   subdivisions;

  GdkSeat               *grab_seat;
};

struct _LigmaHistogramViewClass
{
  GtkDrawingAreaClass  parent_class;

  void (* range_changed) (LigmaHistogramView *view,
                          gint               start,
                          gint               end);
};


GType           ligma_histogram_view_get_type       (void) G_GNUC_CONST;

GtkWidget     * ligma_histogram_view_new            (gboolean             range);

void            ligma_histogram_view_set_histogram  (LigmaHistogramView   *view,
                                                    LigmaHistogram       *histogram);
LigmaHistogram * ligma_histogram_view_get_histogram  (LigmaHistogramView   *view);

void            ligma_histogram_view_set_background (LigmaHistogramView   *view,
                                                    LigmaHistogram       *histogram);
LigmaHistogram * ligma_histogram_view_get_background (LigmaHistogramView   *view);

void            ligma_histogram_view_set_channel    (LigmaHistogramView   *view,
                                                    LigmaHistogramChannel channel);
LigmaHistogramChannel
                ligma_histogram_view_get_channel    (LigmaHistogramView   *view);

void            ligma_histogram_view_set_scale      (LigmaHistogramView   *view,
                                                    LigmaHistogramScale   scale);
LigmaHistogramScale
                ligma_histogram_view_get_scale      (LigmaHistogramView   *view);

void            ligma_histogram_view_set_range      (LigmaHistogramView   *view,
                                                    gint                 start,
                                                    gint                 end);
void            ligma_histogram_view_get_range      (LigmaHistogramView   *view,
                                                    gint                *start,
                                                    gint                *end);


#endif /* __LIGMA_HISTOGRAM_VIEW_H__ */
