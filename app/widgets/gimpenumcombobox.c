/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpenumcombobox.c
 * Copyright (C) 2004  Sven Neumann <sven@gimp.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "gimpenumcombobox.h"
#include "gimpenumstore.h"

#include "gimp-intl.h"


GType
gimp_enum_combo_box_get_type (void)
{
  static GType enum_combo_box_type = 0;

  if (!enum_combo_box_type)
    {
      static const GTypeInfo enum_combo_box_info =
      {
        sizeof (GimpEnumComboBoxClass),
        NULL,           /* base_init      */
        NULL,           /* base_finalize  */
        NULL,           /* class_init     */
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (GimpEnumComboBox),
        0,              /* n_preallocs    */
        NULL            /* instance_init  */
      };

      enum_combo_box_type = g_type_register_static (GTK_TYPE_COMBO_BOX,
                                                    "GimpEnumComboBox",
                                                    &enum_combo_box_info, 0);
    }

  return enum_combo_box_type;
}

/**
 * gimp_enum_combo_box_new:
 * @enum_type: the #GType of an enum.
 *
 * Creates a new #GtkComboBox using the #GimpEnumStore. The enum needs
 * to be registered to the type system and should have translatable
 * value names.
 *
 * Return value: a new #GimpEnumComboBox.
 **/
GtkWidget *
gimp_enum_combo_box_new (GType enum_type)
{
  GtkListStore *store;
  GtkWidget    *combo_box;

  g_return_val_if_fail (G_TYPE_IS_ENUM (enum_type), NULL);

  store = gimp_enum_store_new (enum_type);

  combo_box = gimp_enum_combo_box_new_with_model (GIMP_ENUM_STORE (store));

  g_object_unref (store);

  return combo_box;
}

GtkWidget *
gimp_enum_combo_box_new_with_model (GimpEnumStore *enum_store)
{
  GtkWidget       *combo_box;
  GtkCellRenderer *cell;

  g_return_val_if_fail (GIMP_IS_ENUM_STORE (enum_store), NULL);

  combo_box = g_object_new (GIMP_TYPE_ENUM_COMBO_BOX,
                            "model", enum_store,
                            NULL);

  cell = gtk_cell_renderer_pixbuf_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo_box), cell, FALSE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo_box), cell,
                                  "pixbuf", GIMP_ENUM_STORE_ICON,
                                  NULL);

  cell = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo_box), cell, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo_box), cell,
                                  "text", GIMP_ENUM_STORE_LABEL,
                                  NULL);

  return combo_box;
}

gboolean
gimp_enum_combo_box_set_active (GimpEnumComboBox *combo_box,
                                gint              value)
{
  GtkTreeModel *model;
  GtkTreeIter   iter;

  g_return_val_if_fail (GIMP_IS_ENUM_COMBO_BOX (combo_box), FALSE);

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo_box));

  if (gimp_enum_store_lookup_by_value (model, value, &iter))
    {
      gtk_combo_box_set_active_iter (GTK_COMBO_BOX (combo_box), &iter);
      return TRUE;
    }

  return FALSE;
}

gboolean
gimp_enum_combo_box_get_active (GimpEnumComboBox *combo_box,
                                gint             *value)
{
  GtkTreeIter  iter;

  g_return_val_if_fail (GIMP_IS_ENUM_COMBO_BOX (combo_box), FALSE);

  if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (combo_box), &iter))
    {
      gtk_tree_model_get (gtk_combo_box_get_model (GTK_COMBO_BOX (combo_box)),
                          &iter,
                          GIMP_ENUM_STORE_VALUE, value,
                          -1);
      return TRUE;
    }

  return FALSE;
}

void
gimp_enum_combo_box_set_stock_prefix (GimpEnumComboBox *combo_box,
                                      const gchar      *stock_prefix)
{
  GtkTreeModel *model;

  g_return_if_fail (GIMP_IS_ENUM_COMBO_BOX (combo_box));

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo_box));

  gimp_enum_store_set_icons (GIMP_ENUM_STORE (model),
                             GTK_WIDGET (combo_box),
                             stock_prefix, GTK_ICON_SIZE_MENU);
}

/*  This is a kludge to allow to work around bug #135875  */
void
gimp_enum_combo_box_set_visible (GimpEnumComboBox *combo_box,
                                 GtkTreeModelFilterVisibleFunc func,
                                 gpointer          data)
{
  GtkTreeModel       *model;
  GtkTreeModelFilter *filter;

  g_return_if_fail (GIMP_IS_ENUM_COMBO_BOX (combo_box));

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo_box));

  if (GTK_IS_TREE_MODEL_FILTER (model))
    {
      filter = GTK_TREE_MODEL_FILTER (model);
    }
  else
    {
      filter = GTK_TREE_MODEL_FILTER (gtk_tree_model_filter_new (model, NULL));

      gtk_combo_box_set_model (GTK_COMBO_BOX (combo_box),
                               GTK_TREE_MODEL (filter));

      g_object_unref (filter);
    }

  gtk_tree_model_filter_set_visible_func (filter, func, data, NULL);
  gtk_tree_model_filter_refilter (filter);
}
