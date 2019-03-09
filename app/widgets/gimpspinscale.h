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

#ifndef __GIMP_SPIN_SCALE_H__
#define __GIMP_SPIN_SCALE_H__


#define GIMP_TYPE_SPIN_SCALE            (gimp_spin_scale_get_type ())
#define GIMP_SPIN_SCALE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_SPIN_SCALE, GimpSpinScale))
#define GIMP_SPIN_SCALE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_SPIN_SCALE, GimpSpinScaleClass))
#define GIMP_IS_SPIN_SCALE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_SPIN_SCALE))
#define GIMP_IS_SPIN_SCALE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_SPIN_SCALE))
#define GIMP_SPIN_SCALE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_SPIN_SCALE, GimpSpinScaleClass))


typedef struct _GimpSpinScale      GimpSpinScale;
typedef struct _GimpSpinScaleClass GimpSpinScaleClass;

struct _GimpSpinScale
{
  GimpSpinButton  parent_instance;
};

struct _GimpSpinScaleClass
{
  GimpSpinButtonClass  parent_class;
};


GType         gimp_spin_scale_get_type           (void) G_GNUC_CONST;

GtkWidget   * gimp_spin_scale_new                (GtkAdjustment *adjustment,
                                                  const gchar   *label,
                                                  gint           digits);

void          gimp_spin_scale_set_label          (GimpSpinScale *scale,
                                                  const gchar   *label);
const gchar * gimp_spin_scale_get_label          (GimpSpinScale *scale);

void          gimp_spin_scale_set_scale_limits   (GimpSpinScale *scale,
                                                  gdouble        lower,
                                                  gdouble        upper);
void          gimp_spin_scale_unset_scale_limits (GimpSpinScale *scale);
gboolean      gimp_spin_scale_get_scale_limits   (GimpSpinScale *scale,
                                                  gdouble       *lower,
                                                  gdouble       *upper);

void          gimp_spin_scale_set_gamma          (GimpSpinScale *scale,
                                                  gdouble        gamma);
gdouble       gimp_spin_scale_get_gamma          (GimpSpinScale *scale);

void          gimp_spin_scale_set_constrain_drag (GimpSpinScale *scale,
                                                  gboolean       constrain);
gboolean      gimp_spin_scale_get_constrain_drag (GimpSpinScale *scale);

#endif  /*  __GIMP_SPIN_SCALE_H__  */
