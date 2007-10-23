/* GIMP - The GNU Image Manipulation Program
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

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"

#include "display-types.h"

#include "gimpscalecombobox.h"


#define MAX_ITEMS  10

enum
{
  SCALE,
  LABEL,
  LABEL_ALIGN,
  PERSISTENT,
  SEPARATOR,
  ACTION,
  NUM_COLUMNS
};


static void      gimp_scale_combo_box_finalize   (GObject           *object);
static void      gimp_scale_combo_box_changed    (GimpScaleComboBox *combo_box);

static void      gimp_scale_combo_box_scale_iter_set (GtkListStore  *store,
                                                      GtkTreeIter   *iter,
                                                      gdouble        scale,
                                                      gboolean       persistent);
static gboolean  gimp_scale_combo_box_row_separator  (GtkTreeModel  *model,
                                                      GtkTreeIter   *iter,
                                                      gpointer       data);


G_DEFINE_TYPE (GimpScaleComboBox, gimp_scale_combo_box, GTK_TYPE_COMBO_BOX)

#define parent_class gimp_scale_combo_box_parent_class


static void
gimp_scale_combo_box_class_init (GimpScaleComboBoxClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = gimp_scale_combo_box_finalize;
}

static void
gimp_scale_combo_box_init (GimpScaleComboBox *combo_box)
{
  GtkListStore    *store;
  GtkCellRenderer *cell;
  GtkTreeIter      iter;
  gint             i;

  combo_box->actions_added = FALSE;
  combo_box->last_path     = NULL;

  store = gtk_list_store_new (NUM_COLUMNS,
                              G_TYPE_DOUBLE,    /* SCALE       */
                              G_TYPE_STRING,    /* LABEL       */
                              G_TYPE_DOUBLE,    /* LABEL_ALIGN */
                              G_TYPE_BOOLEAN,   /* PERSISTENT  */
                              G_TYPE_BOOLEAN,   /* SEPARATOR   */
                              GTK_TYPE_ACTION); /* ACTION      */

  gtk_combo_box_set_model (GTK_COMBO_BOX (combo_box), GTK_TREE_MODEL (store));
  g_object_unref (store);

  gtk_combo_box_set_row_separator_func (GTK_COMBO_BOX (combo_box),
                                        gimp_scale_combo_box_row_separator,
                                        NULL, NULL);

  cell = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo_box), cell, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo_box), cell,
                                  "text",   LABEL,
                                  "xalign", LABEL_ALIGN,
                                  NULL);

  for (i = 8; i > 0; i /= 2)
    {
      gtk_list_store_append (store, &iter);
      gimp_scale_combo_box_scale_iter_set (store, &iter, i, TRUE);
    }

  for (i = 2; i <= 8; i *= 2)
    {
      gtk_list_store_append (store, &iter);
      gimp_scale_combo_box_scale_iter_set (store, &iter, 1.0 / i, TRUE);
    }

  g_signal_connect (combo_box, "changed",
                    G_CALLBACK (gimp_scale_combo_box_changed),
                    NULL);
}

static void
gimp_scale_combo_box_finalize (GObject *object)
{
  GimpScaleComboBox *combo_box = GIMP_SCALE_COMBO_BOX (object);

  if (combo_box->last_path)
    {
      gtk_tree_path_free (combo_box->last_path);
      combo_box->last_path = NULL;
    }

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
gimp_scale_combo_box_changed (GimpScaleComboBox *combo_box)
{
  GtkTreeIter iter;

  if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (combo_box), &iter))
    {
      GtkTreeModel *model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo_box));
      GtkAction    *action;
      gdouble       scale;

      gtk_tree_model_get (model, &iter,
                          SCALE,  &scale,
                          ACTION, &action,
                          -1);
      if (action)
        {
          g_signal_stop_emission_by_name (combo_box, "changed");

          gtk_action_activate (action);
          g_object_unref (action);

          if (combo_box->last_path &&
              gtk_tree_model_get_iter (model, &iter, combo_box->last_path))
            {
              gtk_combo_box_set_active_iter (GTK_COMBO_BOX (combo_box), &iter);
            }
        }
      else if (scale > 0.0)
        {
          if (combo_box->last_path)
            gtk_tree_path_free (combo_box->last_path);

          combo_box->last_path = gtk_tree_model_get_path (model, &iter);
        }
    }
}

static gboolean
gimp_scale_combo_box_row_separator (GtkTreeModel *model,
                                    GtkTreeIter  *iter,
                                    gpointer      data)
{
  gboolean separator;

  gtk_tree_model_get (model, iter,
                      SEPARATOR, &separator,
                      -1);

  return separator;
}

static void
gimp_scale_combo_box_scale_iter_set (GtkListStore *store,
                                     GtkTreeIter  *iter,
                                     gdouble       scale,
                                     gboolean      persistent)
{
  gchar label[32];

  g_snprintf (label, sizeof (label), "%d%%", (int) ROUND (100.0 * scale));

  gtk_list_store_set (store, iter,
                      SCALE,       scale,
                      LABEL,       label,
                      LABEL_ALIGN, 1.0,
                      PERSISTENT,  persistent,
                      -1);
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
gimp_scale_combo_box_add_action (GimpScaleComboBox *combo_box,
                                 GtkAction         *action,
                                 const gchar       *label)
{
  GtkTreeModel *model;
  GtkListStore *store;
  GtkTreeIter   iter;

  g_return_if_fail (GIMP_IS_SCALE_COMBO_BOX (combo_box));
  g_return_if_fail (GTK_IS_ACTION (action));

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo_box));
  store = GTK_LIST_STORE (model);

  if (! combo_box->actions_added)
    {
      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter,
                          PERSISTENT, TRUE,
                          SEPARATOR,  TRUE,
                          -1);

      combo_box->actions_added = TRUE;
    }

  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, &iter,
                      LABEL,       label,
                      LABEL_ALIGN, 0.0,
                      PERSISTENT,  TRUE,
                      ACTION,      action,
                      -1);
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

      gtk_list_store_insert_before (store, &iter, iter_valid ? &sibling : NULL);
      gimp_scale_combo_box_scale_iter_set (store, &iter, scale, FALSE);
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
  GtkTreeIter iter;
  gdouble     scale = 1.0;

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
