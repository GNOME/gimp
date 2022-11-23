/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmadatafactoryview.c
 * Copyright (C) 2001-2003 Michael Natterer <mitch@ligma.org>
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

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmabase/ligmabase.h"
#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "core/ligma.h"
#include "core/ligmacontainer.h"
#include "core/ligmacontext.h"
#include "core/ligmadata.h"
#include "core/ligmadatafactory.h"
#include "core/ligmalist.h"
#include "core/ligmataggedcontainer.h"

#include "ligmacombotagentry.h"
#include "ligmacontainertreestore.h"
#include "ligmacontainertreeview.h"
#include "ligmacontainerview.h"
#include "ligmadatafactoryview.h"
#include "ligmadnd.h"
#include "ligmadocked.h"
#include "ligmamenufactory.h"
#include "ligmasessioninfo-aux.h"
#include "ligmatagentry.h"
#include "ligmauimanager.h"
#include "ligmaviewrenderer.h"
#include "ligmawidgets-utils.h"

#include "ligma-intl.h"


enum
{
  PROP_0,
  PROP_DATA_FACTORY,
  PROP_ACTION_GROUP
};


struct _LigmaDataFactoryViewPrivate
{
  LigmaDataFactory *factory;
  gchar           *action_group;

  LigmaContainer   *tagged_container;
  GtkWidget       *query_tag_entry;
  GtkWidget       *assign_tag_entry;
  GList           *selected_items;

  GtkWidget       *edit_button;
  GtkWidget       *new_button;
  GtkWidget       *duplicate_button;
  GtkWidget       *delete_button;
  GtkWidget       *refresh_button;
};


static void    ligma_data_factory_view_docked_iface_init (LigmaDockedInterface *iface);

static GObject *
               ligma_data_factory_view_constructor    (GType                type,
                                                      guint                n_construct_params,
                                                      GObjectConstructParam *construct_params);
static void    ligma_data_factory_view_constructed    (GObject             *object);
static void    ligma_data_factory_view_dispose        (GObject             *object);
static void    ligma_data_factory_view_set_property   (GObject             *object,
                                                      guint                property_id,
                                                      const GValue        *value,
                                                      GParamSpec          *pspec);
static void    ligma_data_factory_view_get_property   (GObject             *object,
                                                      guint                property_id,
                                                      GValue              *value,
                                                      GParamSpec          *pspec);

static void    ligma_data_factory_view_set_aux_info   (LigmaDocked          *docked,
                                                      GList               *aux_info);
static GList * ligma_data_factory_view_get_aux_info   (LigmaDocked          *docked);

static void    ligma_data_factory_view_activate_item  (LigmaContainerEditor *editor,
                                                      LigmaViewable        *viewable);
static void    ligma_data_factory_view_select_item    (LigmaContainerEditor *editor,
                                                      LigmaViewable        *viewable);
static void  ligma_data_factory_view_tree_name_edited (GtkCellRendererText *cell,
                                                      const gchar         *path,
                                                      const gchar         *name,
                                                      LigmaDataFactoryView *view);


G_DEFINE_TYPE_WITH_CODE (LigmaDataFactoryView, ligma_data_factory_view,
                         LIGMA_TYPE_CONTAINER_EDITOR,
                         G_ADD_PRIVATE (LigmaDataFactoryView)
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_DOCKED,
                                                ligma_data_factory_view_docked_iface_init))

#define parent_class ligma_data_factory_view_parent_class

static LigmaDockedInterface *parent_docked_iface = NULL;


static void
ligma_data_factory_view_class_init (LigmaDataFactoryViewClass *klass)
{
  GObjectClass             *object_class = G_OBJECT_CLASS (klass);
  LigmaContainerEditorClass *editor_class = LIGMA_CONTAINER_EDITOR_CLASS (klass);

  object_class->constructor   = ligma_data_factory_view_constructor;
  object_class->constructed   = ligma_data_factory_view_constructed;
  object_class->dispose       = ligma_data_factory_view_dispose;
  object_class->set_property  = ligma_data_factory_view_set_property;
  object_class->get_property  = ligma_data_factory_view_get_property;

  editor_class->select_item   = ligma_data_factory_view_select_item;
  editor_class->activate_item = ligma_data_factory_view_activate_item;

  g_object_class_install_property (object_class, PROP_DATA_FACTORY,
                                   g_param_spec_object ("data-factory",
                                                        NULL, NULL,
                                                        LIGMA_TYPE_DATA_FACTORY,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_ACTION_GROUP,
                                   g_param_spec_string ("action-group",
                                                        NULL, NULL,
                                                        NULL,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
ligma_data_factory_view_init (LigmaDataFactoryView *view)
{
  view->priv = ligma_data_factory_view_get_instance_private (view);

  view->priv->tagged_container = NULL;
  view->priv->query_tag_entry  = NULL;
  view->priv->assign_tag_entry = NULL;
  view->priv->selected_items   = NULL;
  view->priv->edit_button      = NULL;
  view->priv->new_button       = NULL;
  view->priv->duplicate_button = NULL;
  view->priv->delete_button    = NULL;
  view->priv->refresh_button   = NULL;
}

static void
ligma_data_factory_view_docked_iface_init (LigmaDockedInterface *iface)
{
  parent_docked_iface = g_type_interface_peek_parent (iface);

  if (! parent_docked_iface)
    parent_docked_iface = g_type_default_interface_peek (LIGMA_TYPE_DOCKED);

  iface->set_aux_info = ligma_data_factory_view_set_aux_info;
  iface->get_aux_info = ligma_data_factory_view_get_aux_info;
}

static GObject *
ligma_data_factory_view_constructor (GType                  type,
                                    guint                  n_construct_params,
                                    GObjectConstructParam *construct_params)
{
  LigmaDataFactoryView *factory_view;
  GObject             *object;

  object = G_OBJECT_CLASS (parent_class)->constructor (type,
                                                       n_construct_params,
                                                       construct_params);

  factory_view = LIGMA_DATA_FACTORY_VIEW (object);

  ligma_assert (LIGMA_IS_DATA_FACTORY (factory_view->priv->factory));
  ligma_assert (factory_view->priv->action_group != NULL);

  factory_view->priv->tagged_container =
    ligma_tagged_container_new (ligma_data_factory_get_container (factory_view->priv->factory));

  /* this must happen in constructor(), because doing it in
   * set_property() warns about wrong construct property usage
   */
  g_object_set (object,
                "container", factory_view->priv->tagged_container,
                NULL);

  return object;
}

static void
ligma_data_factory_view_constructed (GObject *object)
{
  LigmaDataFactoryView        *factory_view = LIGMA_DATA_FACTORY_VIEW (object);
  LigmaDataFactoryViewPrivate *priv         = factory_view->priv;
  LigmaContainerEditor        *editor       = LIGMA_CONTAINER_EDITOR (object);
  LigmaUIManager              *manager;
  gchar                      *str;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  ligma_container_editor_set_selection_mode (editor, GTK_SELECTION_MULTIPLE);

  if (LIGMA_IS_CONTAINER_TREE_VIEW (editor->view))
    {
      LigmaContainerTreeView *tree_view;

      tree_view = LIGMA_CONTAINER_TREE_VIEW (editor->view);

      ligma_container_tree_view_connect_name_edited (tree_view,
                                                    G_CALLBACK (ligma_data_factory_view_tree_name_edited),
                                                    factory_view);
    }

  manager = ligma_editor_get_ui_manager (LIGMA_EDITOR (editor->view));

  str = g_strdup_printf ("%s-edit", priv->action_group);
  if (ligma_ui_manager_find_action (manager, priv->action_group, str))
    priv->edit_button =
      ligma_editor_add_action_button (LIGMA_EDITOR (editor->view),
                                     priv->action_group,
                                     str, NULL);
  g_free (str);

  if (ligma_data_factory_view_has_data_new_func (factory_view))
    {
      str = g_strdup_printf ("%s-new", priv->action_group);
      priv->new_button =
        ligma_editor_add_action_button (LIGMA_EDITOR (editor->view),
                                       priv->action_group,
                                       str, NULL);
      g_free (str);
    }

  str = g_strdup_printf ("%s-duplicate", priv->action_group);
  if (ligma_ui_manager_find_action (manager, priv->action_group, str))
    priv->duplicate_button =
      ligma_editor_add_action_button (LIGMA_EDITOR (editor->view),
                                     priv->action_group,
                                     str, NULL);
  g_free (str);

  str = g_strdup_printf ("%s-delete", priv->action_group);
  if (ligma_ui_manager_find_action (manager, priv->action_group, str))
    priv->delete_button =
      ligma_editor_add_action_button (LIGMA_EDITOR (editor->view),
                                     priv->action_group,
                                     str, NULL);
  g_free (str);

  str = g_strdup_printf ("%s-refresh", priv->action_group);
  if (ligma_ui_manager_find_action (manager, priv->action_group, str))
    priv->refresh_button =
      ligma_editor_add_action_button (LIGMA_EDITOR (editor->view),
                                     priv->action_group,
                                     str, NULL);
  g_free (str);

  /* Query tag entry */
  priv->query_tag_entry =
    ligma_combo_tag_entry_new (LIGMA_TAGGED_CONTAINER (priv->tagged_container),
                              LIGMA_TAG_ENTRY_MODE_QUERY);
  gtk_box_pack_start (GTK_BOX (editor->view),
                      priv->query_tag_entry,
                      FALSE, FALSE, 0);
  gtk_box_reorder_child (GTK_BOX (editor->view),
                         priv->query_tag_entry, 0);
  gtk_widget_show (priv->query_tag_entry);

  /* Assign tag entry */
  priv->assign_tag_entry =
    ligma_combo_tag_entry_new (LIGMA_TAGGED_CONTAINER (priv->tagged_container),
                              LIGMA_TAG_ENTRY_MODE_ASSIGN);
  ligma_tag_entry_set_selected_items (LIGMA_TAG_ENTRY (priv->assign_tag_entry),
                                     priv->selected_items);
  g_list_free (priv->selected_items);
  priv->selected_items = NULL;
  gtk_box_pack_start (GTK_BOX (editor->view),
                      priv->assign_tag_entry,
                      FALSE, FALSE, 0);
  gtk_widget_show (priv->assign_tag_entry);

  if (priv->edit_button)
    ligma_container_view_enable_dnd (editor->view,
                                    GTK_BUTTON (priv->edit_button),
                                    ligma_data_factory_get_data_type (priv->factory));

  if (priv->duplicate_button)
    ligma_container_view_enable_dnd (editor->view,
                                    GTK_BUTTON (priv->duplicate_button),
                                    ligma_data_factory_get_data_type (priv->factory));

  if (priv->delete_button)
    ligma_container_view_enable_dnd (editor->view,
                                    GTK_BUTTON (priv->delete_button),
                                    ligma_data_factory_get_data_type (priv->factory));

  ligma_ui_manager_update (manager, editor);
}

static void
ligma_data_factory_view_dispose (GObject *object)
{
  LigmaDataFactoryView *factory_view = LIGMA_DATA_FACTORY_VIEW (object);

  g_clear_object (&factory_view->priv->tagged_container);
  g_clear_object (&factory_view->priv->factory);

  g_clear_pointer (&factory_view->priv->action_group, g_free);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
ligma_data_factory_view_set_property (GObject      *object,
                                     guint         property_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  LigmaDataFactoryView *factory_view = LIGMA_DATA_FACTORY_VIEW (object);

  switch (property_id)
    {
    case PROP_DATA_FACTORY:
      factory_view->priv->factory = g_value_dup_object (value);
      break;

    case PROP_ACTION_GROUP:
      factory_view->priv->action_group = g_value_dup_string (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_data_factory_view_get_property (GObject    *object,
                                     guint       property_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  LigmaDataFactoryView *factory_view = LIGMA_DATA_FACTORY_VIEW (object);

  switch (property_id)
    {
    case PROP_DATA_FACTORY:
      g_value_set_object (value, factory_view->priv->factory);
      break;

    case PROP_ACTION_GROUP:
      g_value_set_string (value, factory_view->priv->action_group);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

#define AUX_INFO_TAG_FILTER "tag-filter"

static void
ligma_data_factory_view_set_aux_info (LigmaDocked *docked,
                                     GList      *aux_info)
{
  LigmaDataFactoryView *view = LIGMA_DATA_FACTORY_VIEW (docked);
  GList               *list;

  parent_docked_iface->set_aux_info (docked, aux_info);

  for (list = aux_info; list; list = g_list_next (list))
    {
      LigmaSessionInfoAux *aux = list->data;

      if (! strcmp (aux->name, AUX_INFO_TAG_FILTER))
        {
          gtk_entry_set_text (GTK_ENTRY (view->priv->query_tag_entry),
                              aux->value);
        }
    }
}

static GList *
ligma_data_factory_view_get_aux_info (LigmaDocked *docked)
{
  LigmaDataFactoryView *view = LIGMA_DATA_FACTORY_VIEW (docked);
  GList               *aux_info;
  const gchar         *tag_filter;

  aux_info = parent_docked_iface->get_aux_info (docked);

  tag_filter = gtk_entry_get_text (GTK_ENTRY (view->priv->query_tag_entry));
  if (tag_filter && *tag_filter)
    {
      LigmaSessionInfoAux *aux;

      aux = ligma_session_info_aux_new (AUX_INFO_TAG_FILTER, tag_filter);
      aux_info = g_list_append (aux_info, aux);
    }

  return aux_info;
}

GtkWidget *
ligma_data_factory_view_new (LigmaViewType      view_type,
                            LigmaDataFactory  *factory,
                            LigmaContext      *context,
                            gint              view_size,
                            gint              view_border_width,
                            LigmaMenuFactory  *menu_factory,
                            const gchar      *menu_identifier,
                            const gchar      *ui_path,
                            const gchar      *action_group)
{
  g_return_val_if_fail (LIGMA_IS_DATA_FACTORY (factory), NULL);
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (view_size > 0 &&
                        view_size <= LIGMA_VIEWABLE_MAX_PREVIEW_SIZE, NULL);
  g_return_val_if_fail (view_border_width >= 0 &&
                        view_border_width <= LIGMA_VIEW_MAX_BORDER_WIDTH,
                        NULL);
  g_return_val_if_fail (menu_factory == NULL ||
                        LIGMA_IS_MENU_FACTORY (menu_factory), NULL);
  g_return_val_if_fail (action_group != NULL, NULL);

  return g_object_new (LIGMA_TYPE_DATA_FACTORY_VIEW,
                       "view-type",         view_type,
                       "data-factory",      factory,
                       "context",           context,
                       "view-size",         view_size,
                       "view-border-width", view_border_width,
                       "menu-factory",      menu_factory,
                       "menu-identifier",   menu_identifier,
                       "ui-path",           ui_path,
                       "action-group",      action_group,
                       NULL);
}

GtkWidget *
ligma_data_factory_view_get_edit_button (LigmaDataFactoryView *factory_view)
{
  g_return_val_if_fail (LIGMA_IS_DATA_FACTORY_VIEW (factory_view), NULL);

  return factory_view->priv->edit_button;
}

GtkWidget *
ligma_data_factory_view_get_duplicate_button (LigmaDataFactoryView *factory_view)
{
  g_return_val_if_fail (LIGMA_IS_DATA_FACTORY_VIEW (factory_view), NULL);

  return factory_view->priv->duplicate_button;
}

LigmaDataFactory *
ligma_data_factory_view_get_data_factory (LigmaDataFactoryView *factory_view)
{
  g_return_val_if_fail (LIGMA_IS_DATA_FACTORY_VIEW (factory_view), NULL);

  return factory_view->priv->factory;
}

GType
ligma_data_factory_view_get_children_type (LigmaDataFactoryView *factory_view)
{
  g_return_val_if_fail (LIGMA_IS_DATA_FACTORY_VIEW (factory_view), G_TYPE_NONE);

  return ligma_data_factory_get_data_type (factory_view->priv->factory);
}

gboolean
ligma_data_factory_view_has_data_new_func (LigmaDataFactoryView *factory_view)
{
  g_return_val_if_fail (LIGMA_IS_DATA_FACTORY_VIEW (factory_view), FALSE);

  return ligma_data_factory_has_data_new_func (factory_view->priv->factory);
}

gboolean
ligma_data_factory_view_have (LigmaDataFactoryView *factory_view,
                             LigmaObject          *object)
{
  g_return_val_if_fail (LIGMA_IS_DATA_FACTORY_VIEW (factory_view), FALSE);

  return ligma_container_have (ligma_data_factory_get_container (factory_view->priv->factory),
                              object);
}

static void
ligma_data_factory_view_select_item (LigmaContainerEditor *editor,
                                    LigmaViewable        *viewable)
{
  LigmaDataFactoryView *view = LIGMA_DATA_FACTORY_VIEW (editor);

  if (LIGMA_CONTAINER_EDITOR_CLASS (parent_class)->select_item)
    LIGMA_CONTAINER_EDITOR_CLASS (parent_class)->select_item (editor, viewable);

  if (view->priv->assign_tag_entry)
    {
      LigmaContainerView *container_view = LIGMA_CONTAINER_VIEW (editor->view);
      GList             *active_items   = NULL;

      ligma_container_view_get_selected (container_view, &active_items, NULL);
      ligma_tag_entry_set_selected_items (LIGMA_TAG_ENTRY (view->priv->assign_tag_entry),
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
ligma_data_factory_view_activate_item (LigmaContainerEditor *editor,
                                      LigmaViewable        *viewable)
{
  LigmaDataFactoryView *view = LIGMA_DATA_FACTORY_VIEW (editor);
  LigmaData            *data = LIGMA_DATA (viewable);

  if (LIGMA_CONTAINER_EDITOR_CLASS (parent_class)->activate_item)
    LIGMA_CONTAINER_EDITOR_CLASS (parent_class)->activate_item (editor, viewable);

  if (data && ligma_data_factory_view_have (view,
                                           LIGMA_OBJECT (data)))
    {
      if (view->priv->edit_button &&
          gtk_widget_is_sensitive (view->priv->edit_button))
        gtk_button_clicked (GTK_BUTTON (view->priv->edit_button));
    }
}

static void
ligma_data_factory_view_tree_name_edited (GtkCellRendererText *cell,
                                         const gchar         *path_str,
                                         const gchar         *new_name,
                                         LigmaDataFactoryView *view)
{
  LigmaContainerTreeView *tree_view;
  GtkTreePath           *path;
  GtkTreeIter            iter;

  tree_view = LIGMA_CONTAINER_TREE_VIEW (LIGMA_CONTAINER_EDITOR (view)->view);

  path = gtk_tree_path_new_from_string (path_str);

  if (gtk_tree_model_get_iter (tree_view->model, &iter, path))
    {
      LigmaViewRenderer *renderer;
      LigmaData         *data;
      gchar            *name;

      gtk_tree_model_get (tree_view->model, &iter,
                          LIGMA_CONTAINER_TREE_STORE_COLUMN_RENDERER, &renderer,
                          -1);

      data = LIGMA_DATA (renderer->viewable);

      if (! new_name)
        new_name = "";

      name = g_strstrip (g_strdup (new_name));

      /* We must block the edited callback at this point, because either
       * the call to ligma_object_take_name() or gtk_tree_store_set() below
       * will trigger a re-ordering and emission of the rows_reordered signal.
       * This in turn will stop the editing operation and cause a call to this
       * very callback function again. Because the order of the rows has
       * changed by then, "path_str" will point at another item and cause the
       * name of this item to be changed as well.
       */
      g_signal_handlers_block_by_func (cell,
                                       ligma_data_factory_view_tree_name_edited,
                                       view);

      if (ligma_data_is_writable (data) &&
          strlen (name)                &&
          g_strcmp0 (name, ligma_object_get_name (data)))
        {
          ligma_object_take_name (LIGMA_OBJECT (data), name);
        }
      else
        {
          g_free (name);

          name = ligma_viewable_get_description (renderer->viewable, NULL);
          gtk_tree_store_set (GTK_TREE_STORE (tree_view->model), &iter,
                              LIGMA_CONTAINER_TREE_STORE_COLUMN_NAME, name,
                              -1);
          g_free (name);
        }

      g_signal_handlers_unblock_by_func (cell,
                                         ligma_data_factory_view_tree_name_edited,
                                         view);

      g_object_unref (renderer);
    }

  gtk_tree_path_free (path);
}
