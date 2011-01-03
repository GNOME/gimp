/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpcolorprofilecombobox.c
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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"

#include "gimpwidgetstypes.h"

#include "gimpcolorprofilechooserdialog.h"
#include "gimpcolorprofilecombobox.h"
#include "gimpcolorprofilestore.h"
#include "gimpcolorprofilestore-private.h"


/**
 * SECTION: gimpcolorprofilecombobox
 * @title: GimpColorProfileComboBox
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


typedef struct _GimpColorProfileComboBoxPrivate GimpColorProfileComboBoxPrivate;

struct _GimpColorProfileComboBoxPrivate
{
  GtkWidget   *dialog;
  GtkTreePath *last_path;
};

#define GET_PRIVATE(obj) G_TYPE_INSTANCE_GET_PRIVATE (obj, \
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

static void  gimp_color_profile_combo_dialog_response  (GimpColorProfileChooserDialog *dialog,
                                                        gint                           response,
                                                        GimpColorProfileComboBox      *combo);


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

  /**
   * GimpColorProfileComboBox:dialog:
   *
   * #GtkDialog to present when the user selects the
   * "Select color profile from disk..." item.
   *
   * Since: 2.4
   */
  g_object_class_install_property (object_class,
                                   PROP_DIALOG,
                                   g_param_spec_object ("dialog", NULL, NULL,
                                                        GTK_TYPE_DIALOG,
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        GIMP_PARAM_READWRITE));
  /**
   * GimpColorProfileComboBox:model:
   *
   * Overrides the "model" property of the #GtkComboBox class.
   * #GimpColorProfileComboBox requires the model to be a
   * #GimpColorProfileStore.
   *
   * Since: 2.4
   */
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

  g_object_set (cell,
                "width-chars", 42,
                "ellipsize",   PANGO_ELLIPSIZE_END,
                NULL);


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
  GimpColorProfileComboBoxPrivate *private = GET_PRIVATE (object);

  if (private->dialog)
    {
      if (GIMP_IS_COLOR_PROFILE_CHOOSER_DIALOG (private->dialog))
        gtk_widget_destroy (private->dialog);

      g_object_unref (private->dialog);
      private->dialog = NULL;
    }

  if (private->last_path)
    {
      gtk_tree_path_free (private->last_path);
      private->last_path = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_color_profile_combo_box_set_property (GObject      *object,
                                           guint         property_id,
                                           const GValue *value,
                                           GParamSpec   *pspec)
{
  GimpColorProfileComboBoxPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_DIALOG:
      g_return_if_fail (private->dialog == NULL);
      private->dialog = g_value_dup_object (value);

      if (GIMP_IS_COLOR_PROFILE_CHOOSER_DIALOG (private->dialog))
        g_signal_connect (private->dialog, "response",
                          G_CALLBACK (gimp_color_profile_combo_dialog_response),
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
gimp_color_profile_combo_box_get_property (GObject    *object,
                                           guint       property_id,
                                           GValue     *value,
                                           GParamSpec *pspec)
{
  GimpColorProfileComboBoxPrivate *private = GET_PRIVATE (object);

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
gimp_color_profile_combo_box_changed (GtkComboBox *combo)
{
  GimpColorProfileComboBoxPrivate *priv  = GET_PRIVATE (combo);
  GtkTreeModel                    *model = gtk_combo_box_get_model (combo);
  GtkTreeIter                      iter;
  gint                             type;

  if (! gtk_combo_box_get_active_iter (combo, &iter))
    return;

  gtk_tree_model_get (model, &iter,
                      GIMP_COLOR_PROFILE_STORE_ITEM_TYPE, &type,
                      -1);

  switch (type)
    {
    case GIMP_COLOR_PROFILE_STORE_ITEM_DIALOG:
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

    case GIMP_COLOR_PROFILE_STORE_ITEM_FILE:
      if (priv->last_path)
        gtk_tree_path_free (priv->last_path);

      priv->last_path = gtk_tree_model_get_path (model, &iter);

      _gimp_color_profile_store_history_reorder (GIMP_COLOR_PROFILE_STORE (model),
                                                 &iter);
      break;

    default:
      break;
    }
}


/**
 * gimp_color_profile_combo_box_new:
 * @dialog:  a #GtkDialog to present when the user selects the
 *           "Select color profile from disk..." item
 * @history: filename of the profilerc (or %NULL for no history)
 *
 * Create a combo-box widget for selecting color profiles. The combo-box
 * is populated from the file specified as @history. This filename is
 * typically created using the following code snippet:
 * <informalexample><programlisting>
 *  gchar *history = gimp_personal_rc_file ("profilerc");
 * </programlisting></informalexample>
 *
 * The recommended @dialog type to use is a #GimpColorProfileChooserDialog.
 * If a #GimpColorProfileChooserDialog is passed, #GimpColorProfileComboBox
 * will take complete control over the dialog, which means connecting
 * a GtkDialog::response() callback by itself, and take care of destroying
 * the dialog when the combo box is destroyed.
 *
 * If another type of @dialog is passed, this has to be implemented
 * separately.
 *
 * See also gimp_color_profile_combo_box_new_with_model().
 *
 * Return value: a new #GimpColorProfileComboBox.
 *
 * Since: 2.4
 **/
GtkWidget *
gimp_color_profile_combo_box_new (GtkWidget   *dialog,
                                  const gchar *history)
{
  GtkWidget    *combo;
  GtkListStore *store;

  g_return_val_if_fail (GTK_IS_DIALOG (dialog), NULL);

  store = gimp_color_profile_store_new (history);
  combo = gimp_color_profile_combo_box_new_with_model (dialog,
                                                       GTK_TREE_MODEL (store));
  g_object_unref (store);

  return combo;
}

/**
 * gimp_color_profile_combo_box_new_with_model:
 * @dialog: a #GtkDialog to present when the user selects the
 *          "Select color profile from disk..." item
 * @model:  a #GimpColorProfileStore object
 *
 * This constructor is useful when you want to create several
 * combo-boxes for profile selection that all share the same
 * #GimpColorProfileStore. This is for example done in the
 * GIMP Preferences dialog.
 *
 * See also gimp_color_profile_combo_box_new().
 *
 * Return value: a new #GimpColorProfileComboBox.
 *
 * Since: 2.4
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
 * @combo:    a #GimpColorProfileComboBox
 * @filename: filename of the profile to add (or %NULL)
 * @label:    label to use for the profile
 *            (may only be %NULL if @filename is %NULL)
 *
 * This function delegates to the underlying
 * #GimpColorProfileStore. Please refer to the documentation of
 * gimp_color_profile_store_add_file() for details.
 *
 * Deprecated: use gimp_color_profile_combo_box_add_file() instead.
 *
 * Since: 2.4
 **/
void
gimp_color_profile_combo_box_add (GimpColorProfileComboBox *combo,
                                  const gchar              *filename,
                                  const gchar              *label)
{
  GFile *file = NULL;

  g_return_if_fail (GIMP_IS_COLOR_PROFILE_COMBO_BOX (combo));
  g_return_if_fail (label != NULL || filename == NULL);

  if (filename)
    file = g_file_new_for_path (filename);

  gimp_color_profile_combo_box_add_file (combo, file, label);

  if (file)
    g_object_unref (file);
}

/**
 * gimp_color_profile_combo_box_add_file:
 * @combo: a #GimpColorProfileComboBox
 * @file:  file of the profile to add (or %NULL)
 * @label: label to use for the profile
 *         (may only be %NULL if @file is %NULL)
 *
 * This function delegates to the underlying
 * #GimpColorProfileStore. Please refer to the documentation of
 * gimp_color_profile_store_add_file() for details.
 *
 * Since: 2.10
 **/
void
gimp_color_profile_combo_box_add_file (GimpColorProfileComboBox *combo,
                                       GFile                    *file,
                                       const gchar              *label)
{
  GtkTreeModel *model;

  g_return_if_fail (GIMP_IS_COLOR_PROFILE_COMBO_BOX (combo));
  g_return_if_fail (label != NULL || file == NULL);
  g_return_if_fail (file == NULL || G_IS_FILE (file));

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));

  gimp_color_profile_store_add_file (GIMP_COLOR_PROFILE_STORE (model),
                                     file, label);
}

/**
 * gimp_color_profile_combo_box_set_active:
 * @combo:    a #GimpColorProfileComboBox
 * @filename: filename of the profile to select
 * @label:    label to use when adding a new entry (can be %NULL)
 *
 * Selects a color profile from the @combo and makes it the active
 * item.  If the profile is not listed in the @combo, then it is added
 * with the given @label (or @filename in case that @label is %NULL).
 *
 * Deprecated: use gimp_color_profile_combo_box_set_active_file() instead.
 *
 * Since: 2.4
 **/
void
gimp_color_profile_combo_box_set_active (GimpColorProfileComboBox *combo,
                                         const gchar              *filename,
                                         const gchar              *label)
{
  GFile *file = NULL;

  g_return_if_fail (GIMP_IS_COLOR_PROFILE_COMBO_BOX (combo));

  if (filename)
    file = g_file_new_for_path (filename);

  gimp_color_profile_combo_box_set_active_file (combo, file, label);

  if (file)
    g_object_unref (file);
}

/**
 * gimp_color_profile_combo_box_set_active_file:
 * @combo: a #GimpColorProfileComboBox
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
gimp_color_profile_combo_box_set_active_file (GimpColorProfileComboBox *combo,
                                              GFile                    *file,
                                              const gchar              *label)
{
  GimpColorProfile *profile = NULL;
  GtkTreeModel     *model;
  GtkTreeIter       iter;

  g_return_if_fail (GIMP_IS_COLOR_PROFILE_COMBO_BOX (combo));
  g_return_if_fail (file == NULL || G_IS_FILE (file));

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));

  if (file && ! (label && *label))
    {
      GError *error = NULL;

      profile = gimp_color_profile_new_from_file (file, &error);

      if (! profile)
        {
          g_message ("%s", error->message);
          g_clear_error (&error);
        }
      else
        {
          label = gimp_color_profile_get_label (profile);
        }
    }

  if (_gimp_color_profile_store_history_add (GIMP_COLOR_PROFILE_STORE (model),
                                             file, label, &iter))
    {
      gtk_combo_box_set_active_iter (GTK_COMBO_BOX (combo), &iter);
    }

  if (profile)
    g_object_unref (profile);
}

/**
 * gimp_color_profile_combo_box_get_active:
 * @combo: a #GimpColorProfileComboBox
 *
 * Return value: The filename of the currently selected color profile,
 *               This is a newly allocated string and should be released
 *               using g_free() when it is not any longer needed.
 *
 * Deprecated: use gimp_color_profile_combo_box_get_active_file() instead.
 *
 * Since: 2.4
 **/
gchar *
gimp_color_profile_combo_box_get_active (GimpColorProfileComboBox *combo)
{
  GFile *file;
  gchar *path = NULL;

  g_return_val_if_fail (GIMP_IS_COLOR_PROFILE_COMBO_BOX (combo), NULL);

  file = gimp_color_profile_combo_box_get_active_file (combo);

  if (file)
    {
      path = g_file_get_path (file);
      g_object_unref (file);
    }

  return path;
}

/**
 * gimp_color_profile_combo_box_get_active_file:
 * @combo: a #GimpColorProfileComboBox
 *
 * Return value: The file of the currently selected color profile,
 *               release using g_object_unref() when it is not any
 *               longer needed.
 *
 * Since: 2.10
 **/
GFile *
gimp_color_profile_combo_box_get_active_file (GimpColorProfileComboBox *combo)
{
  GtkTreeModel *model;
  GtkTreeIter   iter;

  g_return_val_if_fail (GIMP_IS_COLOR_PROFILE_COMBO_BOX (combo), NULL);

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));

  if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (combo), &iter))
    {
      GFile *file;
      gint   type;

      gtk_tree_model_get (model, &iter,
                          GIMP_COLOR_PROFILE_STORE_ITEM_TYPE, &type,
                          GIMP_COLOR_PROFILE_STORE_FILE,      &file,
                          -1);

      if (type == GIMP_COLOR_PROFILE_STORE_ITEM_FILE)
        return file;

      if (file)
        g_object_unref (file);
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

static void
gimp_color_profile_combo_dialog_response (GimpColorProfileChooserDialog *dialog,
                                          gint                           response,
                                          GimpColorProfileComboBox      *combo)
{
  if (response == GTK_RESPONSE_ACCEPT)
    {
      GFile *file;

      file = gtk_file_chooser_get_file (GTK_FILE_CHOOSER (dialog));

      if (file)
        {
          gimp_color_profile_combo_box_set_active_file (combo, file, NULL);

          g_object_unref (file);
        }
    }

  gtk_widget_hide (GTK_WIDGET (dialog));
}
