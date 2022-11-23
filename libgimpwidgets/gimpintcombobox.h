/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmaintcombobox.h
 * Copyright (C) 2004  Sven Neumann <sven@ligma.org>
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

#if !defined (__LIGMA_WIDGETS_H_INSIDE__) && !defined (LIGMA_WIDGETS_COMPILATION)
#error "Only <libligmawidgets/ligmawidgets.h> can be included directly."
#endif

#ifndef __LIGMA_INT_COMBO_BOX_H__
#define __LIGMA_INT_COMBO_BOX_H__

G_BEGIN_DECLS


#define LIGMA_TYPE_INT_COMBO_BOX            (ligma_int_combo_box_get_type ())
#define LIGMA_INT_COMBO_BOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_INT_COMBO_BOX, LigmaIntComboBox))
#define LIGMA_INT_COMBO_BOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_INT_COMBO_BOX, LigmaIntComboBoxClass))
#define LIGMA_IS_INT_COMBO_BOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_INT_COMBO_BOX))
#define LIGMA_IS_INT_COMBO_BOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_INT_COMBO_BOX))
#define LIGMA_INT_COMBO_BOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_INT_COMBO_BOX, LigmaIntComboBoxClass))


typedef struct _LigmaIntComboBoxPrivate LigmaIntComboBoxPrivate;
typedef struct _LigmaIntComboBoxClass   LigmaIntComboBoxClass;

struct _LigmaIntComboBox
{
  GtkComboBox             parent_instance;

  LigmaIntComboBoxPrivate *priv;
};

struct _LigmaIntComboBoxClass
{
  GtkComboBoxClass  parent_class;

  /* Padding for future expansion */
  void (* _ligma_reserved1) (void);
  void (* _ligma_reserved2) (void);
  void (* _ligma_reserved3) (void);
  void (* _ligma_reserved4) (void);
  void (* _ligma_reserved5) (void);
  void (* _ligma_reserved6) (void);
  void (* _ligma_reserved7) (void);
  void (* _ligma_reserved8) (void);
};


/**
 * LigmaIntSensitivityFunc:
 * @value:
 * @data: (closure):
 */
typedef  gboolean (* LigmaIntSensitivityFunc) (gint      value,
                                              gpointer  data);



GType         ligma_int_combo_box_get_type        (void) G_GNUC_CONST;

GtkWidget   * ligma_int_combo_box_new             (const gchar     *first_label,
                                                  gint             first_value,
                                                  ...) G_GNUC_NULL_TERMINATED;
GtkWidget   * ligma_int_combo_box_new_valist      (const gchar     *first_label,
                                                  gint             first_value,
                                                  va_list          values);

GtkWidget   * ligma_int_combo_box_new_array       (gint             n_values,
                                                  const gchar     *labels[]);

void          ligma_int_combo_box_prepend         (LigmaIntComboBox *combo_box,
                                                  ...);
void          ligma_int_combo_box_append          (LigmaIntComboBox *combo_box,
                                                  ...);

gboolean      ligma_int_combo_box_set_active      (LigmaIntComboBox *combo_box,
                                                  gint             value);
gboolean      ligma_int_combo_box_get_active      (LigmaIntComboBox *combo_box,
                                                  gint            *value);

gboolean
      ligma_int_combo_box_set_active_by_user_data (LigmaIntComboBox *combo_box,
                                                  gpointer         user_data);
gboolean
      ligma_int_combo_box_get_active_user_data    (LigmaIntComboBox *combo_box,
                                                  gpointer        *user_data);

gulong        ligma_int_combo_box_connect         (LigmaIntComboBox *combo_box,
                                                  gint             value,
                                                  GCallback        callback,
                                                  gpointer         data,
                                                  GDestroyNotify   data_destroy);

void          ligma_int_combo_box_set_label       (LigmaIntComboBox *combo_box,
                                                  const gchar     *label);
const gchar * ligma_int_combo_box_get_label       (LigmaIntComboBox *combo_box);

void          ligma_int_combo_box_set_layout      (LigmaIntComboBox       *combo_box,
                                                  LigmaIntComboBoxLayout  layout);
LigmaIntComboBoxLayout
              ligma_int_combo_box_get_layout      (LigmaIntComboBox       *combo_box);

void          ligma_int_combo_box_set_sensitivity (LigmaIntComboBox        *combo_box,
                                                  LigmaIntSensitivityFunc  func,
                                                  gpointer                data,
                                                  GDestroyNotify          destroy);


G_END_DECLS

#endif  /* __LIGMA_INT_COMBO_BOX_H__ */
