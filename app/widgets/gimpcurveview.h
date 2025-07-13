/* GIMP - The GNU Image Manipulation Program
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

#pragma once

#include "gimphistogramview.h"


#define GIMP_TYPE_CURVE_VIEW            (gimp_curve_view_get_type ())
#define GIMP_CURVE_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_CURVE_VIEW, GimpCurveView))
#define GIMP_CURVE_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_CURVE_VIEW, GimpCurveViewClass))
#define GIMP_IS_CURVE_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_CURVE_VIEW))
#define GIMP_IS_CURVE_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_CURVE_VIEW))
#define GIMP_CURVE_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_CURVE_VIEW, GimpCurveViewClass))


typedef struct _GimpCurveViewClass  GimpCurveViewClass;

struct _GimpCurveView
{
  GimpHistogramView  parent_instance;

  Gimp              *gimp; /* only needed for copy & paste */

  GimpCurve         *curve;
  GeglColor         *curve_color;

  GList             *bg_curves;

  gboolean           draw_base_line;
  gint               grid_rows;
  gint               grid_columns;

  gint               selected;
  gdouble            offset_x;
  gdouble            offset_y;
  GimpCurvePointType point_type;
  gdouble            last_x;
  gdouble            last_y;
  gdouble            leftmost;
  gdouble            rightmost;
  gboolean           grabbed;
  GimpCurve         *orig_curve;

  GdkCursorType      cursor_type;

  gdouble            xpos;

  PangoLayout       *layout;

  gdouble            range_x_min;
  gdouble            range_x_max;
  gdouble            range_y_min;
  gdouble            range_y_max;

  gdouble            cursor_x;
  gdouble            cursor_y;
  PangoLayout       *cursor_layout;
  PangoRectangle     cursor_rect;

  gchar             *x_axis_label;
  gchar             *y_axis_label;
};

struct _GimpCurveViewClass
{
  GimpHistogramViewClass  parent_class;

  /* signals */
  void (* selection_changed) (GimpCurveView *view);

  void (* cut_clipboard)     (GimpCurveView *view);
  void (* copy_clipboard)    (GimpCurveView *view);
  void (* paste_clipboard)   (GimpCurveView *view);
};


GType       gimp_curve_view_get_type          (void) G_GNUC_CONST;

GtkWidget * gimp_curve_view_new               (void);

void        gimp_curve_view_set_curve         (GimpCurveView *view,
                                               GimpCurve     *curve,
                                               GeglColor     *color);
GimpCurve * gimp_curve_view_get_curve         (GimpCurveView *view);

void        gimp_curve_view_add_background    (GimpCurveView *view,
                                               GimpCurve     *curve,
                                               GeglColor     *color);
void        gimp_curve_view_remove_background (GimpCurveView *view,
                                               GimpCurve     *curve);

void   gimp_curve_view_remove_all_backgrounds (GimpCurveView *view);

void        gimp_curve_view_set_selected      (GimpCurveView *view,
                                               gint           selected);
gint        gimp_curve_view_get_selected      (GimpCurveView *view);

void        gimp_curve_view_set_range_x       (GimpCurveView *view,
                                               gdouble        min,
                                               gdouble        max);
void        gimp_curve_view_set_range_y       (GimpCurveView *view,
                                               gdouble        min,
                                               gdouble        max);
void        gimp_curve_view_set_xpos          (GimpCurveView *view,
                                               gdouble        x);

void        gimp_curve_view_set_x_axis_label  (GimpCurveView *view,
                                               const gchar   *label);
void        gimp_curve_view_set_y_axis_label  (GimpCurveView *view,
                                               const gchar   *label);
