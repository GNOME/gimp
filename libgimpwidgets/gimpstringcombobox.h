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


/**
 * GimpStringSensitivityFunc:
 * @id: the string value from the column @id_column as passed to [ctor@StringComboBox.new].
 * @data: (closure): the data passed in [method@StringComboBox.set_sensitivity].
 */
typedef  gboolean (* GimpStringSensitivityFunc) (const gchar *id,
                                                 gpointer     data);


#define GIMP_TYPE_STRING_COMBO_BOX (gimp_string_combo_box_get_type ())
G_DECLARE_DERIVABLE_TYPE (GimpStringComboBox, gimp_string_combo_box, GIMP, STRING_COMBO_BOX, GtkComboBox)

struct _GimpStringComboBoxClass
{
  GtkComboBoxClass  parent_class;

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


GtkWidget * gimp_string_combo_box_new             (GtkTreeModel              *model,
                                                   gint                       id_column,
                                                   gint                       label_column);
gboolean    gimp_string_combo_box_set_active      (GimpStringComboBox        *combo_box,
                                                   const gchar               *id);
gchar     * gimp_string_combo_box_get_active      (GimpStringComboBox        *combo_box);

void        gimp_string_combo_box_set_sensitivity (GimpStringComboBox        *combo_box,
                                                   GimpStringSensitivityFunc  func,
                                                   gpointer                   data,
                                                   GDestroyNotify             destroy);

G_END_DECLS

#endif  /* __GIMP_STRING_COMBO_BOX_H__ */
