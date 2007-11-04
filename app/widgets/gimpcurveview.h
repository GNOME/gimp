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

#ifndef __GIMP_CURVE_VIEW_H__
#define __GIMP_CURVE_VIEW_H__


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

  GimpCurve         *curve;
  gint               selected;

  gint               xpos;
  PangoLayout       *xpos_layout;

  gint               cursor_x;
  gint               cursor_y;
  PangoLayout       *cursor_layout;
  PangoRectangle     cursor_rect;
};

struct _GimpCurveViewClass
{
  GimpHistogramViewClass  parent_class;
};


GType       gimp_curve_view_get_type     (void) G_GNUC_CONST;

GtkWidget * gimp_curve_view_new          (void);

void        gimp_curve_view_set_curve    (GimpCurveView *view,
                                          GimpCurve     *curve);
GimpCurve * gimp_curve_view_get_curve    (GimpCurveView *view);

void        gimp_curve_view_set_selected (GimpCurveView *view,
                                          gint           selected);
void        gimp_curve_view_set_xpos     (GimpCurveView *view,
                                          gint           x);
void        gimp_curve_view_set_cusor    (GimpCurveView *view,
                                          gint           x,
                                          gint           y);


#endif /* __GIMP_CURVE_VIEW_H__ */
