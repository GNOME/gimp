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


#define GIMP_TYPE_SPIN_BUTTON            (gimp_spin_button_get_type ())
#define GIMP_SPIN_BUTTON(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_SPIN_BUTTON, GimpSpinButton))
#define GIMP_SPIN_BUTTON_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_SPIN_BUTTON, GimpSpinButtonClass))
#define GIMP_IS_SPIN_BUTTON(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_SPIN_BUTTON))
#define GIMP_IS_SPIN_BUTTON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_SPIN_BUTTON))
#define GIMP_SPIN_BUTTON_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_SPIN_BUTTON, GimpSpinButtonClass))


typedef struct _GimpSpinButtonPrivate  GimpSpinButtonPrivate;
typedef struct _GimpSpinButtonClass    GimpSpinButtonClass;

struct _GimpSpinButton
{
  GtkSpinButton          parent_instance;

  GimpSpinButtonPrivate *priv;
};

struct _GimpSpinButtonClass
{
  GtkSpinButtonClass  parent_class;

  /* Padding for future expansion */
  void (* _gimp_reserved1) (void);
  void (* _gimp_reserved2) (void);
  void (* _gimp_reserved3) (void);
  void (* _gimp_reserved4) (void);
};


GType       gimp_spin_button_get_type       (void) G_GNUC_CONST;

GtkWidget * gimp_spin_button_new_           (GtkAdjustment *adjustment,
                                             gdouble        climb_rate,
                                             guint          digits);
GtkWidget * gimp_spin_button_new_with_range (gdouble        min,
                                             gdouble        max,
                                             gdouble        step);


/* compatibility magic, expanding to either the old (deprecated)
 * gimp_spin_button_new(), defined in gimpwidgets.h, or the new
 * gimp_spin_button_new(), defined here, based on the number of arguments.
 */
#define gimp_spin_button_new(...) gimp_spin_button_new_I (__VA_ARGS__, \
                                                          9, , , , , , 3)
#define gimp_spin_button_new_I(_1, _2, _3, _4, _5, _6, _7, _8, _9, n, ...) \
  gimp_spin_button_new_I_##n (_1, _2, _3, _4, _5, _6, _7, _8, _9)
#define gimp_spin_button_new_I_3(_1, _2, _3, _4, _5, _6, _7, _8, _9) \
  gimp_spin_button_new_ (_1, _2, _3)
#define gimp_spin_button_new_I_9(_1, _2, _3, _4, _5, _6, _7, _8, _9) \
  gimp_spin_button_new (_1, _2, _3, _4, _5, _6, _7, _8, _9)


G_END_DECLS

#endif /* __GIMP_SPIN_BUTTON_H__ */
