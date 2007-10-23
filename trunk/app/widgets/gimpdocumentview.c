/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdocumentview.c
 * Copyright (C) 2001 Michael Natterer <mitch@gimp.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

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
#include "gimpuimanager.h"
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

  document_view = g_object_new (GIMP_TYPE_DOCUMENT_VIEW, NULL);

  if (! gimp_container_editor_construct (GIMP_CONTAINER_EDITOR (document_view),
                                         view_type,
                                         container, context,
                                         view_size, view_border_width,
                                         menu_factory, "<Documents>",
                                         "/documents-popup"))
    {
      g_object_unref (document_view);
      return NULL;
    }

  editor = GIMP_CONTAINER_EDITOR (document_view);

  document_view->open_button =
    gimp_editor_add_action_button (GIMP_EDITOR (editor->view), "documents",
                                   "documents-open",
                                   "documents-raise-or-open",
                                   GDK_SHIFT_MASK,
                                   "documents-file-open-dialog",
                                   GDK_CONTROL_MASK,
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
                                   GDK_CONTROL_MASK,
                                   NULL);

  if (view_type == GIMP_VIEW_TYPE_LIST)
    {
      GtkWidget *dnd_widget;

      dnd_widget = gimp_container_view_get_dnd_widget (editor->view);

      gimp_dnd_uri_list_source_add (dnd_widget,
                                    gimp_document_view_drag_uri_list,
                                    editor);
    }

  gimp_ui_manager_update (GIMP_EDITOR (editor->view)->ui_manager, editor);

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
      const gchar *uri = gimp_object_get_name (GIMP_OBJECT (viewable));

      return g_list_append (NULL, g_strdup (uri));
    }

  return NULL;
}
