/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdatafactoryview.c
 * Copyright (C) 2001-2003 Michael Natterer <mitch@gimp.org>
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
#include "core/gimpfilteredcontainer.h"
#include "core/gimplist.h"
#include "core/gimpmarshal.h"

#include "gimpcombotagentry.h"
#include "gimpcontainergridview.h"
#include "gimpcontainertreestore.h"
#include "gimpcontainertreeview.h"
#include "gimpcontainerview.h"
#include "gimpdatafactoryview.h"
#include "gimpdnd.h"
#include "gimptagentry.h"
#include "gimpuimanager.h"
#include "gimpviewrenderer.h"
#include "gimpwidgets-utils.h"

#include "gimp-intl.h"


struct _GimpDataFactoryViewPriv
{
  GimpDataFactory *factory;

  GimpContainer   *tag_filtered_container;
  GtkWidget       *query_tag_entry;
  GtkWidget       *assign_tag_entry;
  GList           *selected_items;

  GtkWidget       *edit_button;
  GtkWidget       *new_button;
  GtkWidget       *duplicate_button;
  GtkWidget       *delete_button;
  GtkWidget       *refresh_button;
};


static void   gimp_data_factory_view_activate_item  (GimpContainerEditor *editor,
                                                     GimpViewable        *viewable);
static void   gimp_data_factory_view_select_item    (GimpContainerEditor *editor,
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

  editor_class->select_item   = gimp_data_factory_view_select_item;
  editor_class->activate_item = gimp_data_factory_view_activate_item;

  g_type_class_add_private (klass, sizeof (GimpDataFactoryViewPriv));
}

static void
gimp_data_factory_view_init (GimpDataFactoryView *view)
{
  view->priv = G_TYPE_INSTANCE_GET_PRIVATE (view,
                                            GIMP_TYPE_DATA_FACTORY_VIEW,
                                            GimpDataFactoryViewPriv);

  view->priv->tag_filtered_container = NULL;
  view->priv->query_tag_entry        = NULL;
  view->priv->assign_tag_entry       = NULL;
  view->priv->selected_items         = NULL;
  view->priv->edit_button            = NULL;
  view->priv->new_button             = NULL;
  view->priv->duplicate_button       = NULL;
  view->priv->delete_button          = NULL;
  view->priv->refresh_button         = NULL;
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

GtkWidget *
gimp_data_factory_view_get_edit_button (GimpDataFactoryView *factory_view)
{
  g_return_val_if_fail (GIMP_IS_DATA_FACTORY_VIEW (factory_view), NULL);

  return factory_view->priv->edit_button;
}

GtkWidget *
gimp_data_factory_view_get_duplicate_button (GimpDataFactoryView *factory_view)
{
  g_return_val_if_fail (GIMP_IS_DATA_FACTORY_VIEW (factory_view), NULL);

  return factory_view->priv->duplicate_button;
}

GimpDataFactory *
gimp_data_factory_view_get_data_factory (GimpDataFactoryView *factory_view)
{
  g_return_val_if_fail (GIMP_IS_DATA_FACTORY_VIEW (factory_view), NULL);

  return factory_view->priv->factory;
}

GType
gimp_data_factory_view_get_children_type (GimpDataFactoryView *factory_view)
{
  g_return_val_if_fail (GIMP_IS_DATA_FACTORY_VIEW (factory_view), G_TYPE_NONE);

  return gimp_container_get_children_type (gimp_data_factory_get_container (factory_view->priv->factory));
}

gboolean
gimp_data_factory_view_has_data_new_func (GimpDataFactoryView *factory_view)
{
  g_return_val_if_fail (GIMP_IS_DATA_FACTORY_VIEW (factory_view), FALSE);

  return gimp_data_factory_has_data_new_func (factory_view->priv->factory);
}

gboolean
gimp_data_factory_view_have (GimpDataFactoryView *factory_view,
                             GimpObject          *object)
{
  g_return_val_if_fail (GIMP_IS_DATA_FACTORY_VIEW (factory_view), FALSE);

  return gimp_container_have (gimp_data_factory_get_container (factory_view->priv->factory),
                              object);
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

  factory_view->priv->factory = factory;

  factory_view->priv->tag_filtered_container =
    gimp_filtered_container_new (gimp_data_factory_get_container (factory),
                                 (GCompareFunc) gimp_data_compare);

  if (! gimp_container_editor_construct (GIMP_CONTAINER_EDITOR (factory_view),
                                         view_type,
                                         factory_view->priv->tag_filtered_container, context,
                                         view_size, view_border_width,
                                         menu_factory, menu_identifier,
                                         ui_identifier))
    {
      return FALSE;
    }

  editor = GIMP_CONTAINER_EDITOR (factory_view);

  gimp_container_editor_set_selection_mode (editor, GTK_SELECTION_MULTIPLE);

  if (GIMP_IS_CONTAINER_TREE_VIEW (editor->view))
    {
      GimpContainerTreeView *tree_view;

      tree_view = GIMP_CONTAINER_TREE_VIEW (editor->view);

      gimp_container_tree_view_connect_name_edited (tree_view,
                                                    G_CALLBACK (gimp_data_factory_view_tree_name_edited),
                                                    factory_view);
    }

  str = g_strdup_printf ("%s-edit", action_group);
  factory_view->priv->edit_button =
    gimp_editor_add_action_button (GIMP_EDITOR (editor->view), action_group,
                                   str, NULL);
  g_free (str);

  if (gimp_data_factory_view_has_data_new_func (factory_view))
    {
      str = g_strdup_printf ("%s-new", action_group);
      factory_view->priv->new_button =
        gimp_editor_add_action_button (GIMP_EDITOR (editor->view), action_group,
                                       str, NULL);
      g_free (str);
    }

  str = g_strdup_printf ("%s-duplicate", action_group);
  factory_view->priv->duplicate_button =
    gimp_editor_add_action_button (GIMP_EDITOR (editor->view), action_group,
                                   str, NULL);
  g_free (str);

  str = g_strdup_printf ("%s-delete", action_group);
  factory_view->priv->delete_button =
    gimp_editor_add_action_button (GIMP_EDITOR (editor->view), action_group,
                                   str, NULL);
  g_free (str);

  str = g_strdup_printf ("%s-refresh", action_group);
  factory_view->priv->refresh_button =
    gimp_editor_add_action_button (GIMP_EDITOR (editor->view), action_group,
                                   str, NULL);
  g_free (str);

  /* Query tag entry */
  factory_view->priv->query_tag_entry =
    gimp_combo_tag_entry_new (GIMP_FILTERED_CONTAINER (factory_view->priv->tag_filtered_container),
                              GIMP_TAG_ENTRY_MODE_QUERY);
  gtk_box_pack_start (GTK_BOX (editor->view),
                      factory_view->priv->query_tag_entry,
                      FALSE, FALSE, 0);
  gtk_box_reorder_child (GTK_BOX (editor->view),
                         factory_view->priv->query_tag_entry, 0);
  gtk_widget_show (factory_view->priv->query_tag_entry);

  /* Assign tag entry */
  factory_view->priv->assign_tag_entry =
    gimp_combo_tag_entry_new (GIMP_FILTERED_CONTAINER (factory_view->priv->tag_filtered_container),
                              GIMP_TAG_ENTRY_MODE_ASSIGN);
  gimp_tag_entry_set_selected_items (GIMP_TAG_ENTRY (factory_view->priv->assign_tag_entry),
                                     factory_view->priv->selected_items);
  g_list_free (factory_view->priv->selected_items);
  factory_view->priv->selected_items = NULL;
  gtk_box_pack_start (GTK_BOX (editor->view),
                      factory_view->priv->assign_tag_entry,
                      FALSE, FALSE, 0);
  gtk_widget_show (factory_view->priv->assign_tag_entry);

  gimp_container_view_enable_dnd (editor->view,
                                  GTK_BUTTON (factory_view->priv->edit_button),
                                  gimp_container_get_children_type (gimp_data_factory_get_container (factory)));
  gimp_container_view_enable_dnd (editor->view,
                                  GTK_BUTTON (factory_view->priv->duplicate_button),
                                  gimp_container_get_children_type (gimp_data_factory_get_container (factory)));
  gimp_container_view_enable_dnd (editor->view,
                                  GTK_BUTTON (factory_view->priv->delete_button),
                                  gimp_container_get_children_type (gimp_data_factory_get_container (factory)));

  gimp_ui_manager_update (GIMP_EDITOR (editor->view)->ui_manager, editor);

  return TRUE;
}

static void
gimp_data_factory_view_select_item (GimpContainerEditor *editor,
                                    GimpViewable        *viewable)
{
  GimpDataFactoryView *view = GIMP_DATA_FACTORY_VIEW (editor);

  if (GIMP_CONTAINER_EDITOR_CLASS (parent_class)->select_item)
    GIMP_CONTAINER_EDITOR_CLASS (parent_class)->select_item (editor, viewable);

  if (view->priv->assign_tag_entry)
    {
      GimpContainerView *container_view = GIMP_CONTAINER_VIEW (editor->view);
      GList             *active_items   = NULL;

      gimp_container_view_get_selected (container_view, &active_items);
      gimp_tag_entry_set_selected_items (GIMP_TAG_ENTRY (view->priv->assign_tag_entry),
                                         active_items);
      g_list_free (active_items);
    }
  else if (viewable)
    {
      view->priv->selected_items = g_list_append (view->priv->selected_items,
                                                  viewable);
    }
}

static void
gimp_data_factory_view_activate_item (GimpContainerEditor *editor,
                                      GimpViewable        *viewable)
{
  GimpDataFactoryView *view = GIMP_DATA_FACTORY_VIEW (editor);
  GimpData            *data = GIMP_DATA (viewable);

  if (GIMP_CONTAINER_EDITOR_CLASS (parent_class)->activate_item)
    GIMP_CONTAINER_EDITOR_CLASS (parent_class)->activate_item (editor, viewable);

  if (data && gimp_data_factory_view_have (view,
                                           GIMP_OBJECT (data)))
    {
      if (view->priv->edit_button &&
          gtk_widget_is_sensitive (view->priv->edit_button))
        gtk_button_clicked (GTK_BUTTON (view->priv->edit_button));
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
                          GIMP_CONTAINER_TREE_STORE_COLUMN_RENDERER, &renderer,
                          -1);

      data = GIMP_DATA (renderer->viewable);

      if (! new_name)
        new_name = "";

      name = g_strstrip (g_strdup (new_name));

      if (gimp_data_is_writable (data) && strlen (name))
        {
          gimp_object_take_name (GIMP_OBJECT (data), name);
        }
      else
        {
          g_free (name);

          name = gimp_viewable_get_description (renderer->viewable, NULL);
          gtk_tree_store_set (GTK_TREE_STORE (tree_view->model), &iter,
                              GIMP_CONTAINER_TREE_STORE_COLUMN_NAME, name,
                              -1);
          g_free (name);
        }

      g_object_unref (renderer);
    }

  gtk_tree_path_free (path);
}
