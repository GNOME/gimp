/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpspinscale.h
 * Copyright (C) 2010  Michael Natterer <mitch@gimp.org>
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

#if !defined (__GIMP_WIDGETS_H_INSIDE__) && !defined (GIMP_WIDGETS_COMPILATION)
#error "Only <libgimpwidgets/gimpwidgets.h> can be included directly."
#endif

#ifndef __GIMP_SPIN_SCALE_H__
#define __GIMP_SPIN_SCALE_H__

#include <libgimpwidgets/gimpspinbutton.h>

G_BEGIN_DECLS


#define GIMP_TYPE_SPIN_SCALE (gimp_spin_scale_get_type ())
G_DECLARE_FINAL_TYPE (GimpSpinScale, gimp_spin_scale, GIMP, SPIN_SCALE, GimpSpinButton)


GtkWidget   * gimp_spin_scale_new                 (GtkAdjustment *adjustment,
                                                   const gchar   *label,
                                                   gint           digits);

void          gimp_spin_scale_set_label           (GimpSpinScale *scale,
                                                   const gchar   *label);
const gchar * gimp_spin_scale_get_label           (GimpSpinScale *scale);

void          gimp_spin_scale_set_scale_limits    (GimpSpinScale *scale,
                                                   gdouble        lower,
                                                   gdouble        upper);
void          gimp_spin_scale_unset_scale_limits  (GimpSpinScale *scale);
gboolean      gimp_spin_scale_get_scale_limits    (GimpSpinScale *scale,
                                                   gdouble       *lower,
                                                   gdouble       *upper);

void          gimp_spin_scale_set_gamma           (GimpSpinScale *scale,
                                                   gdouble        gamma);
gdouble       gimp_spin_scale_get_gamma           (GimpSpinScale *scale);

void          gimp_spin_scale_set_constrain_drag  (GimpSpinScale *scale,
                                                   gboolean       constrain);
gboolean      gimp_spin_scale_get_constrain_drag  (GimpSpinScale *scale);

const guint   gimp_spin_scale_get_mnemonic_keyval (GimpSpinScale *scale);

G_END_DECLS

#endif  /*  __GIMP_SPIN_SCALE_H__  */
