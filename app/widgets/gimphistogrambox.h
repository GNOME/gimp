/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#ifndef __GIMP_HISTOGRAM_BOX_H__
#define __GIMP_HISTOGRAM_BOX_H__


#define GIMP_TYPE_HISTOGRAM_BOX            (gimp_histogram_box_get_type ())
#define GIMP_HISTOGRAM_BOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_HISTOGRAM_BOX, GimpHistogramBox))
#define GIMP_HISTOGRAM_BOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_HISTOGRAM_BOX, GimpHistogramBoxClass))
#define GIMP_IS_HISTOGRAM_BOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_HISTOGRAM_BOX))
#define GIMP_IS_HISTOGRAM_BOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_HISTOGRAM_BOX))
#define GIMP_HISTOGRAM_BOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_HISTOGRAM_BOX, GimpHistogramBoxClass))


typedef struct _GimpHistogramBoxClass GimpHistogramBoxClass;

struct _GimpHistogramBox
{
  GtkVBox            parent_instance;

  GtkWidget         *slider_area;
  GimpHistogramView *view;

  GtkAdjustment     *low_adj;
  GtkAdjustment     *high_adj;

  gint              slider_pos[2];
  gint              active_slider;
  gint              low_slider_val;
  gint              high_slider_val;
};

struct _GimpHistogramBoxClass
{
  GtkVBoxClass  parent_class;
};


GType       gimp_histogram_box_get_type    (void) G_GNUC_CONST;

GtkWidget * gimp_histogram_box_new         (void);
void        gimp_histogram_box_set_channel (GimpHistogramBox     *box,
                                            GimpHistogramChannel  channel);


#endif  /*  __GIMP_HISTOGRAM_BOX_H__  */
