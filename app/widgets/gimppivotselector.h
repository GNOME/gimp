/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmapivotselector.h
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

#ifndef __LIGMA_PIVOT_SELECTOR_H__
#define __LIGMA_PIVOT_SELECTOR_H__


#define LIGMA_TYPE_PIVOT_SELECTOR            (ligma_pivot_selector_get_type ())
#define LIGMA_PIVOT_SELECTOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_PIVOT_SELECTOR, LigmaPivotSelector))
#define LIGMA_PIVOT_SELECTOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_PIVOT_SELECTOR, LigmaPivotSelectorClass))
#define LIGMA_IS_PIVOT_SELECTOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, LIGMA_TYPE_PIVOT_SELECTOR))
#define LIGMA_IS_PIVOT_SELECTOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_PIVOT_SELECTOR))
#define LIGMA_PIVOT_SELECTOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_PIVOT_SELECTOR, LigmaPivotSelectorClass))


typedef struct _LigmaPivotSelectorPrivate LigmaPivotSelectorPrivate;
typedef struct _LigmaPivotSelectorClass   LigmaPivotSelectorClass;

struct _LigmaPivotSelector
{
  GtkGrid                   parent_instance;

  LigmaPivotSelectorPrivate *priv;
};

struct _LigmaPivotSelectorClass
{
  GtkGridClass  parent_class;

  /*  signals  */
  void (* changed) (LigmaPivotSelector *selector);
};


GType       ligma_pivot_selector_get_type     (void) G_GNUC_CONST;

GtkWidget * ligma_pivot_selector_new          (gdouble            left,
                                              gdouble            top,
                                              gdouble            right,
                                              gdouble            bottom);

void        ligma_pivot_selector_set_position (LigmaPivotSelector *selector,
                                              gdouble            x,
                                              gdouble            y);
void        ligma_pivot_selector_get_position (LigmaPivotSelector *selector,
                                              gdouble           *x,
                                              gdouble           *y);

void        ligma_pivot_selector_set_bounds   (LigmaPivotSelector *selector,
                                              gdouble            left,
                                              gdouble            top,
                                              gdouble            right,
                                              gdouble            bottom);
void        ligma_pivot_selector_get_bounds   (LigmaPivotSelector *selector,
                                              gdouble           *left,
                                              gdouble           *top,
                                              gdouble           *right,
                                              gdouble           *bottom);


#endif /* __LIGMA_PIVOT_SELECTOR_H__ */
