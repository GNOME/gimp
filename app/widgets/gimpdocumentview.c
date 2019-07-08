/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdocumentview.c
 * Copyright (C) 2001 Michael Natterer <mitch@gimp.org>
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
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpimagefile.h"

#include "gimpcontainerview.h"
#include "gimpdocumentview.h"
#include "gimpdnd.h"
#include "gimpeditor.h"
#include "gimpmenufactory.h"
#include "gimpuimanager.h"
#include "gimpviewrenderer.h"
#include "gimpwidgets-utils.h"

#include "gimp-intl.h"


static void    gimp_document_view_activate_item (GimpContainerEditor *editor,
                                                 GimpViewable        *viewable);
static GList * gimp_document_view_drag_uri_list (GtkWidget           *widget,
                                                 gpointer             data);


G_DEFINE_TYPE (GimpDocumentView, gimp_document_view,
               GIMP_TYPE_CONTAINER_EDITOR)

#define parent_class gimp_document_view_parent_class


static void
gimp_document_view_class_init (GimpDocumentViewClass *klass)
{
  GimpContainerEditorClass *editor_class = GIMP_CONTAINER_EDITOR_CLASS (klass);

  editor_class->activate_item = gimp_document_view_activate_item;
}

static void
gimp_document_view_init (GimpDocumentView *view)
{
  view->open_button    = NULL;
  view->remove_button  = NULL;
  view->refresh_button = NULL;
}

GtkWidget *
gimp_document_view_new (GimpViewType     view_type,
                        GimpContainer   *container,
                        GimpContext     *context,
                        gint             view_size,
                        gint             view_border_width,
                        GimpMenuFactory *menu_factory)
{
  GimpDocumentView    *document_view;
  GimpContainerEditor *editor;

  g_return_val_if_fail (GIMP_IS_CONTAINER (container), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (view_size > 0 &&
                        view_size <= GIMP_VIEWABLE_MAX_PREVIEW_SIZE, FALSE);
  g_return_val_if_fail (view_border_width >= 0 &&
                        view_border_width <= GIMP_VIEW_MAX_BORDER_WIDTH,
                        FALSE);
  g_return_val_if_fail (menu_factory == NULL ||
                        GIMP_IS_MENU_FACTORY (menu_factory), NULL);

  document_view = g_object_new (GIMP_TYPE_DOCUMENT_VIEW,
                                "view-type",         view_type,
                                "container",         container,
                                "context",           context,
                                "view-size",         view_size,
                                "view-border-width", view_border_width,
                                "menu-factory",      menu_factory,
                                "menu-identifier",   "<Documents>",
                                "ui-path",           "/documents-popup",
                                NULL);

  editor = GIMP_CONTAINER_EDITOR (document_view);

  document_view->open_button =
    gimp_editor_add_action_button (GIMP_EDITOR (editor->view), "documents",
                                   "documents-open",
                                   "documents-raise-or-open",
                                   GDK_SHIFT_MASK,
                                   "documents-file-open-dialog",
                                   gimp_get_toggle_behavior_mask (),
                                   NULL);
  gimp_container_view_enable_dnd (editor->view,
                                  GTK_BUTTON (document_view->open_button),
                                  GIMP_TYPE_IMAGEFILE);

  document_view->remove_button =
    gimp_editor_add_action_button (GIMP_EDITOR (editor->view), "documents",
                                   "documents-remove", NULL);
  gimp_container_view_enable_dnd (editor->view,
                                  GTK_BUTTON (document_view->remove_button),
                                  GIMP_TYPE_IMAGEFILE);

  gimp_editor_add_action_button (GIMP_EDITOR (editor->view), "documents",
                                 "documents-clear", NULL);

  document_view->refresh_button =
    gimp_editor_add_action_button (GIMP_EDITOR (editor->view), "documents",
                                   "documents-recreate-preview",
                                   "documents-reload-previews",
                                   GDK_SHIFT_MASK,
                                   "documents-remove-dangling",
                                   gimp_get_toggle_behavior_mask (),
                                   NULL);

  if (view_type == GIMP_VIEW_TYPE_LIST)
    {
      GtkWidget *dnd_widget;

      dnd_widget = gimp_container_view_get_dnd_widget (editor->view);

      gimp_dnd_uri_list_source_add (dnd_widget,
                                    gimp_document_view_drag_uri_list,
                                    editor);
    }

  gimp_ui_manager_update (gimp_editor_get_ui_manager (GIMP_EDITOR (editor->view)),
                          editor);

  return GTK_WIDGET (document_view);
}

static void
gimp_document_view_activate_item (GimpContainerEditor *editor,
                                  GimpViewable        *viewable)
{
  GimpDocumentView *view = GIMP_DOCUMENT_VIEW (editor);
  GimpContainer    *container;

  if (GIMP_CONTAINER_EDITOR_CLASS (parent_class)->activate_item)
    GIMP_CONTAINER_EDITOR_CLASS (parent_class)->activate_item (editor, viewable);

  container = gimp_container_view_get_container (editor->view);

  if (viewable && gimp_container_have (container, GIMP_OBJECT (viewable)))
    {
      gtk_button_clicked (GTK_BUTTON (view->open_button));
    }
}

static GList *
gimp_document_view_drag_uri_list (GtkWidget *widget,
                                  gpointer   data)
{
  GimpViewable *viewable = gimp_dnd_get_drag_data (widget);

  if (viewable)
    {
      const gchar *uri = gimp_object_get_name (viewable);

      return g_list_append (NULL, g_strdup (uri));
    }

  return NULL;
}
