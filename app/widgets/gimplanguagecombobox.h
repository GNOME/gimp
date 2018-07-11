/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimplanguagecombobox.h
 * Copyright (C) 2009  Sven Neumann <sven@gimp.org>
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

#ifndef __GIMP_LANGUAGE_COMBO_BOX_H__
#define __GIMP_LANGUAGE_COMBO_BOX_H__


#define GIMP_TYPE_LANGUAGE_COMBO_BOX            (gimp_language_combo_box_get_type ())
#define GIMP_LANGUAGE_COMBO_BOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_LANGUAGE_COMBO_BOX, GimpLanguageComboBox))
#define GIMP_LANGUAGE_COMBO_BOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_LANGUAGE_COMBO_BOX, GimpLanguageComboBoxClass))
#define GIMP_IS_LANGUAGE_COMBO_BOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_LANGUAGE_COMBO_BOX))
#define GIMP_IS_LANGUAGE_COMBO_BOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_LANGUAGE_COMBO_BOX))
#define GIMP_LANGUAGE_COMBO_BOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_LANGUAGE_COMBO_BOX, GimpLanguageComboBoxClass))


typedef struct _GimpLanguageComboBoxClass  GimpLanguageComboBoxClass;

struct _GimpLanguageComboBoxClass
{
  GtkComboBoxClass  parent_class;
};


GType       gimp_language_combo_box_get_type (void) G_GNUC_CONST;

GtkWidget * gimp_language_combo_box_new      (gboolean              manual_l18n,
                                              const gchar          *empty_label);

gchar     * gimp_language_combo_box_get_code (GimpLanguageComboBox *combo);
gboolean    gimp_language_combo_box_set_code (GimpLanguageComboBox *combo,
                                              const gchar          *code);


#endif  /* __GIMP_LANGUAGE_COMBO_BOX_H__ */
