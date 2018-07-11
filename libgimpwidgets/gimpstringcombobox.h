/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpstringcombobox.h
 * Copyright (C) 2007  Sven Neumann <sven@gimp.org>
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

#ifndef __GIMP_STRING_COMBO_BOX_H__
#define __GIMP_STRING_COMBO_BOX_H__

G_BEGIN_DECLS


#define GIMP_TYPE_STRING_COMBO_BOX            (gimp_string_combo_box_get_type ())
#define GIMP_STRING_COMBO_BOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_STRING_COMBO_BOX, GimpStringComboBox))
#define GIMP_STRING_COMBO_BOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_STRING_COMBO_BOX, GimpStringComboBoxClass))
#define GIMP_IS_STRING_COMBO_BOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_STRING_COMBO_BOX))
#define GIMP_IS_STRING_COMBO_BOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_STRING_COMBO_BOX))
#define GIMP_STRING_COMBO_BOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_STRING_COMBO_BOX, GimpStringComboBoxClass))


typedef struct _GimpStringComboBoxClass  GimpStringComboBoxClass;

struct _GimpStringComboBox
{
  GtkComboBox       parent_instance;

  /*< private >*/
  gpointer          priv;
};

struct _GimpStringComboBoxClass
{
  GtkComboBoxClass  parent_class;

  /* Padding for future expansion */
  void (* _gimp_reserved1) (void);
  void (* _gimp_reserved2) (void);
  void (* _gimp_reserved3) (void);
  void (* _gimp_reserved4) (void);
};


GType       gimp_string_combo_box_get_type   (void) G_GNUC_CONST;

GtkWidget * gimp_string_combo_box_new        (GtkTreeModel       *model,
                                              gint                id_column,
                                              gint                label_column);
gboolean    gimp_string_combo_box_set_active (GimpStringComboBox *combo_box,
                                              const gchar        *id);
gchar     * gimp_string_combo_box_get_active (GimpStringComboBox *combo_box);


G_END_DECLS

#endif  /* __GIMP_STRING_COMBO_BOX_H__ */
