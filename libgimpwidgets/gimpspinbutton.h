/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpspinbutton.h
 * Copyright (C) 2018 Ell
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

#ifndef __GIMP_SPIN_BUTTON_H__
#define __GIMP_SPIN_BUTTON_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#define GIMP_TYPE_SPIN_BUTTON (gimp_spin_button_get_type ())
G_DECLARE_DERIVABLE_TYPE (GimpSpinButton, gimp_spin_button, GIMP, SPIN_BUTTON, GtkSpinButton)


struct _GimpSpinButtonClass
{
  GtkSpinButtonClass  parent_class;

  /* Padding for future expansion */
  void (* _gimp_reserved1) (void);
  void (* _gimp_reserved2) (void);
  void (* _gimp_reserved3) (void);
  void (* _gimp_reserved4) (void);
};


GtkWidget * gimp_spin_button_new            (GtkAdjustment *adjustment,
                                             gdouble        climb_rate,
                                             guint          digits);
GtkWidget * gimp_spin_button_new_with_range (gdouble        min,
                                             gdouble        max,
                                             gdouble        step);


G_END_DECLS

#endif /* __GIMP_SPIN_BUTTON_H__ */
