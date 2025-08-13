/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpintcombobox.h
 * Copyright (C) 2004  Sven Neumann <sven@gimp.org>
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

#ifndef __GIMP_INT_COMBO_BOX_H__
#define __GIMP_INT_COMBO_BOX_H__

G_BEGIN_DECLS


#define GIMP_TYPE_INT_COMBO_BOX (gimp_int_combo_box_get_type ())
G_DECLARE_DERIVABLE_TYPE (GimpIntComboBox, gimp_int_combo_box, GIMP, INT_COMBO_BOX, GtkComboBox)


struct _GimpIntComboBoxClass
{
  GtkComboBoxClass  parent_class;

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


/**
 * GimpIntSensitivityFunc:
 * @value:
 * @data: (closure):
 */
typedef  gboolean (* GimpIntSensitivityFunc) (gint      value,
                                              gpointer  data);


GtkWidget   * gimp_int_combo_box_new             (const gchar     *first_label,
                                                  gint             first_value,
                                                  ...) G_GNUC_NULL_TERMINATED;
GtkWidget   * gimp_int_combo_box_new_valist      (const gchar     *first_label,
                                                  gint             first_value,
                                                  va_list          values);

GtkWidget   * gimp_int_combo_box_new_array       (gint             n_values,
                                                  const gchar     *labels[]);

void          gimp_int_combo_box_prepend         (GimpIntComboBox *combo_box,
                                                  ...);
void          gimp_int_combo_box_append          (GimpIntComboBox *combo_box,
                                                  ...);

gboolean      gimp_int_combo_box_set_active      (GimpIntComboBox *combo_box,
                                                  gint             value);
gboolean      gimp_int_combo_box_get_active      (GimpIntComboBox *combo_box,
                                                  gint            *value);

gboolean
      gimp_int_combo_box_set_active_by_user_data (GimpIntComboBox *combo_box,
                                                  gpointer         user_data);
gboolean
      gimp_int_combo_box_get_active_user_data    (GimpIntComboBox *combo_box,
                                                  gpointer        *user_data);

gulong        gimp_int_combo_box_connect         (GimpIntComboBox *combo_box,
                                                  gint             value,
                                                  GCallback        callback,
                                                  gpointer         data,
                                                  GDestroyNotify   data_destroy);

void          gimp_int_combo_box_set_label       (GimpIntComboBox *combo_box,
                                                  const gchar     *label);
const gchar * gimp_int_combo_box_get_label       (GimpIntComboBox *combo_box);

void          gimp_int_combo_box_set_layout      (GimpIntComboBox       *combo_box,
                                                  GimpIntComboBoxLayout  layout);
GimpIntComboBoxLayout
              gimp_int_combo_box_get_layout      (GimpIntComboBox       *combo_box);

void          gimp_int_combo_box_set_sensitivity (GimpIntComboBox        *combo_box,
                                                  GimpIntSensitivityFunc  func,
                                                  gpointer                data,
                                                  GDestroyNotify          destroy);


G_END_DECLS

#endif  /* __GIMP_INT_COMBO_BOX_H__ */
