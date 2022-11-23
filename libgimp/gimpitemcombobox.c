/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmaitemcombobox.c
 * Copyright (C) 2004 Sven Neumann <sven@ligma.org>
 * Copyright (C) 2006 Simon Budig <simon@ligma.org>
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

#include <stdlib.h>
#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmawidgets/ligmawidgets.h"

#include "ligma.h"

#include "ligmauitypes.h"
#include "ligmaitemcombobox.h"
#include "ligmapixbuf.h"


/**
 * SECTION: ligmaitemcombobox
 * @title: LigmaItemComboBox
 * @short_description: Widgets providing popup menus of items.
 *
 * Widgets providing popup menus of items (layers, channels,
 * drawables, vectors).
 **/


#define THUMBNAIL_SIZE  24
#define WIDTH_REQUEST  200


#define GET_PRIVATE(obj) (g_object_get_data (G_OBJECT (obj), "ligma-item-combo-box-private"))


typedef struct _LigmaItemComboBoxPrivate   LigmaItemComboBoxPrivate;

struct _LigmaItemComboBoxPrivate
{
  LigmaItemConstraintFunc  constraint;
  gpointer                data;
};

typedef struct _LigmaDrawableComboBoxClass LigmaDrawableComboBoxClass;
typedef struct _LigmaChannelComboBoxClass  LigmaChannelComboBoxClass;
typedef struct _LigmaLayerComboBoxClass    LigmaLayerComboBoxClass;
typedef struct _LigmaVectorsComboBoxClass  LigmaVectorsComboBoxClass;

struct _LigmaDrawableComboBox
{
  LigmaIntComboBox  parent_instance;
};

struct _LigmaDrawableComboBoxClass
{
  LigmaIntComboBoxClass  parent_class;
};

struct _LigmaChannelComboBox
{
  LigmaIntComboBox  parent_instance;
};

struct _LigmaChannelComboBoxClass
{
  LigmaIntComboBoxClass  parent_class;
};

struct _LigmaLayerComboBox
{
  LigmaIntComboBox  parent_instance;
};

struct _LigmaLayerComboBoxClass
{
  LigmaIntComboBoxClass  parent_class;
};

struct _LigmaVectorsComboBox
{
  LigmaIntComboBox  parent_instance;
};

struct _LigmaVectorsComboBoxClass
{
  LigmaIntComboBoxClass  parent_class;
};


static GtkWidget * ligma_item_combo_box_new (GType                       type,
                                            LigmaItemConstraintFunc      constraint,
                                            gpointer                    data,
                                            GDestroyNotify              data_destroy);

static void  ligma_item_combo_box_populate  (LigmaIntComboBox            *combo_box);
static void  ligma_item_combo_box_model_add (LigmaIntComboBox            *combo_box,
                                            GtkListStore               *store,
                                            LigmaImage                  *image,
                                            GList                      *items,
                                            gint                        tree_level);

static void  ligma_item_combo_box_drag_data_received (GtkWidget         *widget,
                                                     GdkDragContext    *context,
                                                     gint               x,
                                                     gint               y,
                                                     GtkSelectionData  *selection,
                                                     guint              info,
                                                     guint              time);

static void  ligma_item_combo_box_changed   (LigmaIntComboBox *combo_box);


static const GtkTargetEntry targets[] =
{
  { "application/x-ligma-channel-id", 0 },
  { "application/x-ligma-layer-id",   0 },
  { "application/x-ligma-vectors-id", 0 }
};


G_DEFINE_TYPE (LigmaDrawableComboBox, ligma_drawable_combo_box,
               LIGMA_TYPE_INT_COMBO_BOX)

static void
ligma_drawable_combo_box_class_init (LigmaDrawableComboBoxClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  widget_class->drag_data_received = ligma_item_combo_box_drag_data_received;
}

static void
ligma_drawable_combo_box_init (LigmaDrawableComboBox *combo_box)
{
  gtk_drag_dest_set (GTK_WIDGET (combo_box),
                     GTK_DEST_DEFAULT_HIGHLIGHT |
                     GTK_DEST_DEFAULT_MOTION |
                     GTK_DEST_DEFAULT_DROP,
                     targets, 2,
                     GDK_ACTION_COPY);

  g_object_set_data_full (G_OBJECT (combo_box), "ligma-item-combo-box-private",
                          g_new0 (LigmaItemComboBoxPrivate, 1),
                          (GDestroyNotify) g_free);
}

/**
 * ligma_drawable_combo_box_new:
 * @constraint: (nullable):       A #LigmaItemConstraintFunc or %NULL
 * @data: (closure constraint):   A pointer that is passed to @constraint
 * @data_destroy: (destroy data): Destroy function for @data
 *
 * Creates a new #LigmaIntComboBox filled with all currently opened
 * drawables. If a @constraint function is specified, it is called for
 * each drawable and only if the function returns %TRUE, the drawable
 * is added to the combobox.
 *
 * You should use ligma_int_combo_box_connect() to initialize and connect
 * the combo.  Use ligma_int_combo_box_set_active() to get the active
 * drawable ID and ligma_int_combo_box_get_active() to retrieve the ID
 * of the selected drawable.
 *
 * Returns: a new #LigmaIntComboBox.
 *
 * Since: 2.2
 **/
GtkWidget *
ligma_drawable_combo_box_new (LigmaItemConstraintFunc constraint,
                             gpointer               data,
                             GDestroyNotify         data_destroy)
{
  return ligma_item_combo_box_new (LIGMA_TYPE_DRAWABLE_COMBO_BOX,
                                  constraint, data, data_destroy);
}


G_DEFINE_TYPE (LigmaChannelComboBox, ligma_channel_combo_box,
               LIGMA_TYPE_INT_COMBO_BOX)

static void
ligma_channel_combo_box_class_init (LigmaChannelComboBoxClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  widget_class->drag_data_received = ligma_item_combo_box_drag_data_received;
}

static void
ligma_channel_combo_box_init (LigmaChannelComboBox *combo_box)
{
  gtk_drag_dest_set (GTK_WIDGET (combo_box),
                     GTK_DEST_DEFAULT_HIGHLIGHT |
                     GTK_DEST_DEFAULT_MOTION |
                     GTK_DEST_DEFAULT_DROP,
                     targets, 1,
                     GDK_ACTION_COPY);

  g_object_set_data_full (G_OBJECT (combo_box), "ligma-item-combo-box-private",
                          g_new0 (LigmaItemComboBoxPrivate, 1),
                          (GDestroyNotify) g_free);
}

/**
 * ligma_channel_combo_box_new:
 * @constraint: (nullable):       A #LigmaItemConstraintFunc or %NULL
 * @data: (closure constraint):   A pointer that is passed to @constraint
 * @data_destroy: (destroy data): Destroy function for @data
 *
 * Creates a new #LigmaIntComboBox filled with all currently opened
 * channels. See ligma_drawable_combo_box_new() for more information.
 *
 * Returns: a new #LigmaIntComboBox.
 *
 * Since: 2.2
 **/
GtkWidget *
ligma_channel_combo_box_new (LigmaItemConstraintFunc constraint,
                            gpointer               data,
                            GDestroyNotify         data_destroy)
{
  return ligma_item_combo_box_new (LIGMA_TYPE_CHANNEL_COMBO_BOX,
                                  constraint, data, data_destroy);
}


G_DEFINE_TYPE (LigmaLayerComboBox, ligma_layer_combo_box,
               LIGMA_TYPE_INT_COMBO_BOX)

static void
ligma_layer_combo_box_class_init (LigmaLayerComboBoxClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  widget_class->drag_data_received = ligma_item_combo_box_drag_data_received;
}

static void
ligma_layer_combo_box_init (LigmaLayerComboBox *combo_box)
{
  gtk_drag_dest_set (GTK_WIDGET (combo_box),
                     GTK_DEST_DEFAULT_HIGHLIGHT |
                     GTK_DEST_DEFAULT_MOTION |
                     GTK_DEST_DEFAULT_DROP,
                     targets + 1, 1,
                     GDK_ACTION_COPY);

  g_object_set_data_full (G_OBJECT (combo_box), "ligma-item-combo-box-private",
                          g_new0 (LigmaItemComboBoxPrivate, 1),
                          (GDestroyNotify) g_free);
}

/**
 * ligma_layer_combo_box_new:
 * @constraint: (nullable):       A #LigmaItemConstraintFunc or %NULL
 * @data: (closure constraint):   A pointer that is passed to @constraint
 * @data_destroy: (destroy data): Destroy function for @data
 *
 * Creates a new #LigmaIntComboBox filled with all currently opened
 * layers. See ligma_drawable_combo_box_new() for more information.
 *
 * Returns: a new #LigmaIntComboBox.
 *
 * Since: 2.2
 **/
GtkWidget *
ligma_layer_combo_box_new (LigmaItemConstraintFunc constraint,
                          gpointer               data,
                          GDestroyNotify         data_destroy)
{
  return ligma_item_combo_box_new (LIGMA_TYPE_LAYER_COMBO_BOX,
                                  constraint, data, data_destroy);
}


G_DEFINE_TYPE (LigmaVectorsComboBox, ligma_vectors_combo_box,
               LIGMA_TYPE_INT_COMBO_BOX)

static void
ligma_vectors_combo_box_class_init (LigmaVectorsComboBoxClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  widget_class->drag_data_received = ligma_item_combo_box_drag_data_received;
}

static void
ligma_vectors_combo_box_init (LigmaVectorsComboBox *combo_box)
{
  gtk_drag_dest_set (GTK_WIDGET (combo_box),
                     GTK_DEST_DEFAULT_HIGHLIGHT |
                     GTK_DEST_DEFAULT_MOTION |
                     GTK_DEST_DEFAULT_DROP,
                     targets + 2, 1,
                     GDK_ACTION_COPY);

  g_object_set_data_full (G_OBJECT (combo_box), "ligma-item-combo-box-private",
                          g_new0 (LigmaItemComboBoxPrivate, 1),
                          (GDestroyNotify) g_free);
}


/**
 * ligma_vectors_combo_box_new:
 * @constraint: (nullable):       A #LigmaItemConstraintFunc or %NULL
 * @data: (closure constraint):   A pointer that is passed to @constraint
 * @data_destroy: (destroy data): Destroy function for @data
 *
 * Creates a new #LigmaIntComboBox filled with all currently opened
 * vectors objects. If a @constraint function is specified, it is called for
 * each vectors object and only if the function returns %TRUE, the vectors
 * object is added to the combobox.
 *
 * You should use ligma_int_combo_box_connect() to initialize and connect
 * the combo.  Use ligma_int_combo_box_set_active() to set the active
 * vectors ID and ligma_int_combo_box_get_active() to retrieve the ID
 * of the selected vectors object.
 *
 * Returns: a new #LigmaIntComboBox.
 *
 * Since: 2.4
 **/
GtkWidget *
ligma_vectors_combo_box_new (LigmaItemConstraintFunc constraint,
                            gpointer               data,
                            GDestroyNotify         data_destroy)
{
  return ligma_item_combo_box_new (LIGMA_TYPE_VECTORS_COMBO_BOX,
                                  constraint, data, data_destroy);
}


static GtkWidget *
ligma_item_combo_box_new (GType                  type,
                         LigmaItemConstraintFunc constraint,
                         gpointer               data,
                         GDestroyNotify         data_destroy)
{
  LigmaIntComboBox         *combo_box;
  LigmaItemComboBoxPrivate *private;

  combo_box = g_object_new (type,
                            "width-request", WIDTH_REQUEST,
                            "ellipsize",     PANGO_ELLIPSIZE_MIDDLE,
                            NULL);

  private = GET_PRIVATE (combo_box);

  private->constraint = constraint;
  private->data       = data;

  if (data_destroy)
    g_object_weak_ref (G_OBJECT (combo_box), (GWeakNotify) data_destroy, data);

  ligma_item_combo_box_populate (combo_box);

  g_signal_connect (combo_box, "changed",
                    G_CALLBACK (ligma_item_combo_box_changed),
                    NULL);

  return GTK_WIDGET (combo_box);
}

static void
ligma_item_combo_box_populate (LigmaIntComboBox *combo_box)
{
  GtkTreeModel *model;
  GtkTreeIter   iter;
  GList        *images;
  GList        *list;

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo_box));

  images = ligma_list_images ();

  for (list = images; list; list = g_list_next (list))
    {
      LigmaImage *image = list->data;
      GList     *items;

      if (LIGMA_IS_DRAWABLE_COMBO_BOX (combo_box) ||
          LIGMA_IS_LAYER_COMBO_BOX (combo_box))
        {
          items = ligma_image_list_layers (image);
          ligma_item_combo_box_model_add (combo_box, GTK_LIST_STORE (model),
                                         image, items, 0);
          g_list_free (items);
        }

      if (LIGMA_IS_DRAWABLE_COMBO_BOX (combo_box) ||
          LIGMA_IS_CHANNEL_COMBO_BOX (combo_box))
        {
          items = ligma_image_list_channels (image);
          ligma_item_combo_box_model_add (combo_box, GTK_LIST_STORE (model),
                                         image, items, 0);
          g_list_free (items);
        }

      if (LIGMA_IS_VECTORS_COMBO_BOX (combo_box))
        {
          items = ligma_image_list_vectors (image);
          ligma_item_combo_box_model_add (combo_box, GTK_LIST_STORE (model),
                                         image, items, 0);
          g_list_free (items);
        }
    }

  g_list_free (images);

  if (gtk_tree_model_get_iter_first (model, &iter))
    gtk_combo_box_set_active_iter (GTK_COMBO_BOX (combo_box), &iter);
}

static void
ligma_item_combo_box_model_add (LigmaIntComboBox *combo_box,
                               GtkListStore    *store,
                               LigmaImage       *image,
                               GList           *items,
                               gint             tree_level)
{
  LigmaItemComboBoxPrivate *private = GET_PRIVATE (combo_box);
  GtkTreeIter              iter;
  GList                   *list;
  gchar                   *indent;

  if (tree_level > 0)
    {
      indent = g_new (gchar, tree_level + 2);
      memset (indent, '-', tree_level);
      indent[tree_level] = ' ';
      indent[tree_level + 1] = '\0';
    }
  else
    {
      indent = g_strdup ("");
    }

  for (list = items; list; list = g_list_next (list))
    {
      LigmaItem *item    = list->data;
      gint32    item_id = ligma_item_get_id (item);

      if (! private->constraint ||
          private->constraint (image, item, private->data))
        {
          gchar     *image_name = ligma_image_get_name (image);
          gchar     *item_name  = ligma_item_get_name (item);
          gchar     *label;
          GdkPixbuf *thumb;

          label = g_strdup_printf ("%s%s-%d / %s-%d",
                                   indent, image_name,
                                   ligma_image_get_id (image),
                                   item_name, item_id);

          g_free (item_name);
          g_free (image_name);

          if (LIGMA_IS_VECTORS_COMBO_BOX (combo_box))
            thumb = NULL;
          else
            thumb = ligma_drawable_get_thumbnail (LIGMA_DRAWABLE (item),
                                                 THUMBNAIL_SIZE, THUMBNAIL_SIZE,
                                                 LIGMA_PIXBUF_SMALL_CHECKS);

          gtk_list_store_append (store, &iter);
          gtk_list_store_set (store, &iter,
                              LIGMA_INT_STORE_VALUE,  item_id,
                              LIGMA_INT_STORE_LABEL,  label,
                              LIGMA_INT_STORE_PIXBUF, thumb,
                              -1);

          if (thumb)
            g_object_unref (thumb);

          g_free (label);
        }

      if (ligma_item_is_group (item))
        {
          GList *children;

          children = ligma_item_list_children (item);
          ligma_item_combo_box_model_add (combo_box, store,
                                         image, children,
                                         tree_level + 1);
          g_list_free (children);
        }
    }

  g_free (indent);
}

static void
ligma_item_combo_box_drag_data_received (GtkWidget        *widget,
                                        GdkDragContext   *context,
                                        gint              x,
                                        gint              y,
                                        GtkSelectionData *selection,
                                        guint             info,
                                        guint             time)
{
  gint   length = gtk_selection_data_get_length (selection);
  gchar *str;

  if (gtk_selection_data_get_format (selection) != 8 || length < 1)
    {
      g_warning ("%s: received invalid item ID data", G_STRFUNC);
      return;
    }

  str = g_strndup ((const gchar *) gtk_selection_data_get_data (selection),
                   length);

  if (g_utf8_validate (str, -1, NULL))
    {
      gint pid;
      gint ID;

      if (sscanf (str, "%i:%i", &pid, &ID) == 2 &&
          pid == ligma_getpid ())
        {
          ligma_int_combo_box_set_active (LIGMA_INT_COMBO_BOX (widget), ID);
        }
    }

  g_free (str);
}

static gboolean
ligma_item_combo_box_remove_items (GtkTreeModel *model,
                                  GtkTreePath  *path,
                                  GtkTreeIter  *iter,
                                  gpointer      data)
{
  gint    item_ID;
  GList **remove = data;

  gtk_tree_model_get (model, iter,
                      LIGMA_INT_STORE_VALUE, &item_ID,
                      -1);

  if (item_ID > 0)
    *remove = g_list_prepend (*remove, g_memdup2 (iter, sizeof (GtkTreeIter)));

  return FALSE;
}

static void
ligma_item_combo_box_changed (LigmaIntComboBox *combo_box)
{
  gint item_ID;

  if (ligma_int_combo_box_get_active (combo_box, &item_ID))
    {
      if (item_ID > 0 && ! ligma_item_get_by_id (item_ID))
        {
          GtkTreeModel *model;
          GList        *remove = NULL;
          GList        *list;

          model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo_box));

          g_signal_stop_emission_by_name (combo_box, "changed");

          gtk_tree_model_foreach (model,
                                  ligma_item_combo_box_remove_items,
                                  &remove);

          for (list = remove; list; list = g_list_next (list))
            gtk_list_store_remove (GTK_LIST_STORE (model), list->data);

          g_list_free_full (remove, (GDestroyNotify) g_free);

          ligma_item_combo_box_populate (combo_box);
        }
    }
}
