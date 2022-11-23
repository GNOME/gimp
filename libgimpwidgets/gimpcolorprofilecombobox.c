/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmacolorprofilecombobox.c
 * Copyright (C) 2007  Sven Neumann <sven@ligma.org>
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
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmabase/ligmabase.h"
#include "libligmacolor/ligmacolor.h"

#include "ligmawidgetstypes.h"

#include "ligmacolorprofilechooserdialog.h"
#include "ligmacolorprofilecombobox.h"
#include "ligmacolorprofilestore.h"
#include "ligmacolorprofilestore-private.h"


/**
 * SECTION: ligmacolorprofilecombobox
 * @title: LigmaColorProfileComboBox
 * @short_description: A combo box for selecting color profiles.
 *
 * A combo box for selecting color profiles.
 **/


enum
{
  PROP_0,
  PROP_DIALOG,
  PROP_MODEL
};


struct _LigmaColorProfileComboBoxPrivate
{
  GtkWidget   *dialog;
  GtkTreePath *last_path;
};

#define GET_PRIVATE(obj) (((LigmaColorProfileComboBox *) (obj))->priv)


static void  ligma_color_profile_combo_box_finalize     (GObject      *object);
static void  ligma_color_profile_combo_box_set_property (GObject      *object,
                                                        guint         property_id,
                                                        const GValue *value,
                                                        GParamSpec   *pspec);
static void  ligma_color_profile_combo_box_get_property (GObject      *object,
                                                        guint         property_id,
                                                        GValue       *value,
                                                        GParamSpec   *pspec);
static void  ligma_color_profile_combo_box_changed      (GtkComboBox  *combo);

static gboolean  ligma_color_profile_row_separator_func (GtkTreeModel *model,
                                                        GtkTreeIter  *iter,
                                                        gpointer      data);

static void  ligma_color_profile_combo_dialog_response  (LigmaColorProfileChooserDialog *dialog,
                                                        gint                           response,
                                                        LigmaColorProfileComboBox      *combo);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaColorProfileComboBox,
                            ligma_color_profile_combo_box, GTK_TYPE_COMBO_BOX)

#define parent_class ligma_color_profile_combo_box_parent_class


static void
ligma_color_profile_combo_box_class_init (LigmaColorProfileComboBoxClass *klass)
{
  GObjectClass     *object_class = G_OBJECT_CLASS (klass);
  GtkComboBoxClass *combo_class  = GTK_COMBO_BOX_CLASS (klass);

  object_class->set_property = ligma_color_profile_combo_box_set_property;
  object_class->get_property = ligma_color_profile_combo_box_get_property;
  object_class->finalize     = ligma_color_profile_combo_box_finalize;

  combo_class->changed       = ligma_color_profile_combo_box_changed;

  /**
   * LigmaColorProfileComboBox:dialog:
   *
   * #GtkDialog to present when the user selects the
   * "Select color profile from disk..." item.
   *
   * Since: 2.4
   */
  g_object_class_install_property (object_class,
                                   PROP_DIALOG,
                                   g_param_spec_object ("dialog",
                                                        "Dialog",
                                                        "The dialog to present when selecting profiles from disk",
                                                        GTK_TYPE_DIALOG,
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        LIGMA_PARAM_READWRITE));
  /**
   * LigmaColorProfileComboBox:model:
   *
   * Overrides the "model" property of the #GtkComboBox class.
   * #LigmaColorProfileComboBox requires the model to be a
   * #LigmaColorProfileStore.
   *
   * Since: 2.4
   */
  g_object_class_install_property (object_class,
                                   PROP_MODEL,
                                   g_param_spec_object ("model",
                                                        "Model",
                                                        "The profile store used for this combo box",
                                                        LIGMA_TYPE_COLOR_PROFILE_STORE,
                                                        LIGMA_PARAM_READWRITE));
}

static void
ligma_color_profile_combo_box_init (LigmaColorProfileComboBox *combo_box)
{
  GtkCellRenderer *cell;

  combo_box->priv = ligma_color_profile_combo_box_get_instance_private (combo_box);

  cell = gtk_cell_renderer_text_new ();

  g_object_set (cell,
                "width-chars", 42,
                "ellipsize",   PANGO_ELLIPSIZE_END,
                NULL);


  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo_box), cell, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo_box), cell,
                                  "text", LIGMA_COLOR_PROFILE_STORE_LABEL,
                                  NULL);

  gtk_combo_box_set_row_separator_func (GTK_COMBO_BOX (combo_box),
                                        ligma_color_profile_row_separator_func,
                                        NULL, NULL);
}

static void
ligma_color_profile_combo_box_finalize (GObject *object)
{
  LigmaColorProfileComboBoxPrivate *private = GET_PRIVATE (object);

  if (private->dialog)
    {
      if (LIGMA_IS_COLOR_PROFILE_CHOOSER_DIALOG (private->dialog))
        gtk_widget_destroy (private->dialog);

      g_object_unref (private->dialog);
      private->dialog = NULL;
    }

  g_clear_pointer (&private->last_path, gtk_tree_path_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_color_profile_combo_box_set_property (GObject      *object,
                                           guint         property_id,
                                           const GValue *value,
                                           GParamSpec   *pspec)
{
  LigmaColorProfileComboBoxPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_DIALOG:
      g_return_if_fail (private->dialog == NULL);
      private->dialog = g_value_dup_object (value);

      if (LIGMA_IS_COLOR_PROFILE_CHOOSER_DIALOG (private->dialog))
        g_signal_connect (private->dialog, "response",
                          G_CALLBACK (ligma_color_profile_combo_dialog_response),
                          object);
      break;

    case PROP_MODEL:
      gtk_combo_box_set_model (GTK_COMBO_BOX (object),
                               g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_color_profile_combo_box_get_property (GObject    *object,
                                           guint       property_id,
                                           GValue     *value,
                                           GParamSpec *pspec)
{
  LigmaColorProfileComboBoxPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_DIALOG:
      g_value_set_object (value, private->dialog);
      break;

    case PROP_MODEL:
      g_value_set_object (value,
                          gtk_combo_box_get_model (GTK_COMBO_BOX (object)));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_color_profile_combo_box_changed (GtkComboBox *combo)
{
  LigmaColorProfileComboBoxPrivate *priv  = GET_PRIVATE (combo);
  GtkTreeModel                    *model = gtk_combo_box_get_model (combo);
  GtkTreeIter                      iter;
  gint                             type;

  if (! gtk_combo_box_get_active_iter (combo, &iter))
    return;

  gtk_tree_model_get (model, &iter,
                      LIGMA_COLOR_PROFILE_STORE_ITEM_TYPE, &type,
                      -1);

  switch (type)
    {
    case LIGMA_COLOR_PROFILE_STORE_ITEM_DIALOG:
      {
        GtkWidget *parent = gtk_widget_get_toplevel (GTK_WIDGET (combo));

        if (GTK_IS_WINDOW (parent))
          gtk_window_set_transient_for (GTK_WINDOW (priv->dialog),
                                        GTK_WINDOW (parent));

        gtk_window_present (GTK_WINDOW (priv->dialog));

        if (priv->last_path &&
            gtk_tree_model_get_iter (model, &iter, priv->last_path))
          {
            gtk_combo_box_set_active_iter (combo, &iter);
          }
      }
      break;

    case LIGMA_COLOR_PROFILE_STORE_ITEM_FILE:
      if (priv->last_path)
        gtk_tree_path_free (priv->last_path);

      priv->last_path = gtk_tree_model_get_path (model, &iter);

      _ligma_color_profile_store_history_reorder (LIGMA_COLOR_PROFILE_STORE (model),
                                                 &iter);
      break;

    default:
      break;
    }
}


/**
 * ligma_color_profile_combo_box_new:
 * @dialog:  a #GtkDialog to present when the user selects the
 *           "Select color profile from disk..." item
 * @history: #GFile of the profilerc (or %NULL for no history)
 *
 * Create a combo-box widget for selecting color profiles. The combo-box
 * is populated from the file specified as @history. This filename is
 * typically created using the following code snippet:
 * <informalexample><programlisting>
 *  gchar *history = ligma_personal_rc_file ("profilerc");
 * </programlisting></informalexample>
 *
 * The recommended @dialog type to use is a #LigmaColorProfileChooserDialog.
 * If a #LigmaColorProfileChooserDialog is passed, #LigmaColorProfileComboBox
 * will take complete control over the dialog, which means connecting
 * a GtkDialog::response() callback by itself, and take care of destroying
 * the dialog when the combo box is destroyed.
 *
 * If another type of @dialog is passed, this has to be implemented
 * separately.
 *
 * See also ligma_color_profile_combo_box_new_with_model().
 *
 * Returns: a new #LigmaColorProfileComboBox.
 *
 * Since: 2.4
 **/
GtkWidget *
ligma_color_profile_combo_box_new (GtkWidget *dialog,
                                  GFile     *history)
{
  GtkWidget    *combo;
  GtkListStore *store;

  g_return_val_if_fail (GTK_IS_DIALOG (dialog), NULL);
  g_return_val_if_fail (history == NULL || G_IS_FILE (history), NULL);

  store = ligma_color_profile_store_new (history);
  combo = ligma_color_profile_combo_box_new_with_model (dialog,
                                                       GTK_TREE_MODEL (store));
  g_object_unref (store);

  return combo;
}

/**
 * ligma_color_profile_combo_box_new_with_model:
 * @dialog: a #GtkDialog to present when the user selects the
 *          "Select color profile from disk..." item
 * @model:  a #LigmaColorProfileStore object
 *
 * This constructor is useful when you want to create several
 * combo-boxes for profile selection that all share the same
 * #LigmaColorProfileStore. This is for example done in the
 * LIGMA Preferences dialog.
 *
 * See also ligma_color_profile_combo_box_new().
 *
 * Returns: a new #LigmaColorProfileComboBox.
 *
 * Since: 2.4
 **/
GtkWidget *
ligma_color_profile_combo_box_new_with_model (GtkWidget    *dialog,
                                             GtkTreeModel *model)
{
  g_return_val_if_fail (GTK_IS_DIALOG (dialog), NULL);
  g_return_val_if_fail (LIGMA_IS_COLOR_PROFILE_STORE (model), NULL);

  return g_object_new (LIGMA_TYPE_COLOR_PROFILE_COMBO_BOX,
                       "dialog", dialog,
                       "model",  model,
                       NULL);
}

/**
 * ligma_color_profile_combo_box_add_file:
 * @combo: a #LigmaColorProfileComboBox
 * @file:  file of the profile to add (or %NULL)
 * @label: label to use for the profile
 *         (may only be %NULL if @file is %NULL)
 *
 * This function delegates to the underlying
 * #LigmaColorProfileStore. Please refer to the documentation of
 * ligma_color_profile_store_add_file() for details.
 *
 * Since: 2.10
 **/
void
ligma_color_profile_combo_box_add_file (LigmaColorProfileComboBox *combo,
                                       GFile                    *file,
                                       const gchar              *label)
{
  GtkTreeModel *model;

  g_return_if_fail (LIGMA_IS_COLOR_PROFILE_COMBO_BOX (combo));
  g_return_if_fail (label != NULL || file == NULL);
  g_return_if_fail (file == NULL || G_IS_FILE (file));

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));

  ligma_color_profile_store_add_file (LIGMA_COLOR_PROFILE_STORE (model),
                                     file, label);
}

/**
 * ligma_color_profile_combo_box_set_active_file:
 * @combo: a #LigmaColorProfileComboBox
 * @file:  file of the profile to select
 * @label: label to use when adding a new entry (can be %NULL)
 *
 * Selects a color profile from the @combo and makes it the active
 * item.  If the profile is not listed in the @combo, then it is added
 * with the given @label (or @file in case that @label is %NULL).
 *
 * Since: 2.10
 **/
void
ligma_color_profile_combo_box_set_active_file (LigmaColorProfileComboBox *combo,
                                              GFile                    *file,
                                              const gchar              *label)
{
  LigmaColorProfile *profile = NULL;
  GtkTreeModel     *model;
  GtkTreeIter       iter;

  g_return_if_fail (LIGMA_IS_COLOR_PROFILE_COMBO_BOX (combo));
  g_return_if_fail (file == NULL || G_IS_FILE (file));

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));

  if (file && ! (label && *label))
    {
      GError *error = NULL;

      profile = ligma_color_profile_new_from_file (file, &error);

      if (! profile)
        {
          g_message ("%s", error->message);
          g_clear_error (&error);
        }
      else
        {
          label = ligma_color_profile_get_label (profile);
        }
    }

  if (_ligma_color_profile_store_history_add (LIGMA_COLOR_PROFILE_STORE (model),
                                             file, label, &iter))
    {
      gtk_combo_box_set_active_iter (GTK_COMBO_BOX (combo), &iter);
    }

  if (profile)
    g_object_unref (profile);
}

/**
 * ligma_color_profile_combo_box_set_active_profile:
 * @combo:   a #LigmaColorProfileComboBox
 * @profile: a #LigmaColorProfile to set
 *
 * Selects a color profile from the @combo and makes it the active
 * item.
 *
 * Since: 3.0
 **/
void
ligma_color_profile_combo_box_set_active_profile (LigmaColorProfileComboBox *combo,
                                                 LigmaColorProfile         *profile)
{
  GtkTreeModel     *model;
  GtkTreeIter       iter;

  g_return_if_fail (LIGMA_IS_COLOR_PROFILE_COMBO_BOX (combo));
  g_return_if_fail (profile == NULL || LIGMA_IS_COLOR_PROFILE (profile));

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));

  if (_ligma_color_profile_store_history_find_profile (LIGMA_COLOR_PROFILE_STORE (model),
                                                      profile, &iter))
    {
      gtk_combo_box_set_active_iter (GTK_COMBO_BOX (combo), &iter);
    }
}

/**
 * ligma_color_profile_combo_box_get_active_file:
 * @combo: a #LigmaColorProfileComboBox
 *
 * Returns: (transfer none): The file of the currently selected
 *               color profile, release using g_object_unref() when it
 *               is not any longer needed.
 *
 * Since: 2.10
 **/
GFile *
ligma_color_profile_combo_box_get_active_file (LigmaColorProfileComboBox *combo)
{
  GtkTreeModel *model;
  GtkTreeIter   iter;

  g_return_val_if_fail (LIGMA_IS_COLOR_PROFILE_COMBO_BOX (combo), NULL);

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));

  if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (combo), &iter))
    {
      GFile *file;
      gint   type;

      gtk_tree_model_get (model, &iter,
                          LIGMA_COLOR_PROFILE_STORE_ITEM_TYPE, &type,
                          LIGMA_COLOR_PROFILE_STORE_FILE,      &file,
                          -1);

      if (type == LIGMA_COLOR_PROFILE_STORE_ITEM_FILE)
        return file;

      if (file)
        g_object_unref (file);
    }

  return NULL;
}

static gboolean
ligma_color_profile_row_separator_func (GtkTreeModel *model,
                                       GtkTreeIter  *iter,
                                       gpointer      data)
{
  gint type;

  gtk_tree_model_get (model, iter,
                      LIGMA_COLOR_PROFILE_STORE_ITEM_TYPE, &type,
                      -1);

  switch (type)
    {
    case LIGMA_COLOR_PROFILE_STORE_ITEM_SEPARATOR_TOP:
    case LIGMA_COLOR_PROFILE_STORE_ITEM_SEPARATOR_BOTTOM:
      return TRUE;

    default:
      return FALSE;
    }
}

static void
ligma_color_profile_combo_dialog_response (LigmaColorProfileChooserDialog *dialog,
                                          gint                           response,
                                          LigmaColorProfileComboBox      *combo)
{
  if (response == GTK_RESPONSE_ACCEPT)
    {
      GFile *file;

      file = gtk_file_chooser_get_file (GTK_FILE_CHOOSER (dialog));

      if (file)
        {
          ligma_color_profile_combo_box_set_active_file (combo, file, NULL);

          g_object_unref (file);
        }
    }

  gtk_widget_hide (GTK_WIDGET (dialog));
}
