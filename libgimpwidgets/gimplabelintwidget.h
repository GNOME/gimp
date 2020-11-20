/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimplabelintwidget.h
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

#ifndef __GIMP_LABEL_INT_WIDGET_H__
#define __GIMP_LABEL_INT_WIDGET_H__

#include <libgimpwidgets/gimplabeled.h>

G_BEGIN_DECLS

#define GIMP_TYPE_LABEL_INT_WIDGET (gimp_label_int_widget_get_type ())
G_DECLARE_DERIVABLE_TYPE (GimpLabelIntWidget, gimp_label_int_widget, GIMP, LABEL_INT_WIDGET, GimpLabeled)

struct _GimpLabelIntWidgetClass
{
  GimpLabeledClass   parent_class;

  /*  Signals */
  void            (* value_changed)    (GtkWidget *int_widget);

  /* Padding for future expansion */
  void (* _gimp_reserved1) (void);
  void (* _gimp_reserved2) (void);
  void (* _gimp_reserved3) (void);
  void (* _gimp_reserved4) (void);
  void (* _gimp_reserved5) (void);
  void (* _gimp_reserved6) (void);
  void (* _gimp_reserved7) (void);
  void (* _gimp_reserved8) (void);
};

GtkWidget  * gimp_label_int_widget_new        (const gchar        *text,
                                               GtkWidget          *widget);

GtkWidget  * gimp_label_int_widget_get_widget (GimpLabelIntWidget *widget);


G_END_DECLS

#endif /* __GIMP_LABEL_INT_WIDGET_H__ */
