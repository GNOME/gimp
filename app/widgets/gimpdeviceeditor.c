/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmadeviceeditor.c
 * Copyright (C) 2010 Michael Natterer <mitch@ligma.org>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmabase/ligmabase.h"
#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "core/ligma.h"
#include "core/ligmacontext.h"
#include "core/ligmafilteredcontainer.h"
#include "core/ligmalist.h"

#include "ligmacontainerview.h"
#include "ligmacontainertreestore.h"
#include "ligmacontainertreeview.h"
#include "ligmadeviceeditor.h"
#include "ligmadeviceinfo.h"
#include "ligmadeviceinfoeditor.h"
#include "ligmadevicemanager.h"
#include "ligmadevices.h"
#include "ligmamessagebox.h"
#include "ligmamessagedialog.h"
#include "ligmaviewrenderer.h"

#include "ligma-intl.h"


enum
{
  PROP_0,
  PROP_LIGMA
};


typedef struct _LigmaDeviceEditorPrivate LigmaDeviceEditorPrivate;

struct _LigmaDeviceEditorPrivate
{
  Ligma      *ligma;

  GQuark     name_changed_handler;

  GtkWidget *treeview;
  GtkWidget *delete_button;

  GtkWidget *label;
  GtkWidget *image;

  GtkWidget *stack;
};


#define LIGMA_DEVICE_EDITOR_GET_PRIVATE(editor) \
        ((LigmaDeviceEditorPrivate *) ligma_device_editor_get_instance_private ((LigmaDeviceEditor *) (editor)))


static void   ligma_device_editor_constructed    (GObject           *object);
static void   ligma_device_editor_dispose        (GObject           *object);
static void   ligma_device_editor_set_property   (GObject           *object,
                                                 guint              property_id,
                                                 const GValue      *value,
                                                 GParamSpec        *pspec);
static void   ligma_device_editor_get_property   (GObject           *object,
                                                 guint              property_id,
                                                 GValue            *value,
                                                 GParamSpec        *pspec);

static void   ligma_device_editor_add_device     (LigmaContainer     *container,
                                                 LigmaDeviceInfo    *info,
                                                 LigmaDeviceEditor  *editor);
static void   ligma_device_editor_remove_device  (LigmaContainer     *container,
                                                 LigmaDeviceInfo    *info,
                                                 LigmaDeviceEditor  *editor);
static void   ligma_device_editor_device_changed (LigmaDeviceInfo    *info,
                                                 LigmaDeviceEditor  *editor);

static gboolean ligma_device_editor_select_device (LigmaContainerView *view,
                                                  GList             *viewables,
                                                  GList             *paths,
                                                  LigmaDeviceEditor  *editor);

static void   ligma_device_editor_delete_clicked (GtkWidget         *button,
                                                 LigmaDeviceEditor  *editor);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaDeviceEditor, ligma_device_editor,
                            GTK_TYPE_PANED)

#define parent_class ligma_device_editor_parent_class


static void
ligma_device_editor_class_init (LigmaDeviceEditorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = ligma_device_editor_constructed;
  object_class->dispose      = ligma_device_editor_dispose;
  object_class->set_property = ligma_device_editor_set_property;
  object_class->get_property = ligma_device_editor_get_property;

  g_object_class_install_property (object_class, PROP_LIGMA,
                                   g_param_spec_object ("ligma",
                                                        NULL, NULL,
                                                        LIGMA_TYPE_LIGMA,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
ligma_device_editor_init (LigmaDeviceEditor *editor)
{
  LigmaDeviceEditorPrivate *private = LIGMA_DEVICE_EDITOR_GET_PRIVATE (editor);
  GtkWidget               *vbox;
  GtkWidget               *ebox;
  GtkWidget               *hbox;
  gint                     icon_width;
  gint                     icon_height;

  gtk_orientable_set_orientation (GTK_ORIENTABLE (editor),
                                  GTK_ORIENTATION_HORIZONTAL);
  gtk_paned_set_wide_handle (GTK_PANED (editor), TRUE);

  gtk_icon_size_lookup (GTK_ICON_SIZE_BUTTON, &icon_width, &icon_height);
  private->treeview = ligma_container_tree_view_new (NULL, NULL, icon_height, 0);
  gtk_widget_set_size_request (private->treeview, 300, -1);
  gtk_paned_pack1 (GTK_PANED (editor), private->treeview, TRUE, FALSE);
  gtk_widget_show (private->treeview);

  g_signal_connect_object (private->treeview, "select-items",
                           G_CALLBACK (ligma_device_editor_select_device),
                           G_OBJECT (editor), 0);

  private->delete_button =
    ligma_editor_add_button (LIGMA_EDITOR (private->treeview),
                            "edit-delete",
                            _("Delete the selected device"),
                            NULL,
                            G_CALLBACK (ligma_device_editor_delete_clicked),
                            NULL,
                            editor);

  gtk_widget_set_sensitive (private->delete_button, FALSE);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_paned_pack2 (GTK_PANED (editor), vbox, TRUE, FALSE);
  gtk_widget_show (vbox);

  ebox = gtk_event_box_new ();
  gtk_widget_set_state_flags (ebox, GTK_STATE_FLAG_SELECTED, TRUE);
  gtk_style_context_add_class (gtk_widget_get_style_context (ebox),
                               GTK_STYLE_CLASS_VIEW);
  gtk_box_pack_start (GTK_BOX (vbox), ebox, FALSE, FALSE, 0);
  gtk_widget_show (ebox);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 6);
  gtk_container_add (GTK_CONTAINER (ebox), hbox);
  gtk_widget_show (hbox);

  private->label = gtk_label_new (NULL);
  gtk_widget_set_state_flags (private->label, GTK_STATE_FLAG_SELECTED, TRUE);
  gtk_label_set_xalign (GTK_LABEL (private->label), 0.0);
  gtk_label_set_ellipsize (GTK_LABEL (private->label), PANGO_ELLIPSIZE_END);
  ligma_label_set_attributes (GTK_LABEL (private->label),
                             PANGO_ATTR_WEIGHT, PANGO_WEIGHT_BOLD,
                             -1);
  gtk_box_pack_start (GTK_BOX (hbox), private->label, TRUE, TRUE, 0);
  gtk_widget_show (private->label);

  private->image = gtk_image_new ();
  gtk_widget_set_state_flags (private->image, GTK_STATE_FLAG_SELECTED, TRUE);
  gtk_widget_set_size_request (private->image, -1, 24);
  gtk_box_pack_end (GTK_BOX (hbox), private->image, FALSE, FALSE, 0);
  gtk_widget_show (private->image);

  private->stack = gtk_stack_new ();
  gtk_container_set_border_width (GTK_CONTAINER (private->stack), 12);
  gtk_stack_set_transition_type (GTK_STACK (private->stack),
                                 GTK_STACK_TRANSITION_TYPE_SLIDE_UP_DOWN);
  gtk_box_pack_start (GTK_BOX (vbox), private->stack, TRUE, TRUE, 0);
  gtk_widget_show (private->stack);
}

static gboolean
ligma_device_editor_filter (LigmaObject *object,
                           gpointer    user_data)
{
  LigmaDeviceInfo *info   = LIGMA_DEVICE_INFO (object);
  GdkDevice      *device = ligma_device_info_get_device (info, NULL);

  /* In the device editor, we filter out virtual devices (useless from a
   * configuration standpoint) as well as the xtest API device.
   * They only leave you wondering what you should do with them and
   * should be ignored.
   * We show device info with no actual associated device. These will
   * simply be shown as grayed out and represent older physical devices
   * which may simply be unplugged right now (but it's nice to show it
   * in the list and can be manually deleted).
   */

  return ((! device || gdk_device_get_device_type (device) != GDK_DEVICE_TYPE_MASTER) &&
          /* Technically only a useful check on X11 but I could also
           * imagine for instance a devicerc used on X11 then Wayland
           * and it would be useless to show there the now absent XTEST
           * device. So we run this name comparison all the time (the
           * name is so specific that no real device ever would have
           * this name; and this is the only way available to recognize
           * this XTEST device which is meant to look like any other
           * device).
           */
          g_strcmp0 (ligma_object_get_name (info), "Virtual core XTEST pointer") != 0);
}

static void
ligma_device_editor_constructed (GObject *object)
{
  LigmaDeviceEditor        *editor  = LIGMA_DEVICE_EDITOR (object);
  LigmaDeviceEditorPrivate *private = LIGMA_DEVICE_EDITOR_GET_PRIVATE (editor);
  LigmaContainer           *devices;
  LigmaContainer           *filtered;
  LigmaContext             *context;
  GList                   *list;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  ligma_assert (LIGMA_IS_LIGMA (private->ligma));

  devices = LIGMA_CONTAINER (ligma_devices_get_manager (private->ligma));
  filtered = ligma_filtered_container_new (devices, ligma_device_editor_filter, NULL);

  /*  connect to "remove" before the container view does so we can get
   *  the stack child stored in its model
   */
  g_signal_connect (devices, "remove",
                    G_CALLBACK (ligma_device_editor_remove_device),
                    editor);

  ligma_container_view_set_container (LIGMA_CONTAINER_VIEW (private->treeview),
                                     filtered);

  context = ligma_context_new (private->ligma, "device-editor-list", NULL);
  ligma_container_view_set_context (LIGMA_CONTAINER_VIEW (private->treeview),
                                   context);
  g_object_unref (context);

  g_signal_connect (devices, "add",
                    G_CALLBACK (ligma_device_editor_add_device),
                    editor);

  private->name_changed_handler =
    ligma_container_add_handler (devices, "name-changed",
                                G_CALLBACK (ligma_device_editor_device_changed),
                                editor);

  for (list = LIGMA_LIST (devices)->queue->head;
       list;
       list = g_list_next (list))
    {
      ligma_device_editor_add_device (devices, list->data, editor);
    }

  g_object_unref (devices);
}

static void
ligma_device_editor_dispose (GObject *object)
{
  LigmaDeviceEditorPrivate *private = LIGMA_DEVICE_EDITOR_GET_PRIVATE (object);
  LigmaContainer           *devices;

  devices = LIGMA_CONTAINER (ligma_devices_get_manager (private->ligma));

  g_signal_handlers_disconnect_by_func (devices,
                                        ligma_device_editor_add_device,
                                        object);

  g_signal_handlers_disconnect_by_func (devices,
                                        ligma_device_editor_remove_device,
                                        object);

  if (private->name_changed_handler)
    {
      ligma_container_remove_handler (devices, private->name_changed_handler);
      private->name_changed_handler = 0;
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
ligma_device_editor_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  LigmaDeviceEditorPrivate *private = LIGMA_DEVICE_EDITOR_GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_LIGMA:
      private->ligma = g_value_get_object (value); /* don't ref */
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_device_editor_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  LigmaDeviceEditorPrivate *private = LIGMA_DEVICE_EDITOR_GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_LIGMA:
      g_value_set_object (value, private->ligma);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_device_editor_add_device (LigmaContainer    *container,
                               LigmaDeviceInfo   *info,
                               LigmaDeviceEditor *editor)
{
  LigmaDeviceEditorPrivate *private = LIGMA_DEVICE_EDITOR_GET_PRIVATE (editor);
  GtkWidget               *widget;
  GtkTreeIter             *iter;

  widget = ligma_device_info_editor_new (info);
  gtk_stack_add_named (GTK_STACK (private->stack), widget,
                       ligma_object_get_name (info));
  gtk_widget_show (widget);

  iter = ligma_container_view_lookup (LIGMA_CONTAINER_VIEW (private->treeview),
                                     LIGMA_VIEWABLE (info));

  if (iter)
    {
      LigmaContainerTreeView *treeview;

      treeview = LIGMA_CONTAINER_TREE_VIEW (private->treeview);

      gtk_tree_store_set (GTK_TREE_STORE (treeview->model), iter,
                          LIGMA_CONTAINER_TREE_STORE_COLUMN_USER_DATA, widget,
                          LIGMA_CONTAINER_TREE_STORE_COLUMN_NAME_SENSITIVE,
                          ligma_device_info_get_device (info, NULL) != NULL,
                          -1);
    }
}

static void
ligma_device_editor_remove_device (LigmaContainer    *container,
                                  LigmaDeviceInfo   *info,
                                  LigmaDeviceEditor *editor)
{
  LigmaDeviceEditorPrivate *private = LIGMA_DEVICE_EDITOR_GET_PRIVATE (editor);
  GtkTreeIter             *iter;

  iter = ligma_container_view_lookup (LIGMA_CONTAINER_VIEW (private->treeview),
                                     LIGMA_VIEWABLE (info));

  if (iter)
    {
      LigmaContainerTreeView *treeview;
      GtkWidget             *widget;

      treeview = LIGMA_CONTAINER_TREE_VIEW (private->treeview);

      gtk_tree_model_get (treeview->model, iter,
                          LIGMA_CONTAINER_TREE_STORE_COLUMN_USER_DATA, &widget,
                          -1);

      if (widget)
        gtk_widget_destroy (widget);
    }
}

static void
ligma_device_editor_device_changed (LigmaDeviceInfo   *info,
                                   LigmaDeviceEditor *editor)
{
  LigmaDeviceEditorPrivate *private = LIGMA_DEVICE_EDITOR_GET_PRIVATE (editor);
  GtkTreeIter             *iter;

  iter = ligma_container_view_lookup (LIGMA_CONTAINER_VIEW (private->treeview),
                                     LIGMA_VIEWABLE (info));

  if (iter)
    {
      LigmaContainerTreeView *treeview;

      treeview = LIGMA_CONTAINER_TREE_VIEW (private->treeview);

      gtk_tree_store_set (GTK_TREE_STORE (treeview->model), iter,
                          LIGMA_CONTAINER_TREE_STORE_COLUMN_NAME_SENSITIVE,
                          ligma_device_info_get_device (info, NULL) != NULL,
                          -1);
    }
}

static gboolean
ligma_device_editor_select_device (LigmaContainerView *view,
                                  GList             *viewables,
                                  GList             *paths,
                                  LigmaDeviceEditor  *editor)
{
  LigmaDeviceEditorPrivate *private = LIGMA_DEVICE_EDITOR_GET_PRIVATE (editor);

  g_return_val_if_fail (g_list_length (viewables) < 2, FALSE);

  if (viewables)
    {
      LigmaContainerTreeView *treeview;
      GtkWidget             *widget;
      GtkTreeIter            iter;
      gboolean               iter_valid;

      treeview = LIGMA_CONTAINER_TREE_VIEW (private->treeview);

      for (iter_valid = gtk_tree_model_get_iter_first (treeview->model, &iter);
           iter_valid;
           iter_valid = gtk_tree_model_iter_next (treeview->model, &iter))
        {
          LigmaViewRenderer *renderer;

          gtk_tree_model_get (treeview->model, &iter,
                              LIGMA_CONTAINER_TREE_STORE_COLUMN_USER_DATA, &widget,
                              LIGMA_CONTAINER_TREE_STORE_COLUMN_RENDERER,  &renderer,
                              -1);
          if (renderer->viewable == viewables->data && widget)
            {
              LigmaDeviceInfo *info;
              gboolean        delete_sensitive = FALSE;

              gtk_stack_set_visible_child (GTK_STACK (private->stack), widget);

              g_object_get (widget ,"info", &info, NULL);

              gtk_label_set_text (GTK_LABEL (private->label),
                                  ligma_object_get_name (info));
              gtk_image_set_from_icon_name (GTK_IMAGE (private->image),
                                            ligma_viewable_get_icon_name (LIGMA_VIEWABLE (info)),
                                            GTK_ICON_SIZE_BUTTON);

              if (! ligma_device_info_get_device (info, NULL))
                delete_sensitive = TRUE;

              gtk_widget_set_sensitive (private->delete_button, delete_sensitive);

              g_object_unref (info);
              g_object_unref (renderer);

              break;
            }
          g_object_unref (renderer);
        }
    }

  return TRUE;
}

static void
ligma_device_editor_delete_response (GtkWidget        *dialog,
                                    gint              response_id,
                                    LigmaDeviceEditor *editor)
{
  LigmaDeviceEditorPrivate *private = LIGMA_DEVICE_EDITOR_GET_PRIVATE (editor);

  gtk_widget_destroy (dialog);

  if (response_id == GTK_RESPONSE_OK)
    {
      GList *selected;

      if (ligma_container_view_get_selected (LIGMA_CONTAINER_VIEW (private->treeview),
                                            &selected, NULL))
        {
          LigmaContainer *devices;

          devices = LIGMA_CONTAINER (ligma_devices_get_manager (private->ligma));

          ligma_container_remove (devices, selected->data);

          g_list_free (selected);
        }
    }

  gtk_widget_set_sensitive (GTK_WIDGET (editor), TRUE);
}

static void
ligma_device_editor_delete_clicked (GtkWidget        *button,
                                   LigmaDeviceEditor *editor)
{
  LigmaDeviceEditorPrivate *private = LIGMA_DEVICE_EDITOR_GET_PRIVATE (editor);
  GtkWidget               *dialog;
  GList                   *selected;

  if (! ligma_container_view_get_selected (LIGMA_CONTAINER_VIEW (private->treeview),
                                          &selected, NULL))
    return;

  dialog = ligma_message_dialog_new (_("Delete Device Settings"),
                                    LIGMA_ICON_DIALOG_QUESTION,
                                    gtk_widget_get_toplevel (GTK_WIDGET (editor)),
                                    GTK_DIALOG_DESTROY_WITH_PARENT,
                                    ligma_standard_help_func, NULL,

                                    _("_Cancel"), GTK_RESPONSE_CANCEL,
                                    _("_Delete"), GTK_RESPONSE_OK,

                                    NULL);

  ligma_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (ligma_device_editor_delete_response),
                    editor);

  ligma_message_box_set_primary_text (LIGMA_MESSAGE_DIALOG (dialog)->box,
                                     _("Delete \"%s\"?"),
                                     ligma_object_get_name (selected->data));
  ligma_message_box_set_text (LIGMA_MESSAGE_DIALOG (dialog)->box,
                             _("You are about to delete this device's "
                               "stored settings.\n"
                               "The next time this device is plugged, "
                               "default settings will be used."));

  g_list_free (selected);

  gtk_widget_set_sensitive (GTK_WIDGET (editor), FALSE);

  gtk_widget_show (dialog);
}


/*  public functions  */

GtkWidget *
ligma_device_editor_new (Ligma *ligma)
{
  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);

  return g_object_new (LIGMA_TYPE_DEVICE_EDITOR,
                       "ligma", ligma,
                       NULL);
}
