/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpunitcombobox.c
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

#include "widgets-types.h"

#include "gimpunitcombobox.h"
#include "gimpunitstore.h"


G_DEFINE_TYPE (GimpUnitComboBox, gimp_unit_combo_box, GTK_TYPE_COMBO_BOX)

#define parent_class gimp_unit_combo_box_parent_class


static void
gimp_unit_combo_box_class_init (GimpUnitComboBoxClass *klass)
{
}

static void
gimp_unit_combo_box_init (GimpUnitComboBox *combo)
{
  GtkCellLayout   *layout = GTK_CELL_LAYOUT (combo);
  GtkCellRenderer *cell;

  cell = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (layout, cell, TRUE);
  gtk_cell_layout_set_attributes (layout, cell,
                                  "text", GIMP_UNIT_STORE_UNIT_PLURAL,
                                  NULL);

  /*  hackedehack ...  */
  layout = GTK_CELL_LAYOUT (GTK_BIN (combo)->child);
  gtk_cell_layout_clear (layout);

  cell = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (layout, cell, TRUE);
  gtk_cell_layout_set_attributes (layout, cell,
                                  "text", GIMP_UNIT_STORE_UNIT_ABBREVIATION,
                                  NULL);
}

/**
 * gimp_unit_combo_box_new:
 *
 * Return value: a new #GimpUnitComboBox.
 **/
GtkWidget *
gimp_unit_combo_box_new (void)
{
  GtkWidget     *combo_box;
  GimpUnitStore *store;

  store = gimp_unit_store_new (0);

  combo_box = g_object_new (GIMP_TYPE_UNIT_COMBO_BOX,
                            "model", store,
                            NULL);

  g_object_unref (store);

  return combo_box;
}

/**
 * gimp_unit_combo_box_new_with_model:
 * @model: a GimpUnitStore
 *
 * Return value: a new #GimpUnitComboBox.
 **/
GtkWidget *
gimp_unit_combo_box_new_with_model (GimpUnitStore *model)
{
  return g_object_new (GIMP_TYPE_UNIT_COMBO_BOX,
                       "model", model,
                       NULL);
}

GimpUnit
gimp_unit_combo_box_get_active (GimpUnitComboBox *combo)
{
  g_return_val_if_fail (GIMP_IS_UNIT_COMBO_BOX (combo), -1);

  return gtk_combo_box_get_active (GTK_COMBO_BOX (combo));
}

void
gimp_unit_combo_box_set_active (GimpUnitComboBox *combo,
                                GimpUnit          unit)
{
  g_return_if_fail (GIMP_IS_UNIT_COMBO_BOX (combo));

  gtk_combo_box_set_active (GTK_COMBO_BOX (combo), unit);
}
