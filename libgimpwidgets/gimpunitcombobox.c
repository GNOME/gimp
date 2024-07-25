/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1999 Peter Mattis and Spencer Kimball
 *
 * gimpunitcombobox.c
 * Copyright (C) 2004, 2008  Sven Neumann <sven@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "gimpwidgetstypes.h"

#include "gimpunitcombobox.h"
#include "gimpunitstore.h"


/**
 * SECTION: gimpunitcombobox
 * @title: GimpUnitComboBox
 * @short_description: A #GtkComboBox to select a #GimpUnit.
 * @see_also: #GimpUnit, #GimpUnitStore
 *
 * #GimpUnitComboBox selects units stored in a #GimpUnitStore.
 **/


static void gimp_unit_combo_box_popup_shown (GimpUnitComboBox *widget);
static void gimp_unit_combo_box_constructed (GObject          *object);

G_DEFINE_TYPE (GimpUnitComboBox, gimp_unit_combo_box, GTK_TYPE_COMBO_BOX)

#define parent_class gimp_unit_combo_box_parent_class


static void
gimp_unit_combo_box_class_init (GimpUnitComboBoxClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = gimp_unit_combo_box_constructed;
}

static void
gimp_unit_combo_box_init (GimpUnitComboBox *combo)
{
  g_signal_connect (combo, "notify::popup-shown",
                    G_CALLBACK (gimp_unit_combo_box_popup_shown),
                    NULL);
}

static void
gimp_unit_combo_box_constructed (GObject *object)
{
  GimpUnitComboBox *combo = GIMP_UNIT_COMBO_BOX (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gimp_unit_combo_box_popup_shown (combo);
}

static void
gimp_unit_combo_box_popup_shown (GimpUnitComboBox *widget)
{
  GimpUnitStore   *store;
  gboolean         shown;
  GtkCellRenderer *cell;

  g_object_get (widget,
                "model",       &store,
                "popup-shown", &shown,
                NULL);

  if (store)
    {
      if (shown)
        _gimp_unit_store_sync_units (store);

      g_object_unref (store);
    }

  gtk_cell_layout_clear (GTK_CELL_LAYOUT (widget));
  cell = gtk_cell_renderer_text_new ();

  if (shown)
    {
      gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (widget), cell, TRUE);
      gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (widget), cell,
                                      "text", GIMP_UNIT_STORE_UNIT_LONG_FORMAT,
                                      NULL);

      /* XXX This is ugly but it seems that GtkComboBox won't resize its popup
       * menu when the contents changes (it only resizes the main "chosen item"
       * area). We force a redraw by popping down then up after a contents
       * change.
       */
      g_signal_handlers_disconnect_by_func (widget,
                                            G_CALLBACK (gimp_unit_combo_box_popup_shown),
                                            NULL);
      gtk_combo_box_popdown (GTK_COMBO_BOX (widget));
      gtk_combo_box_popup (GTK_COMBO_BOX (widget));
      g_signal_connect (widget, "notify::popup-shown",
                        G_CALLBACK (gimp_unit_combo_box_popup_shown),
                        NULL);
    }
  else
    {
      gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (widget), cell, FALSE);
      gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (widget), cell,
                                      "text", GIMP_UNIT_STORE_UNIT_SHORT_FORMAT,
                                      NULL);
    }
}


/**
 * gimp_unit_combo_box_new:
 *
 * Returns: a new #GimpUnitComboBox.
 **/
GtkWidget *
gimp_unit_combo_box_new (void)
{
  GtkWidget     *combo_box;
  GimpUnitStore *store;

  store = gimp_unit_store_new (0);

  combo_box = g_object_new (GIMP_TYPE_UNIT_COMBO_BOX,
                            "model",     store,
                            "id-column", GIMP_UNIT_STORE_UNIT_LONG_FORMAT,
                            NULL);

  g_object_unref (store);

  return combo_box;
}

/**
 * gimp_unit_combo_box_new_with_model:
 * @model: a #GimpUnitStore
 *
 * Returns: a new #GimpUnitComboBox.
 **/
GtkWidget *
gimp_unit_combo_box_new_with_model (GimpUnitStore *model)
{
  return g_object_new (GIMP_TYPE_UNIT_COMBO_BOX,
                       "model",     model,
                       "id-column", GIMP_UNIT_STORE_UNIT_LONG_FORMAT,
                       NULL);
}

/**
 * gimp_unit_combo_box_get_active:
 * @combo: a #GimpUnitComboBox
 *
 * Returns the #GimpUnit currently selected in the combo box.
 *
 * Returns: (transfer none): The selected #GimpUnit.
 **/
GimpUnit *
gimp_unit_combo_box_get_active (GimpUnitComboBox *combo)
{
  GtkTreeIter  iter;
  GimpUnit    *unit;

  g_return_val_if_fail (GIMP_IS_UNIT_COMBO_BOX (combo), NULL);

  gtk_combo_box_get_active_iter (GTK_COMBO_BOX (combo), &iter);

  gtk_tree_model_get (gtk_combo_box_get_model (GTK_COMBO_BOX (combo)), &iter,
                      GIMP_UNIT_STORE_UNIT, &unit,
                      -1);

  return unit;
}

/**
 * gimp_unit_combo_box_set_active:
 * @combo: a #GimpUnitComboBox
 * @unit:  a #GimpUnit
 *
 * Sets @unit as the currently selected #GimpUnit on @combo.
 **/
void
gimp_unit_combo_box_set_active (GimpUnitComboBox *combo,
                                GimpUnit         *unit)
{
  GtkTreeModel *model;
  GtkTreeIter   iter;
  gboolean      iter_valid;

  g_return_if_fail (GIMP_IS_UNIT_COMBO_BOX (combo));

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));

  _gimp_unit_store_sync_units (GIMP_UNIT_STORE (model));

  for (iter_valid = gtk_tree_model_get_iter_first (model, &iter);
       iter_valid;
       iter_valid = gtk_tree_model_iter_next (model, &iter))
    {
      GimpUnit *iter_unit;

      gtk_tree_model_get (model, &iter,
                          GIMP_UNIT_STORE_UNIT, &iter_unit,
                          -1);

      if (unit == iter_unit)
        {
          gtk_combo_box_set_active_iter (GTK_COMBO_BOX (combo), &iter);
          break;
        }
    }
}
