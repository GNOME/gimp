/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpitemcombobox.c
 * Copyright (C) 2004 Sven Neumann <sven@gimp.org>
 * Copyright (C) 2006 Simon Budig <simon@gimp.org>
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

#include "libgimpwidgets/gimpwidgets.h"

#include "gimp.h"

#include "gimpuitypes.h"
#include "gimpitemcombobox.h"
#include "gimppixbuf.h"


/**
 * SECTION: gimpitemcombobox
 * @title: GimpItemComboBox
 * @short_description: Widgets providing popup menus of items.
 *
 * Widgets providing popup menus of items (layers, channels,
 * drawables, vectors).
 **/


#define THUMBNAIL_SIZE  24
#define WIDTH_REQUEST  200


#define GET_PRIVATE(obj) (g_object_get_data (G_OBJECT (obj), "gimp-item-combo-box-private"))


typedef struct _GimpItemComboBoxPrivate   GimpItemComboBoxPrivate;

struct _GimpItemComboBoxPrivate
{
  GimpItemConstraintFunc  constraint;
  gpointer                data;
};

typedef struct _GimpDrawableComboBoxClass GimpDrawableComboBoxClass;
typedef struct _GimpChannelComboBoxClass  GimpChannelComboBoxClass;
typedef struct _GimpLayerComboBoxClass    GimpLayerComboBoxClass;
typedef struct _GimpVectorsComboBoxClass  GimpVectorsComboBoxClass;

struct _GimpDrawableComboBox
{
  GimpIntComboBox  parent_instance;
};

struct _GimpDrawableComboBoxClass
{
  GimpIntComboBoxClass  parent_class;
};

struct _GimpChannelComboBox
{
  GimpIntComboBox  parent_instance;
};

struct _GimpChannelComboBoxClass
{
  GimpIntComboBoxClass  parent_class;
};

struct _GimpLayerComboBox
{
  GimpIntComboBox  parent_instance;
};

struct _GimpLayerComboBoxClass
{
  GimpIntComboBoxClass  parent_class;
};

struct _GimpVectorsComboBox
{
  GimpIntComboBox  parent_instance;
};

struct _GimpVectorsComboBoxClass
{
  GimpIntComboBoxClass  parent_class;
};


static GtkWidget * gimp_item_combo_box_new (GType                       type,
                                            GimpItemConstraintFunc      constraint,
                                            gpointer                    data);

static void  gimp_item_combo_box_populate  (GimpIntComboBox            *combo_box);
static void  gimp_item_combo_box_model_add (GimpIntComboBox            *combo_box,
                                            GtkListStore               *store,
                                            gint32                      image,
                                            gint                        num_items,
                                            gint32                     *items,
                                            gint                        tree_level);

static void  gimp_item_combo_box_drag_data_received (GtkWidget         *widget,
                                                     GdkDragContext    *context,
                                                     gint               x,
                                                     gint               y,
                                                     GtkSelectionData  *selection,
                                                     guint              info,
                                                     guint              time);

static void  gimp_item_combo_box_changed   (GimpIntComboBox *combo_box);


static const GtkTargetEntry targets[] =
{
  { "application/x-gimp-channel-id", 0 },
  { "application/x-gimp-layer-id",   0 },
  { "application/x-gimp-vectors-id", 0 }
};


G_DEFINE_TYPE (GimpDrawableComboBox, gimp_drawable_combo_box,
               GIMP_TYPE_INT_COMBO_BOX)

static void
gimp_drawable_combo_box_class_init (GimpDrawableComboBoxClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  widget_class->drag_data_received = gimp_item_combo_box_drag_data_received;
}

static void
gimp_drawable_combo_box_init (GimpDrawableComboBox *combo_box)
{
  gtk_drag_dest_set (GTK_WIDGET (combo_box),
                     GTK_DEST_DEFAULT_HIGHLIGHT |
                     GTK_DEST_DEFAULT_MOTION |
                     GTK_DEST_DEFAULT_DROP,
                     targets, 2,
                     GDK_ACTION_COPY);

  g_object_set_data_full (G_OBJECT (combo_box), "gimp-item-combo-box-private",
                          g_new0 (GimpItemComboBoxPrivate, 1),
                          (GDestroyNotify) g_free);
}

/**
 * gimp_drawable_combo_box_new:
 * @constraint: a #GimpDrawableConstraintFunc or %NULL
 * @data:       a pointer that is passed to @constraint
 *
 * Creates a new #GimpIntComboBox filled with all currently opened
 * drawables. If a @constraint function is specified, it is called for
 * each drawable and only if the function returns %TRUE, the drawable
 * is added to the combobox.
 *
 * You should use gimp_int_combo_box_connect() to initialize and connect
 * the combo.  Use gimp_int_combo_box_set_active() to get the active
 * drawable ID and gimp_int_combo_box_get_active() to retrieve the ID
 * of the selected drawable.
 *
 * Return value: a new #GimpIntComboBox.
 *
 * Since: 2.2
 **/
GtkWidget *
gimp_drawable_combo_box_new (GimpDrawableConstraintFunc constraint,
                             gpointer                   data)
{
  return gimp_item_combo_box_new (GIMP_TYPE_DRAWABLE_COMBO_BOX,
                                  constraint, data);
}


G_DEFINE_TYPE (GimpChannelComboBox, gimp_channel_combo_box,
               GIMP_TYPE_INT_COMBO_BOX)

static void
gimp_channel_combo_box_class_init (GimpChannelComboBoxClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  widget_class->drag_data_received = gimp_item_combo_box_drag_data_received;
}

static void
gimp_channel_combo_box_init (GimpChannelComboBox *combo_box)
{
  gtk_drag_dest_set (GTK_WIDGET (combo_box),
                     GTK_DEST_DEFAULT_HIGHLIGHT |
                     GTK_DEST_DEFAULT_MOTION |
                     GTK_DEST_DEFAULT_DROP,
                     targets, 1,
                     GDK_ACTION_COPY);

  g_object_set_data_full (G_OBJECT (combo_box), "gimp-item-combo-box-private",
                          g_new0 (GimpItemComboBoxPrivate, 1),
                          (GDestroyNotify) g_free);
}

/**
 * gimp_channel_combo_box_new:
 * @constraint: a #GimpDrawableConstraintFunc or %NULL
 * @data:       a pointer that is passed to @constraint
 *
 * Creates a new #GimpIntComboBox filled with all currently opened
 * channels. See gimp_drawable_combo_box_new() for more information.
 *
 * Return value: a new #GimpIntComboBox.
 *
 * Since: 2.2
 **/
GtkWidget *
gimp_channel_combo_box_new (GimpDrawableConstraintFunc constraint,
                            gpointer                   data)
{
  return gimp_item_combo_box_new (GIMP_TYPE_CHANNEL_COMBO_BOX,
                                  constraint, data);
}


G_DEFINE_TYPE (GimpLayerComboBox, gimp_layer_combo_box,
               GIMP_TYPE_INT_COMBO_BOX)

static void
gimp_layer_combo_box_class_init (GimpLayerComboBoxClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  widget_class->drag_data_received = gimp_item_combo_box_drag_data_received;
}

static void
gimp_layer_combo_box_init (GimpLayerComboBox *combo_box)
{
  gtk_drag_dest_set (GTK_WIDGET (combo_box),
                     GTK_DEST_DEFAULT_HIGHLIGHT |
                     GTK_DEST_DEFAULT_MOTION |
                     GTK_DEST_DEFAULT_DROP,
                     targets + 1, 1,
                     GDK_ACTION_COPY);

  g_object_set_data_full (G_OBJECT (combo_box), "gimp-item-combo-box-private",
                          g_new0 (GimpItemComboBoxPrivate, 1),
                          (GDestroyNotify) g_free);
}

/**
 * gimp_layer_combo_box_new:
 * @constraint: a #GimpDrawableConstraintFunc or %NULL
 * @data:       a pointer that is passed to @constraint
 *
 * Creates a new #GimpIntComboBox filled with all currently opened
 * layers. See gimp_drawable_combo_box_new() for more information.
 *
 * Return value: a new #GimpIntComboBox.
 *
 * Since: 2.2
 **/
GtkWidget *
gimp_layer_combo_box_new (GimpDrawableConstraintFunc constraint,
                          gpointer                   data)
{
  return gimp_item_combo_box_new (GIMP_TYPE_LAYER_COMBO_BOX,
                                  constraint, data);
}


G_DEFINE_TYPE (GimpVectorsComboBox, gimp_vectors_combo_box,
               GIMP_TYPE_INT_COMBO_BOX)

static void
gimp_vectors_combo_box_class_init (GimpVectorsComboBoxClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  widget_class->drag_data_received = gimp_item_combo_box_drag_data_received;
}

static void
gimp_vectors_combo_box_init (GimpVectorsComboBox *combo_box)
{
  gtk_drag_dest_set (GTK_WIDGET (combo_box),
                     GTK_DEST_DEFAULT_HIGHLIGHT |
                     GTK_DEST_DEFAULT_MOTION |
                     GTK_DEST_DEFAULT_DROP,
                     targets + 2, 1,
                     GDK_ACTION_COPY);

  g_object_set_data_full (G_OBJECT (combo_box), "gimp-item-combo-box-private",
                          g_new0 (GimpItemComboBoxPrivate, 1),
                          (GDestroyNotify) g_free);
}


/**
 * gimp_vectors_combo_box_new:
 * @constraint: a #GimpVectorsConstraintFunc or %NULL
 * @data:       a pointer that is passed to @constraint
 *
 * Creates a new #GimpIntComboBox filled with all currently opened
 * vectors objects. If a @constraint function is specified, it is called for
 * each vectors object and only if the function returns %TRUE, the vectors
 * object is added to the combobox.
 *
 * You should use gimp_int_combo_box_connect() to initialize and connect
 * the combo.  Use gimp_int_combo_box_set_active() to set the active
 * vectors ID and gimp_int_combo_box_get_active() to retrieve the ID
 * of the selected vectors object.
 *
 * Return value: a new #GimpIntComboBox.
 *
 * Since: 2.4
 **/
GtkWidget *
gimp_vectors_combo_box_new (GimpVectorsConstraintFunc constraint,
                            gpointer                  data)
{
  return gimp_item_combo_box_new (GIMP_TYPE_VECTORS_COMBO_BOX,
                                  constraint, data);
}


static GtkWidget *
gimp_item_combo_box_new (GType                  type,
                         GimpItemConstraintFunc constraint,
                         gpointer               data)
{
  GimpIntComboBox         *combo_box;
  GimpItemComboBoxPrivate *private;

  combo_box = g_object_new (type,
                            "width-request", WIDTH_REQUEST,
                            "ellipsize",     PANGO_ELLIPSIZE_MIDDLE,
                            NULL);

  private = GET_PRIVATE (combo_box);

  private->constraint = constraint;
  private->data       = data;

  gimp_item_combo_box_populate (combo_box);

  g_signal_connect (combo_box, "changed",
                    G_CALLBACK (gimp_item_combo_box_changed),
                    NULL);

  return GTK_WIDGET (combo_box);
}

static void
gimp_item_combo_box_populate (GimpIntComboBox *combo_box)
{
  GtkTreeModel *model;
  GtkTreeIter   iter;
  gint32       *images;
  gint          num_images;
  gint          i;

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo_box));

  images = gimp_image_list (&num_images);

  for (i = 0; i < num_images; i++)
    {
      gint32 *items;
      gint    num_items;

      if (GIMP_IS_DRAWABLE_COMBO_BOX (combo_box) ||
          GIMP_IS_LAYER_COMBO_BOX (combo_box))
        {
          items = gimp_image_get_layers (images[i], &num_items);
          gimp_item_combo_box_model_add (combo_box, GTK_LIST_STORE (model),
                                         images[i],
                                         num_items, items, 0);
          g_free (items);
        }

      if (GIMP_IS_DRAWABLE_COMBO_BOX (combo_box) ||
          GIMP_IS_CHANNEL_COMBO_BOX (combo_box))
        {
          items = gimp_image_get_channels (images[i], &num_items);
          gimp_item_combo_box_model_add (combo_box, GTK_LIST_STORE (model),
                                         images[i],
                                         num_items, items, 0);
          g_free (items);
        }

      if (GIMP_IS_VECTORS_COMBO_BOX (combo_box))
        {
          items = gimp_image_get_vectors (images[i], &num_items);
          gimp_item_combo_box_model_add (combo_box, GTK_LIST_STORE (model),
                                         images[i],
                                         num_items, items, 0);
          g_free (items);
        }
    }

  g_free (images);

  if (gtk_tree_model_get_iter_first (model, &iter))
    gtk_combo_box_set_active_iter (GTK_COMBO_BOX (combo_box), &iter);
}

static void
gimp_item_combo_box_model_add (GimpIntComboBox *combo_box,
                               GtkListStore    *store,
                               gint32           image,
                               gint             num_items,
                               gint32          *items,
                               gint             tree_level)
{
  GimpItemComboBoxPrivate *private = GET_PRIVATE (combo_box);
  GtkTreeIter              iter;
  gint                     i;
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

  for (i = 0; i < num_items; i++)
    {
      if (! private->constraint ||
          (* private->constraint) (image, items[i], private->data))
        {
          gchar     *image_name = gimp_image_get_name (image);
          gchar     *item_name  = gimp_item_get_name (items[i]);
          gchar     *label;
          GdkPixbuf *thumb;

          label = g_strdup_printf ("%s%s-%d / %s-%d",
                                   indent, image_name, image,
                                   item_name, items[i]);

          g_free (item_name);
          g_free (image_name);

          if (GIMP_IS_VECTORS_COMBO_BOX (combo_box))
            thumb = NULL;
          else
            thumb = gimp_drawable_get_thumbnail (items[i],
                                                 THUMBNAIL_SIZE, THUMBNAIL_SIZE,
                                                 GIMP_PIXBUF_SMALL_CHECKS);

          gtk_list_store_append (store, &iter);
          gtk_list_store_set (store, &iter,
                              GIMP_INT_STORE_VALUE,  items[i],
                              GIMP_INT_STORE_LABEL,  label,
                              GIMP_INT_STORE_PIXBUF, thumb,
                              -1);

          if (thumb)
            g_object_unref (thumb);

          g_free (label);
        }

      if (gimp_item_is_group (items[i]))
        {
          gint32 *children;
          gint    n_children;

          children = gimp_item_get_children (items[i], &n_children);
          gimp_item_combo_box_model_add (combo_box, store,
                                         image,
                                         n_children, children,
                                         tree_level + 1);
          g_free (children);
        }
    }

  g_free (indent);
}

static void
gimp_item_combo_box_drag_data_received (GtkWidget        *widget,
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
          pid == gimp_getpid ())
        {
          gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (widget), ID);
        }
    }

  g_free (str);
}

static gboolean
gimp_item_combo_box_remove_items (GtkTreeModel *model,
                                  GtkTreePath  *path,
                                  GtkTreeIter  *iter,
                                  gpointer      data)
{
  gint    item_ID;
  GList **remove = data;

  gtk_tree_model_get (model, iter,
                      GIMP_INT_STORE_VALUE, &item_ID,
                      -1);

  if (item_ID > 0)
    *remove = g_list_prepend (*remove, g_memdup (iter, sizeof (GtkTreeIter)));

  return FALSE;
}

static void
gimp_item_combo_box_changed (GimpIntComboBox *combo_box)
{
  gint item_ID;

  if (gimp_int_combo_box_get_active (combo_box, &item_ID))
    {
      if (item_ID > 0 && ! gimp_item_is_valid (item_ID))
        {
          GtkTreeModel *model;
          GList        *remove = NULL;
          GList        *list;

          model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo_box));

          g_signal_stop_emission_by_name (combo_box, "changed");

          gtk_tree_model_foreach (model,
                                  gimp_item_combo_box_remove_items,
                                  &remove);

          for (list = remove; list; list = g_list_next (list))
            gtk_list_store_remove (GTK_LIST_STORE (model), list->data);

          g_list_free_full (remove, (GDestroyNotify) g_free);

          gimp_item_combo_box_populate (combo_box);
        }
    }
}
