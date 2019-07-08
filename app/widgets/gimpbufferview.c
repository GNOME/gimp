/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpbufferview.c
 * Copyright (C) 2001-2004 Michael Natterer <mitch@gimp.org>
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

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimpbuffer.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"

#include "gimpcontainertreeview.h"
#include "gimpcontainerview.h"
#include "gimpbufferview.h"
#include "gimpdnd.h"
#include "gimpdocked.h"
#include "gimpeditor.h"
#include "gimphelp-ids.h"
#include "gimpview.h"
#include "gimpviewrendererbuffer.h"
#include "gimpuimanager.h"
#include "gimpwidgets-utils.h"

#include "gimp-intl.h"


static void   gimp_buffer_view_docked_iface_init (GimpDockedInterface *iface);

static void   gimp_buffer_view_set_context       (GimpDocked          *docked,
                                                  GimpContext         *context);

static void   gimp_buffer_view_activate_item     (GimpContainerEditor *editor,
                                                  GimpViewable        *viewable);

static void   gimp_buffer_view_clipboard_changed (Gimp                *gimp,
                                                  GimpBufferView      *buffer_view);
static void   gimp_buffer_view_view_notify       (GimpContainerView   *view,
                                                  GParamSpec          *pspec,
                                                  GimpBufferView      *buffer_view);


G_DEFINE_TYPE_WITH_CODE (GimpBufferView, gimp_buffer_view,
                         GIMP_TYPE_CONTAINER_EDITOR,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_DOCKED,
                                                gimp_buffer_view_docked_iface_init))

#define parent_class gimp_buffer_view_parent_class

static GimpDockedInterface *parent_docked_iface = NULL;


static void
gimp_buffer_view_class_init (GimpBufferViewClass *klass)
{
  GimpContainerEditorClass *editor_class = GIMP_CONTAINER_EDITOR_CLASS (klass);

  editor_class->activate_item = gimp_buffer_view_activate_item;
}

static void
gimp_buffer_view_docked_iface_init (GimpDockedInterface *iface)
{
  parent_docked_iface = g_type_interface_peek_parent (iface);

  if (! parent_docked_iface)
    parent_docked_iface = g_type_default_interface_peek (GIMP_TYPE_DOCKED);

  iface->set_context = gimp_buffer_view_set_context;
}

static void
gimp_buffer_view_init (GimpBufferView *view)
{
}

static void
gimp_buffer_view_set_context (GimpDocked  *docked,
                              GimpContext *context)
{
  GimpBufferView *view = GIMP_BUFFER_VIEW (docked);

  parent_docked_iface->set_context (docked, context);

  gimp_view_renderer_set_context (GIMP_VIEW (view->clipboard_view)->renderer,
                                  context);
}


/*  public functions  */

GtkWidget *
gimp_buffer_view_new (GimpViewType     view_type,
                      GimpContainer   *container,
                      GimpContext     *context,
                      gint             view_size,
                      gint             view_border_width,
                      GimpMenuFactory *menu_factory)
{
  GimpBufferView      *buffer_view;
  GimpContainerEditor *editor;
  GtkWidget           *frame;
  GtkWidget           *hbox;

  g_return_val_if_fail (GIMP_IS_CONTAINER (container), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (view_size > 0 &&
                        view_size <= GIMP_VIEWABLE_MAX_PREVIEW_SIZE, FALSE);
  g_return_val_if_fail (view_border_width >= 0 &&
                        view_border_width <= GIMP_VIEW_MAX_BORDER_WIDTH,
                        FALSE);

  buffer_view = g_object_new (GIMP_TYPE_BUFFER_VIEW,
                              "view-type",         view_type,
                              "container",         container,
                              "context",           context,
                              "view-size",         view_size,
                              "view-border-width", view_border_width,
                              "menu-factory",      menu_factory,
                              "menu-identifier",   "<Buffers>",
                              "ui-path",           "/buffers-popup",
                              NULL);

  editor = GIMP_CONTAINER_EDITOR (buffer_view);

  if (GIMP_IS_CONTAINER_TREE_VIEW (editor->view))
    {
      GimpContainerTreeView *tree_view;

      tree_view = GIMP_CONTAINER_TREE_VIEW (editor->view);

      gimp_container_tree_view_connect_name_edited (tree_view,
                                                    G_CALLBACK (gimp_container_tree_view_name_edited),
                                                    tree_view);
    }

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (editor), frame, FALSE, FALSE, 0);
  gtk_box_reorder_child (GTK_BOX (editor), frame, 0);
  gtk_widget_show (frame);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 2);
  gtk_container_add (GTK_CONTAINER (frame), hbox);
  gtk_widget_show (hbox);

  /* FIXME: enable preview of a clipboard image, not just buffer */
  buffer_view->clipboard_view =
    gimp_view_new_full_by_types (NULL,
                                 GIMP_TYPE_VIEW,
                                 GIMP_TYPE_BUFFER,
                                 view_size, view_size, view_border_width,
                                 FALSE, FALSE, TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), buffer_view->clipboard_view,
                      FALSE, FALSE, 0);
  gtk_widget_show (buffer_view->clipboard_view);

  g_signal_connect_object (editor->view, "notify::view-size",
                           G_CALLBACK (gimp_buffer_view_view_notify),
                           buffer_view, 0);
  g_signal_connect_object (editor->view, "notify::view-border-width",
                           G_CALLBACK (gimp_buffer_view_view_notify),
                           buffer_view, 0);

  buffer_view->clipboard_label = gtk_label_new (_("(None)"));
  gtk_box_pack_start (GTK_BOX (hbox), buffer_view->clipboard_label,
                      FALSE, FALSE, 0);
  gtk_widget_show (buffer_view->clipboard_label);

  g_signal_connect_object (context->gimp, "clipboard-changed",
                           G_CALLBACK (gimp_buffer_view_clipboard_changed),
                           G_OBJECT (buffer_view), 0);

  gimp_buffer_view_clipboard_changed (context->gimp, buffer_view);

  buffer_view->paste_button =
    gimp_editor_add_action_button (GIMP_EDITOR (editor->view), "buffers",
                                   "buffers-paste",
                                   "buffers-paste-in-place",
                                   gimp_get_extend_selection_mask (),
                                   NULL);

  buffer_view->paste_into_button =
    gimp_editor_add_action_button (GIMP_EDITOR (editor->view), "buffers",
                                   "buffers-paste-into",
                                   "buffers-paste-into-in-place",
                                   gimp_get_extend_selection_mask (),
                                   NULL);

  buffer_view->paste_as_new_layer_button =
    gimp_editor_add_action_button (GIMP_EDITOR (editor->view), "buffers",
                                   "buffers-paste-as-new-layer",
                                   "buffers-paste-as-new-layer-in-place",
                                   gimp_get_extend_selection_mask (),
                                   NULL);

  buffer_view->paste_as_new_image_button =
    gimp_editor_add_action_button (GIMP_EDITOR (editor->view), "buffers",
                                   "buffers-paste-as-new-image", NULL);

  buffer_view->delete_button =
    gimp_editor_add_action_button (GIMP_EDITOR (editor->view), "buffers",
                                   "buffers-delete", NULL);

  gimp_container_view_enable_dnd (editor->view,
                                  GTK_BUTTON (buffer_view->paste_button),
                                  GIMP_TYPE_BUFFER);
  gimp_container_view_enable_dnd (editor->view,
                                  GTK_BUTTON (buffer_view->paste_into_button),
                                  GIMP_TYPE_BUFFER);
  gimp_container_view_enable_dnd (editor->view,
                                  GTK_BUTTON (buffer_view->paste_as_new_layer_button),
                                  GIMP_TYPE_BUFFER);
  gimp_container_view_enable_dnd (editor->view,
                                  GTK_BUTTON (buffer_view->paste_as_new_image_button),
                                  GIMP_TYPE_BUFFER);
  gimp_container_view_enable_dnd (editor->view,
                                  GTK_BUTTON (buffer_view->delete_button),
                                  GIMP_TYPE_BUFFER);

  gimp_ui_manager_update (gimp_editor_get_ui_manager (GIMP_EDITOR (editor->view)),
                          editor);

  return GTK_WIDGET (buffer_view);
}


/*  private functions  */

static void
gimp_buffer_view_activate_item (GimpContainerEditor *editor,
                                GimpViewable        *viewable)
{
  GimpBufferView *view = GIMP_BUFFER_VIEW (editor);
  GimpContainer  *container;

  if (GIMP_CONTAINER_EDITOR_CLASS (parent_class)->activate_item)
    GIMP_CONTAINER_EDITOR_CLASS (parent_class)->activate_item (editor, viewable);

  container = gimp_container_view_get_container (editor->view);

  if (viewable && gimp_container_have (container, GIMP_OBJECT (viewable)))
    {
      gtk_button_clicked (GTK_BUTTON (view->paste_button));
    }
}

static void
gimp_buffer_view_clipboard_changed (Gimp           *gimp,
                                    GimpBufferView *buffer_view)
{
  GimpBuffer *buffer = gimp_get_clipboard_buffer (gimp);

  gimp_view_set_viewable (GIMP_VIEW (buffer_view->clipboard_view),
                          GIMP_VIEWABLE (buffer));

  if (buffer)
    {
      gchar *desc = gimp_viewable_get_description (GIMP_VIEWABLE (buffer),
                                                   NULL);
      gtk_label_set_text (GTK_LABEL (buffer_view->clipboard_label), desc);
      g_free (desc);
    }
  else
    {
      gtk_label_set_text (GTK_LABEL (buffer_view->clipboard_label), _("(None)"));
    }
}

static void
gimp_buffer_view_view_notify (GimpContainerView *container_view,
                              GParamSpec        *pspec,
                              GimpBufferView    *buffer_view)
{
  GimpView *view = GIMP_VIEW (buffer_view->clipboard_view);
  gint      view_size;
  gint      view_border_width;

  view_size = gimp_container_view_get_view_size (container_view,
                                                 &view_border_width);

  gimp_view_renderer_set_size_full (view->renderer,
                                    view_size, view_size, view_border_width);
}
