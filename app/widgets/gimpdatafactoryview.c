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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpdata.h"
#include "core/gimpdatafactory.h"
#include "core/gimplist.h"
#include "core/gimptaggedcontainer.h"

#include "gimpcombotagentry.h"
#include "gimpcontainertreestore.h"
#include "gimpcontainertreeview.h"
#include "gimpcontainerview.h"
#include "gimpdatafactoryview.h"
#include "gimpdnd.h"
#include "gimpdocked.h"
#include "gimpmenufactory.h"
#include "gimpsessioninfo-aux.h"
#include "gimptagentry.h"
#include "gimpuimanager.h"
#include "gimpviewrenderer.h"
#include "gimpwidgets-utils.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_DATA_FACTORY,
  PROP_ACTION_GROUP
};


struct _GimpDataFactoryViewPrivate
{
  GimpDataFactory *factory;
  gchar           *action_group;

  GimpContainer   *tagged_container;
  GtkWidget       *query_tag_entry;
  GtkWidget       *assign_tag_entry;
  GList           *selected_items;

  GtkWidget       *follow_theme_toggle;
  gboolean         show_follow_theme_toggle;

  GtkWidget       *edit_button;
  GtkWidget       *new_button;
  GtkWidget       *duplicate_button;
  GtkWidget       *delete_button;
  GtkWidget       *refresh_button;
};


static void    gimp_data_factory_view_docked_iface_init (GimpDockedInterface *iface);

static GObject *
               gimp_data_factory_view_constructor    (GType                type,
                                                      guint                n_construct_params,
                                                      GObjectConstructParam *construct_params);
static void    gimp_data_factory_view_constructed    (GObject             *object);
static void    gimp_data_factory_view_dispose        (GObject             *object);
static void    gimp_data_factory_view_set_property   (GObject             *object,
                                                      guint                property_id,
                                                      const GValue        *value,
                                                      GParamSpec          *pspec);
static void    gimp_data_factory_view_get_property   (GObject             *object,
                                                      guint                property_id,
                                                      GValue              *value,
                                                      GParamSpec          *pspec);

static void    gimp_data_factory_view_set_aux_info   (GimpDocked          *docked,
                                                      GList               *aux_info);
static GList * gimp_data_factory_view_get_aux_info   (GimpDocked          *docked);

static void    gimp_data_factory_view_activate_item  (GimpContainerEditor *editor,
                                                      GimpViewable        *viewable);
static void    gimp_data_factory_view_select_item    (GimpContainerEditor *editor,
                                                      GimpViewable        *viewable);
static void  gimp_data_factory_view_tree_name_edited (GtkCellRendererText *cell,
                                                      const gchar         *path,
                                                      const gchar         *name,
                                                      GimpDataFactoryView *view);


G_DEFINE_TYPE_WITH_CODE (GimpDataFactoryView, gimp_data_factory_view,
                         GIMP_TYPE_CONTAINER_EDITOR,
                         G_ADD_PRIVATE (GimpDataFactoryView)
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_DOCKED,
                                                gimp_data_factory_view_docked_iface_init))

#define parent_class gimp_data_factory_view_parent_class

static GimpDockedInterface *parent_docked_iface = NULL;


static void
gimp_data_factory_view_class_init (GimpDataFactoryViewClass *klass)
{
  GObjectClass             *object_class = G_OBJECT_CLASS (klass);
  GimpContainerEditorClass *editor_class = GIMP_CONTAINER_EDITOR_CLASS (klass);

  object_class->constructor   = gimp_data_factory_view_constructor;
  object_class->constructed   = gimp_data_factory_view_constructed;
  object_class->dispose       = gimp_data_factory_view_dispose;
  object_class->set_property  = gimp_data_factory_view_set_property;
  object_class->get_property  = gimp_data_factory_view_get_property;

  editor_class->select_item   = gimp_data_factory_view_select_item;
  editor_class->activate_item = gimp_data_factory_view_activate_item;

  g_object_class_install_property (object_class, PROP_DATA_FACTORY,
                                   g_param_spec_object ("data-factory",
                                                        NULL, NULL,
                                                        GIMP_TYPE_DATA_FACTORY,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_ACTION_GROUP,
                                   g_param_spec_string ("action-group",
                                                        NULL, NULL,
                                                        NULL,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
gimp_data_factory_view_init (GimpDataFactoryView *view)
{
  view->priv = gimp_data_factory_view_get_instance_private (view);

  view->priv->tagged_container = NULL;
  view->priv->query_tag_entry  = NULL;
  view->priv->assign_tag_entry = NULL;
  view->priv->selected_items   = NULL;
  view->priv->edit_button      = NULL;
  view->priv->new_button       = NULL;
  view->priv->duplicate_button = NULL;
  view->priv->delete_button    = NULL;
  view->priv->refresh_button   = NULL;

  view->priv->show_follow_theme_toggle = FALSE;
}

static void
gimp_data_factory_view_docked_iface_init (GimpDockedInterface *iface)
{
  parent_docked_iface = g_type_interface_peek_parent (iface);

  if (! parent_docked_iface)
    parent_docked_iface = g_type_default_interface_peek (GIMP_TYPE_DOCKED);

  iface->set_aux_info = gimp_data_factory_view_set_aux_info;
  iface->get_aux_info = gimp_data_factory_view_get_aux_info;
}

static GObject *
gimp_data_factory_view_constructor (GType                  type,
                                    guint                  n_construct_params,
                                    GObjectConstructParam *construct_params)
{
  GimpDataFactoryView *factory_view;
  GObject             *object;

  object = G_OBJECT_CLASS (parent_class)->constructor (type,
                                                       n_construct_params,
                                                       construct_params);

  factory_view = GIMP_DATA_FACTORY_VIEW (object);

  gimp_assert (GIMP_IS_DATA_FACTORY (factory_view->priv->factory));
  gimp_assert (factory_view->priv->action_group != NULL);

  factory_view->priv->tagged_container =
    gimp_tagged_container_new (gimp_data_factory_get_container (factory_view->priv->factory));

  /* this must happen in constructor(), because doing it in
   * set_property() warns about wrong construct property usage
   */
  g_object_set (object,
                "container", factory_view->priv->tagged_container,
                NULL);

  return object;
}

static void
gimp_data_factory_view_constructed (GObject *object)
{
  GimpDataFactoryView        *factory_view = GIMP_DATA_FACTORY_VIEW (object);
  GimpDataFactoryViewPrivate *priv         = factory_view->priv;
  GimpContainerEditor        *editor       = GIMP_CONTAINER_EDITOR (object);
  GimpUIManager              *manager;
  GimpContext                *context;
  GtkWidget                  *image;
  GtkWidget                  *hbox;
  gchar                      *str;
  GtkIconSize                 button_icon_size;
  GtkReliefStyle              button_relief;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gimp_container_editor_set_selection_mode (editor, GTK_SELECTION_SINGLE);

  if (GIMP_IS_CONTAINER_TREE_VIEW (editor->view))
    {
      GimpContainerTreeView *tree_view;

      tree_view = GIMP_CONTAINER_TREE_VIEW (editor->view);

      gimp_container_tree_view_connect_name_edited (tree_view,
                                                    G_CALLBACK (gimp_data_factory_view_tree_name_edited),
                                                    factory_view);
    }

  manager = gimp_editor_get_ui_manager (GIMP_EDITOR (editor->view));

  str = g_strdup_printf ("%s-edit", priv->action_group);
  if (gimp_ui_manager_find_action (manager, priv->action_group, str))
    priv->edit_button =
      gimp_editor_add_action_button (GIMP_EDITOR (editor->view),
                                     priv->action_group,
                                     str, NULL);
  g_free (str);

  if (gimp_data_factory_view_has_data_new_func (factory_view))
    {
      str = g_strdup_printf ("%s-new", priv->action_group);
      priv->new_button =
        gimp_editor_add_action_button (GIMP_EDITOR (editor->view),
                                       priv->action_group,
                                       str, NULL);
      g_free (str);
    }

  str = g_strdup_printf ("%s-duplicate", priv->action_group);
  if (gimp_ui_manager_find_action (manager, priv->action_group, str))
    priv->duplicate_button =
      gimp_editor_add_action_button (GIMP_EDITOR (editor->view),
                                     priv->action_group,
                                     str, NULL);
  g_free (str);

  str = g_strdup_printf ("%s-delete", priv->action_group);
  if (gimp_ui_manager_find_action (manager, priv->action_group, str))
    priv->delete_button =
      gimp_editor_add_action_button (GIMP_EDITOR (editor->view),
                                     priv->action_group,
                                     str, NULL);
  g_free (str);

  str = g_strdup_printf ("%s-refresh", priv->action_group);
  if (gimp_ui_manager_find_action (manager, priv->action_group, str))
    priv->refresh_button =
      gimp_editor_add_action_button (GIMP_EDITOR (editor->view),
                                     priv->action_group,
                                     str, NULL);
  g_free (str);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 1);
  gtk_box_pack_start (GTK_BOX (editor->view), hbox, FALSE, FALSE, 0);
  gtk_box_reorder_child (GTK_BOX (editor->view), hbox, 0);
  gtk_widget_set_visible (hbox, TRUE);

  /* Query tag entry */
  priv->query_tag_entry =
    gimp_combo_tag_entry_new (GIMP_TAGGED_CONTAINER (priv->tagged_container),
                              GIMP_TAG_ENTRY_MODE_QUERY);
  gtk_box_pack_start (GTK_BOX (hbox), priv->query_tag_entry, TRUE, TRUE, 0);
  gtk_widget_show (priv->query_tag_entry);

  /* Follow Theme toggle */
  g_object_get (object,
                "context", &context,
                NULL);
  gtk_widget_style_get (GTK_WIDGET (editor->view),
                        "button-icon-size", &button_icon_size,
                        "button-relief",    &button_relief,
                        NULL);
#define GIMP_ICON_FOLLOW_THEME "gimp-prefs-theme"
  priv->follow_theme_toggle  = gimp_prop_toggle_new (G_OBJECT (context->gimp->config),
                                                     "viewables-follow-theme",
                                                     GIMP_ICON_FOLLOW_THEME, NULL,
                                                     &image);
  gtk_button_set_relief (GTK_BUTTON (priv->follow_theme_toggle), button_relief);
  /* Re-setting the image to make sure we use the correct size from theme.  */
  gtk_image_set_from_icon_name (GTK_IMAGE (image), GIMP_ICON_FOLLOW_THEME, button_icon_size);
#undef GIMP_ICON_FOLLOW_THEME
  gtk_box_pack_start (GTK_BOX (hbox), priv->follow_theme_toggle, FALSE, FALSE, 0);
  gtk_widget_set_visible (priv->follow_theme_toggle, priv->show_follow_theme_toggle);
  g_object_unref (context);

  /* Assign tag entry */
  priv->assign_tag_entry =
    gimp_combo_tag_entry_new (GIMP_TAGGED_CONTAINER (priv->tagged_container),
                              GIMP_TAG_ENTRY_MODE_ASSIGN);
  gimp_tag_entry_set_selected_items (GIMP_TAG_ENTRY (priv->assign_tag_entry),
                                     priv->selected_items);
  g_list_free (priv->selected_items);
  priv->selected_items = NULL;
  gtk_box_pack_start (GTK_BOX (editor->view),
                      priv->assign_tag_entry,
                      FALSE, FALSE, 0);
  gtk_widget_show (priv->assign_tag_entry);

  if (priv->edit_button)
    gimp_container_view_enable_dnd (editor->view,
                                    GTK_BUTTON (priv->edit_button),
                                    gimp_data_factory_get_data_type (priv->factory));

  if (priv->duplicate_button)
    gimp_container_view_enable_dnd (editor->view,
                                    GTK_BUTTON (priv->duplicate_button),
                                    gimp_data_factory_get_data_type (priv->factory));

  if (priv->delete_button)
    gimp_container_view_enable_dnd (editor->view,
                                    GTK_BUTTON (priv->delete_button),
                                    gimp_data_factory_get_data_type (priv->factory));

  gimp_ui_manager_update (manager, editor);
}

static void
gimp_data_factory_view_dispose (GObject *object)
{
  GimpDataFactoryView *factory_view = GIMP_DATA_FACTORY_VIEW (object);

  g_clear_object (&factory_view->priv->tagged_container);
  g_clear_object (&factory_view->priv->factory);

  g_clear_pointer (&factory_view->priv->action_group, g_free);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_data_factory_view_set_property (GObject      *object,
                                     guint         property_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  GimpDataFactoryView *factory_view = GIMP_DATA_FACTORY_VIEW (object);

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
gimp_data_factory_view_get_property (GObject    *object,
                                     guint       property_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  GimpDataFactoryView *factory_view = GIMP_DATA_FACTORY_VIEW (object);

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
gimp_data_factory_view_set_aux_info (GimpDocked *docked,
                                     GList      *aux_info)
{
  GimpDataFactoryView *view = GIMP_DATA_FACTORY_VIEW (docked);
  GList               *list;

  parent_docked_iface->set_aux_info (docked, aux_info);

  for (list = aux_info; list; list = g_list_next (list))
    {
      GimpSessionInfoAux *aux = list->data;

      if (! strcmp (aux->name, AUX_INFO_TAG_FILTER))
        {
          gtk_entry_set_text (GTK_ENTRY (view->priv->query_tag_entry),
                              aux->value);
        }
    }
}

static GList *
gimp_data_factory_view_get_aux_info (GimpDocked *docked)
{
  GimpDataFactoryView *view = GIMP_DATA_FACTORY_VIEW (docked);
  GList               *aux_info;
  const gchar         *tag_filter;

  aux_info = parent_docked_iface->get_aux_info (docked);

  tag_filter = gtk_entry_get_text (GTK_ENTRY (view->priv->query_tag_entry));
  if (tag_filter && *tag_filter)
    {
      GimpSessionInfoAux *aux;

      aux = gimp_session_info_aux_new (AUX_INFO_TAG_FILTER, tag_filter);
      aux_info = g_list_append (aux_info, aux);
    }

  return aux_info;
}

GtkWidget *
gimp_data_factory_view_new (GimpViewType      view_type,
                            GimpDataFactory  *factory,
                            GimpContext      *context,
                            gint              view_size,
                            gint              view_border_width,
                            GimpMenuFactory  *menu_factory,
                            const gchar      *menu_identifier,
                            const gchar      *ui_path,
                            const gchar      *action_group)
{
  g_return_val_if_fail (GIMP_IS_DATA_FACTORY (factory), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (view_size > 0 &&
                        view_size <= GIMP_VIEWABLE_MAX_PREVIEW_SIZE, NULL);
  g_return_val_if_fail (view_border_width >= 0 &&
                        view_border_width <= GIMP_VIEW_MAX_BORDER_WIDTH,
                        NULL);
  g_return_val_if_fail (menu_factory == NULL ||
                        GIMP_IS_MENU_FACTORY (menu_factory), NULL);
  g_return_val_if_fail (action_group != NULL, NULL);

  return g_object_new (GIMP_TYPE_DATA_FACTORY_VIEW,
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

void
gimp_data_factory_view_show_follow_theme_toggle (GimpDataFactoryView *view,
                                                 gboolean             show)
{
  view->priv->show_follow_theme_toggle = show;

  if (view->priv->follow_theme_toggle != NULL)
    gtk_widget_set_visible (view->priv->follow_theme_toggle, show);
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
gimp_data_factory_view_get_child_type (GimpDataFactoryView *factory_view)
{
  g_return_val_if_fail (GIMP_IS_DATA_FACTORY_VIEW (factory_view), G_TYPE_NONE);

  return gimp_data_factory_get_data_type (factory_view->priv->factory);
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

      /* We must block the edited callback at this point, because either
       * the call to gimp_object_take_name() or gtk_tree_store_set() below
       * will trigger a re-ordering and emission of the rows_reordered signal.
       * This in turn will stop the editing operation and cause a call to this
       * very callback function again. Because the order of the rows has
       * changed by then, "path_str" will point at another item and cause the
       * name of this item to be changed as well.
       */
      g_signal_handlers_block_by_func (cell,
                                       gimp_data_factory_view_tree_name_edited,
                                       view);

      if (gimp_data_is_writable (data) &&
          strlen (name)                &&
          g_strcmp0 (name, gimp_object_get_name (data)))
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

      g_signal_handlers_unblock_by_func (cell,
                                         gimp_data_factory_view_tree_name_edited,
                                         view);

      g_object_unref (renderer);
    }

  gtk_tree_path_free (path);
}
