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

#ifndef __LIGMA_CURVE_VIEW_H__
#define __LIGMA_CURVE_VIEW_H__


#include "ligmahistogramview.h"


#define LIGMA_TYPE_CURVE_VIEW            (ligma_curve_view_get_type ())
#define LIGMA_CURVE_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_CURVE_VIEW, LigmaCurveView))
#define LIGMA_CURVE_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_CURVE_VIEW, LigmaCurveViewClass))
#define LIGMA_IS_CURVE_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_CURVE_VIEW))
#define LIGMA_IS_CURVE_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_CURVE_VIEW))
#define LIGMA_CURVE_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_CURVE_VIEW, LigmaCurveViewClass))


typedef struct _LigmaCurveViewClass  LigmaCurveViewClass;

struct _LigmaCurveView
{
  LigmaHistogramView  parent_instance;

  Ligma              *ligma; /* only needed for copy & paste */

  LigmaCurve         *curve;
  LigmaRGB           *curve_color;

  GList             *bg_curves;

  gboolean           draw_base_line;
  gint               grid_rows;
  gint               grid_columns;

  gint               selected;
  gdouble            offset_x;
  gdouble            offset_y;
  LigmaCurvePointType point_type;
  gdouble            last_x;
  gdouble            last_y;
  gdouble            leftmost;
  gdouble            rightmost;
  gboolean           grabbed;
  LigmaCurve         *orig_curve;

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

struct _LigmaCurveViewClass
{
  LigmaHistogramViewClass  parent_class;

  /* signals */
  void (* selection_changed) (LigmaCurveView *view);

  void (* cut_clipboard)     (LigmaCurveView *view);
  void (* copy_clipboard)    (LigmaCurveView *view);
  void (* paste_clipboard)   (LigmaCurveView *view);
};


GType       ligma_curve_view_get_type          (void) G_GNUC_CONST;

GtkWidget * ligma_curve_view_new               (void);

void        ligma_curve_view_set_curve         (LigmaCurveView *view,
                                               LigmaCurve     *curve,
                                               const LigmaRGB *color);
LigmaCurve * ligma_curve_view_get_curve         (LigmaCurveView *view);

void        ligma_curve_view_add_background    (LigmaCurveView *view,
                                               LigmaCurve     *curve,
                                               const LigmaRGB *color);
void        ligma_curve_view_remove_background (LigmaCurveView *view,
                                               LigmaCurve     *curve);

void   ligma_curve_view_remove_all_backgrounds (LigmaCurveView *view);

void        ligma_curve_view_set_selected      (LigmaCurveView *view,
                                               gint           selected);
gint        ligma_curve_view_get_selected      (LigmaCurveView *view);

void        ligma_curve_view_set_range_x       (LigmaCurveView *view,
                                               gdouble        min,
                                               gdouble        max);
void        ligma_curve_view_set_range_y       (LigmaCurveView *view,
                                               gdouble        min,
                                               gdouble        max);
void        ligma_curve_view_set_xpos          (LigmaCurveView *view,
                                               gdouble        x);

void        ligma_curve_view_set_x_axis_label  (LigmaCurveView *view,
                                               const gchar   *label);
void        ligma_curve_view_set_y_axis_label  (LigmaCurveView *view,
                                               const gchar   *label);


#endif /* __LIGMA_CURVE_VIEW_H__ */
