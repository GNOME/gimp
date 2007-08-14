/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpcolorprofilecombobox.c
 * Copyright (C) 2007  Sven Neumann <sven@gimp.org>
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

#include "gimpcolorprofilecombobox.h"
#include "gimpcolorprofilestore.h"
#include "gimpcolorprofilestore-private.h"


enum
{
  PROP_0,
  PROP_DIALOG,
  PROP_MODEL
};


typedef struct
{
  GtkTreePath *last_path;
} GimpColorProfileComboBoxPrivate;

#define GIMP_COLOR_PROFILE_COMBO_BOX_GET_PRIVATE(obj) \
  G_TYPE_INSTANCE_GET_PRIVATE (obj, \
                               GIMP_TYPE_COLOR_PROFILE_COMBO_BOX, \
                               GimpColorProfileComboBoxPrivate)


static void  gimp_color_profile_combo_box_finalize     (GObject      *object);
static void  gimp_color_profile_combo_box_set_property (GObject      *object,
                                                        guint         property_id,
                                                        const GValue *value,
                                                        GParamSpec   *pspec);
static void  gimp_color_profile_combo_box_get_property (GObject      *object,
                                                        guint         property_id,
                                                        GValue       *value,
                                                        GParamSpec   *pspec);
static void  gimp_color_profile_combo_box_changed      (GtkComboBox  *combo);

static gboolean  gimp_color_profile_row_separator_func (GtkTreeModel *model,
                                                        GtkTreeIter  *iter,
                                                        gpointer      data);


G_DEFINE_TYPE (GimpColorProfileComboBox,
               gimp_color_profile_combo_box, GTK_TYPE_COMBO_BOX)

#define parent_class gimp_color_profile_combo_box_parent_class


static void
gimp_color_profile_combo_box_class_init (GimpColorProfileComboBoxClass *klass)
{
  GObjectClass     *object_class = G_OBJECT_CLASS (klass);
  GtkComboBoxClass *combo_class  = GTK_COMBO_BOX_CLASS (klass);

  object_class->set_property = gimp_color_profile_combo_box_set_property;
  object_class->get_property = gimp_color_profile_combo_box_get_property;
  object_class->finalize     = gimp_color_profile_combo_box_finalize;

  combo_class->changed       = gimp_color_profile_combo_box_changed;

  g_object_class_install_property (object_class,
                                   PROP_DIALOG,
                                   g_param_spec_object ("dialog", NULL, NULL,
                                                        GTK_TYPE_DIALOG,
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        GIMP_PARAM_READWRITE));
  g_object_class_install_property (object_class,
                                   PROP_MODEL,
                                   g_param_spec_object ("model", NULL, NULL,
                                                        GIMP_TYPE_COLOR_PROFILE_STORE,
                                                        GIMP_PARAM_READWRITE));

  g_type_class_add_private (object_class,
                            sizeof (GimpColorProfileComboBoxPrivate));
}

static void
gimp_color_profile_combo_box_init (GimpColorProfileComboBox *combo_box)
{
  GtkCellRenderer *cell = gtk_cell_renderer_text_new ();

  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo_box), cell, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo_box), cell,
                                  "text", GIMP_COLOR_PROFILE_STORE_LABEL,
                                  NULL);

  gtk_combo_box_set_row_separator_func (GTK_COMBO_BOX (combo_box),
                                        gimp_color_profile_row_separator_func,
                                        NULL, NULL);
}

static void
gimp_color_profile_combo_box_finalize (GObject *object)
{
  GimpColorProfileComboBox        *combo;
  GimpColorProfileComboBoxPrivate *priv;

  combo = GIMP_COLOR_PROFILE_COMBO_BOX (object);

  if (combo->dialog)
    {
      g_object_unref (combo->dialog);
      combo->dialog = NULL;
    }

  priv = GIMP_COLOR_PROFILE_COMBO_BOX_GET_PRIVATE (combo);

  if (priv->last_path)
    {
      gtk_tree_path_free (priv->last_path);
      priv->last_path = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_color_profile_combo_box_set_property (GObject      *object,
                                           guint         property_id,
                                           const GValue *value,
                                           GParamSpec   *pspec)
{
  GimpColorProfileComboBox *combo_box = GIMP_COLOR_PROFILE_COMBO_BOX (object);

  switch (property_id)
    {
    case PROP_DIALOG:
      g_return_if_fail (combo_box->dialog == NULL);
      combo_box->dialog = (GtkWidget *) g_value_dup_object (value);
      break;

    case PROP_MODEL:
      gtk_combo_box_set_model (GTK_COMBO_BOX (combo_box),
                               g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_color_profile_combo_box_get_property (GObject    *object,
                                           guint       property_id,
                                           GValue     *value,
                                           GParamSpec *pspec)
{
  GimpColorProfileComboBox *combo_box = GIMP_COLOR_PROFILE_COMBO_BOX (object);

  switch (property_id)
    {
    case PROP_DIALOG:
      g_value_set_object (value, combo_box->dialog);
      break;

    case PROP_MODEL:
      g_value_set_object (value,
                          gtk_combo_box_get_model (GTK_COMBO_BOX (combo_box)));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_color_profile_combo_box_changed (GtkComboBox *combo)
{
  GimpColorProfileComboBoxPrivate *priv;

  GtkTreeModel *model = gtk_combo_box_get_model (combo);
  GtkTreeIter   iter;
  gint          type;

  if (! gtk_combo_box_get_active_iter (combo, &iter))
    return;

  gtk_tree_model_get (model, &iter,
                      GIMP_COLOR_PROFILE_STORE_ITEM_TYPE, &type,
                      -1);

  priv = GIMP_COLOR_PROFILE_COMBO_BOX_GET_PRIVATE (combo);

  switch (type)
    {
    case GIMP_COLOR_PROFILE_STORE_ITEM_DIALOG:
      {
        GtkWidget *dialog = GIMP_COLOR_PROFILE_COMBO_BOX (combo)->dialog;
        GtkWidget *parent = gtk_widget_get_toplevel (GTK_WIDGET (combo));

        if (GTK_IS_WINDOW (parent))
          gtk_window_set_transient_for (GTK_WINDOW (dialog),
                                        GTK_WINDOW (parent));

        gtk_window_present (GTK_WINDOW (dialog));

        if (priv->last_path &&
            gtk_tree_model_get_iter (model, &iter, priv->last_path))
          {
            gtk_combo_box_set_active_iter (combo, &iter);
          }
      }
      break;

    case GIMP_COLOR_PROFILE_STORE_ITEM_FILE:
      if (priv->last_path)
        gtk_tree_path_free (priv->last_path);

      priv->last_path = gtk_tree_model_get_path (model, &iter);

      /*  FIXME: update order of history  */
      break;

    default:
      break;
    }
}


/**
 * gimp_color_profile_combo_box_new:
 * @dialog:
 * @history:
 *
 * Return value: a new #GimpColorProfileComboBox.
 *
 * Since: GIMP 2.4
 **/
GtkWidget *
gimp_color_profile_combo_box_new (GtkWidget   *dialog,
                                  const gchar *history)
{
  g_return_val_if_fail (GTK_IS_DIALOG (dialog), NULL);

  return g_object_new (GIMP_TYPE_COLOR_PROFILE_COMBO_BOX,
                       "dialog", dialog,
                       "model",  gimp_color_profile_store_new (history),
                       NULL);
}

/**
 * gimp_color_profile_combo_box_new_with_model:
 * @dialog:
 * @model:
 *
 * Return value: a new #GimpColorProfileComboBox.
 *
 * Since: GIMP 2.4
 **/
GtkWidget *
gimp_color_profile_combo_box_new_with_model (GtkWidget    *dialog,
                                             GtkTreeModel *model)
{
  g_return_val_if_fail (GTK_IS_DIALOG (dialog), NULL);
  g_return_val_if_fail (GIMP_IS_COLOR_PROFILE_STORE (model), NULL);

  return g_object_new (GIMP_TYPE_COLOR_PROFILE_COMBO_BOX,
                       "dialog", dialog,
                       "model",  model,
                       NULL);
}

/**
 * gimp_color_profile_combo_box_add:
 * @combo:
 * @uri:
 * @label:
 *
 * Since: GIMP 2.4
 **/
void
gimp_color_profile_combo_box_add (GimpColorProfileComboBox *combo,
                                  const gchar              *uri,
                                  const gchar              *label)
{
  GtkTreeModel *model;

  g_return_if_fail (GIMP_IS_COLOR_PROFILE_COMBO_BOX (combo));
  g_return_if_fail (label != NULL || uri == NULL);

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));

  gimp_color_profile_store_add (GIMP_COLOR_PROFILE_STORE (model), uri, label);
}

/**
 * gimp_color_profile_combo_box_set_active:
 * @combo:
 * @uri:
 * @label:
 *
 * Since: GIMP 2.4
 **/
void
gimp_color_profile_combo_box_set_active (GimpColorProfileComboBox *combo,
                                         const gchar              *uri,
                                         const gchar              *label)
{
  GtkTreeModel *model;
  GtkTreeIter   iter;

  g_return_if_fail (GIMP_IS_COLOR_PROFILE_COMBO_BOX (combo));

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));

  if (_gimp_color_profile_store_history_add (GIMP_COLOR_PROFILE_STORE (model),
                                             uri, label, &iter))
    gtk_combo_box_set_active_iter (GTK_COMBO_BOX (combo), &iter);
}

/**
 * gimp_color_profile_combo_box_get_active:
 * @combo:
 *
 * Since: GIMP 2.4
 **/
gchar *
gimp_color_profile_combo_box_get_active (GimpColorProfileComboBox *combo)
{
  GtkTreeModel *model;
  GtkTreeIter   iter;

  g_return_val_if_fail (GIMP_IS_COLOR_PROFILE_COMBO_BOX (combo), NULL);

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));

  if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (combo), &iter))
    {
      gchar *uri;
      gint   type;

      gtk_tree_model_get (model, &iter,
                          GIMP_COLOR_PROFILE_STORE_ITEM_TYPE, &type,
                          GIMP_COLOR_PROFILE_STORE_URI,       &uri,
                          -1);

      if (type == GIMP_COLOR_PROFILE_STORE_ITEM_FILE)
        return uri;

      g_free (uri);
    }

  return NULL;
}

static gboolean
gimp_color_profile_row_separator_func (GtkTreeModel *model,
                                       GtkTreeIter  *iter,
                                       gpointer      data)
{
  gint type;

  gtk_tree_model_get (model, iter,
                      GIMP_COLOR_PROFILE_STORE_ITEM_TYPE, &type,
                      -1);

  switch (type)
    {
    case GIMP_COLOR_PROFILE_STORE_ITEM_SEPARATOR_TOP:
    case GIMP_COLOR_PROFILE_STORE_ITEM_SEPARATOR_BOTTOM:
      return TRUE;

    default:
      return FALSE;
    }
}
