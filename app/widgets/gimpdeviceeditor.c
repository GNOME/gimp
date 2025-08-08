/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdeviceeditor.c
 * Copyright (C) 2010 Michael Natterer <mitch@gimp.org>
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

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpfilteredcontainer.h"

#include "gimpcontainerview.h"
#include "gimpcontainerlistview.h"
#include "gimpdeviceeditor.h"
#include "gimpdeviceinfo.h"
#include "gimpdeviceinfoeditor.h"
#include "gimpdevicemanager.h"
#include "gimpdevices.h"
#include "gimpmessagebox.h"
#include "gimpmessagedialog.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_GIMP
};


typedef struct _GimpDeviceEditorPrivate GimpDeviceEditorPrivate;

struct _GimpDeviceEditorPrivate
{
  Gimp          *gimp;

  GimpContainer *filtered;

  GtkWidget     *list_view;
  GtkWidget     *delete_button;

  GtkWidget     *label;
  GtkWidget     *image;

  GtkWidget     *stack;
};


#define GIMP_DEVICE_EDITOR_GET_PRIVATE(editor) \
        ((GimpDeviceEditorPrivate *) gimp_device_editor_get_instance_private ((GimpDeviceEditor *) (editor)))


static void   gimp_device_editor_constructed    (GObject           *object);
static void   gimp_device_editor_dispose        (GObject           *object);
static void   gimp_device_editor_finalize       (GObject           *object);
static void   gimp_device_editor_set_property   (GObject           *object,
                                                 guint              property_id,
                                                 const GValue      *value,
                                                 GParamSpec        *pspec);
static void   gimp_device_editor_get_property   (GObject           *object,
                                                 guint              property_id,
                                                 GValue            *value,
                                                 GParamSpec        *pspec);

static void   gimp_device_editor_style_updated  (GtkWidget         *widget);

static void   gimp_device_editor_add_device     (GimpContainer     *container,
                                                 GimpDeviceInfo    *info,
                                                 GimpDeviceEditor  *editor);
static void   gimp_device_editor_remove_device  (GimpContainer     *container,
                                                 GimpDeviceInfo    *info,
                                                 GimpDeviceEditor  *editor);

static void   gimp_device_editor_select_device  (GimpContext       *view,
                                                 GimpDeviceInfo    *info,
                                                 GimpDeviceEditor  *editor);

static void   gimp_device_editor_delete_clicked (GtkWidget         *button,
                                                 GimpDeviceEditor  *editor);


G_DEFINE_TYPE_WITH_PRIVATE (GimpDeviceEditor, gimp_device_editor,
                            GTK_TYPE_PANED)

#define parent_class gimp_device_editor_parent_class


static void
gimp_device_editor_class_init (GimpDeviceEditorClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed   = gimp_device_editor_constructed;
  object_class->dispose       = gimp_device_editor_dispose;
  object_class->finalize      = gimp_device_editor_finalize;
  object_class->set_property  = gimp_device_editor_set_property;
  object_class->get_property  = gimp_device_editor_get_property;

  widget_class->style_updated = gimp_device_editor_style_updated;

  g_object_class_install_property (object_class, PROP_GIMP,
                                   g_param_spec_object ("gimp",
                                                        NULL, NULL,
                                                        GIMP_TYPE_GIMP,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
gimp_device_editor_init (GimpDeviceEditor *editor)
{
  GimpDeviceEditorPrivate *private = GIMP_DEVICE_EDITOR_GET_PRIVATE (editor);
  GtkWidget               *vbox;
  GtkWidget               *ebox;
  GtkWidget               *hbox;

  gtk_orientable_set_orientation (GTK_ORIENTABLE (editor),
                                  GTK_ORIENTATION_HORIZONTAL);
  gtk_paned_set_wide_handle (GTK_PANED (editor), TRUE);

  private->list_view = gimp_container_list_view_new (NULL, NULL,
                                                     GIMP_VIEW_SIZE_MEDIUM, 0);
  gtk_widget_set_size_request (private->list_view, 300, -1);
  gtk_paned_pack1 (GTK_PANED (editor), private->list_view, TRUE, FALSE);
  gtk_widget_show (private->list_view);

  private->delete_button =
    gimp_editor_add_button (GIMP_EDITOR (private->list_view),
                            GIMP_ICON_EDIT_DELETE,
                            _("Delete the selected device"),
                            NULL,
                            G_CALLBACK (gimp_device_editor_delete_clicked),
                            NULL,
                            G_OBJECT (editor));

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
  gimp_label_set_attributes (GTK_LABEL (private->label),
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
                                 gimp_widget_animation_enabled () ?
                                 GTK_STACK_TRANSITION_TYPE_CROSSFADE :
                                 GTK_STACK_TRANSITION_TYPE_NONE);
  gtk_box_pack_start (GTK_BOX (vbox), private->stack, TRUE, TRUE, 0);
  gtk_widget_show (private->stack);
}

static gboolean
gimp_device_editor_filter (GimpObject *object,
                           gpointer    user_data)
{
  GimpDeviceInfo *info   = GIMP_DEVICE_INFO (object);
  GdkDevice      *device = gimp_device_info_get_device (info, NULL);

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
          g_strcmp0 (gimp_object_get_name (info), "Virtual core XTEST pointer") != 0);
}

static void
gimp_device_editor_constructed (GObject *object)
{
  GimpDeviceEditor        *editor  = GIMP_DEVICE_EDITOR (object);
  GimpDeviceEditorPrivate *private = GIMP_DEVICE_EDITOR_GET_PRIVATE (editor);
  GimpContainer           *devices;
  GimpContext             *context;
  GList                   *list;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gimp_assert (GIMP_IS_GIMP (private->gimp));

  devices = GIMP_CONTAINER (gimp_devices_get_manager (private->gimp));

  private->filtered = gimp_filtered_container_new (devices,
                                                   gimp_device_editor_filter,
                                                   NULL);
  gimp_container_view_set_container (GIMP_CONTAINER_VIEW (private->list_view),
                                     private->filtered);

  g_signal_connect (private->filtered, "add",
                    G_CALLBACK (gimp_device_editor_add_device),
                    editor);
  g_signal_connect (private->filtered, "remove",
                    G_CALLBACK (gimp_device_editor_remove_device),
                    editor);

  context = gimp_context_new (private->gimp, "device-editor-list", NULL);
  gimp_container_view_set_context (GIMP_CONTAINER_VIEW (private->list_view),
                                   context);
  g_object_unref (context);

  g_signal_connect_object (context, "tool-preset-changed",
                           G_CALLBACK (gimp_device_editor_select_device),
                           G_OBJECT (editor), 0);

  for (list = GIMP_LIST (private->filtered)->queue->head;
       list;
       list = g_list_next (list))
    {
      gimp_device_editor_add_device (devices, list->data, editor);
    }

}

static void
gimp_device_editor_dispose (GObject *object)
{
  GimpDeviceEditorPrivate *private = GIMP_DEVICE_EDITOR_GET_PRIVATE (object);

  g_signal_handlers_disconnect_by_func (private->filtered,
                                        gimp_device_editor_add_device,
                                        object);
  g_signal_handlers_disconnect_by_func (private->filtered,
                                        gimp_device_editor_remove_device,
                                        object);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_device_editor_finalize (GObject *object)
{
  GimpDeviceEditorPrivate *private = GIMP_DEVICE_EDITOR_GET_PRIVATE (object);

  g_clear_object (&private->filtered);

  G_OBJECT_CLASS (parent_class)->finalize (object);
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
gimp_device_editor_style_updated (GtkWidget *widget)
{
  GimpDeviceEditorPrivate *private = GIMP_DEVICE_EDITOR_GET_PRIVATE (widget);
  gint                     icon_width;
  gint                     icon_height;

  gtk_icon_size_lookup (GTK_ICON_SIZE_BUTTON, &icon_width, &icon_height);

  gimp_container_view_set_view_size (GIMP_CONTAINER_VIEW (private->list_view),
                                     icon_height, 0);

  GTK_WIDGET_CLASS (parent_class)->style_updated (widget);
}

static void
gimp_device_editor_add_device (GimpContainer    *container,
                               GimpDeviceInfo   *info,
                               GimpDeviceEditor *editor)
{
  GimpDeviceEditorPrivate *private = GIMP_DEVICE_EDITOR_GET_PRIVATE (editor);
  GtkWidget               *widget;

  widget = gimp_device_info_editor_new (info);
  gtk_stack_add_named (GTK_STACK (private->stack), widget,
                       gimp_object_get_name (info));
  gtk_widget_show (widget);
}

static void
gimp_device_editor_remove_device (GimpContainer    *container,
                                  GimpDeviceInfo   *info,
                                  GimpDeviceEditor *editor)
{
  GimpDeviceEditorPrivate *private = GIMP_DEVICE_EDITOR_GET_PRIVATE (editor);
  GtkWidget               *widget;

  widget = gtk_stack_get_child_by_name (GTK_STACK (private->stack),
                                        gimp_object_get_name (info));

  if (widget)
    gtk_widget_destroy (widget);
}

static void
gimp_device_editor_select_device (GimpContext      *context,
                                  GimpDeviceInfo   *info,
                                  GimpDeviceEditor *editor)
{
  GimpDeviceEditorPrivate *private = GIMP_DEVICE_EDITOR_GET_PRIVATE (editor);
  GtkWidget               *widget;

  widget = gtk_stack_get_child_by_name (GTK_STACK (private->stack),
                                        gimp_object_get_name (info));

  if (widget)
    {
      gtk_stack_set_visible_child (GTK_STACK (private->stack), widget);

      gtk_label_set_text (GTK_LABEL (private->label),
                          gimp_object_get_name (info));
      gtk_image_set_from_icon_name (GTK_IMAGE (private->image),
                                    gimp_viewable_get_icon_name (GIMP_VIEWABLE (info)),
                                    GTK_ICON_SIZE_BUTTON);

      gtk_widget_set_sensitive (private->delete_button,
                                ! gimp_device_info_get_device (info, NULL));
    }
}

static void
gimp_device_editor_delete_response (GtkWidget        *dialog,
                                    gint              response_id,
                                    GimpDeviceEditor *editor)
{
  GimpDeviceEditorPrivate *private = GIMP_DEVICE_EDITOR_GET_PRIVATE (editor);

  gtk_widget_destroy (dialog);

  if (response_id == GTK_RESPONSE_OK)
    {
      GimpViewable *item;

      item = gimp_container_view_get_1_selected (GIMP_CONTAINER_VIEW (private->list_view));

      if (item)
        {
          GimpContainer *devices;

          devices = GIMP_CONTAINER (gimp_devices_get_manager (private->gimp));

          gimp_container_remove (devices, GIMP_OBJECT (item));
        }
    }

  gtk_widget_set_sensitive (GTK_WIDGET (editor), TRUE);
}

static void
gimp_device_editor_delete_clicked (GtkWidget        *button,
                                   GimpDeviceEditor *editor)
{
  GimpDeviceEditorPrivate *private = GIMP_DEVICE_EDITOR_GET_PRIVATE (editor);
  GtkWidget               *dialog;
  GimpViewable            *item;

  item = gimp_container_view_get_1_selected (GIMP_CONTAINER_VIEW (private->list_view));

  if (! item)
    return;

  dialog = gimp_message_dialog_new (_("Delete Device Settings"),
                                    GIMP_ICON_DIALOG_QUESTION,
                                    gtk_widget_get_toplevel (GTK_WIDGET (editor)),
                                    GTK_DIALOG_DESTROY_WITH_PARENT,
                                    gimp_standard_help_func, NULL,

                                    _("_Cancel"), GTK_RESPONSE_CANCEL,
                                    _("_Delete"), GTK_RESPONSE_OK,

                                    NULL);

  gimp_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (gimp_device_editor_delete_response),
                    editor);

  gimp_message_box_set_primary_text (GIMP_MESSAGE_DIALOG (dialog)->box,
                                     _("Delete \"%s\"?"),
                                     gimp_object_get_name (item));
  gimp_message_box_set_text (GIMP_MESSAGE_DIALOG (dialog)->box,
                             _("You are about to delete this device's "
                               "stored settings.\n"
                               "The next time this device is plugged, "
                               "default settings will be used."));

  gtk_widget_set_sensitive (GTK_WIDGET (editor), FALSE);

  gtk_widget_show (dialog);
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
