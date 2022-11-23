/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmalanguagecombobox.h
 * Copyright (C) 2009  Sven Neumann <sven@ligma.org>
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

#ifndef __LIGMA_LANGUAGE_COMBO_BOX_H__
#define __LIGMA_LANGUAGE_COMBO_BOX_H__


#define LIGMA_TYPE_LANGUAGE_COMBO_BOX            (ligma_language_combo_box_get_type ())
#define LIGMA_LANGUAGE_COMBO_BOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_LANGUAGE_COMBO_BOX, LigmaLanguageComboBox))
#define LIGMA_LANGUAGE_COMBO_BOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_LANGUAGE_COMBO_BOX, LigmaLanguageComboBoxClass))
#define LIGMA_IS_LANGUAGE_COMBO_BOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_LANGUAGE_COMBO_BOX))
#define LIGMA_IS_LANGUAGE_COMBO_BOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_LANGUAGE_COMBO_BOX))
#define LIGMA_LANGUAGE_COMBO_BOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_LANGUAGE_COMBO_BOX, LigmaLanguageComboBoxClass))


typedef struct _LigmaLanguageComboBoxClass  LigmaLanguageComboBoxClass;

struct _LigmaLanguageComboBoxClass
{
  GtkComboBoxClass  parent_class;
};


GType       ligma_language_combo_box_get_type (void) G_GNUC_CONST;

GtkWidget * ligma_language_combo_box_new      (gboolean              manual_l18n,
                                              const gchar          *empty_label);

gchar     * ligma_language_combo_box_get_code (LigmaLanguageComboBox *combo);
gboolean    ligma_language_combo_box_set_code (LigmaLanguageComboBox *combo,
                                              const gchar          *code);


#endif  /* __LIGMA_LANGUAGE_COMBO_BOX_H__ */
