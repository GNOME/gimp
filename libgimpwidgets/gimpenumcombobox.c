/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpenumcombobox.c
 * Copyright (C) 2004  Sven Neumann <sven@gimp.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "gimpwidgetstypes.h"

#include "gimpenumcombobox.h"
#include "gimpenumstore.h"

#include "libgimp/libgimp-intl.h"


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

      enum_combo_box_type = g_type_register_static (GIMP_TYPE_INT_COMBO_BOX,
                                                    "GimpEnumComboBox",
                                                    &enum_combo_box_info, 0);
    }

  return enum_combo_box_type;
}

/**
 * gimp_enum_combo_box_new:
 * @enum_type: the #GType of an enum.
 *
 * Creates a #GtkComboBox readily filled with all enum values from a
 * given @enum_type. The enum needs to be registered to the type
 * system. It should also have %GimpEnumDesc descriptions registered
 * that contain translatable value names. This is the case for the
 * enums used in the GIMP PDB functions.
 *
 * This is just a convenience function. If you need more control over
 * the enum values that appear in the combo_box, you can create your
 * own #GimpEnumStore and use gimp_enum_combo_box_new_with_model().
 *
 * Return value: a new #GimpEnumComboBox.
 *
 * Since: GIMP 2.4
 **/
GtkWidget *
gimp_enum_combo_box_new (GType enum_type)
{
  GtkListStore *store;
  GtkWidget    *combo_box;

  g_return_val_if_fail (G_TYPE_IS_ENUM (enum_type), NULL);

  store = gimp_enum_store_new (enum_type);

  combo_box = g_object_new (GIMP_TYPE_ENUM_COMBO_BOX,
                            "model", store,
                            NULL);

  g_object_unref (store);

  return combo_box;
}

/**
 * gimp_enum_combo_box_set_stock_prefix:
 * @combo_box:    a #GimpEnumComboBox
 * @stock_prefix: a prefix to create icon stock ID from enum values
 *
 * Attempts to create stock icons for all items in the @combo_box. See
 * gimp_enum_store_set_icons() to find out what to use for
 * @stock_prefix.
 *
 * Since: GIMP 2.4
 **/
void
gimp_enum_combo_box_set_stock_prefix (GimpEnumComboBox *combo_box,
                                      const gchar      *stock_prefix)
{
  GtkTreeModel *model;

  g_return_if_fail (GIMP_IS_ENUM_COMBO_BOX (combo_box));

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo_box));

  gimp_enum_store_set_stock_prefix (GIMP_ENUM_STORE (model), stock_prefix);
}

/**
 * gimp_enum_combo_box_set_visible:
 * @combo_box: a #GimpEnumComboBox
 * @func:      a #GtkTreeModelFilterVisibleFunc
 * @data:      a pointer that is passed to @func
 *
 * Sets a filter on the combo_box that selectively hides items. The
 * registered callback @func is called with an iter for each item and
 * must return %TRUE or %FALSE indicating whether the respective row
 * should be visible or not.
 *
 * This function must only be called once for a @combo_box. If you
 * want to refresh the visibility of the items in the @combo_box
 * later, call gtk_tree_model_filter_refilter() on the @combo_box's
 * model.
 *
 * This is a kludge to allow to work around the inability of
 * #GtkComboBox to set the sensitivity of it's items (bug #135875).
 * It should be removed as soon as this bug is fixed (probably with
 * GTK+-2.6).
 *
 * This function is only temporarily in GIMP 2.3, it will not become
 * part of the GIMP 2.0 API!!
 **/
void
gimp_enum_combo_box_set_visible (GimpEnumComboBox              *combo_box,
                                 GtkTreeModelFilterVisibleFunc  func,
                                 gpointer                       data)
{
  GtkTreeModel       *model;
  GtkTreeModelFilter *filter;

  g_return_if_fail (GIMP_IS_ENUM_COMBO_BOX (combo_box));
  g_return_if_fail (func != NULL);

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo_box));

  filter = GTK_TREE_MODEL_FILTER (gtk_tree_model_filter_new (model, NULL));
  gtk_combo_box_set_model (GTK_COMBO_BOX (combo_box), GTK_TREE_MODEL (filter));
  g_object_unref (filter);

  gtk_tree_model_filter_set_visible_func (filter, func, data, NULL);
  gtk_tree_model_filter_refilter (filter);
}
