/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimptemplateview.c
 * Copyright (C) 2003 Michael Natterer <mitch@gimp.org>
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

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimptemplate.h"

#include "gimpcontainertreeview.h"
#include "gimpcontainerview.h"
#include "gimptemplateview.h"
#include "gimpdnd.h"
#include "gimphelp-ids.h"
#include "gimpviewrenderer.h"
#include "gimpuimanager.h"

#include "gimp-intl.h"


static void gimp_template_view_activate_item    (GimpContainerEditor *editor,
                                                 GimpViewable        *viewable);
static void gimp_template_view_tree_name_edited (GtkCellRendererText *cell,
                                                 const gchar         *path_str,
                                                 const gchar         *new_name,
                                                 GimpTemplateView    *view);


G_DEFINE_TYPE (GimpTemplateView, gimp_template_view,
               GIMP_TYPE_CONTAINER_EDITOR);

#define parent_class gimp_template_view_parent_class


static void
gimp_template_view_class_init (GimpTemplateViewClass *klass)
{
  GimpContainerEditorClass *editor_class = GIMP_CONTAINER_EDITOR_CLASS (klass);

  editor_class->activate_item = gimp_template_view_activate_item;
}

static void
gimp_template_view_init (GimpTemplateView *view)
{
  view->create_button    = NULL;
  view->new_button       = NULL;
  view->duplicate_button = NULL;
  view->edit_button      = NULL;
  view->delete_button    = NULL;
}

GtkWidget *
gimp_template_view_new (GimpViewType     view_type,
                        GimpContainer   *container,
                        GimpContext     *context,
                        gint             view_size,
                        gint             view_border_width,
                        GimpMenuFactory *menu_factory)
{
  GimpTemplateView    *template_view;
  GimpContainerEditor *editor;

  template_view = g_object_new (GIMP_TYPE_TEMPLATE_VIEW, NULL);

  if (! gimp_container_editor_construct (GIMP_CONTAINER_EDITOR (template_view),
                                         view_type,
                                         container, context,
                                         view_size, view_border_width,
                                         menu_factory, "<Templates>",
                                         "/templates-popup"))
    {
      g_object_unref (template_view);
      return NULL;
    }

  editor = GIMP_CONTAINER_EDITOR (template_view);

  if (GIMP_IS_CONTAINER_TREE_VIEW (editor->view))
    {
      GimpContainerTreeView *tree_view;

      tree_view = GIMP_CONTAINER_TREE_VIEW (editor->view);

      tree_view->name_cell->mode = GTK_CELL_RENDERER_MODE_EDITABLE;
      GTK_CELL_RENDERER_TEXT (tree_view->name_cell)->editable = TRUE;

      tree_view->editable_cells = g_list_prepend (tree_view->editable_cells,
                                                  tree_view->name_cell);

      g_signal_connect (tree_view->name_cell, "edited",
                        G_CALLBACK (gimp_template_view_tree_name_edited),
                        template_view);
    }

  template_view->create_button =
    gimp_editor_add_action_button (GIMP_EDITOR (editor->view), "templates",
                                   "templates-create-image", NULL);

  template_view->new_button =
    gimp_editor_add_action_button (GIMP_EDITOR (editor->view), "templates",
                                   "templates-new", NULL);

  template_view->duplicate_button =
    gimp_editor_add_action_button (GIMP_EDITOR (editor->view), "templates",
                                   "templates-duplicate", NULL);

  template_view->edit_button =
    gimp_editor_add_action_button (GIMP_EDITOR (editor->view), "templates",
                                   "templates-edit", NULL);

  template_view->delete_button =
    gimp_editor_add_action_button (GIMP_EDITOR (editor->view), "templates",
                                   "templates-delete", NULL);

  gimp_container_view_enable_dnd (editor->view,
                                  GTK_BUTTON (template_view->create_button),
                                  GIMP_TYPE_TEMPLATE);
  gimp_container_view_enable_dnd (editor->view,
                                  GTK_BUTTON (template_view->duplicate_button),
                                  GIMP_TYPE_TEMPLATE);
  gimp_container_view_enable_dnd (editor->view,
                                  GTK_BUTTON (template_view->edit_button),
                                  GIMP_TYPE_TEMPLATE);
  gimp_container_view_enable_dnd (editor->view,
                                  GTK_BUTTON (template_view->delete_button),
                                  GIMP_TYPE_TEMPLATE);

  gimp_ui_manager_update (GIMP_EDITOR (editor->view)->ui_manager, editor);

  return GTK_WIDGET (template_view);
}

static void
gimp_template_view_activate_item (GimpContainerEditor *editor,
                                  GimpViewable        *viewable)
{
  GimpTemplateView *view = GIMP_TEMPLATE_VIEW (editor);
  GimpContainer    *container;
  GimpContext      *context;

  if (GIMP_CONTAINER_EDITOR_CLASS (parent_class)->activate_item)
    GIMP_CONTAINER_EDITOR_CLASS (parent_class)->activate_item (editor, viewable);

  container = gimp_container_view_get_container (editor->view);
  context   = gimp_container_view_get_context (editor->view);

  if (viewable && gimp_container_have (container, GIMP_OBJECT (viewable)))
    {
      gtk_button_clicked (GTK_BUTTON (view->create_button));
    }
}

static void
gimp_template_view_tree_name_edited (GtkCellRendererText *cell,
                                     const gchar         *path_str,
                                     const gchar         *new_name,
                                     GimpTemplateView    *view)
{
  GimpContainerTreeView *tree_view;
  GtkTreePath           *path;
  GtkTreeIter            iter;

  tree_view = GIMP_CONTAINER_TREE_VIEW (GIMP_CONTAINER_EDITOR (view)->view);

  path = gtk_tree_path_new_from_string (path_str);

  if (gtk_tree_model_get_iter (tree_view->model, &iter, path))
    {
      GimpViewRenderer *renderer;
      GimpObject       *object;
      const gchar      *old_name;

      gtk_tree_model_get (tree_view->model, &iter,
                          tree_view->model_column_renderer, &renderer,
                          -1);

      object = GIMP_OBJECT (renderer->viewable);

      old_name = gimp_object_get_name (object);

      if (! old_name) old_name = "";
      if (! new_name) new_name = "";

      if (strcmp (old_name, new_name))
        {
          gimp_object_set_name (object, new_name);
        }
      else
        {
          gchar *name = gimp_viewable_get_description (renderer->viewable,
                                                       NULL);

          gtk_list_store_set (GTK_LIST_STORE (tree_view->model), &iter,
                              tree_view->model_column_name, name,
                              -1);
          g_free (name);
        }

      g_object_unref (renderer);
    }

  gtk_tree_path_free (path);
}
