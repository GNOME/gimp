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

#ifndef __LIGMA_HISTOGRAM_BOX_H__
#define __LIGMA_HISTOGRAM_BOX_H__


#define LIGMA_TYPE_HISTOGRAM_BOX            (ligma_histogram_box_get_type ())
#define LIGMA_HISTOGRAM_BOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_HISTOGRAM_BOX, LigmaHistogramBox))
#define LIGMA_HISTOGRAM_BOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_HISTOGRAM_BOX, LigmaHistogramBoxClass))
#define LIGMA_IS_HISTOGRAM_BOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_HISTOGRAM_BOX))
#define LIGMA_IS_HISTOGRAM_BOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_HISTOGRAM_BOX))
#define LIGMA_HISTOGRAM_BOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_HISTOGRAM_BOX, LigmaHistogramBoxClass))


typedef struct _LigmaHistogramBoxClass LigmaHistogramBoxClass;

struct _LigmaHistogramBox
{
  GtkBox             parent_instance;

  LigmaHistogramView *view;
  GtkWidget         *color_bar;
  GtkWidget         *slider_bar;

  gint               n_bins;

  GtkAdjustment     *low_adj;
  GtkAdjustment     *high_adj;

  GtkWidget         *low_spinbutton;
  GtkWidget         *high_spinbutton;
};

struct _LigmaHistogramBoxClass
{
  GtkBoxClass  parent_class;
};


GType       ligma_histogram_box_get_type    (void) G_GNUC_CONST;

GtkWidget * ligma_histogram_box_new         (void);
void        ligma_histogram_box_set_channel (LigmaHistogramBox     *box,
                                            LigmaHistogramChannel  channel);


#endif  /*  __LIGMA_HISTOGRAM_BOX_H__  */
