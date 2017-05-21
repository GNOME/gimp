/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * animation-layer_view.c
 * Copyright (C) 2015 Jehan <jehan@gimp.org>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

#include "animation-layer-view.h"

/* Properties. */
enum
{
  PROP_0,
  PROP_IMAGE
};

/* Tree model rows. */
enum
{
  COLUMN_LAYER_TATTOO,
  COLUMN_LAYER_NAME,
  COLUMN_SIZE
};

/* Signals. */
enum
{
  LAYER_SELECTION,
  LAST_SIGNAL
};

struct _AnimationLayerViewPrivate
{
  gint32 image_id;
};

/* GObject handlers */
static void animation_layer_view_constructed  (GObject      *object);
static void animation_layer_view_set_property (GObject      *object,
                                               guint         property_id,
                                               const GValue *value,
                                               GParamSpec   *pspec);
static void animation_layer_view_get_property (GObject      *object,
                                               guint         property_id,
                                               GValue       *value,
                                               GParamSpec   *pspec);

/* GtkWidget handlers */
static gboolean animation_layer_view_button_press (GtkWidget          *widget,
                                                   GdkEventButton     *event);

/* Utils */
static void          animation_layer_view_fill    (AnimationLayerView *view,
                                                   GtkTreeStore       *store,
                                                   gint                parent_layer,
                                                   GtkTreeIter        *parent);
static GtkTreePath * animation_layer_view_get_row (AnimationLayerView *view,
                                                   gint                tattoo,
                                                   GtkTreeIter        *parent);

/* Signal handlers */
static void          on_selection_changed         (GtkTreeSelection   *selection,
                                                   AnimationLayerView *view);

G_DEFINE_TYPE (AnimationLayerView, animation_layer_view, GTK_TYPE_TREE_VIEW)

#define parent_class animation_layer_view_parent_class

static guint   animation_layer_view_signals[LAST_SIGNAL] = { 0 };

static void
animation_layer_view_class_init (AnimationLayerViewClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  /**
   * AnimationLayerView::layer-selection:
   * @layer_view: the widget which received the signal.
   * @layers: the #GList of layer tattoos which are currently selected.
   *
   * The ::layer-selection signal is emitted each time the selection changes
   * in @layer_view.
   */
  animation_layer_view_signals[LAYER_SELECTION] =
    g_signal_new ("layer-selection",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  0,
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE,
                  1,
                  G_TYPE_POINTER);

  object_class->constructed  = animation_layer_view_constructed;
  object_class->get_property = animation_layer_view_get_property;
  object_class->set_property = animation_layer_view_set_property;

  widget_class->button_press_event = animation_layer_view_button_press;

  /**
   * AnimationLayerView:animation:
   *
   * The associated #GimpImage id.
   */
  g_object_class_install_property (object_class, PROP_IMAGE,
                                   g_param_spec_int ("image", NULL, NULL,
                                                     0, G_MAXINT, 0,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT_ONLY));

  g_type_class_add_private (klass, sizeof (AnimationLayerViewPrivate));
}

static void
animation_layer_view_init (AnimationLayerView *view)
{
  view->priv = G_TYPE_INSTANCE_GET_PRIVATE (view,
                                            ANIMATION_TYPE_LAYER_VIEW,
                                            AnimationLayerViewPrivate);

  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (view), 0,
                                               _("Layer"),
                                               gtk_cell_renderer_text_new (),
                                               "text", COLUMN_LAYER_NAME,
                                               NULL);
  gtk_tree_view_set_rubber_banding (GTK_TREE_VIEW (view), TRUE);
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (view), FALSE);
}

/************ Public Functions ****************/

/**
 * animation_layer_view_new:
 * @image_id: the #GimpImage id.
 *
 * Creates a new layer view tied to @image_id, ready to be displayed.
 */
GtkWidget *
animation_layer_view_new (gint32 image_id)
{
  GtkWidget        *layer_view;
  GtkTreeStore     *store;

  store = gtk_tree_store_new (COLUMN_SIZE, G_TYPE_INT, G_TYPE_STRING);

  layer_view = g_object_new (ANIMATION_TYPE_LAYER_VIEW,
                             "image", image_id,
                             "model", store,
                             NULL);
  g_object_unref (store);

  return layer_view;
}

/**
 * animation_layer_view_refresh:
 * @view: the #AnimationLayerView.
 *
 * Refresh the list of layers by reloading the #GimpImage layers.
 */
void
animation_layer_view_refresh (AnimationLayerView *view)
{
  GtkTreeStore *store;

  store = GTK_TREE_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (view)));
  gtk_tree_store_clear (store);
  animation_layer_view_fill (view, store, 0, NULL);
}

/**
 * animation_layer_view_select:
 * @view: the #AnimationLayerView.
 * @layers: a #GList of #GimpLayer ids.
 *
 * Selects the rows for all @layers in @view.
 */
void
animation_layer_view_select (AnimationLayerView *view,
                             const GList        *layers)
{
  GtkTreeSelection *selection;
  const GList      *layer;

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));
  g_signal_handlers_block_by_func (selection,
                                   G_CALLBACK (on_selection_changed),
                                   view);
  gtk_tree_selection_unselect_all (selection);
  for (layer = layers; layer; layer = layer->next)
    {
      GtkTreePath *path;
      gint         tattoo = GPOINTER_TO_INT (layer->data);

      path = animation_layer_view_get_row (view, tattoo, NULL);
      g_warn_if_fail (path != NULL);
      if (path)
        {
          gtk_tree_selection_select_path (selection, path);
          gtk_tree_path_free (path);
        }
    }
  g_signal_handlers_unblock_by_func (selection,
                                     G_CALLBACK (on_selection_changed),
                                     view);
}

/************ Private Functions ****************/

static void
animation_layer_view_constructed (GObject *object)
{
  GtkTreeView      *view = GTK_TREE_VIEW (object);
  GtkTreeSelection *selection;

  selection = gtk_tree_view_get_selection (view);
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);

  g_signal_connect (selection, "changed",
                    G_CALLBACK (on_selection_changed),
                    view);
}

static void
animation_layer_view_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  AnimationLayerView *view = ANIMATION_LAYER_VIEW (object);

  switch (property_id)
    {
    case PROP_IMAGE:
      view->priv->image_id = g_value_get_int (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
animation_layer_view_get_property (GObject      *object,
                                   guint         property_id,
                                   GValue       *value,
                                   GParamSpec   *pspec)
{
  AnimationLayerView *view = ANIMATION_LAYER_VIEW (object);

  switch (property_id)
    {
    case PROP_IMAGE:
      g_value_set_int (value, view->priv->image_id);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gboolean
animation_layer_view_button_press (GtkWidget      *widget,
                                   GdkEventButton *event)
{
  GtkTreeView         *tree_view = GTK_TREE_VIEW (widget);
  GtkTreeModel        *model;
  GtkTreeSelection    *selection;
  GtkTreeRowReference *reference = NULL;
  GList               *rows;

  model = gtk_tree_view_get_model (tree_view);
  selection = gtk_tree_view_get_selection (tree_view);

  rows = gtk_tree_selection_get_selected_rows (selection, &model);
  if (g_list_length (rows) == 1)
    {
      reference = gtk_tree_row_reference_new (model, rows->data);
    }
  g_list_foreach (rows, (GFunc) gtk_tree_path_free, NULL);
  g_list_free (rows);

  g_signal_handlers_block_by_func (selection,
                                   G_CALLBACK (on_selection_changed),
                                   tree_view);
  GTK_WIDGET_CLASS (animation_layer_view_parent_class)->button_press_event (widget, event);
  g_signal_handlers_unblock_by_func (selection,
                                     G_CALLBACK (on_selection_changed),
                                     tree_view);

  rows = gtk_tree_selection_get_selected_rows (selection, &model);
  if (g_list_length (rows) == 1 && reference != NULL)
    {
      GtkTreePath *prev_path = gtk_tree_row_reference_get_path (reference);
      GtkTreePath *new_path = rows->data;

      /* We keep globally the same behavior as default tree view, except that
       * when there is only 1 item selected and you click it, you unselect it.
       * You also unselect it by clicking outside any item.
       */
      if (gtk_tree_path_compare (new_path, prev_path) == 0)
        {
          gtk_tree_selection_unselect_path (selection, new_path);
        }
      else
        {
          /* Since we blocked the signal callback, call it ourselves. */
          on_selection_changed (selection, ANIMATION_LAYER_VIEW (widget));
        }
      gtk_tree_path_free (prev_path);
      gtk_tree_row_reference_free (reference);
    }
  else
    {
      /* Since we blocked the signal callback, call it ourselves. */
      on_selection_changed (selection, ANIMATION_LAYER_VIEW (widget));
    }
  g_list_foreach (rows, (GFunc) gtk_tree_path_free, NULL);
  g_list_free (rows);

  return TRUE;
}

/* animation_layer_view_fill:
 * @view: the #AnimationLayerView.
 * @store: the #GtkTreeStore to fill.
 * @parent_layer: the parent #GimpLayer id. Set 0 for first call.
 * @parent: %NULL to search from the first call (used for recursivity).
 *
 * Recursively fills @store with the #GimpLayers data of the #GimpImage
 * tied to @view.
 */
static void
animation_layer_view_fill (AnimationLayerView *view,
                           GtkTreeStore       *store,
                           gint                parent_layer,
                           GtkTreeIter        *parent)
{
  gint        *layers;
  gint         num_layers;
  GtkTreeIter  iter;
  gint         i;

  if (parent_layer == 0)
    {
      layers = gimp_image_get_layers (view->priv->image_id,
                                      &num_layers);
    }
  else
    {
      layers = gimp_item_get_children (parent_layer, &num_layers);
    }

  for (i = 0; i < num_layers; i++)
    {
      gtk_tree_store_insert (store, &iter, parent, i);
      gtk_tree_store_set (store, &iter,
                          COLUMN_LAYER_TATTOO, gimp_item_get_tattoo (layers[i]),
                          COLUMN_LAYER_NAME, gimp_item_get_name (layers[i]),
                          -1);
      if (gimp_item_is_group (layers[i]))
        {
          animation_layer_view_fill (view, store, layers[i], &iter);
        }
    }
  g_free (layers);
}

/* animation_layer_view_get_row:
 * @view: the #AnimationLayerView.
 * @tattoo: the #GimpLayer tattoo.
 * @parent: %NULL to search from the first call (used for recursivity).
 *
 * Returns: the #GtkTreePath for the row of @tattoo, NULL if not found
 * in @view.
 * The returned path should be freed with gtk_tree_path_free()
 */
static GtkTreePath *
animation_layer_view_get_row (AnimationLayerView *view,
                              gint                tattoo,
                              GtkTreeIter        *parent)
{
  GtkTreeModel *model;
  GtkTreeIter   iter;

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (view));
  if (! gtk_tree_model_iter_children (model, &iter, parent))
    return NULL;

  do
    {
      GtkTreePath *path = NULL;
      GValue       value = { 0, };

      gtk_tree_model_get_value (model, &iter,
                                COLUMN_LAYER_TATTOO,
                                &value);
      if (g_value_get_int (&value) == tattoo)
        path = gtk_tree_model_get_path (model, &iter);

      g_value_unset (&value);

      if (path)
        {
          return path;
        }
      else if (gtk_tree_model_iter_has_child (model, &iter))
        {
          GtkTreePath *found_path;

          found_path = animation_layer_view_get_row (view, tattoo, &iter);

          if (found_path)
            return found_path;
        }
    }
  while (gtk_tree_model_iter_next (model, &iter));

  return NULL;
}

static void
on_selection_changed (GtkTreeSelection   *selection,
                      AnimationLayerView *view)
{
  GtkTreeView         *tree_view = GTK_TREE_VIEW (view);
  GList               *layers = NULL;
  GtkTreeModel        *model;
  GList               *rows;
  GList               *row;

  model = gtk_tree_view_get_model (tree_view);
  rows = gtk_tree_selection_get_selected_rows (selection, &model);

  for (row = rows; row; row = row->next)
    {
      GtkTreePath *path = row->data;
      GtkTreeIter  iter;

      if (gtk_tree_model_get_iter (model, &iter, path))
        {
          GValue value = { 0, };

          gtk_tree_model_get_value (model, &iter,
                                    COLUMN_LAYER_TATTOO,
                                    &value);
          layers = g_list_prepend (layers,
                                   GINT_TO_POINTER (g_value_get_int (&value)));
          g_value_unset (&value);

          gtk_tree_model_get_value (model, &iter,
                                    COLUMN_LAYER_NAME,
                                    &value);
          g_value_unset (&value);
        }
    }

  g_list_foreach (rows, (GFunc) gtk_tree_path_free, NULL);
  g_list_free (rows);

  g_signal_emit (view, animation_layer_view_signals[LAYER_SELECTION], 0,
                 layers);
  g_list_free (layers);
}
