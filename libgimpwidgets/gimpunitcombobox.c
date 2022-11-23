/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1999 Peter Mattis and Spencer Kimball
 *
 * ligmaunitcombobox.c
 * Copyright (C) 2004, 2008  Sven Neumann <sven@ligma.org>
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

#include "ligmawidgetstypes.h"

#include "ligmaunitcombobox.h"
#include "ligmaunitstore.h"


/**
 * SECTION: ligmaunitcombobox
 * @title: LigmaUnitComboBox
 * @short_description: A #GtkComboBox to select a #LigmaUnit.
 * @see_also: #LigmaUnit, #LigmaUnitStore
 *
 * #LigmaUnitComboBox selects units stored in a #LigmaUnitStore.
 **/


static void   ligma_unit_combo_box_style_updated (GtkWidget        *widget);
static void   ligma_unit_combo_box_popup_shown   (GtkWidget        *widget,
                                                 const GParamSpec *pspec);


G_DEFINE_TYPE (LigmaUnitComboBox, ligma_unit_combo_box, GTK_TYPE_COMBO_BOX)

#define parent_class ligma_unit_combo_box_parent_class


static void
ligma_unit_combo_box_class_init (LigmaUnitComboBoxClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  widget_class->style_updated = ligma_unit_combo_box_style_updated;
}

static void
ligma_unit_combo_box_init (LigmaUnitComboBox *combo)
{
  GtkCellLayout   *layout = GTK_CELL_LAYOUT (combo);
  GtkCellRenderer *cell;

  cell = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (layout, cell, TRUE);
  gtk_cell_layout_set_attributes (layout, cell,
                                  "text", LIGMA_UNIT_STORE_UNIT_LONG_FORMAT,
                                  NULL);

  g_signal_connect (combo, "notify::popup-shown",
                    G_CALLBACK (ligma_unit_combo_box_popup_shown),
                    NULL);
}

static void
ligma_unit_combo_box_style_updated (GtkWidget *widget)
{
  GtkCellLayout   *layout;
  GtkCellRenderer *cell;

  /*  hackedehack ...  */
  layout = GTK_CELL_LAYOUT (gtk_bin_get_child (GTK_BIN (widget)));
  gtk_cell_layout_clear (layout);

  cell = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (layout, cell, TRUE);
  gtk_cell_layout_set_attributes (layout, cell,
                                  "text",  LIGMA_UNIT_STORE_UNIT_SHORT_FORMAT,
                                  NULL);

  GTK_WIDGET_CLASS (parent_class)->style_updated (widget);
}

static void
ligma_unit_combo_box_popup_shown (GtkWidget        *widget,
                                 const GParamSpec *pspec)
{
  LigmaUnitStore *store;
  gboolean       shown;

  g_object_get (widget,
                "model",       &store,
                "popup-shown", &shown,
                NULL);

  if (store)
    {
      if (shown)
        _ligma_unit_store_sync_units (store);

      g_object_unref (store);
    }
}


/**
 * ligma_unit_combo_box_new:
 *
 * Returns: a new #LigmaUnitComboBox.
 **/
GtkWidget *
ligma_unit_combo_box_new (void)
{
  GtkWidget     *combo_box;
  LigmaUnitStore *store;

  store = ligma_unit_store_new (0);

  combo_box = g_object_new (LIGMA_TYPE_UNIT_COMBO_BOX,
                            "model", store,
                            NULL);

  g_object_unref (store);

  return combo_box;
}

/**
 * ligma_unit_combo_box_new_with_model:
 * @model: a #LigmaUnitStore
 *
 * Returns: a new #LigmaUnitComboBox.
 **/
GtkWidget *
ligma_unit_combo_box_new_with_model (LigmaUnitStore *model)
{
  return g_object_new (LIGMA_TYPE_UNIT_COMBO_BOX,
                       "model", model,
                       NULL);
}

/**
 * ligma_unit_combo_box_get_active:
 * @combo: a #LigmaUnitComboBox
 *
 * Returns the #LigmaUnit currently selected in the combo box.
 *
 * Returns: (transfer none): The selected #LigmaUnit.
 **/
LigmaUnit
ligma_unit_combo_box_get_active (LigmaUnitComboBox *combo)
{
  GtkTreeIter iter;
  gint        unit;

  g_return_val_if_fail (LIGMA_IS_UNIT_COMBO_BOX (combo), -1);

  gtk_combo_box_get_active_iter (GTK_COMBO_BOX (combo), &iter);

  gtk_tree_model_get (gtk_combo_box_get_model (GTK_COMBO_BOX (combo)), &iter,
                      LIGMA_UNIT_STORE_UNIT, &unit,
                      -1);

  return (LigmaUnit) unit;
}

/**
 * ligma_unit_combo_box_set_active:
 * @combo: a #LigmaUnitComboBox
 * @unit:  a #LigmaUnit
 *
 * Sets @unit as the currently selected #LigmaUnit on @combo.
 **/
void
ligma_unit_combo_box_set_active (LigmaUnitComboBox *combo,
                                LigmaUnit          unit)
{
  GtkTreeModel *model;
  GtkTreeIter   iter;
  gboolean      iter_valid;

  g_return_if_fail (LIGMA_IS_UNIT_COMBO_BOX (combo));

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));

  _ligma_unit_store_sync_units (LIGMA_UNIT_STORE (model));

  for (iter_valid = gtk_tree_model_get_iter_first (model, &iter);
       iter_valid;
       iter_valid = gtk_tree_model_iter_next (model, &iter))
    {
      gint iter_unit;

      gtk_tree_model_get (model, &iter,
                          LIGMA_UNIT_STORE_UNIT, &iter_unit,
                          -1);

      if (unit == (LigmaUnit) iter_unit)
        {
          gtk_combo_box_set_active_iter (GTK_COMBO_BOX (combo), &iter);
          break;
        }
    }
}
