/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpscalecombobox.c
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

#include "display-types.h"

#include "gimpscalecombobox.h"


#define MAX_ITEMS 10

enum
{
  SCALE,
  LABEL,
  PERSISTENT,
  NUM_COLUMNS
};


static void  gimp_scale_combo_box_class_init (GimpScaleComboBoxClass *klass);
static void  gimp_scale_combo_box_init       (GimpScaleComboBox *combo_box);
static void  gimp_scale_combo_box_finalize   (GObject           *object);


static GtkComboBoxClass *parent_class = NULL;


GType
gimp_scale_combo_box_get_type (void)
{
  static GType combo_box_type = 0;

  if (!combo_box_type)
    {
      static const GTypeInfo combo_box_info =
      {
        sizeof (GimpScaleComboBoxClass),
        NULL,           /* base_init      */
        NULL,           /* base_finalize  */
        (GClassInitFunc) gimp_scale_combo_box_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (GimpScaleComboBox),
        0,              /* n_preallocs    */
        (GInstanceInitFunc) gimp_scale_combo_box_init
      };

      combo_box_type = g_type_register_static (GTK_TYPE_COMBO_BOX,
                                               "GimpScaleComboBox",
                                               &combo_box_info, 0);
    }

  return combo_box_type;
}

static void
gimp_scale_combo_box_class_init (GimpScaleComboBoxClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = gimp_scale_combo_box_finalize;
}

static void
gimp_scale_combo_box_init (GimpScaleComboBox *combo_box)
{
  GtkListStore    *store;
  GtkCellRenderer *cell;
  GtkTreeIter      iter;
  gint             i;

  store = gtk_list_store_new (NUM_COLUMNS,
                              G_TYPE_DOUBLE,
                              G_TYPE_STRING,
                              G_TYPE_BOOLEAN);

  gtk_combo_box_set_model (GTK_COMBO_BOX (combo_box), GTK_TREE_MODEL (store));

  g_object_unref (store);

  cell = g_object_new (GTK_TYPE_CELL_RENDERER_TEXT,
                       "xalign", 1.0,
                       NULL);
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo_box), cell, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo_box), cell,
                                  "text", LABEL,
                                  NULL);

  for (i = 8; i > 0; i /= 2)
    {
      gchar  label[32];

      g_snprintf (label, sizeof (label), "%d%%", i * 100);

      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter,
                          SCALE,      (gdouble) i,
                          LABEL,      label,
                          PERSISTENT, TRUE,
                          -1);
    }

  for (i = 2; i <= 8; i *= 2)
    {
      gchar  label[32];

      g_snprintf (label, sizeof (label), "%d%%", 100 / i);

      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter,
                          SCALE,      1.0 / (gdouble) i,
                          LABEL,      label,
                          PERSISTENT, TRUE,
                          -1);
    }
}

static void
gimp_scale_combo_box_finalize (GObject *object)
{
  GimpScaleComboBox *combo_box = GIMP_SCALE_COMBO_BOX (object);

  if (combo_box->mru)
    {
      g_list_foreach (combo_box->mru,
                      (GFunc) gtk_tree_row_reference_free, NULL);
      g_list_free (combo_box->mru);
      combo_box->mru = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_scale_combo_box_mru_add (GimpScaleComboBox *combo_box,
                              GtkTreeIter       *iter)
{
  GtkTreeModel *model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo_box));
  GtkTreePath  *path  = gtk_tree_model_get_path (model, iter);
  GList        *list;
  gboolean      found;

  for (list = combo_box->mru, found = FALSE; list && !found; list = list->next)
    {
      GtkTreePath *this = gtk_tree_row_reference_get_path (list->data);

      if (gtk_tree_path_compare (this, path) == 0)
        {
          if (list->prev)
            {
              combo_box->mru = g_list_remove_link (combo_box->mru, list);
              combo_box->mru = g_list_concat (list, combo_box->mru);
            }

          found = TRUE;
        }

      gtk_tree_path_free (this);
    }

  if (! found)
    combo_box->mru = g_list_prepend (combo_box->mru,
                                     gtk_tree_row_reference_new (model, path));

  gtk_tree_path_free (path);
}

static void
gimp_scale_combo_box_mru_remove_last (GimpScaleComboBox *combo_box)
{
  GtkTreeModel *model;
  GtkTreePath  *path;
  GList        *last;
  GtkTreeIter   iter;

  if (! combo_box->mru)
    return;

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo_box));

  last = g_list_last (combo_box->mru);
  path = gtk_tree_row_reference_get_path (last->data);

  if (gtk_tree_model_get_iter (model, &iter, path))
    {
      gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
      gtk_tree_row_reference_free (last->data);
      combo_box->mru = g_list_delete_link (combo_box->mru, last);
    }

  gtk_tree_path_free (path);
}


/**
 * gimp_scale_combo_box_new:
 *
 * Return value: a new #GimpScaleComboBox.
 **/
GtkWidget *
gimp_scale_combo_box_new (void)
{
  return g_object_new (GIMP_TYPE_SCALE_COMBO_BOX, NULL);
}

void
gimp_scale_combo_box_set_scale (GimpScaleComboBox *combo_box,
                                gdouble            scale)
{
  GtkTreeModel *model;
  GtkListStore *store;
  GtkTreeIter   iter;
  gboolean      iter_valid;
  gboolean      persistent;

  g_return_if_fail (GIMP_IS_SCALE_COMBO_BOX (combo_box));

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo_box));
  store = GTK_LIST_STORE (model);

  for (iter_valid = gtk_tree_model_get_iter_first (model, &iter);
       iter_valid;
       iter_valid = gtk_tree_model_iter_next (model, &iter))
    {
      gdouble  this;

      gtk_tree_model_get (model, &iter,
                          SCALE, &this,
                          -1);

      if (fabs (this - scale) < 0.01)
        break;
    }

  if (! iter_valid)
    {
      GtkTreeIter  sibling;
      gchar        label[32];

      for (iter_valid = gtk_tree_model_get_iter_first (model, &sibling);
           iter_valid;
           iter_valid = gtk_tree_model_iter_next (model, &sibling))
        {
          gdouble  this;

          gtk_tree_model_get (model, &sibling,
                              SCALE, &this,
                              -1);

          if (this < scale)
            break;
        }

      g_snprintf (label, sizeof (label), "%d%%", (gint) (100.0 * scale));

      gtk_list_store_insert_before (store, &iter,
                                    iter_valid ? &sibling : NULL);
      gtk_list_store_set (store, &iter,
                          SCALE,      scale,
                          LABEL,      label,
                          PERSISTENT, FALSE,
                          -1);
    }

  gtk_combo_box_set_active_iter (GTK_COMBO_BOX (combo_box), &iter);

  gtk_tree_model_get (model, &iter,
                      PERSISTENT, &persistent,
                      -1);
  if (! persistent)
    {
      gimp_scale_combo_box_mru_add (combo_box, &iter);

      if (gtk_tree_model_iter_n_children (model, NULL) > MAX_ITEMS)
        gimp_scale_combo_box_mru_remove_last (combo_box);
    }
}

gdouble
gimp_scale_combo_box_get_scale (GimpScaleComboBox *combo_box)
{
  GtkTreeIter  iter;
  gdouble      scale = 1.0;

  g_return_val_if_fail (GIMP_IS_SCALE_COMBO_BOX (combo_box), FALSE);

  if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (combo_box), &iter))
    {
      gtk_tree_model_get (gtk_combo_box_get_model (GTK_COMBO_BOX (combo_box)),
                          &iter,
                          SCALE, &scale,
                          -1);
    }

  return scale;
}
