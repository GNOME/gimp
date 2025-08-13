/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpcolorscaleentry.h
 * Copyright (C) 2020 Jehan
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#if !defined (__GIMP_WIDGETS_H_INSIDE__) && !defined (GIMP_WIDGETS_COMPILATION)
#error "Only <libgimpwidgets/gimpwidgets.h> can be included directly."
#endif

#ifndef __GIMP_LABEL_SPIN_H__
#define __GIMP_LABEL_SPIN_H__

#include <libgimpwidgets/gimplabeled.h>

G_BEGIN_DECLS

#define GIMP_TYPE_LABEL_SPIN (gimp_label_spin_get_type ())
G_DECLARE_DERIVABLE_TYPE (GimpLabelSpin, gimp_label_spin, GIMP, LABEL_SPIN, GimpLabeled)

struct _GimpLabelSpinClass
{
  GimpLabeledClass   parent_class;

  /*  Signals */
  void            (* value_changed)    (GimpLabelSpin *spin);

  /* Padding for future expansion */
  void (* _gimp_reserved0) (void);
  void (* _gimp_reserved1) (void);
  void (* _gimp_reserved2) (void);
  void (* _gimp_reserved3) (void);
  void (* _gimp_reserved4) (void);
  void (* _gimp_reserved5) (void);
  void (* _gimp_reserved6) (void);
  void (* _gimp_reserved7) (void);
  void (* _gimp_reserved8) (void);
  void (* _gimp_reserved9) (void);
};

GtkWidget  * gimp_label_spin_new             (const gchar   *text,
                                              gdouble        value,
                                              gdouble        lower,
                                              gdouble        upper,
                                              gint           digits);

void         gimp_label_spin_set_value       (GimpLabelSpin *spin,
                                              gdouble        value);
gdouble      gimp_label_spin_get_value       (GimpLabelSpin *spin);

void         gimp_label_spin_set_increments  (GimpLabelSpin *spin,
                                              gdouble        step,
                                              gdouble        page);
void         gimp_label_spin_set_digits      (GimpLabelSpin *spin,
                                              gint           digits);

GtkWidget  * gimp_label_spin_get_spin_button (GimpLabelSpin *spin);

G_END_DECLS

#endif /* __GIMP_LABEL_SPIN_H__ */
