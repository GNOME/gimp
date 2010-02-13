/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimplist.h"
#include "core/gimpmarshal.h"

#include "gimpcontainerview.h"
#include "gimpcontainertreeview.h"
#include "gimpdeviceeditor.h"
#include "gimpdeviceinfo.h"
#include "gimpdeviceinfoeditor.h"
#include "gimpdevices.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_GIMP
};


typedef struct _GimpDeviceEditorPrivate GimpDeviceEditorPrivate;

struct _GimpDeviceEditorPrivate
{
  Gimp      *gimp;

  GtkWidget *treeview;

  GtkWidget *label;
  GtkWidget *image;

  GtkWidget *notebook;
};


#define GIMP_DEVICE_EDITOR_GET_PRIVATE(editor) \
        G_TYPE_INSTANCE_GET_PRIVATE (editor, \
                                     GIMP_TYPE_DEVICE_EDITOR, \
                                     GimpDeviceEditorPrivate)


static GObject * gimp_device_editor_constructor    (GType                  type,
                                                    guint                  n_params,
                                                    GObjectConstructParam *params);
static void      gimp_device_editor_set_property   (GObject               *object,
                                                    guint                  property_id,
                                                    const GValue          *value,
                                                    GParamSpec            *pspec);
static void      gimp_device_editor_get_property   (GObject               *object,
                                                    guint                  property_id,
                                                    GValue                *value,
                                                    GParamSpec            *pspec);

static void      gimp_device_editor_add_device     (GimpContainer         *container,
                                                    GimpDeviceInfo        *info,
                                                    GimpDeviceEditor      *editor);
static void      gimp_device_editor_remove_device  (GimpContainer         *container,
                                                    GimpDeviceInfo        *info,
                                                    GimpDeviceEditor      *editor);

static void      gimp_device_editor_select_device  (GimpContainerView     *view,
                                                    GimpViewable          *viewable,
                                                    gpointer               insert_data,
                                                    GimpDeviceEditor      *editor);


G_DEFINE_TYPE (GimpDeviceEditor, gimp_device_editor, GTK_TYPE_HBOX)

#define parent_class gimp_device_editor_parent_class


static void
gimp_device_editor_class_init (GimpDeviceEditorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructor  = gimp_device_editor_constructor;
  object_class->set_property = gimp_device_editor_set_property;
  object_class->get_property = gimp_device_editor_get_property;

  g_object_class_install_property (object_class, PROP_GIMP,
                                   g_param_spec_object ("gimp",
                                                        NULL, NULL,
                                                        GIMP_TYPE_GIMP,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_type_class_add_private (object_class, sizeof (GimpDeviceEditorPrivate));
}

static void
gimp_device_editor_init (GimpDeviceEditor *editor)
{
  GimpDeviceEditorPrivate *private = GIMP_DEVICE_EDITOR_GET_PRIVATE (editor);
  GtkWidget               *vbox;
  GtkWidget               *ebox;
  GtkWidget               *hbox;
  gint                     icon_width;
  gint                     icon_height;

  gtk_box_set_spacing (GTK_BOX (editor), 12);

  gtk_icon_size_lookup_for_settings (gtk_widget_get_settings (GTK_WIDGET (editor)),
                                     GTK_ICON_SIZE_BUTTON,
                                     &icon_width, &icon_height);

  private->treeview = gimp_container_tree_view_new (NULL, NULL, icon_height, 0);
  gtk_widget_set_size_request (private->treeview, 200, -1);
  gtk_box_pack_start (GTK_BOX (editor), private->treeview, FALSE, FALSE, 0);
  gtk_widget_show (private->treeview);

  g_signal_connect_object (private->treeview, "select-item",
                           G_CALLBACK (gimp_device_editor_select_device),
                           G_OBJECT (editor), 0);

#if 0
  list->edit_button =
    gimp_editor_add_button (GIMP_EDITOR (list->dest),
                            GTK_STOCK_PROPERTIES,
                            _("Configure the selected controller"),
                            NULL,
                            G_CALLBACK (gimp_controller_list_edit_clicked),
                            NULL,
                            list);
  list->up_button =
    gimp_editor_add_button (GIMP_EDITOR (list->dest),
                            GTK_STOCK_GO_UP,
                            _("Move the selected controller up"),
                            NULL,
                            G_CALLBACK (gimp_controller_list_up_clicked),
                            NULL,
                            list);
  list->down_button =
    gimp_editor_add_button (GIMP_EDITOR (list->dest),
                            GTK_STOCK_GO_DOWN,
                            _("Move the selected controller down"),
                            NULL,
                            G_CALLBACK (gimp_controller_list_down_clicked),
                            NULL,
                            list);

  gtk_widget_set_sensitive (list->edit_button, FALSE);
  gtk_widget_set_sensitive (list->up_button,   FALSE);
  gtk_widget_set_sensitive (list->down_button, FALSE);
#endif

  vbox = gtk_vbox_new (FALSE, 12);
  gtk_box_pack_start (GTK_BOX (editor), vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  ebox = gtk_event_box_new ();
  gtk_widget_set_state (ebox, GTK_STATE_SELECTED);
  gtk_box_pack_start (GTK_BOX (vbox), ebox, FALSE, FALSE, 0);
  gtk_widget_show (ebox);

  hbox = gtk_hbox_new (FALSE, 6);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 4);
  gtk_container_add (GTK_CONTAINER (ebox), hbox);
  gtk_widget_show (hbox);

  private->label = gtk_label_new (NULL);
  gtk_misc_set_alignment (GTK_MISC (private->label), 0.0, 0.5);
  gtk_label_set_ellipsize (GTK_LABEL (private->label), PANGO_ELLIPSIZE_END);
  gimp_label_set_attributes (GTK_LABEL (private->label),
                             PANGO_ATTR_WEIGHT, PANGO_WEIGHT_BOLD,
                             -1);
  gtk_box_pack_start (GTK_BOX (hbox), private->label, TRUE, TRUE, 0);
  gtk_widget_show (private->label);

  private->image = gtk_image_new ();
  gtk_widget_set_size_request (private->image, -1, 24);
  gtk_box_pack_end (GTK_BOX (hbox), private->image, FALSE, FALSE, 0);
  gtk_widget_show (private->image);

  private->notebook = gtk_notebook_new ();
  gtk_notebook_set_show_border (GTK_NOTEBOOK (private->notebook), FALSE);
  gtk_notebook_set_show_tabs (GTK_NOTEBOOK (private->notebook), FALSE);
  gtk_box_pack_start (GTK_BOX (vbox), private->notebook, TRUE, TRUE, 0);
  gtk_widget_show (private->notebook);
}

static GObject *
gimp_device_editor_constructor (GType                   type,
                                guint                   n_params,
                                GObjectConstructParam  *params)
{
  GObject                 *object;
  GimpDeviceEditor        *editor;
  GimpDeviceEditorPrivate *private;
  GimpContainer           *devices;
  GList                   *list;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  editor  = GIMP_DEVICE_EDITOR (object);
  private = GIMP_DEVICE_EDITOR_GET_PRIVATE (editor);

  g_assert (GIMP_IS_GIMP (private->gimp));

  devices = gimp_devices_get_list (private->gimp);

  gimp_container_view_set_container (GIMP_CONTAINER_VIEW (private->treeview),
                                     devices);

  gimp_container_view_set_context (GIMP_CONTAINER_VIEW (private->treeview),
                                   gimp_get_user_context (private->gimp));

  g_signal_connect (devices, "add",
                    G_CALLBACK (gimp_device_editor_add_device),
                    editor);
  g_signal_connect (devices, "remove",
                    G_CALLBACK (gimp_device_editor_remove_device),
                    editor);

  for (list = GIMP_LIST (devices)->list;
       list;
       list = g_list_next (list))
    {
      gimp_device_editor_add_device (devices, list->data, editor);
    }

  return object;
}

static void
gimp_device_editor_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  GimpDeviceEditorPrivate *private = GIMP_DEVICE_EDITOR_GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_GIMP:
      private->gimp = g_value_get_object (value); /* don't ref */
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_device_editor_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  GimpDeviceEditorPrivate *private = GIMP_DEVICE_EDITOR_GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_GIMP:
      g_value_set_object (value, private->gimp);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_device_editor_add_device (GimpContainer    *container,
                               GimpDeviceInfo   *info,
                               GimpDeviceEditor *editor)
{
  GimpDeviceEditorPrivate *private = GIMP_DEVICE_EDITOR_GET_PRIVATE (editor);
  GtkWidget               *widget;
  GtkTreeIter             *iter;

  widget = gimp_device_info_editor_new (info);
  gtk_notebook_append_page (GTK_NOTEBOOK (private->notebook), widget, NULL);
  gtk_widget_show (widget);

  iter = gimp_container_view_lookup (GIMP_CONTAINER_VIEW (private->treeview),
                                     GIMP_VIEWABLE (info));

  if (iter)
    {
      GimpContainerTreeView *treeview;

      treeview = GIMP_CONTAINER_TREE_VIEW (private->treeview);

      gtk_tree_store_set (GTK_TREE_STORE (treeview->model), iter,
                          GIMP_CONTAINER_TREE_VIEW_COLUMN_USER_DATA, widget,
                          -1);
    }
}

static void
gimp_device_editor_remove_device (GimpContainer    *container,
                                  GimpDeviceInfo   *info,
                                  GimpDeviceEditor *editor)
{
}

static void
gimp_device_editor_select_device (GimpContainerView *view,
                                  GimpViewable      *viewable,
                                  gpointer           insert_data,
                                  GimpDeviceEditor  *editor)
{
  GimpDeviceEditorPrivate *private = GIMP_DEVICE_EDITOR_GET_PRIVATE (editor);

  if (viewable && insert_data)
    {
      GimpContainerTreeView *treeview;
      GtkWidget             *widget;

      treeview = GIMP_CONTAINER_TREE_VIEW (private->treeview);

      gtk_tree_model_get (treeview->model, insert_data,
                          GIMP_CONTAINER_TREE_VIEW_COLUMN_USER_DATA, &widget,
                          -1);

      if (widget)
        {
          gint page_num = gtk_notebook_page_num (GTK_NOTEBOOK (private->notebook),
                                                 widget);

          gtk_notebook_set_current_page (GTK_NOTEBOOK (private->notebook),
                                         page_num);
        }

      gtk_label_set_text (GTK_LABEL (private->label),
                          gimp_object_get_name (viewable));
      gtk_image_set_from_stock (GTK_IMAGE (private->image),
                                gimp_viewable_get_stock_id (viewable),
                                GTK_ICON_SIZE_BUTTON);
    }
}


/*  public functions  */

GtkWidget *
gimp_device_editor_new (Gimp *gimp)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  return g_object_new (GIMP_TYPE_DEVICE_EDITOR,
                       "gimp", gimp,
                       NULL);
}
