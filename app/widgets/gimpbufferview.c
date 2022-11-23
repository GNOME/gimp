/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmabufferview.c
 * Copyright (C) 2001-2004 Michael Natterer <mitch@ligma.org>
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

#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "core/ligma.h"
#include "core/ligmabuffer.h"
#include "core/ligmacontainer.h"
#include "core/ligmacontext.h"
#include "core/ligmaimage.h"

#include "ligmacontainertreeview.h"
#include "ligmacontainerview.h"
#include "ligmabufferview.h"
#include "ligmadnd.h"
#include "ligmadocked.h"
#include "ligmaeditor.h"
#include "ligmahelp-ids.h"
#include "ligmaview.h"
#include "ligmaviewrendererbuffer.h"
#include "ligmauimanager.h"
#include "ligmawidgets-utils.h"

#include "ligma-intl.h"


static void   ligma_buffer_view_docked_iface_init (LigmaDockedInterface *iface);

static void   ligma_buffer_view_set_context       (LigmaDocked          *docked,
                                                  LigmaContext         *context);

static void   ligma_buffer_view_activate_item     (LigmaContainerEditor *editor,
                                                  LigmaViewable        *viewable);

static void   ligma_buffer_view_clipboard_changed (Ligma                *ligma,
                                                  LigmaBufferView      *buffer_view);
static void   ligma_buffer_view_view_notify       (LigmaContainerView   *view,
                                                  GParamSpec          *pspec,
                                                  LigmaBufferView      *buffer_view);


G_DEFINE_TYPE_WITH_CODE (LigmaBufferView, ligma_buffer_view,
                         LIGMA_TYPE_CONTAINER_EDITOR,
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_DOCKED,
                                                ligma_buffer_view_docked_iface_init))

#define parent_class ligma_buffer_view_parent_class

static LigmaDockedInterface *parent_docked_iface = NULL;


static void
ligma_buffer_view_class_init (LigmaBufferViewClass *klass)
{
  LigmaContainerEditorClass *editor_class = LIGMA_CONTAINER_EDITOR_CLASS (klass);

  editor_class->activate_item = ligma_buffer_view_activate_item;
}

static void
ligma_buffer_view_docked_iface_init (LigmaDockedInterface *iface)
{
  parent_docked_iface = g_type_interface_peek_parent (iface);

  if (! parent_docked_iface)
    parent_docked_iface = g_type_default_interface_peek (LIGMA_TYPE_DOCKED);

  iface->set_context = ligma_buffer_view_set_context;
}

static void
ligma_buffer_view_init (LigmaBufferView *view)
{
}

static void
ligma_buffer_view_set_context (LigmaDocked  *docked,
                              LigmaContext *context)
{
  LigmaBufferView *view = LIGMA_BUFFER_VIEW (docked);

  parent_docked_iface->set_context (docked, context);

  ligma_view_renderer_set_context (LIGMA_VIEW (view->clipboard_view)->renderer,
                                  context);
}


/*  public functions  */

GtkWidget *
ligma_buffer_view_new (LigmaViewType     view_type,
                      LigmaContainer   *container,
                      LigmaContext     *context,
                      gint             view_size,
                      gint             view_border_width,
                      LigmaMenuFactory *menu_factory)
{
  LigmaBufferView      *buffer_view;
  LigmaContainerEditor *editor;
  GtkWidget           *frame;
  GtkWidget           *hbox;

  g_return_val_if_fail (LIGMA_IS_CONTAINER (container), NULL);
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (view_size > 0 &&
                        view_size <= LIGMA_VIEWABLE_MAX_PREVIEW_SIZE, FALSE);
  g_return_val_if_fail (view_border_width >= 0 &&
                        view_border_width <= LIGMA_VIEW_MAX_BORDER_WIDTH,
                        FALSE);

  buffer_view = g_object_new (LIGMA_TYPE_BUFFER_VIEW,
                              "view-type",         view_type,
                              "container",         container,
                              "context",           context,
                              "view-size",         view_size,
                              "view-border-width", view_border_width,
                              "menu-factory",      menu_factory,
                              "menu-identifier",   "<Buffers>",
                              "ui-path",           "/buffers-popup",
                              NULL);

  editor = LIGMA_CONTAINER_EDITOR (buffer_view);

  if (LIGMA_IS_CONTAINER_TREE_VIEW (editor->view))
    {
      LigmaContainerTreeView *tree_view;

      tree_view = LIGMA_CONTAINER_TREE_VIEW (editor->view);

      ligma_container_tree_view_connect_name_edited (tree_view,
                                                    G_CALLBACK (ligma_container_tree_view_name_edited),
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
    ligma_view_new_full_by_types (NULL,
                                 LIGMA_TYPE_VIEW,
                                 LIGMA_TYPE_BUFFER,
                                 view_size, view_size, view_border_width,
                                 FALSE, FALSE, TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), buffer_view->clipboard_view,
                      FALSE, FALSE, 0);
  gtk_widget_show (buffer_view->clipboard_view);

  g_signal_connect_object (editor->view, "notify::view-size",
                           G_CALLBACK (ligma_buffer_view_view_notify),
                           buffer_view, 0);
  g_signal_connect_object (editor->view, "notify::view-border-width",
                           G_CALLBACK (ligma_buffer_view_view_notify),
                           buffer_view, 0);

  buffer_view->clipboard_label = gtk_label_new (_("(None)"));
  gtk_box_pack_start (GTK_BOX (hbox), buffer_view->clipboard_label,
                      FALSE, FALSE, 0);
  gtk_widget_show (buffer_view->clipboard_label);

  g_signal_connect_object (context->ligma, "clipboard-changed",
                           G_CALLBACK (ligma_buffer_view_clipboard_changed),
                           G_OBJECT (buffer_view), 0);

  ligma_buffer_view_clipboard_changed (context->ligma, buffer_view);

  buffer_view->paste_button =
    ligma_editor_add_action_button (LIGMA_EDITOR (editor->view), "buffers",
                                   "buffers-paste",
                                   "buffers-paste-in-place",
                                   ligma_get_extend_selection_mask (),
                                   NULL);

  buffer_view->paste_into_button =
    ligma_editor_add_action_button (LIGMA_EDITOR (editor->view), "buffers",
                                   "buffers-paste-into",
                                   "buffers-paste-into-in-place",
                                   ligma_get_extend_selection_mask (),
                                   NULL);

  buffer_view->paste_as_new_layer_button =
    ligma_editor_add_action_button (LIGMA_EDITOR (editor->view), "buffers",
                                   "buffers-paste-as-new-layer",
                                   "buffers-paste-as-new-layer-in-place",
                                   ligma_get_extend_selection_mask (),
                                   NULL);

  buffer_view->paste_as_new_image_button =
    ligma_editor_add_action_button (LIGMA_EDITOR (editor->view), "buffers",
                                   "buffers-paste-as-new-image", NULL);

  buffer_view->delete_button =
    ligma_editor_add_action_button (LIGMA_EDITOR (editor->view), "buffers",
                                   "buffers-delete", NULL);

  ligma_container_view_enable_dnd (editor->view,
                                  GTK_BUTTON (buffer_view->paste_button),
                                  LIGMA_TYPE_BUFFER);
  ligma_container_view_enable_dnd (editor->view,
                                  GTK_BUTTON (buffer_view->paste_into_button),
                                  LIGMA_TYPE_BUFFER);
  ligma_container_view_enable_dnd (editor->view,
                                  GTK_BUTTON (buffer_view->paste_as_new_layer_button),
                                  LIGMA_TYPE_BUFFER);
  ligma_container_view_enable_dnd (editor->view,
                                  GTK_BUTTON (buffer_view->paste_as_new_image_button),
                                  LIGMA_TYPE_BUFFER);
  ligma_container_view_enable_dnd (editor->view,
                                  GTK_BUTTON (buffer_view->delete_button),
                                  LIGMA_TYPE_BUFFER);

  ligma_ui_manager_update (ligma_editor_get_ui_manager (LIGMA_EDITOR (editor->view)),
                          editor);

  return GTK_WIDGET (buffer_view);
}


/*  private functions  */

static void
ligma_buffer_view_activate_item (LigmaContainerEditor *editor,
                                LigmaViewable        *viewable)
{
  LigmaBufferView *view = LIGMA_BUFFER_VIEW (editor);
  LigmaContainer  *container;

  if (LIGMA_CONTAINER_EDITOR_CLASS (parent_class)->activate_item)
    LIGMA_CONTAINER_EDITOR_CLASS (parent_class)->activate_item (editor, viewable);

  container = ligma_container_view_get_container (editor->view);

  if (viewable && ligma_container_have (container, LIGMA_OBJECT (viewable)))
    {
      gtk_button_clicked (GTK_BUTTON (view->paste_button));
    }
}

static void
ligma_buffer_view_clipboard_changed (Ligma           *ligma,
                                    LigmaBufferView *buffer_view)
{
  LigmaBuffer *buffer = ligma_get_clipboard_buffer (ligma);

  ligma_view_set_viewable (LIGMA_VIEW (buffer_view->clipboard_view),
                          LIGMA_VIEWABLE (buffer));

  if (buffer)
    {
      gchar *desc = ligma_viewable_get_description (LIGMA_VIEWABLE (buffer),
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
ligma_buffer_view_view_notify (LigmaContainerView *container_view,
                              GParamSpec        *pspec,
                              LigmaBufferView    *buffer_view)
{
  LigmaView *view = LIGMA_VIEW (buffer_view->clipboard_view);
  gint      view_size;
  gint      view_border_width;

  view_size = ligma_container_view_get_view_size (container_view,
                                                 &view_border_width);

  ligma_view_renderer_set_size_full (view->renderer,
                                    view_size, view_size, view_border_width);
}
