/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpstringcombobox.c
 * Copyright (C) 2007  Sven Neumann <sven@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gtk/gtk.h>

#include "gimpwidgetstypes.h"

#include "gimpstringcombobox.h"


/**
 * SECTION: gimpstringcombobox
 * @title: GimpStringComboBox
 * @short_description: A #GtkComboBox subclass to select strings.
 *
 * A #GtkComboBox subclass to select strings.
 **/


enum
{
  PROP_0,
  PROP_ID_COLUMN,
  PROP_LABEL_COLUMN,
  PROP_ELLIPSIZE
};


struct _GimpStringComboBoxPrivate
{
  gint             id_column;
  gint             label_column;
  GtkCellRenderer *text_renderer;
};

#define GET_PRIVATE(obj) (((GimpStringComboBox *) (obj))->priv)


static void   gimp_string_combo_box_constructed  (GObject      *object);
static void   gimp_string_combo_box_set_property (GObject      *object,
                                                  guint         property_id,
                                                  const GValue *value,
                                                  GParamSpec   *pspec);
static void   gimp_string_combo_box_get_property (GObject      *object,
                                                  guint         property_id,
                                                  GValue       *value,
                                                  GParamSpec   *pspec);


G_DEFINE_TYPE (GimpStringComboBox, gimp_string_combo_box, GTK_TYPE_COMBO_BOX)

#define parent_class gimp_string_combo_box_parent_class


static void
gimp_string_combo_box_class_init (GimpStringComboBoxClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = gimp_string_combo_box_constructed;
  object_class->set_property = gimp_string_combo_box_set_property;
  object_class->get_property = gimp_string_combo_box_get_property;

  /**
   * GimpStringComboBox:id-column:
   *
   * The column in the associated GtkTreeModel that holds unique
   * string IDs.
   *
   * Since: 2.4
   */
  g_object_class_install_property (object_class,
                                   PROP_ID_COLUMN,
                                   g_param_spec_int ("id-column",
                                                     "ID Column",
                                                     "The model column that holds the ID",
                                                     0, G_MAXINT,
                                                     0,
                                                     GIMP_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT_ONLY));
  /**
   * GimpStringComboBox:id-column:
   *
   * The column in the associated GtkTreeModel that holds strings to
   * be used as labels in the combo-box.
   *
   * Since: 2.4
   */
  g_object_class_install_property (object_class,
                                   PROP_LABEL_COLUMN,
                                   g_param_spec_int ("label-column",
                                                     "Label Column",
                                                     "The model column that holds the label",
                                                     0, G_MAXINT,
                                                     0,
                                                     GIMP_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT_ONLY));

  /**
   * GimpStringComboBox:ellipsize:
   *
   * Specifies the preferred place to ellipsize text in the combo-box,
   * if the cell renderer does not have enough room to display the
   * entire string.
   *
   * Since: 2.4
   */
  g_object_class_install_property (object_class,
                                   PROP_ELLIPSIZE,
                                   g_param_spec_enum ("ellipsize",
                                                      "Ellipsize",
                                                      "Ellipsize mode for the text cell renderer",
                                                      PANGO_TYPE_ELLIPSIZE_MODE,
                                                      PANGO_ELLIPSIZE_NONE,
                                                      GIMP_PARAM_READWRITE));

  g_type_class_add_private (object_class, sizeof (GimpStringComboBoxPrivate));
}

static void
gimp_string_combo_box_init (GimpStringComboBox *combo_box)
{
  combo_box->priv = G_TYPE_INSTANCE_GET_PRIVATE (combo_box,
                                                 GIMP_TYPE_STRING_COMBO_BOX,
                                                 GimpStringComboBoxPrivate);
}

static void
gimp_string_combo_box_constructed (GObject *object)
{
  GimpStringComboBoxPrivate *priv = GET_PRIVATE (object);
  GtkCellRenderer           *cell;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  priv->text_renderer = cell = gtk_cell_renderer_text_new ();

  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (object), cell, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (object), cell,
                                  "text", priv->label_column,
                                  NULL);
}

static void
gimp_string_combo_box_set_property (GObject      *object,
                                    guint         property_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  GimpStringComboBoxPrivate *priv = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_ID_COLUMN:
      priv->id_column = g_value_get_int (value);
      break;

    case PROP_LABEL_COLUMN:
      priv->label_column = g_value_get_int (value);
      break;

    case PROP_ELLIPSIZE:
      g_object_set_property (G_OBJECT (priv->text_renderer),
                             pspec->name, value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_string_combo_box_get_property (GObject    *object,
                                    guint       property_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  GimpStringComboBoxPrivate *priv = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_ID_COLUMN:
      g_value_set_int (value, priv->id_column);
      break;

    case PROP_LABEL_COLUMN:
      g_value_set_int (value, priv->label_column);
      break;

    case PROP_ELLIPSIZE:
      g_object_get_property (G_OBJECT (priv->text_renderer),
                             pspec->name, value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gboolean
gimp_string_model_lookup (GtkTreeModel *model,
                          gint          column,
                          const gchar  *id,
                          GtkTreeIter  *iter)
{
  GValue    value = G_VALUE_INIT;
  gboolean  iter_valid;

  /*  This lookup could be backed up by a hash table or some other
   *  data structure instead of doing a list traversal. But since this
   *  is a GtkComboBox, there shouldn't be many entries anyway...
   */

  for (iter_valid = gtk_tree_model_get_iter_first (model, iter);
       iter_valid;
       iter_valid = gtk_tree_model_iter_next (model, iter))
    {
      const gchar *str;

      gtk_tree_model_get_value (model, iter, column, &value);

      str = g_value_get_string (&value);

      if (str && strcmp (str, id) == 0)
        {
          g_value_unset (&value);
          break;
        }

      g_value_unset (&value);
    }

  return iter_valid;
}


/**
 * gimp_string_combo_box_new:
 * @model:        a #GtkTreeModel
 * @id_column:    the model column of the ID
 * @label_column: the modl column of the label
 *
 * Return value: a new #GimpStringComboBox.
 *
 * Since: 2.4
 **/
GtkWidget *
gimp_string_combo_box_new (GtkTreeModel *model,
                           gint          id_column,
                           gint          label_column)
{
  g_return_val_if_fail (GTK_IS_TREE_MODEL (model), NULL);
  g_return_val_if_fail (gtk_tree_model_get_column_type (model,
                                                        id_column) == G_TYPE_STRING, NULL);
  g_return_val_if_fail (gtk_tree_model_get_column_type (model,
                                                        label_column) == G_TYPE_STRING, NULL);

  return g_object_new (GIMP_TYPE_STRING_COMBO_BOX,
                       "model",        model,
                       "id-column",    id_column,
                       "label-column", label_column,
                       NULL);
}

/**
 * gimp_string_combo_box_set_active:
 * @combo_box: a #GimpStringComboBox
 * @id:        the ID of the item to select
 *
 * Looks up the item that belongs to the given @id and makes it the
 * selected item in the @combo_box.
 *
 * Return value: %TRUE on success or %FALSE if there was no item for
 *               this value.
 *
 * Since: 2.4
 **/
gboolean
gimp_string_combo_box_set_active (GimpStringComboBox *combo_box,
                                  const gchar        *id)
{
  g_return_val_if_fail (GIMP_IS_STRING_COMBO_BOX (combo_box), FALSE);

  if (id)
    {
      GtkTreeModel *model;
      GtkTreeIter   iter;
      gint          column;

      model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo_box));

      column = GET_PRIVATE (combo_box)->id_column;

      if (gimp_string_model_lookup (model, column, id, &iter))
        {
          gtk_combo_box_set_active_iter (GTK_COMBO_BOX (combo_box), &iter);
          return TRUE;
        }

      return FALSE;
    }
  else
    {
      gtk_combo_box_set_active (GTK_COMBO_BOX (combo_box), -1);

      return TRUE;
    }
}

/**
 * gimp_string_combo_box_get_active:
 * @combo_box: a #GimpStringComboBox
 *
 * Retrieves the value of the selected (active) item in the @combo_box.
 *
 * Return value: newly allocated ID string or %NULL if nothing was selected
 *
 * Since: 2.4
 **/
gchar *
gimp_string_combo_box_get_active (GimpStringComboBox *combo_box)
{
  GtkTreeIter  iter;

  g_return_val_if_fail (GIMP_IS_STRING_COMBO_BOX (combo_box), NULL);

  if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (combo_box), &iter))
    {
      GtkTreeModel *model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo_box));
      gchar        *value;
      gint          column;

      column = GET_PRIVATE (combo_box)->id_column;

      gtk_tree_model_get (model, &iter,
                          column, &value,
                          -1);

      return value;
    }

  return NULL;
}
