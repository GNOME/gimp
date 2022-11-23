/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaspinscale.h
 * Copyright (C) 2010  Michael Natterer <mitch@ligma.org>
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

#if !defined (__LIGMA_WIDGETS_H_INSIDE__) && !defined (LIGMA_WIDGETS_COMPILATION)
#error "Only <libligmawidgets/ligmawidgets.h> can be included directly."
#endif

#ifndef __LIGMA_SPIN_SCALE_H__
#define __LIGMA_SPIN_SCALE_H__

#include <libligmawidgets/ligmaspinbutton.h>

G_BEGIN_DECLS


#define LIGMA_TYPE_SPIN_SCALE            (ligma_spin_scale_get_type ())
#define LIGMA_SPIN_SCALE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_SPIN_SCALE, LigmaSpinScale))
#define LIGMA_SPIN_SCALE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_SPIN_SCALE, LigmaSpinScaleClass))
#define LIGMA_IS_SPIN_SCALE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_SPIN_SCALE))
#define LIGMA_IS_SPIN_SCALE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_SPIN_SCALE))
#define LIGMA_SPIN_SCALE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_SPIN_SCALE, LigmaSpinScaleClass))


typedef struct _LigmaSpinScale      LigmaSpinScale;
typedef struct _LigmaSpinScaleClass LigmaSpinScaleClass;

struct _LigmaSpinScale
{
  LigmaSpinButton  parent_instance;
};

struct _LigmaSpinScaleClass
{
  LigmaSpinButtonClass  parent_class;
};


GType         ligma_spin_scale_get_type           (void) G_GNUC_CONST;

GtkWidget   * ligma_spin_scale_new                (GtkAdjustment *adjustment,
                                                  const gchar   *label,
                                                  gint           digits);

void          ligma_spin_scale_set_label          (LigmaSpinScale *scale,
                                                  const gchar   *label);
const gchar * ligma_spin_scale_get_label          (LigmaSpinScale *scale);

void          ligma_spin_scale_set_scale_limits   (LigmaSpinScale *scale,
                                                  gdouble        lower,
                                                  gdouble        upper);
void          ligma_spin_scale_unset_scale_limits (LigmaSpinScale *scale);
gboolean      ligma_spin_scale_get_scale_limits   (LigmaSpinScale *scale,
                                                  gdouble       *lower,
                                                  gdouble       *upper);

void          ligma_spin_scale_set_gamma          (LigmaSpinScale *scale,
                                                  gdouble        gamma);
gdouble       ligma_spin_scale_get_gamma          (LigmaSpinScale *scale);

void          ligma_spin_scale_set_constrain_drag (LigmaSpinScale *scale,
                                                  gboolean       constrain);
gboolean      ligma_spin_scale_get_constrain_drag (LigmaSpinScale *scale);


G_END_DECLS

#endif  /*  __LIGMA_SPIN_SCALE_H__  */
