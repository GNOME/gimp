/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdatafactoryview.c
 * Copyright (C) 2001-2003 Michael Natterer <mitch@gimp.org>
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

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "config/gimpbaseconfig.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpdata.h"
#include "core/gimpdatafactory.h"
#include "core/gimpmarshal.h"

#include "gimpcontainerview.h"
#include "gimpdatafactoryview.h"
#include "gimpcontainergridview.h"
#include "gimpcontainertreeview.h"
#include "gimpdnd.h"
#include "gimpviewrenderer.h"
#include "gimpuimanager.h"
#include "gimpwidgets-utils.h"

#include "gimp-intl.h"


static void   gimp_data_factory_view_activate_item  (GimpContainerEditor *editor,
                                                     GimpViewable        *viewable);
static void gimp_data_factory_view_tree_name_edited (GtkCellRendererText *cell,
                                                     const gchar         *path,
                                                     const gchar         *name,
                                                     GimpDataFactoryView *view);


G_DEFINE_TYPE (GimpDataFactoryView, gimp_data_factory_view,
               GIMP_TYPE_CONTAINER_EDITOR)

#define parent_class gimp_data_factory_view_parent_class


static void
gimp_data_factory_view_class_init (GimpDataFactoryViewClass *klass)
{
  GimpContainerEditorClass *editor_class = GIMP_CONTAINER_EDITOR_CLASS (klass);

  editor_class->activate_item = gimp_data_factory_view_activate_item;
}

static void
gimp_data_factory_view_init (GimpDataFactoryView *view)
{
  view->edit_button      = NULL;
  view->new_button       = NULL;
  view->duplicate_button = NULL;
  view->delete_button    = NULL;
  view->refresh_button   = NULL;
}

GtkWidget *
gimp_data_factory_view_new (GimpViewType      view_type,
                            GimpDataFactory  *factory,
                            GimpContext      *context,
                            gint              view_size,
                            gint              view_border_width,
                            GimpMenuFactory  *menu_factory,
                            const gchar      *menu_identifier,
                            const gchar      *ui_identifier,
                            const gchar      *action_group)
{
  GimpDataFactoryView *factory_view;

  factory_view = g_object_new (GIMP_TYPE_DATA_FACTORY_VIEW, NULL);

  if (! gimp_data_factory_view_construct (factory_view,
                                          view_type,
                                          factory,
                                          context,
                                          view_size,
                                          view_border_width,
                                          menu_factory,
                                          menu_identifier,
                                          ui_identifier,
                                          action_group))
    {
      g_object_unref (factory_view);
      return NULL;
    }

  return GTK_WIDGET (factory_view);
}

gboolean
gimp_data_factory_view_construct (GimpDataFactoryView *factory_view,
                                  GimpViewType         view_type,
                                  GimpDataFactory     *factory,
                                  GimpContext         *context,
                                  gint                 view_size,
                                  gint                 view_border_width,
                                  GimpMenuFactory     *menu_factory,
                                  const gchar         *menu_identifier,
                                  const gchar         *ui_identifier,
                                  const gchar         *action_group)
{
  GimpContainerEditor *editor;
  gchar               *str;

  g_return_val_if_fail (GIMP_IS_DATA_FACTORY_VIEW (factory_view), FALSE);
  g_return_val_if_fail (GIMP_IS_DATA_FACTORY (factory), FALSE);
  g_return_val_if_fail (view_size >  0 &&
                        view_size <= GIMP_VIEWABLE_MAX_PREVIEW_SIZE, FALSE);
  g_return_val_if_fail (view_border_width >= 0 &&
                        view_border_width <= GIMP_VIEW_MAX_BORDER_WIDTH,
                        FALSE);

  factory_view->factory = factory;

  if (! gimp_container_editor_construct (GIMP_CONTAINER_EDITOR (factory_view),
                                         view_type,
                                         factory->container, context,
                                         view_size, view_border_width,
                                         menu_factory, menu_identifier,
                                         ui_identifier))
    {
      return FALSE;
    }

  editor = GIMP_CONTAINER_EDITOR (factory_view);

  if (GIMP_IS_CONTAINER_TREE_VIEW (editor->view))
    {
      GimpContainerTreeView *tree_view;

      tree_view = GIMP_CONTAINER_TREE_VIEW (editor->view);

      tree_view->name_cell->mode = GTK_CELL_RENDERER_MODE_EDITABLE;
      GTK_CELL_RENDERER_TEXT (tree_view->name_cell)->editable = TRUE;

      tree_view->editable_cells = g_list_prepend (tree_view->editable_cells,
                                                  tree_view->name_cell);

      g_signal_connect (tree_view->name_cell, "edited",
                        G_CALLBACK (gimp_data_factory_view_tree_name_edited),
                        factory_view);
    }

  str = g_strdup_printf ("%s-edit", action_group);
  factory_view->edit_button =
    gimp_editor_add_action_button (GIMP_EDITOR (editor->view), action_group,
                                   str, NULL);
  g_free (str);

  if (factory_view->factory->data_new_func)
    {
      str = g_strdup_printf ("%s-new", action_group);
      factory_view->new_button =
        gimp_editor_add_action_button (GIMP_EDITOR (editor->view), action_group,
                                       str, NULL);
      g_free (str);
    }

  str = g_strdup_printf ("%s-duplicate", action_group);
  factory_view->duplicate_button =
    gimp_editor_add_action_button (GIMP_EDITOR (editor->view), action_group,
                                   str, NULL);
  g_free (str);

  str = g_strdup_printf ("%s-delete", action_group);
  factory_view->delete_button =
    gimp_editor_add_action_button (GIMP_EDITOR (editor->view), action_group,
                                   str, NULL);
  g_free (str);

  str = g_strdup_printf ("%s-refresh", action_group);
  factory_view->refresh_button =
    gimp_editor_add_action_button (GIMP_EDITOR (editor->view), action_group,
                                   str, NULL);
  g_free (str);

  gimp_container_view_enable_dnd (editor->view,
                                  GTK_BUTTON (factory_view->edit_button),
                                  factory->container->children_type);
  gimp_container_view_enable_dnd (editor->view,
                                  GTK_BUTTON (factory_view->duplicate_button),
                                  factory->container->children_type);
  gimp_container_view_enable_dnd (editor->view,
                                  GTK_BUTTON (factory_view->delete_button),
                                  factory->container->children_type);

  gimp_ui_manager_update (GIMP_EDITOR (editor->view)->ui_manager, editor);

  return TRUE;
}

static void
gimp_data_factory_view_activate_item (GimpContainerEditor *editor,
                                      GimpViewable        *viewable)
{
  GimpDataFactoryView *view = GIMP_DATA_FACTORY_VIEW (editor);
  GimpData            *data = GIMP_DATA (viewable);

  if (GIMP_CONTAINER_EDITOR_CLASS (parent_class)->activate_item)
    GIMP_CONTAINER_EDITOR_CLASS (parent_class)->activate_item (editor, viewable);

  if (data && gimp_container_have (view->factory->container,
                                   GIMP_OBJECT (data)))
    {
      if (view->edit_button && GTK_WIDGET_SENSITIVE (view->edit_button))
        gtk_button_clicked (GTK_BUTTON (view->edit_button));
    }
}

static void
gimp_data_factory_view_tree_name_edited (GtkCellRendererText *cell,
                                         const gchar         *path_str,
                                         const gchar         *new_name,
                                         GimpDataFactoryView *view)
{
  GimpContainerTreeView *tree_view;
  GtkTreePath           *path;
  GtkTreeIter            iter;

  tree_view = GIMP_CONTAINER_TREE_VIEW (GIMP_CONTAINER_EDITOR (view)->view);

  path = gtk_tree_path_new_from_string (path_str);

  if (gtk_tree_model_get_iter (tree_view->model, &iter, path))
    {
      GimpViewRenderer *renderer;
      GimpData         *data;
      gchar            *name;

      gtk_tree_model_get (tree_view->model, &iter,
                          tree_view->model_column_renderer, &renderer,
                          -1);

      data = GIMP_DATA (renderer->viewable);

      if (! new_name)
        new_name = "";

      name = g_strstrip (g_strdup (new_name));

      if (data->writable && strlen (name))
        {
          gimp_object_take_name (GIMP_OBJECT (data), name);
        }
      else
        {
          g_free (name);

          name = gimp_viewable_get_description (renderer->viewable, NULL);
          gtk_list_store_set (GTK_LIST_STORE (tree_view->model), &iter,
                              tree_view->model_column_name, name,
                              -1);
          g_free (name);
        }

      g_object_unref (renderer);
    }

  gtk_tree_path_free (path);
}
