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
 * <http://www.gnu.org/licenses/>.
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
 * It replaces the deprecated #GimpUnitMenu.
 **/


static void   gimp_unit_combo_box_style_updated (GtkWidget        *widget);
static void   gimp_unit_combo_box_popup_shown   (GtkWidget        *widget,
                                                 const GParamSpec *pspec);


G_DEFINE_TYPE (GimpUnitComboBox, gimp_unit_combo_box, GTK_TYPE_COMBO_BOX)

#define parent_class gimp_unit_combo_box_parent_class


static void
gimp_unit_combo_box_class_init (GimpUnitComboBoxClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  widget_class->style_updated = gimp_unit_combo_box_style_updated;

  gtk_widget_class_install_style_property (widget_class,
                                           g_param_spec_double ("label-scale",
                                                                NULL, NULL,
                                                                0.0,
                                                                G_MAXDOUBLE,
                                                                1.0,
                                                                GIMP_PARAM_READABLE));
}

static void
gimp_unit_combo_box_init (GimpUnitComboBox *combo)
{
  GtkCellLayout   *layout = GTK_CELL_LAYOUT (combo);
  GtkCellRenderer *cell;

  cell = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (layout, cell, TRUE);
  gtk_cell_layout_set_attributes (layout, cell,
                                  "text", GIMP_UNIT_STORE_UNIT_LONG_FORMAT,
                                  NULL);

  g_signal_connect (combo, "notify::popup-shown",
                    G_CALLBACK (gimp_unit_combo_box_popup_shown),
                    NULL);
}

static void
gimp_unit_combo_box_style_updated (GtkWidget *widget)
{
  GtkCellLayout   *layout;
  GtkCellRenderer *cell;
  gdouble          scale;

  GTK_WIDGET_CLASS (parent_class)->style_updated (widget);

  gtk_widget_style_get (widget, "label-scale", &scale, NULL);

  /*  hackedehack ...  */
  layout = GTK_CELL_LAYOUT (gtk_bin_get_child (GTK_BIN (widget)));
  gtk_cell_layout_clear (layout);

  cell = g_object_new (GTK_TYPE_CELL_RENDERER_TEXT,
                       "scale", scale,
                       NULL);
  gtk_cell_layout_pack_start (layout, cell, TRUE);
  gtk_cell_layout_set_attributes (layout, cell,
                                  "text",  GIMP_UNIT_STORE_UNIT_SHORT_FORMAT,
                                  NULL);
}

static void
gimp_unit_combo_box_popup_shown (GtkWidget        *widget,
                                 const GParamSpec *pspec)
{
  GimpUnitStore *store;
  gboolean       shown;

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
  GtkTreeIter iter;
  gint        unit;

  g_return_val_if_fail (GIMP_IS_UNIT_COMBO_BOX (combo), -1);

  gtk_combo_box_get_active_iter (GTK_COMBO_BOX (combo), &iter);

  gtk_tree_model_get (gtk_combo_box_get_model (GTK_COMBO_BOX (combo)), &iter,
                      GIMP_UNIT_STORE_UNIT, &unit,
                      -1);

  return (GimpUnit) unit;
}

void
gimp_unit_combo_box_set_active (GimpUnitComboBox *combo,
                                GimpUnit          unit)
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
      gint iter_unit;

      gtk_tree_model_get (model, &iter,
                          GIMP_UNIT_STORE_UNIT, &iter_unit,
                          -1);

      if (unit == (GimpUnit) iter_unit)
        {
          gtk_combo_box_set_active_iter (GTK_COMBO_BOX (combo), &iter);
          break;
        }
    }
}
