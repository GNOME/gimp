/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmadocumentview.c
 * Copyright (C) 2001 Michael Natterer <mitch@ligma.org>
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
#include "core/ligmacontainer.h"
#include "core/ligmacontext.h"
#include "core/ligmaimagefile.h"

#include "ligmacontainerview.h"
#include "ligmadocumentview.h"
#include "ligmadnd.h"
#include "ligmaeditor.h"
#include "ligmamenufactory.h"
#include "ligmauimanager.h"
#include "ligmaviewrenderer.h"
#include "ligmawidgets-utils.h"

#include "ligma-intl.h"


static void    ligma_document_view_activate_item (LigmaContainerEditor *editor,
                                                 LigmaViewable        *viewable);
static GList * ligma_document_view_drag_uri_list (GtkWidget           *widget,
                                                 gpointer             data);


G_DEFINE_TYPE (LigmaDocumentView, ligma_document_view,
               LIGMA_TYPE_CONTAINER_EDITOR)

#define parent_class ligma_document_view_parent_class


static void
ligma_document_view_class_init (LigmaDocumentViewClass *klass)
{
  LigmaContainerEditorClass *editor_class = LIGMA_CONTAINER_EDITOR_CLASS (klass);

  editor_class->activate_item = ligma_document_view_activate_item;
}

static void
ligma_document_view_init (LigmaDocumentView *view)
{
  view->open_button    = NULL;
  view->remove_button  = NULL;
  view->refresh_button = NULL;
}

GtkWidget *
ligma_document_view_new (LigmaViewType     view_type,
                        LigmaContainer   *container,
                        LigmaContext     *context,
                        gint             view_size,
                        gint             view_border_width,
                        LigmaMenuFactory *menu_factory)
{
  LigmaDocumentView    *document_view;
  LigmaContainerEditor *editor;

  g_return_val_if_fail (LIGMA_IS_CONTAINER (container), NULL);
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (view_size > 0 &&
                        view_size <= LIGMA_VIEWABLE_MAX_PREVIEW_SIZE, FALSE);
  g_return_val_if_fail (view_border_width >= 0 &&
                        view_border_width <= LIGMA_VIEW_MAX_BORDER_WIDTH,
                        FALSE);
  g_return_val_if_fail (menu_factory == NULL ||
                        LIGMA_IS_MENU_FACTORY (menu_factory), NULL);

  document_view = g_object_new (LIGMA_TYPE_DOCUMENT_VIEW,
                                "view-type",         view_type,
                                "container",         container,
                                "context",           context,
                                "view-size",         view_size,
                                "view-border-width", view_border_width,
                                "menu-factory",      menu_factory,
                                "menu-identifier",   "<Documents>",
                                "ui-path",           "/documents-popup",
                                NULL);

  editor = LIGMA_CONTAINER_EDITOR (document_view);

  document_view->open_button =
    ligma_editor_add_action_button (LIGMA_EDITOR (editor->view), "documents",
                                   "documents-open",
                                   "documents-raise-or-open",
                                   GDK_SHIFT_MASK,
                                   "documents-file-open-dialog",
                                   ligma_get_toggle_behavior_mask (),
                                   NULL);
  ligma_container_view_enable_dnd (editor->view,
                                  GTK_BUTTON (document_view->open_button),
                                  LIGMA_TYPE_IMAGEFILE);

  document_view->remove_button =
    ligma_editor_add_action_button (LIGMA_EDITOR (editor->view), "documents",
                                   "documents-remove", NULL);
  ligma_container_view_enable_dnd (editor->view,
                                  GTK_BUTTON (document_view->remove_button),
                                  LIGMA_TYPE_IMAGEFILE);

  ligma_editor_add_action_button (LIGMA_EDITOR (editor->view), "documents",
                                 "documents-clear", NULL);

  document_view->refresh_button =
    ligma_editor_add_action_button (LIGMA_EDITOR (editor->view), "documents",
                                   "documents-recreate-preview",
                                   "documents-reload-previews",
                                   GDK_SHIFT_MASK,
                                   "documents-remove-dangling",
                                   ligma_get_toggle_behavior_mask (),
                                   NULL);

  if (view_type == LIGMA_VIEW_TYPE_LIST)
    {
      GtkWidget *dnd_widget;

      dnd_widget = ligma_container_view_get_dnd_widget (editor->view);

      ligma_dnd_uri_list_source_add (dnd_widget,
                                    ligma_document_view_drag_uri_list,
                                    editor);
    }

  ligma_ui_manager_update (ligma_editor_get_ui_manager (LIGMA_EDITOR (editor->view)),
                          editor);

  return GTK_WIDGET (document_view);
}

static void
ligma_document_view_activate_item (LigmaContainerEditor *editor,
                                  LigmaViewable        *viewable)
{
  LigmaDocumentView *view = LIGMA_DOCUMENT_VIEW (editor);
  LigmaContainer    *container;

  if (LIGMA_CONTAINER_EDITOR_CLASS (parent_class)->activate_item)
    LIGMA_CONTAINER_EDITOR_CLASS (parent_class)->activate_item (editor, viewable);

  container = ligma_container_view_get_container (editor->view);

  if (viewable && ligma_container_have (container, LIGMA_OBJECT (viewable)))
    {
      gtk_button_clicked (GTK_BUTTON (view->open_button));
    }
}

static GList *
ligma_document_view_drag_uri_list (GtkWidget *widget,
                                  gpointer   data)
{
  LigmaViewable *viewable = ligma_dnd_get_drag_viewable (widget);

  if (viewable)
    {
      const gchar *uri = ligma_object_get_name (viewable);

      return g_list_append (NULL, g_strdup (uri));
    }

  return NULL;
}
