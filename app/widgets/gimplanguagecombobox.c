/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimplanguagecombobox.c
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

/* GimpLanguageComboBox is a combo-box widget to select the user
 * interface language.
 */

#include "config.h"

#include <string.h>

#include <gtk/gtk.h>

#include "widgets-types.h"

#include "gimplanguagecombobox.h"
#include "gimptranslationstore.h"


struct _GimpLanguageComboBox
{
  GtkComboBox parent_instance;
};


G_DEFINE_TYPE (GimpLanguageComboBox,
               gimp_language_combo_box, GTK_TYPE_COMBO_BOX)

#define parent_class gimp_language_combo_box_parent_class


static void
gimp_language_combo_box_class_init (GimpLanguageComboBoxClass *klass)
{
}

static void
gimp_language_combo_box_init (GimpLanguageComboBox *combo)
{
  GtkCellRenderer *renderer;

  renderer = gtk_cell_renderer_text_new ();

  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), renderer, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo), renderer,
                                  "text",  GIMP_LANGUAGE_STORE_LABEL,
                                  NULL);
}

/**
 * gimp_language_combo_box_new:
 * @manual_l18n: get only the sublist of manual languages.
 * @empty_label: the label for empty language code.
 *
 * Returns a combo box containing all GUI localization languages if
 * @manual_l18n is %FALSE, or all manual localization languages
 * otherwise. If @empty_label is not %NULL, an entry with this label
 * will be created for the language code "", otherwise if @empty_label
 * is %NULL and @manual_l18n is %FALSE, the entry will be "System
 * Language" localized in itself (not in the GUI language).
 */
GtkWidget *
gimp_language_combo_box_new (gboolean     manual_l18n,
                             const gchar *empty_label)
{
  GtkWidget    *combo;
  GtkListStore *store;

  store = gimp_translation_store_new (manual_l18n, empty_label);
  combo = g_object_new (GIMP_TYPE_LANGUAGE_COMBO_BOX,
                        "model", store,
                        NULL);

  g_object_unref (store);

  return combo;
}

gchar *
gimp_language_combo_box_get_code (GimpLanguageComboBox *combo)
{
  GtkTreeIter  iter;
  gchar       *code;

  g_return_val_if_fail (GIMP_IS_LANGUAGE_COMBO_BOX (combo), NULL);

  if (! gtk_combo_box_get_active_iter (GTK_COMBO_BOX (combo), &iter))
    return NULL;

  gtk_tree_model_get (gtk_combo_box_get_model (GTK_COMBO_BOX (combo)), &iter,
                      GIMP_LANGUAGE_STORE_CODE, &code,
                      -1);

  return code;
}

gboolean
gimp_language_combo_box_set_code (GimpLanguageComboBox *combo,
                                  const gchar          *code)
{
  GtkTreeModel *model;
  GtkTreeIter   iter;

  g_return_val_if_fail (GIMP_IS_LANGUAGE_COMBO_BOX (combo), FALSE);

  if (! code || ! strlen (code))
    {
      gtk_combo_box_set_active (GTK_COMBO_BOX (combo), 0);
      return TRUE;
    }

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));

  if (gimp_language_store_lookup (GIMP_LANGUAGE_STORE (model), code, &iter))
    {
      gtk_combo_box_set_active_iter (GTK_COMBO_BOX (combo), &iter);
      return TRUE;
    }

  return FALSE;
}
