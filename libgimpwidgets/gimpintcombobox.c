/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpintcombobox.c
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

#include <libintl.h>

#include <gtk/gtk.h>

#include "gimpwidgetstypes.h"

#include "gimpintcombobox.h"
#include "gimpintstore.h"


static void  gimp_int_combo_box_init (GimpIntComboBox *combo_box);


GType
gimp_int_combo_box_get_type (void)
{
  static GType box_type = 0;

  if (! box_type)
    {
      static const GTypeInfo box_info =
      {
        sizeof (GimpIntComboBoxClass),
        NULL,           /* base_init      */
        NULL,           /* base_finalize  */
        NULL,           /* class_init     */
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (GimpIntComboBox),
        0,              /* n_preallocs    */
        (GInstanceInitFunc) gimp_int_combo_box_init
      };

      box_type = g_type_register_static (GTK_TYPE_COMBO_BOX,
                                         "GimpIntComboBox",
                                         &box_info, 0);
    }

  return box_type;
}

static void
gimp_int_combo_box_init (GimpIntComboBox *combo_box)
{
  GtkListStore    *store;
  GtkCellRenderer *cell;

  store = gimp_int_store_new ();

  gtk_combo_box_set_model (GTK_COMBO_BOX (combo_box), GTK_TREE_MODEL (store));

  g_object_unref (store);

  cell = gtk_cell_renderer_pixbuf_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo_box), cell, FALSE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo_box), cell,
                                  "stock_id", GIMP_INT_STORE_STOCK_ID,
                                  "pixbuf",   GIMP_INT_STORE_PIXBUF,
                                  NULL);

  cell = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo_box), cell, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo_box), cell,
                                  "text", GIMP_INT_STORE_LABEL,
                                  NULL);
}


/**
 * gimp_int_combo_box_new:
 * @first_label: the label of the first item
 * @first_value: the value of the first item
 * @...: a %NULL terminated list of more label, value pairs
 *
 * Creates a GtkComboBox that has integer values associated with each
 * item. The items to fill the combo box with are specified as a %NULL
 * terminated list of label/value pairs.
 *
 * Return value: a new #GimpIntComboBox.
 *
 * Since: GIMP 2.2
 **/
GtkWidget *
gimp_int_combo_box_new (const gchar *first_label,
                        gint         first_value,
                        ...)
{
  GtkWidget *combo_box;
  va_list    args;

  g_return_val_if_fail (first_label != NULL, NULL);

  va_start (args, first_value);

  combo_box = gimp_int_combo_box_new_valist (first_label, first_value, args);

  va_end (args);

  return combo_box;
}

/**
 * gimp_int_combo_box_new_valist:
 * @first_label: the label of the first item
 * @first_value: the value of the first item
 * @values: a va_list with more values
 *
 * A variant of gimp_int_combo_box_new() that takes a va_list of
 * label/value pairs. Probably only useful for language bindings.
 *
 * Return value: a new #GimpIntComboBox.
 *
 * Since: GIMP 2.2
 **/
GtkWidget *
gimp_int_combo_box_new_valist (const gchar *first_label,
                               gint         first_value,
                               va_list      values)
{
  GtkWidget    *combo_box;
  GtkListStore *store;
  const gchar  *label;
  gint          value;

  g_return_val_if_fail (first_label != NULL, NULL);

  store = gimp_int_store_new ();

  combo_box = g_object_new (GIMP_TYPE_INT_COMBO_BOX,
                            "model", store,
                            NULL);
  g_object_unref (store);

  for (label = first_label, value = first_value;
       label;
       label = va_arg (values, const gchar *), value = va_arg (values, gint))
    {
      GtkTreeIter  iter;

      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter,
                          GIMP_INT_STORE_VALUE, value,
                          GIMP_INT_STORE_LABEL, label,
                          -1);
    }

  return combo_box;
}

/**
 * gimp_int_combo_box_new_array:
 * @n_values: the number of values
 * @labels:   an array of labels (array length must be @n_values)
 *
 * A variant of gimp_int_combo_box_new() that takes an array of labels.
 * The array indices are used as values.
 *
 * Return value: a new #GimpIntComboBox.
 *
 * Since: GIMP 2.2
 **/
GtkWidget *
gimp_int_combo_box_new_array (gint         n_values,
                              const gchar *labels[])
{
  GtkWidget    *combo_box;
  GtkListStore *store;
  gint          i;

  g_return_val_if_fail (n_values > 0, NULL);
  g_return_val_if_fail (labels != NULL, NULL);

  store = gimp_int_store_new ();

  combo_box = g_object_new (GIMP_TYPE_INT_COMBO_BOX,
                            "model", store,
                            NULL);
  g_object_unref (store);

  for (i = 0; i < n_values; i++)
    {
      GtkTreeIter  iter;

      if (labels[i])
        {
          gtk_list_store_append (store, &iter);
          gtk_list_store_set (store, &iter,
                              GIMP_INT_STORE_VALUE, i,
                              GIMP_INT_STORE_LABEL, gettext (labels[i]),
                              -1);
        }
    }

  return combo_box;
}

/**
 * gimp_int_combo_box_set_active:
 * @combo_box: a #GimpIntComboBox
 * @value:     an integer value
 *
 * Looks up the item that belongs to the given @value and makes it the
 * selected item in the @combo_box.
 *
 * Return value: %TRUE on success or %FALSE if there was no item for
 *               this value.
 *
 * Since: GIMP 2.2
 **/
gboolean
gimp_int_combo_box_set_active (GimpIntComboBox *combo_box,
                               gint             value)
{
  GtkTreeModel *model;
  GtkTreeIter   iter;

  g_return_val_if_fail (GIMP_IS_INT_COMBO_BOX (combo_box), FALSE);

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo_box));

  if (gimp_int_store_lookup_by_value (model, value, &iter))
    {
      gtk_combo_box_set_active_iter (GTK_COMBO_BOX (combo_box), &iter);
      return TRUE;
    }

  return FALSE;
}

/**
 * gimp_int_combo_box_get_active:
 * @combo_box: a #GimpIntComboBox
 * @value:     return location for the integer value
 *
 * Retrieves the value of the selected (active) item in the @combo_box.
 *
 * Return value: %TRUE if @value has been set or %FALSE if no item was
 *               active.
 *
 * Since: GIMP 2.2
 **/
gboolean
gimp_int_combo_box_get_active (GimpIntComboBox *combo_box,
                               gint            *value)
{
  GtkTreeIter  iter;

  g_return_val_if_fail (GIMP_IS_INT_COMBO_BOX (combo_box), FALSE);
  g_return_val_if_fail (value != NULL, FALSE);

  if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (combo_box), &iter))
    {
      gtk_tree_model_get (gtk_combo_box_get_model (GTK_COMBO_BOX (combo_box)),
                          &iter,
                          GIMP_INT_STORE_VALUE, value,
                          -1);
      return TRUE;
    }

  return FALSE;
}
