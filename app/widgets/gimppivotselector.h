/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimppivotselector.h
 * Copyright (C) 2019 Ell
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


#define GIMP_TYPE_PIVOT_SELECTOR            (gimp_pivot_selector_get_type ())
#define GIMP_PIVOT_SELECTOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_PIVOT_SELECTOR, GimpPivotSelector))
#define GIMP_PIVOT_SELECTOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_PIVOT_SELECTOR, GimpPivotSelectorClass))
#define GIMP_IS_PIVOT_SELECTOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, GIMP_TYPE_PIVOT_SELECTOR))
#define GIMP_IS_PIVOT_SELECTOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_PIVOT_SELECTOR))
#define GIMP_PIVOT_SELECTOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_PIVOT_SELECTOR, GimpPivotSelectorClass))


typedef struct _GimpPivotSelectorPrivate GimpPivotSelectorPrivate;
typedef struct _GimpPivotSelectorClass   GimpPivotSelectorClass;

struct _GimpPivotSelector
{
  GtkGrid                   parent_instance;

  GimpPivotSelectorPrivate *priv;
};

struct _GimpPivotSelectorClass
{
  GtkGridClass  parent_class;

  /*  signals  */
  void (* changed) (GimpPivotSelector *selector);
};


GType       gimp_pivot_selector_get_type     (void) G_GNUC_CONST;

GtkWidget * gimp_pivot_selector_new          (gdouble            left,
                                              gdouble            top,
                                              gdouble            right,
                                              gdouble            bottom);

void        gimp_pivot_selector_set_position (GimpPivotSelector *selector,
                                              gdouble            x,
                                              gdouble            y);
void        gimp_pivot_selector_get_position (GimpPivotSelector *selector,
                                              gdouble           *x,
                                              gdouble           *y);

void        gimp_pivot_selector_set_bounds   (GimpPivotSelector *selector,
                                              gdouble            left,
                                              gdouble            top,
                                              gdouble            right,
                                              gdouble            bottom);
void        gimp_pivot_selector_get_bounds   (GimpPivotSelector *selector,
                                              gdouble           *left,
                                              gdouble           *top,
                                              gdouble           *right,
                                              gdouble           *bottom);
