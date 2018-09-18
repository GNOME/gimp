/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpintcombobox.c
 * Copyright (C) 2004  Sven Neumann <sven@gimp.org>
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

#include <libintl.h>

#include <gtk/gtk.h>

#include "gimpwidgetstypes.h"

#include "gimpintcombobox.h"
#include "gimpintstore.h"


/**
 * SECTION: gimpintcombobox
 * @title: GimpIntComboBox
 * @short_description: A widget providing a popup menu of integer
 *                     values (e.g. enums).
 *
 * A widget providing a popup menu of integer values (e.g. enums).
 **/


enum
{
  PROP_0,
  PROP_ELLIPSIZE,
  PROP_LABEL,
  PROP_LAYOUT
};


typedef struct
{
  GtkCellRenderer        *pixbuf_renderer;
  GtkCellRenderer        *text_renderer;

  GtkCellRenderer        *menu_pixbuf_renderer;
  GtkCellRenderer        *menu_text_renderer;

  PangoEllipsizeMode      ellipsize;
  gchar                  *label;
  GtkCellRenderer        *label_renderer;
  GimpIntComboBoxLayout   layout;

  GimpIntSensitivityFunc  sensitivity_func;
  gpointer                sensitivity_data;
  GDestroyNotify          sensitivity_destroy;
} GimpIntComboBoxPrivate;

#define GIMP_INT_COMBO_BOX_GET_PRIVATE(obj) \
  ((GimpIntComboBoxPrivate *) ((GimpIntComboBox *) (obj))->priv)


static void  gimp_int_combo_box_finalize     (GObject         *object);
static void  gimp_int_combo_box_set_property (GObject         *object,
                                              guint            property_id,
                                              const GValue    *value,
                                              GParamSpec      *pspec);
static void  gimp_int_combo_box_get_property (GObject         *object,
                                              guint            property_id,
                                              GValue          *value,
                                              GParamSpec      *pspec);

static void  gimp_int_combo_box_create_cells (GimpIntComboBox *combo_box);
static void  gimp_int_combo_box_data_func    (GtkCellLayout   *layout,
                                              GtkCellRenderer *cell,
                                              GtkTreeModel    *model,
                                              GtkTreeIter     *iter,
                                              gpointer         data);


G_DEFINE_TYPE_WITH_PRIVATE (GimpIntComboBox, gimp_int_combo_box,
                            GTK_TYPE_COMBO_BOX)

#define parent_class gimp_int_combo_box_parent_class


static void
gimp_int_combo_box_class_init (GimpIntComboBoxClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize     = gimp_int_combo_box_finalize;
  object_class->set_property = gimp_int_combo_box_set_property;
  object_class->get_property = gimp_int_combo_box_get_property;

  /**
   * GimpIntComboBox:ellipsize:
   *
   * Specifies the preferred place to ellipsize text in the combo-box,
   * if the cell renderer does not have enough room to display the
   * entire string.
   *
   * Since: 2.4
   */
  g_object_class_install_property (object_class, PROP_ELLIPSIZE,
                                   g_param_spec_enum ("ellipsize",
                                                      "Ellipsize",
                                                      "Ellipsize mode for the used text cell renderer",
                                                      PANGO_TYPE_ELLIPSIZE_MODE,
                                                      PANGO_ELLIPSIZE_NONE,
                                                      GIMP_PARAM_READWRITE));

  /**
   * GimpIntComboBox:label:
   *
   * Sets a label on the combo-box, see gimp_int_combo_box_set_label().
   *
   * Since: 2.10
   */
  g_object_class_install_property (object_class, PROP_LABEL,
                                   g_param_spec_string ("label",
                                                        "Label",
                                                        "An optional label to be displayed",
                                                        NULL,
                                                        GIMP_PARAM_READWRITE));

  /**
   * GimpIntComboBox:layout:
   *
   * Specifies the combo box layout.
   *
   * Since: 2.10
   */
  g_object_class_install_property (object_class, PROP_LAYOUT,
                                   g_param_spec_enum ("layout",
                                                      "Layout",
                                                      "Combo box layout",
                                                      GIMP_TYPE_INT_COMBO_BOX_LAYOUT,
                                                      GIMP_INT_COMBO_BOX_LAYOUT_ABBREVIATED,
                                                      GIMP_PARAM_READWRITE));
}

static void
gimp_int_combo_box_init (GimpIntComboBox *combo_box)
{
  GimpIntComboBoxPrivate *priv;
  GtkListStore           *store;

  combo_box->priv = priv = gimp_int_combo_box_get_instance_private (combo_box);

  store = gimp_int_store_new ();
  gtk_combo_box_set_model (GTK_COMBO_BOX (combo_box), GTK_TREE_MODEL (store));
  g_object_unref (store);

  priv->layout = GIMP_INT_COMBO_BOX_LAYOUT_ABBREVIATED;

  gimp_int_combo_box_create_cells (GIMP_INT_COMBO_BOX (combo_box));
}

static void
gimp_int_combo_box_finalize (GObject *object)
{
  GimpIntComboBoxPrivate *priv = GIMP_INT_COMBO_BOX_GET_PRIVATE (object);

  g_clear_pointer (&priv->label, g_free);

  if (priv->sensitivity_destroy)
    {
      GDestroyNotify d = priv->sensitivity_destroy;

      priv->sensitivity_destroy = NULL;
      d (priv->sensitivity_data);
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_int_combo_box_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  GimpIntComboBoxPrivate *priv = GIMP_INT_COMBO_BOX_GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_ELLIPSIZE:
      priv->ellipsize = g_value_get_enum (value);
      if (priv->text_renderer)
        {
          g_object_set_property (G_OBJECT (priv->text_renderer),
                                 pspec->name, value);
        }
      break;
    case PROP_LABEL:
      gimp_int_combo_box_set_label (GIMP_INT_COMBO_BOX (object),
                                    g_value_get_string (value));
      break;
    case PROP_LAYOUT:
      gimp_int_combo_box_set_layout (GIMP_INT_COMBO_BOX (object),
                                     g_value_get_enum (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_int_combo_box_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  GimpIntComboBoxPrivate *priv = GIMP_INT_COMBO_BOX_GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_ELLIPSIZE:
      g_value_set_enum (value, priv->ellipsize);
      break;
    case PROP_LABEL:
      g_value_set_string (value, priv->label);
      break;
    case PROP_LAYOUT:
      g_value_set_enum (value, priv->layout);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
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
 * If you need to construct an empty #GimpIntComboBox, it's best to use
 * g_object_new (GIMP_TYPE_INT_COMBO_BOX, NULL).
 *
 * Return value: a new #GimpIntComboBox.
 *
 * Since: 2.2
 **/
GtkWidget *
gimp_int_combo_box_new (const gchar *first_label,
                        gint         first_value,
                        ...)
{
  GtkWidget *combo_box;
  va_list    args;

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
 * Since: 2.2
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

  combo_box = g_object_new (GIMP_TYPE_INT_COMBO_BOX, NULL);

  store = GTK_LIST_STORE (gtk_combo_box_get_model (GTK_COMBO_BOX (combo_box)));

  for (label = first_label, value = first_value;
       label;
       label = va_arg (values, const gchar *), value = va_arg (values, gint))
    {
      GtkTreeIter  iter = { 0, };

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
 * Since: 2.2
 **/
GtkWidget *
gimp_int_combo_box_new_array (gint         n_values,
                              const gchar *labels[])
{
  GtkWidget    *combo_box;
  GtkListStore *store;
  gint          i;

  g_return_val_if_fail (n_values >= 0, NULL);
  g_return_val_if_fail (labels != NULL || n_values == 0, NULL);

  combo_box = g_object_new (GIMP_TYPE_INT_COMBO_BOX, NULL);

  store = GTK_LIST_STORE (gtk_combo_box_get_model (GTK_COMBO_BOX (combo_box)));

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
 * gimp_int_combo_box_prepend:
 * @combo_box: a #GimpIntComboBox
 * @...:       pairs of column number and value, terminated with -1
 *
 * This function provides a convenient way to prepend items to a
 * #GimpIntComboBox. It prepends a row to the @combo_box's list store
 * and calls gtk_list_store_set() for you.
 *
 * The column number must be taken from the enum #GimpIntStoreColumns.
 *
 * Since: 2.2
 **/
void
gimp_int_combo_box_prepend (GimpIntComboBox *combo_box,
                            ...)
{
  GtkListStore *store;
  GtkTreeIter   iter;
  va_list       args;

  g_return_if_fail (GIMP_IS_INT_COMBO_BOX (combo_box));

  store = GTK_LIST_STORE (gtk_combo_box_get_model (GTK_COMBO_BOX (combo_box)));

  va_start (args, combo_box);

  gtk_list_store_prepend (store, &iter);
  gtk_list_store_set_valist (store, &iter, args);

  va_end (args);
}

/**
 * gimp_int_combo_box_append:
 * @combo_box: a #GimpIntComboBox
 * @...:       pairs of column number and value, terminated with -1
 *
 * This function provides a convenient way to append items to a
 * #GimpIntComboBox. It appends a row to the @combo_box's list store
 * and calls gtk_list_store_set() for you.
 *
 * The column number must be taken from the enum #GimpIntStoreColumns.
 *
 * Since: 2.2
 **/
void
gimp_int_combo_box_append (GimpIntComboBox *combo_box,
                           ...)
{
  GtkListStore *store;
  GtkTreeIter   iter;
  va_list       args;

  g_return_if_fail (GIMP_IS_INT_COMBO_BOX (combo_box));

  store = GTK_LIST_STORE (gtk_combo_box_get_model (GTK_COMBO_BOX (combo_box)));

  va_start (args, combo_box);

  gtk_list_store_append (store, &iter);
  gtk_list_store_set_valist (store, &iter, args);

  va_end (args);
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
 * Since: 2.2
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
 * Since: 2.2
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

/**
 * gimp_int_combo_box_set_active_by_user_data:
 * @combo_box: a #GimpIntComboBox
 * @user_data: an integer value
 *
 * Looks up the item that has the given @user_data and makes it the
 * selected item in the @combo_box.
 *
 * Return value: %TRUE on success or %FALSE if there was no item for
 *               this user-data.
 *
 * Since: 2.10
 **/
gboolean
gimp_int_combo_box_set_active_by_user_data (GimpIntComboBox *combo_box,
                                            gpointer         user_data)
{
  GtkTreeModel *model;
  GtkTreeIter   iter;

  g_return_val_if_fail (GIMP_IS_INT_COMBO_BOX (combo_box), FALSE);

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo_box));

  if (gimp_int_store_lookup_by_user_data (model, user_data, &iter))
    {
      gtk_combo_box_set_active_iter (GTK_COMBO_BOX (combo_box), &iter);
      return TRUE;
    }

  return FALSE;
}

/**
 * gimp_int_combo_box_get_active_user_data:
 * @combo_box: a #GimpIntComboBox
 * @user_data: return location for the gpointer value
 *
 * Retrieves the user-data of the selected (active) item in the @combo_box.
 *
 * Return value: %TRUE if @user_data has been set or %FALSE if no item was
 *               active.
 *
 * Since: 2.10
 **/
gboolean
gimp_int_combo_box_get_active_user_data (GimpIntComboBox *combo_box,
                                         gpointer        *user_data)
{
  GtkTreeIter  iter;

  g_return_val_if_fail (GIMP_IS_INT_COMBO_BOX (combo_box), FALSE);
  g_return_val_if_fail (user_data != NULL, FALSE);

  if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (combo_box), &iter))
    {
      gtk_tree_model_get (gtk_combo_box_get_model (GTK_COMBO_BOX (combo_box)),
                          &iter,
                          GIMP_INT_STORE_USER_DATA, user_data,
                          -1);
      return TRUE;
    }

  return FALSE;
}

/**
 * gimp_int_combo_box_connect:
 * @combo_box: a #GimpIntComboBox
 * @value:     the value to set
 * @callback:  a callback to connect to the @combo_box's "changed" signal
 * @data:      a pointer passed as data to g_signal_connect()
 *
 * A convenience function that sets the initial @value of a
 * #GimpIntComboBox and connects @callback to the "changed"
 * signal.
 *
 * This function also calls the @callback once after setting the
 * initial @value. This is often convenient when working with combo
 * boxes that select a default active item, like for example
 * gimp_drawable_combo_box_new(). If you pass an invalid initial
 * @value, the @callback will be called with the default item active.
 *
 * Return value: the signal handler ID as returned by g_signal_connect()
 *
 * Since: 2.2
 **/
gulong
gimp_int_combo_box_connect (GimpIntComboBox *combo_box,
                            gint             value,
                            GCallback        callback,
                            gpointer         data)
{
  gulong handler = 0;

  g_return_val_if_fail (GIMP_IS_INT_COMBO_BOX (combo_box), 0);

  if (callback)
    handler = g_signal_connect (combo_box, "changed", callback, data);

  if (! gimp_int_combo_box_set_active (combo_box, value))
    g_signal_emit_by_name (combo_box, "changed", NULL);

  return handler;
}

/**
 * gimp_int_combo_box_set_label:
 * @combo_box: a #GimpIntComboBox
 * @label:     a string to be shown as label
 *
 * Sets a caption on the @combo_box that will be displayed
 * left-aligned inside the box. When a label is set, the remaining
 * contents of the box will be right-aligned. This is useful for
 * places where screen estate is rare, like in tool options.
 *
 * Since: 2.10
 **/
void
gimp_int_combo_box_set_label (GimpIntComboBox *combo_box,
                              const gchar     *label)
{
  GimpIntComboBoxPrivate *priv;

  g_return_if_fail (GIMP_IS_INT_COMBO_BOX (combo_box));

  priv = GIMP_INT_COMBO_BOX_GET_PRIVATE (combo_box);

  if (label == priv->label)
    return;

  g_free (priv->label);

  priv->label = g_strdup (label);

  gimp_int_combo_box_create_cells (combo_box);

  g_object_notify (G_OBJECT (combo_box), "label");
}

/**
 * gimp_int_combo_box_get_label:
 * @combo_box: a #GimpIntComboBox
 *
 * Returns the label previously set with gimp_int_combo_box_set_label(),
 * or %NULL,
 *
 * Return value: the @combo_box' label.
 *
 * Since: 2.10
 **/
const gchar *
gimp_int_combo_box_get_label (GimpIntComboBox *combo_box)
{
  g_return_val_if_fail (GIMP_IS_INT_COMBO_BOX (combo_box), NULL);

  return GIMP_INT_COMBO_BOX_GET_PRIVATE (combo_box)->label;
}

/**
 * gimp_int_combo_box_set_layout:
 * @combo_box: a #GimpIntComboBox
 * @layout:    the combo box layout
 *
 * Sets the layout of @combo_box to @layout.
 *
 * Since: 2.10
 **/
void
gimp_int_combo_box_set_layout (GimpIntComboBox       *combo_box,
                               GimpIntComboBoxLayout  layout)
{
  GimpIntComboBoxPrivate *priv;

  g_return_if_fail (GIMP_IS_INT_COMBO_BOX (combo_box));

  priv = GIMP_INT_COMBO_BOX_GET_PRIVATE (combo_box);

  if (layout == priv->layout)
    return;

  priv->layout = layout;

  gimp_int_combo_box_create_cells (combo_box);

  g_object_notify (G_OBJECT (combo_box), "layout");
}

/**
 * gimp_int_combo_box_get_layout:
 * @combo_box: a #GimpIntComboBox
 *
 * Returns the layout of @combo_box
 *
 * Return value: the @combo_box's layout.
 *
 * Since: 2.10
 **/
GimpIntComboBoxLayout
gimp_int_combo_box_get_layout (GimpIntComboBox *combo_box)
{
  g_return_val_if_fail (GIMP_IS_INT_COMBO_BOX (combo_box),
                        GIMP_INT_COMBO_BOX_LAYOUT_ABBREVIATED);

  return GIMP_INT_COMBO_BOX_GET_PRIVATE (combo_box)->layout;
}

/**
 * gimp_int_combo_box_set_sensitivity:
 * @combo_box: a #GimpIntComboBox
 * @func: a function that returns a boolean value, or %NULL to unset
 * @data: data to pass to @func
 * @destroy: destroy notification for @data
 *
 * Sets a function that is used to decide about the sensitivity of
 * rows in the @combo_box. Use this if you want to set certain rows
 * insensitive.
 *
 * Calling gtk_widget_queue_draw() on the @combo_box will cause the
 * sensitivity to be updated.
 *
 * Since: 2.4
 **/
void
gimp_int_combo_box_set_sensitivity (GimpIntComboBox        *combo_box,
                                    GimpIntSensitivityFunc  func,
                                    gpointer                data,
                                    GDestroyNotify          destroy)
{
  GimpIntComboBoxPrivate *priv;

  g_return_if_fail (GIMP_IS_INT_COMBO_BOX (combo_box));

  priv = GIMP_INT_COMBO_BOX_GET_PRIVATE (combo_box);

  if (priv->sensitivity_destroy)
    {
      GDestroyNotify d = priv->sensitivity_destroy;

      priv->sensitivity_destroy = NULL;
      d (priv->sensitivity_data);
    }

  priv->sensitivity_func    = func;
  priv->sensitivity_data    = data;
  priv->sensitivity_destroy = destroy;

  gimp_int_combo_box_create_cells (combo_box);
}


/*  private functions  */

static void
queue_resize_cell_view (GtkContainer *container)
{
  GList *children = gtk_container_get_children (container);
  GList *list;

  for (list = children; list; list = g_list_next (list))
    {
      if (GTK_IS_CELL_VIEW (list->data))
        {
          gtk_widget_queue_resize (list->data);
          break;
        }
      else if (GTK_IS_CONTAINER (list->data))
        {
          queue_resize_cell_view (list->data);
        }
    }

  g_list_free (children);
}

static void
gimp_int_combo_box_create_cells (GimpIntComboBox *combo_box)
{
  GimpIntComboBoxPrivate *priv = GIMP_INT_COMBO_BOX_GET_PRIVATE (combo_box);
  GtkCellLayout          *layout;

  /*  menu layout  */

  layout = GTK_CELL_LAYOUT (combo_box);

  gtk_cell_layout_clear (layout);

  priv->menu_pixbuf_renderer = gtk_cell_renderer_pixbuf_new ();
  g_object_set (priv->menu_pixbuf_renderer,
                "xpad", 2,
                NULL);

  priv->menu_text_renderer = gtk_cell_renderer_text_new ();

  gtk_cell_layout_pack_start (layout,
                              priv->menu_pixbuf_renderer, FALSE);
  gtk_cell_layout_pack_start (layout,
                              priv->menu_text_renderer, TRUE);

  gtk_cell_layout_set_attributes (layout,
                                  priv->menu_pixbuf_renderer,
                                  "icon-name", GIMP_INT_STORE_ICON_NAME,
                                  "pixbuf",    GIMP_INT_STORE_PIXBUF,
                                  NULL);
  gtk_cell_layout_set_attributes (layout,
                                  priv->menu_text_renderer,
                                  "text", GIMP_INT_STORE_LABEL,
                                  NULL);

  if (priv->sensitivity_func)
    {
      gtk_cell_layout_set_cell_data_func (layout,
                                          priv->menu_pixbuf_renderer,
                                          gimp_int_combo_box_data_func,
                                          priv, NULL);

      gtk_cell_layout_set_cell_data_func (layout,
                                          priv->menu_text_renderer,
                                          gimp_int_combo_box_data_func,
                                          priv, NULL);
    }

  /*  combo box layout  */

  layout = GTK_CELL_LAYOUT (gtk_bin_get_child (GTK_BIN (combo_box)));

  gtk_cell_layout_clear (layout);

  if (priv->layout != GIMP_INT_COMBO_BOX_LAYOUT_ICON_ONLY)
    {
      priv->text_renderer = gtk_cell_renderer_text_new ();
      g_object_set (priv->text_renderer,
                    "ellipsize", priv->ellipsize,
                    NULL);
    }
  else
    {
      priv->text_renderer = NULL;
    }

  priv->pixbuf_renderer = gtk_cell_renderer_pixbuf_new ();

  if (priv->text_renderer)
    {
      g_object_set (priv->pixbuf_renderer,
                    "xpad", 2,
                    NULL);
    }

  if (priv->label)
    {
      priv->label_renderer = gtk_cell_renderer_text_new ();
      g_object_set (priv->label_renderer,
                    "text", priv->label,
                    NULL);

      gtk_cell_layout_pack_start (layout,
                                  priv->label_renderer, FALSE);

      gtk_cell_layout_pack_end (layout,
                                priv->pixbuf_renderer, FALSE);

      if (priv->text_renderer)
        {
          gtk_cell_layout_pack_end (layout,
                                    priv->text_renderer, TRUE);

          g_object_set (priv->text_renderer,
                        "xalign", 1.0,
                        NULL);
        }
    }
  else
    {
      gtk_cell_layout_pack_start (layout,
                                  priv->pixbuf_renderer, FALSE);

      if (priv->text_renderer)
        {
          gtk_cell_layout_pack_start (layout,
                                      priv->text_renderer, TRUE);
        }
    }

  gtk_cell_layout_set_attributes (layout,
                                  priv->pixbuf_renderer,
                                  "icon-name", GIMP_INT_STORE_ICON_NAME,
                                  NULL);

  if (priv->text_renderer)
    {
      gtk_cell_layout_set_attributes (layout,
                                      priv->text_renderer,
                                      "text", GIMP_INT_STORE_LABEL,
                                      NULL);
    }

  if (priv->layout == GIMP_INT_COMBO_BOX_LAYOUT_ABBREVIATED ||
      priv->sensitivity_func)
    {
      gtk_cell_layout_set_cell_data_func (layout,
                                          priv->pixbuf_renderer,
                                          gimp_int_combo_box_data_func,
                                          priv, NULL);

      if (priv->text_renderer)
        {
          gtk_cell_layout_set_cell_data_func (layout,
                                              priv->text_renderer,
                                              gimp_int_combo_box_data_func,
                                              priv, NULL);
        }
    }

  /* HACK: GtkCellView doesn't invalidate itself when stuff is
   * added/removed, work around this bug until GTK+ 2.24.19
   */
  if (gtk_check_version (2, 24, 19))
    {
      GList *attached_menus;

      queue_resize_cell_view (GTK_CONTAINER (combo_box));

      /* HACK HACK HACK OMG */
      attached_menus = g_object_get_data (G_OBJECT (combo_box),
                                          "gtk-attached-menus");

      for (; attached_menus; attached_menus = g_list_next (attached_menus))
        queue_resize_cell_view (attached_menus->data);
    }
}

static void
gimp_int_combo_box_data_func (GtkCellLayout   *layout,
                              GtkCellRenderer *cell,
                              GtkTreeModel    *model,
                              GtkTreeIter     *iter,
                              gpointer         data)
{
  GimpIntComboBoxPrivate *priv = data;

  if (priv->layout == GIMP_INT_COMBO_BOX_LAYOUT_ABBREVIATED &&
      cell == priv->text_renderer)
    {
      gchar *abbrev;

      gtk_tree_model_get (model, iter,
                          GIMP_INT_STORE_ABBREV, &abbrev,
                          -1);

      if (abbrev)
        {
          g_object_set (cell,
                        "text", abbrev,
                        NULL);

          g_free (abbrev);
        }
    }

  if (priv->sensitivity_func)
    {
      gint      value;
      gboolean  sensitive;

      gtk_tree_model_get (model, iter,
                          GIMP_INT_STORE_VALUE, &value,
                          -1);

      sensitive = priv->sensitivity_func (value, priv->sensitivity_data);

      g_object_set (cell,
                    "sensitive", sensitive,
                    NULL);
    }
}
