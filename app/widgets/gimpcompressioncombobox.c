/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcompressioncombobox.c
 * Copyright (C) 2004, 2008  Sven Neumann <sven@gimp.org>
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

#include "config.h"

#include "stdlib.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "gimpcompressioncombobox.h"

#include "gimp-intl.h"


enum
{
  COLUMN_ID,
  COLUMN_LABEL,
  N_COLUMNS
};


/*  local function prototypes  */

static void       gimp_compression_combo_box_constructed    (GObject      *object);

static gboolean   gimp_compression_combo_box_separator_func (GtkTreeModel *model,
                                                             GtkTreeIter  *iter,
                                                             gpointer      data);


G_DEFINE_TYPE (GimpCompressionComboBox, gimp_compression_combo_box,
               GIMP_TYPE_STRING_COMBO_BOX)

#define parent_class gimp_compression_combo_box_parent_class


/*  private functions  */

static void
gimp_compression_combo_box_class_init (GimpCompressionComboBoxClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = gimp_compression_combo_box_constructed;
}

static void
gimp_compression_combo_box_init (GimpCompressionComboBox *combo_box)
{
}

static void
gimp_compression_combo_box_constructed (GObject *object)
{
  GimpCompressionComboBox *combo_box = GIMP_COMPRESSION_COMBO_BOX (object);
  GtkCellLayout           *layout;
  GtkCellRenderer         *cell;
  GtkListStore            *store;
  GtkTreeIter              iter;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  store = gtk_list_store_new (N_COLUMNS,
                              G_TYPE_STRING,   /* ID    */
                              G_TYPE_STRING);  /* LABEL */

  gtk_combo_box_set_model (GTK_COMBO_BOX (combo_box), GTK_TREE_MODEL (store));
  g_object_unref (store);

  gtk_combo_box_set_row_separator_func (
    GTK_COMBO_BOX (combo_box),
    gimp_compression_combo_box_separator_func,
    NULL,
    NULL);

  gtk_list_store_append (store, &iter);
  gtk_list_store_set    (store, &iter,
                         COLUMN_ID,    "none",
                         COLUMN_LABEL, C_("compression", "None"),
                         -1);

  gtk_list_store_append (store, &iter);
  gtk_list_store_set    (store, &iter,
                         COLUMN_ID,    NULL,
                         COLUMN_LABEL, NULL,
                         -1);

  gtk_list_store_append (store, &iter);
  gtk_list_store_set    (store, &iter,
                         COLUMN_ID,    "fast",
                         COLUMN_LABEL, C_("compression", "Best performance"),
                         -1);

  gtk_list_store_append (store, &iter);
  gtk_list_store_set    (store, &iter,
                         COLUMN_ID,    "balanced",
                         COLUMN_LABEL, C_("compression", "Balanced"),
                         -1);

  gtk_list_store_append (store, &iter);
  gtk_list_store_set    (store, &iter,
                         COLUMN_ID,    "best",
                         COLUMN_LABEL, C_("compression", "Best compression"),
                         -1);

  gtk_combo_box_set_entry_text_column (GTK_COMBO_BOX (combo_box),
                                       COLUMN_LABEL);

  layout = GTK_CELL_LAYOUT (combo_box);

  cell = gtk_cell_renderer_text_new ();

  gtk_cell_layout_clear (layout);
  gtk_cell_layout_pack_start (layout, cell, TRUE);
  gtk_cell_layout_set_attributes (layout, cell,
                                  "text", COLUMN_LABEL,
                                  NULL);
}

static gboolean
gimp_compression_combo_box_separator_func (GtkTreeModel *model,
                                           GtkTreeIter  *iter,
                                           gpointer      data)
{
  gchar    *value;
  gboolean  result;

  gtk_tree_model_get (model, iter, COLUMN_ID, &value, -1);

  result = ! value;

  g_free (value);

  return result;
}


/*  public functions  */

GtkWidget *
gimp_compression_combo_box_new (void)
{
  return g_object_new (GIMP_TYPE_COMPRESSION_COMBO_BOX,
                       "has-entry",    TRUE,
                       "id-column",    COLUMN_ID,
                       "label-column", COLUMN_LABEL,
                       NULL);
}

void
gimp_compression_combo_box_set_compression (GimpCompressionComboBox *combo_box,
                                            const gchar             *compression)
{
  g_return_if_fail (GIMP_IS_COMPRESSION_COMBO_BOX (combo_box));
  g_return_if_fail (compression != NULL);

  if (! gimp_string_combo_box_set_active (GIMP_STRING_COMBO_BOX (combo_box),
                                          compression))
    {
      GtkWidget *entry;

      entry = gtk_bin_get_child (GTK_BIN (combo_box));

      gtk_combo_box_set_active (GTK_COMBO_BOX (combo_box), -1);

      gtk_entry_set_text (GTK_ENTRY (entry), compression);
    }
}

gchar *
gimp_compression_combo_box_get_compression (GimpCompressionComboBox *combo_box)
{
  gchar *result;

  g_return_val_if_fail (GIMP_IS_COMPRESSION_COMBO_BOX (combo_box), NULL);

  result = gimp_string_combo_box_get_active (GIMP_STRING_COMBO_BOX (combo_box));

  if (! result)
    {
      GtkWidget *entry;

      entry = gtk_bin_get_child (GTK_BIN (combo_box));

      result = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));
    }

  return result;
}
