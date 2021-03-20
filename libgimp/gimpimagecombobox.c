/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpimagecombobox.c
 * Copyright (C) 2004 Sven Neumann <sven@gimp.org>
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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "gimp.h"

#include "gimpuitypes.h"
#include "gimpimagecombobox.h"
#include "gimppixbuf.h"


/**
 * SECTION: gimpimagecombobox
 * @title: GimpImageComboBox
 * @short_description: A widget providing a popup menu of images.
 *
 * A widget providing a popup menu of images.
 **/


#define THUMBNAIL_SIZE   24
#define WIDTH_REQUEST   200


typedef struct _GimpImageComboBoxClass GimpImageComboBoxClass;

struct _GimpImageComboBox
{
  GimpIntComboBox          parent_instance;

  GimpImageConstraintFunc  constraint;
  gpointer                 data;
  GDestroyNotify           data_destroy;
};

struct _GimpImageComboBoxClass
{
  GimpIntComboBoxClass  parent_class;
};


static void  gimp_image_combo_box_finalize  (GObject                 *object);

static void  gimp_image_combo_box_populate  (GimpImageComboBox       *combo_box);
static void  gimp_image_combo_box_model_add (GtkListStore            *store,
                                             GList                   *images,
                                             GimpImageConstraintFunc  constraint,
                                             gpointer                 data);

static void  gimp_image_combo_box_drag_data_received (GtkWidget        *widget,
                                                      GdkDragContext   *context,
                                                      gint              x,
                                                      gint              y,
                                                      GtkSelectionData *selection,
                                                      guint             info,
                                                      guint             time);

static void  gimp_image_combo_box_changed   (GimpImageComboBox *combo_box);


static const GtkTargetEntry target = { "application/x-gimp-image-id", 0 };


G_DEFINE_TYPE (GimpImageComboBox, gimp_image_combo_box,
               GIMP_TYPE_INT_COMBO_BOX)

#define parent_class gimp_image_combo_box_parent_class


static void
gimp_image_combo_box_class_init (GimpImageComboBoxClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize           = gimp_image_combo_box_finalize;

  widget_class->drag_data_received = gimp_image_combo_box_drag_data_received;
}

static void
gimp_image_combo_box_init (GimpImageComboBox *combo_box)
{
  gtk_drag_dest_set (GTK_WIDGET (combo_box),
                     GTK_DEST_DEFAULT_HIGHLIGHT |
                     GTK_DEST_DEFAULT_MOTION |
                     GTK_DEST_DEFAULT_DROP,
                     &target, 1,
                     GDK_ACTION_COPY);
}

static void
gimp_image_combo_box_finalize (GObject *object)
{
  GimpImageComboBox *combo = GIMP_IMAGE_COMBO_BOX (object);

  if (combo->data_destroy)
    combo->data_destroy (combo->data);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

/**
 * gimp_image_combo_box_new:
 * @constraint: (nullable):       A #GimpImageConstraintFunc or %NULL
 * @data: (closure constraint):   A pointer that is passed to @constraint
 * @data_destroy: (destroy data): Destroy function for @data.
 *
 * Creates a new #GimpIntComboBox filled with all currently opened
 * images. If a @constraint function is specified, it is called for
 * each image and only if the function returns %TRUE, the image is
 * added to the combobox.
 *
 * You should use gimp_int_combo_box_connect() to initialize and
 * connect the combo. Use gimp_int_combo_box_set_active() to get the
 * active image ID and gimp_int_combo_box_get_active() to retrieve the
 * ID of the selected image.
 *
 * Returns: a new #GimpIntComboBox.
 *
 * Since: 2.2
 **/
GtkWidget *
gimp_image_combo_box_new (GimpImageConstraintFunc constraint,
                          gpointer                data,
                          GDestroyNotify          data_destroy)
{
  GimpImageComboBox *combo_box;

  combo_box = g_object_new (GIMP_TYPE_IMAGE_COMBO_BOX,
                            "width-request", WIDTH_REQUEST,
                            "ellipsize",     PANGO_ELLIPSIZE_MIDDLE,
                            NULL);

  combo_box->constraint   = constraint;
  combo_box->data         = data;
  combo_box->data_destroy = data_destroy;

  gimp_image_combo_box_populate (combo_box);

  g_signal_connect (combo_box, "changed",
                    G_CALLBACK (gimp_image_combo_box_changed),
                    NULL);

  return GTK_WIDGET (combo_box);
}

static void
gimp_image_combo_box_populate (GimpImageComboBox *combo_box)
{
  GtkTreeModel *model;
  GtkTreeIter   iter;
  GList        *images;

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo_box));

  images = gimp_list_images ();

  gimp_image_combo_box_model_add (GTK_LIST_STORE (model), images,
                                  combo_box->constraint,
                                  combo_box->data);

  g_list_free (images);

  if (gtk_tree_model_get_iter_first (model, &iter))
    gtk_combo_box_set_active_iter (GTK_COMBO_BOX (combo_box), &iter);
}

static void
gimp_image_combo_box_model_add (GtkListStore            *store,
                                GList                   *images,
                                GimpImageConstraintFunc  constraint,
                                gpointer                 data)
{
  GtkTreeIter  iter;
  GList       *list;

  for (list = images; list; list = g_list_next (list))
    {
      GimpImage *image    = list->data;
      gint32     image_id = gimp_image_get_id (image);

      if (! constraint || constraint (image, data))
        {
          gchar     *image_name = gimp_image_get_name (image);
          gchar     *label;
          GdkPixbuf *thumb;

          label = g_strdup_printf ("%s-%d", image_name, image_id);

          g_free (image_name);

          thumb = gimp_image_get_thumbnail (image,
                                            THUMBNAIL_SIZE, THUMBNAIL_SIZE,
                                            GIMP_PIXBUF_SMALL_CHECKS);

          gtk_list_store_append (store, &iter);
          gtk_list_store_set (store, &iter,
                              GIMP_INT_STORE_VALUE,  image_id,
                              GIMP_INT_STORE_LABEL,  label,
                              GIMP_INT_STORE_PIXBUF, thumb,
                              -1);

          if (thumb)
            g_object_unref (thumb);

          g_free (label);
        }
    }
}

static void
gimp_image_combo_box_drag_data_received (GtkWidget        *widget,
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
      g_warning ("%s: received invalid image ID data", G_STRFUNC);
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

static void
gimp_image_combo_box_changed (GimpImageComboBox *combo_box)
{
  gint image_ID;

  if (gimp_int_combo_box_get_active (GIMP_INT_COMBO_BOX (combo_box),
                                     &image_ID))
    {
      if (! gimp_image_get_by_id (image_ID))
        {
          GtkTreeModel *model;

          model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo_box));

          g_signal_stop_emission_by_name (combo_box, "changed");

          gtk_list_store_clear (GTK_LIST_STORE (model));
          gimp_image_combo_box_populate (combo_box);
        }
    }
}
