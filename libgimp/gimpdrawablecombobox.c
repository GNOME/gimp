/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpdrawablecombobox.c
 * Copyright (C) 2004 Sven Neumann <sven@gimp.org>
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

#include "libgimpwidgets/gimpwidgets.h"

#include "gimp.h"

#include "gimpdrawablecombobox.h"
#include "gimppixbuf.h"


#define MENU_THUMBNAIL_SIZE  24


static gint  gimp_drawable_combo_box_model_add (GtkListStore               *store,
                                                gint32                      image,
                                                gint                        num_drawables,
                                                gint32                     *drawables,
                                                GimpDrawableConstraintFunc  constraint,
                                                gpointer                    data);


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
 * Since: GIMP 2.2
 **/
GtkWidget *
gimp_drawable_combo_box_new (GimpDrawableConstraintFunc constraint,
                             gpointer                   data)
{
  GtkWidget    *combo_box;
  GtkTreeModel *model;
  GtkTreeIter   iter;
  gint32       *images;
  gint          num_images;
  gint          i;

  combo_box = g_object_new (GIMP_TYPE_INT_COMBO_BOX, NULL);

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo_box));

  images = gimp_image_list (&num_images);

  for (i = 0; i < num_images; i++)
    {
      gint32 *drawables;
      gint    num_drawables;

      drawables = gimp_image_get_layers (images[i], &num_drawables);
      gimp_drawable_combo_box_model_add (GTK_LIST_STORE (model),
                                         images[i],
                                         num_drawables, drawables,
                                         constraint, data);
      g_free (drawables);

      drawables = gimp_image_get_channels (images[i], &num_drawables);
      gimp_drawable_combo_box_model_add (GTK_LIST_STORE (model),
                                         images[i],
                                         num_drawables, drawables,
                                         constraint, data);
      g_free (drawables);
    }

  g_free (images);

  if (gtk_tree_model_get_iter_first (model, &iter))
    gtk_combo_box_set_active_iter (GTK_COMBO_BOX (combo_box), &iter);

  return combo_box;
}

/**
 * gimp_channel_combo_box_new:
 * @constraint: a #GimpDrawableConstraintFunc or %NULL
 * @data:       a pointer that is passed to @constraint
 *
 * Creates a new #GimpIntComboBox filled with all currently opened
 * channels. See gimp_drawable_combo_box() for more info.
 *
 * Return value: a new #GimpIntComboBox.
 *
 * Since: GIMP 2.2
 **/
GtkWidget *
gimp_channel_combo_box_new (GimpDrawableConstraintFunc constraint,
                            gpointer                   data)
{
  GtkWidget    *combo_box;
  GtkTreeModel *model;
  GtkTreeIter   iter;
  gint32       *images;
  gint          num_images;
  gint          i;

  combo_box = g_object_new (GIMP_TYPE_INT_COMBO_BOX, NULL);

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo_box));

  images = gimp_image_list (&num_images);

  for (i = 0; i < num_images; i++)
    {
      gint32 *drawables;
      gint    num_drawables;

      drawables = gimp_image_get_channels (images[i], &num_drawables);
      gimp_drawable_combo_box_model_add (GTK_LIST_STORE (model),
                                         images[i],
                                         num_drawables, drawables,
                                         constraint, data);
      g_free (drawables);
    }

  g_free (images);

  if (gtk_tree_model_get_iter_first (model, &iter))
    gtk_combo_box_set_active_iter (GTK_COMBO_BOX (combo_box), &iter);

  return combo_box;
}

/**
 * gimp_layer_combo_box_new:
 * @constraint: a #GimpDrawableConstraintFunc or %NULL
 * @data:       a pointer that is passed to @constraint
 *
 * Creates a new #GimpIntComboBox filled with all currently opened
 * layers. See gimp_drawable_combo_box() for more info.
 *
 * Return value: a new #GimpIntComboBox.
 *
 * Since: GIMP 2.2
 **/
GtkWidget *
gimp_layer_combo_box_new (GimpDrawableConstraintFunc constraint,
                          gpointer                   data)
{
  GtkWidget    *combo_box;
  GtkTreeModel *model;
  GtkTreeIter   iter;
  gint32       *images;
  gint          num_images;
  gint          i;

  combo_box = g_object_new (GIMP_TYPE_INT_COMBO_BOX, NULL);

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo_box));

  images = gimp_image_list (&num_images);

  for (i = 0; i < num_images; i++)
    {
      gint32 *drawables;
      gint    num_drawables;

      drawables = gimp_image_get_layers (images[i], &num_drawables);
      gimp_drawable_combo_box_model_add (GTK_LIST_STORE (model),
                                         images[i],
                                         num_drawables, drawables,
                                         constraint, data);
      g_free (drawables);
    }

  g_free (images);

  if (gtk_tree_model_get_iter_first (model, &iter))
    gtk_combo_box_set_active_iter (GTK_COMBO_BOX (combo_box), &iter);

  return combo_box;
}

static gint
gimp_drawable_combo_box_model_add (GtkListStore               *store,
                                   gint32                      image,
                                   gint                        num_drawables,
                                   gint32                     *drawables,
                                   GimpDrawableConstraintFunc  constraint,
                                   gpointer                    data)
{
  GtkTreeIter  iter;
  gint         added = 0;
  gint         i;

  for (i = 0; i < num_drawables; i++)
    {
      if (! constraint || (* constraint) (image, drawables[i], data))
        {
          gchar     *image_name    = gimp_image_get_name (image);
          gchar     *drawable_name = gimp_drawable_get_name (drawables[i]);
          gchar     *label;
          GdkPixbuf *thumb;

          label = g_strdup_printf ("%s-%d/%s-%d",
                                   image_name,    image,
                                   drawable_name, drawables[i]);

          g_free (drawable_name);
          g_free (image_name);

          thumb = gimp_drawable_get_thumbnail (drawables[i],
                                               MENU_THUMBNAIL_SIZE,
                                               MENU_THUMBNAIL_SIZE,
                                               GIMP_PIXBUF_SMALL_CHECKS);

          gtk_list_store_append (store, &iter);
          gtk_list_store_set (store, &iter,
                              GIMP_INT_STORE_VALUE,  drawables[i],
                              GIMP_INT_STORE_LABEL,  label,
                              GIMP_INT_STORE_PIXBUF, thumb,
                              -1);
          added++;

          if (thumb)
            g_object_unref (thumb);

          g_free (label);
        }
    }

  return added;
}
