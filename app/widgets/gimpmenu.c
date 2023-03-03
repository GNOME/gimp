/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpmenu.c
 * Copyright (C) 2023 Jehan
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

#include "widgets-types.h"

#include "libgimpwidgets/gimpwidgets.h"

#include "core/gimp.h"

#include "gimpaction.h"
#include "gimpenumaction.h"
#include "gimphelp-ids.h"
#include "gimpmenu.h"
#include "gimpmenushell.h"
#include "gimpprocedureaction.h"
#include "gimpradioaction.h"
#include "gimpuimanager.h"

#define GIMP_MENU_ACTION_KEY "gimp-menu-action"


/**
 * GimpMenu:
 *
 * Our own menu widget.
 *
 * We cannot use the simpler gtk_menu_new_from_model() because it lacks
 * tooltip support and unfortunately GTK does not plan to implement this:
 * https://gitlab.gnome.org/GNOME/gtk/-/issues/785
 *
 * This is why we need to implement our own GimpMenu subclass. It looks very
 * minimal right now, but the whole point is that it is also a GimpMenuShell
 * (where all the real work, with proxy items, syncing with actions and such,
 * happens).
 */


struct _GimpMenuPrivate
{
  GTree *submenus;
  GTree *placeholders;
};


static void     gimp_menu_iface_init              (GimpMenuShellInterface  *iface);

static void     gimp_menu_finalize                (GObject                 *object);

static void     gimp_menu_append                  (GimpMenuShell           *shell,
                                                   GMenuModel              *model);
static void     gimp_menu_add_ui                  (GimpMenuShell           *shell,
                                                   const gchar            **paths,
                                                   const gchar             *action_name,
                                                   const gchar             *placeholder_key,
                                                   gboolean                 top);
static void     gimp_menu_remove_ui               (GimpMenuShell           *shell,
                                                   const gchar            **paths,
                                                   const gchar             *action_name);

static void     gimp_menu_add_placeholder         (GimpMenu                *menu,
                                                   const gchar             *label);
static void     gimp_menu_add_action              (GimpMenu                *menu,
                                                   const gchar             *action_name,
                                                   const gchar             *placeholder_key,
                                                   gboolean                 top,
                                                   GtkRadioMenuItem       **group);
static void     gimp_menu_remove_action           (GimpMenu                *menu,
                                                   const gchar             *action_name);

static void     gimp_menu_toggle_item_toggled     (GtkWidget               *item,
                                                   GAction                 *action);

static void     gimp_menu_notify_group_label      (GimpRadioAction         *action,
                                                   const GParamSpec        *pspec,
                                                   GtkMenuItem             *item);
static void     gimp_menu_toggle_action_toggled   (GimpAction              *action,
                                                   GtkCheckMenuItem        *item);
static void     gimp_menu_action_notify_sensitive (GimpAction              *action,
                                                   const GParamSpec        *pspec,
                                                   GtkCheckMenuItem        *item);
static void     gimp_menu_action_notify_visible   (GimpAction              *action,
                                                   const GParamSpec        *pspec,
                                                   GtkWidget               *item);

static gboolean gimp_menu_copy_placeholders       (gpointer                  key,
                                                   gpointer                  item,
                                                   GTree                    *placeholders);

static void     gimp_menu_help_fun                (const gchar             *bogus_help_id,
                                                   gpointer                 help_data);


G_DEFINE_TYPE_WITH_CODE (GimpMenu, gimp_menu, GTK_TYPE_MENU,
                         G_ADD_PRIVATE (GimpMenu)
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_MENU_SHELL, gimp_menu_iface_init))

#define parent_class gimp_menu_parent_class


/*  Class functions  */

static void
gimp_menu_class_init (GimpMenuClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize     = gimp_menu_finalize;
  object_class->get_property = gimp_menu_shell_get_property;
  object_class->set_property = gimp_menu_shell_set_property;

  gimp_menu_shell_install_properties (object_class);
}

static void
gimp_menu_init (GimpMenu *menu)
{
  menu->priv = gimp_menu_get_instance_private (menu);

  menu->priv->submenus     = g_tree_new_full ((GCompareDataFunc) g_strcmp0, NULL,
                                              g_free, NULL);
  menu->priv->placeholders = g_tree_new_full ((GCompareDataFunc) g_strcmp0, NULL,
                                              g_free, NULL);

  gimp_menu_shell_init (GIMP_MENU_SHELL (menu));

  gimp_help_connect (GTK_WIDGET (menu), gimp_menu_help_fun,
                     GIMP_HELP_MAIN, menu, NULL);
}

static void
gimp_menu_iface_init (GimpMenuShellInterface *iface)
{
  iface->append    = gimp_menu_append;
  iface->add_ui    = gimp_menu_add_ui;
  iface->remove_ui = gimp_menu_remove_ui;
}

static void
gimp_menu_finalize (GObject *object)
{
  GimpMenu *menu = GIMP_MENU (object);

  g_clear_pointer (&menu->priv->submenus, g_tree_unref);
  g_clear_pointer (&menu->priv->placeholders, g_tree_unref);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_menu_append (GimpMenuShell *shell,
                  GMenuModel    *model)
{
  static GtkRadioMenuItem *group   = NULL;
  GimpMenu                *menu    = GIMP_MENU (shell);
  GimpUIManager           *manager = gimp_menu_shell_get_manager (GIMP_MENU_SHELL (shell));
  gint                     n_items;

  g_return_if_fail (GTK_IS_CONTAINER (shell));

  n_items = g_menu_model_get_n_items (model);
  for (gint i = 0; i < n_items; i++)
    {
      GMenuModel *subsection;
      GMenuModel *submenu;
      gchar      *label       = NULL;
      gchar      *action_name = NULL;

      subsection = g_menu_model_get_item_link (model, i, G_MENU_LINK_SECTION);
      submenu    = g_menu_model_get_item_link (model, i, G_MENU_LINK_SUBMENU);
      g_menu_model_get_item_attribute (model, i, G_MENU_ATTRIBUTE_LABEL, "s", &label);
      g_menu_model_get_item_attribute (model, i, G_MENU_ATTRIBUTE_ACTION, "s", &action_name);

      if (subsection != NULL)
        {
          GtkWidget *item;

          group = NULL;

          item = gtk_separator_menu_item_new ();
          gtk_container_add (GTK_CONTAINER (shell), item);
          gtk_widget_show (item);

          gimp_menu_append (shell, subsection);

          item = gtk_separator_menu_item_new ();
          gtk_container_add (GTK_CONTAINER (shell), item);
          gtk_widget_show (item);
        }
      else if (submenu != NULL && label == NULL)
        {
          GApplication *app;
          GAction      *action;
          const gchar  *group_label;
          GtkWidget    *subcontainer;
          GtkWidget    *item;

          group = NULL;

          g_return_if_fail (action_name != NULL);

          app    = manager->gimp->app;
          action = g_action_map_lookup_action (G_ACTION_MAP (app), action_name + 4);

          /* As a special case, when a submenu has no label, we expect it to
           * have an action attribute, which must be for a radio action. In such
           * a case, we'll use the radio actions' group label as submenu title.
           * See e.g.: menus/gradient-editor-menu.ui
           */
          g_return_if_fail (GIMP_IS_RADIO_ACTION (action));

          group_label = gimp_radio_action_get_group_label (GIMP_RADIO_ACTION (action));

          item = gtk_menu_item_new_with_mnemonic (group_label);
          g_signal_connect_object (action, "notify::group-label",
                                   G_CALLBACK (gimp_menu_notify_group_label),
                                   item, 0);
          gtk_container_add (GTK_CONTAINER (shell), item);

          subcontainer = gimp_menu_new (manager);
          gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), subcontainer);
          gimp_menu_append (GIMP_MENU_SHELL (subcontainer), submenu);
          gtk_widget_show (subcontainer);

          g_tree_insert (menu->priv->submenus,
                         gimp_menu_shell_make_canonical_path (group_label),
                         subcontainer);
        }
      else if (submenu != NULL)
        {
          GtkWidget *subcontainer;
          GtkWidget *item;

          group = NULL;

          /* I don't show the item on purpose because
           * gimp_menu_append() will show the parent item if any of
           * the added actions are visible.
           */
          item = gtk_menu_item_new_with_mnemonic (label);
          gtk_container_add (GTK_CONTAINER (shell), item);

          subcontainer = gimp_menu_new (manager);
          gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), subcontainer);
          gimp_menu_append (GIMP_MENU_SHELL (subcontainer), submenu);
          gtk_widget_show (subcontainer);

          g_tree_insert (menu->priv->submenus,
                         gimp_menu_shell_make_canonical_path (label),
                         subcontainer);
        }
      else if (action_name == NULL)
        {
          /* Special case: we use items with no action and a label as
           * placeholder which allows us to specify a placement in menus, which
           * might not be only top or bottom.
           */
          g_return_if_fail (label != NULL);

          group = NULL;

          gimp_menu_add_placeholder (menu, label);
        }
      else
        {
          gimp_menu_add_action (menu, action_name, NULL, FALSE, &group);
        }

      g_free (label);
      g_free (action_name);
      g_clear_object (&submenu);
      g_clear_object (&subsection);
    }
}

static void
gimp_menu_add_ui (GimpMenuShell  *shell,
                  const gchar   **paths,
                  const gchar    *action_name,
                  const gchar    *placeholder_key,
                  gboolean        top)
{
  GimpMenu      *menu    = GIMP_MENU (shell);
  GimpUIManager *manager = gimp_menu_shell_get_manager (GIMP_MENU_SHELL (shell));

  g_return_if_fail (paths != NULL);

  if (paths[0] == NULL)
    {
      gimp_menu_add_action (menu, action_name, placeholder_key, top, NULL);
    }
  else
    {
      GtkWidget *submenu = NULL;

      submenu = g_tree_lookup (menu->priv->submenus, paths[0]);

      if (submenu == NULL)
        {
          GtkWidget *item;

          item = gtk_menu_item_new_with_mnemonic (paths[0]);
          gtk_container_add (GTK_CONTAINER (shell), item);

          submenu = gimp_menu_new (manager);
          gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), submenu);
          gtk_widget_show (submenu);

          g_tree_insert (menu->priv->submenus, g_strdup (paths[0]), submenu);
        }

      gimp_menu_add_ui (GIMP_MENU_SHELL (submenu), paths + 1,
                        action_name, placeholder_key, top);
    }
}

static void
gimp_menu_remove_ui (GimpMenuShell  *shell,
                     const gchar   **paths,
                     const gchar    *action_name)
{
  GimpMenu *menu = GIMP_MENU (shell);

  g_return_if_fail (paths != NULL);

  if (paths[0] == NULL)
    {
      gimp_menu_remove_action (menu, action_name);
    }
  else
    {
      GtkWidget *submenu = NULL;

      submenu = g_tree_lookup (menu->priv->submenus, paths[0]);

      g_return_if_fail (submenu != NULL);

      gimp_menu_remove_ui (GIMP_MENU_SHELL (submenu), paths + 1, action_name);
    }
}


/* Public functions */

GtkWidget *
gimp_menu_new (GimpUIManager *manager)
{
  g_return_val_if_fail (GIMP_IS_UI_MANAGER (manager), NULL);

  return g_object_new (GIMP_TYPE_MENU,
                       "manager", manager,
                       NULL);
}

void
gimp_menu_merge (GimpMenu *menu,
                 GimpMenu *menu2,
                 gboolean  top)
{
  GList *children;
  GList *iter;

  children = gtk_container_get_children (GTK_CONTAINER (menu2));
  iter     = top ? g_list_last (children) : children;
  for (; iter; iter = top ? iter->prev : iter->next)
    {
      GtkWidget *item = iter->data;

      g_object_ref (item);
      gtk_container_remove (GTK_CONTAINER (menu2), item);
      if (top)
        gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), item);
      else
        gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
      g_object_unref (item);
    }

  g_tree_foreach (menu2->priv->placeholders,
                  (GTraverseFunc) gimp_menu_copy_placeholders,
                  menu->priv->placeholders);

  gtk_widget_destroy (GTK_WIDGET (menu2));
  g_list_free (children);
}


/* Private functions */

static void
gimp_menu_add_placeholder (GimpMenu    *menu,
                           const gchar *label)
{
  GtkWidget *item;

  /* Placeholders are inserted yet never shown, on purpose. */
  item = gtk_menu_item_new_with_mnemonic (label);
  gtk_container_add (GTK_CONTAINER (menu), item);

  g_tree_insert (menu->priv->placeholders, g_strdup (label), item);
}

static void
gimp_menu_add_action (GimpMenu          *menu,
                      const gchar       *action_name,
                      const gchar       *placeholder_key,
                      gboolean           top,
                      GtkRadioMenuItem **group)
{
  GimpUIManager *manager;
  GApplication  *app;
  GAction       *action;
  const gchar   *action_label;
  GtkWidget     *item;
  GtkWidget     *sibling = NULL;
  gboolean       visible;

  g_return_if_fail (GIMP_IS_MENU (menu));

  manager = gimp_menu_shell_get_manager (GIMP_MENU_SHELL (menu));
  app     = manager->gimp->app;

  if (g_str_has_prefix (action_name, "app."))
    action = g_action_map_lookup_action (G_ACTION_MAP (app), action_name + 4);
  else
    action = g_action_map_lookup_action (G_ACTION_MAP (app), action_name);

  g_return_if_fail (GIMP_IS_ACTION (action));

  action_label = gimp_action_get_label (GIMP_ACTION (action));
  g_return_if_fail (action_label != NULL);

  if (GIMP_IS_TOGGLE_ACTION (action))
    {
      if (GIMP_IS_RADIO_ACTION (action))
        item = gtk_radio_menu_item_new_with_mnemonic_from_widget (group ? *group : NULL,
                                                                  action_label);
      else
        item = gtk_check_menu_item_new_with_mnemonic (action_label);

      gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (item),
                                      gimp_toggle_action_get_active (GIMP_TOGGLE_ACTION (action)));

      if (group)
        {
          if (GIMP_IS_RADIO_ACTION (action))
            *group = GTK_RADIO_MENU_ITEM (item);
          else
            *group = NULL;
        }

      g_signal_connect (item, "toggled",
                        G_CALLBACK (gimp_menu_toggle_item_toggled),
                        action);
      g_signal_connect_object (action, "toggled",
                               G_CALLBACK (gimp_menu_toggle_action_toggled),
                               item, 0);
    }
  else if (GIMP_IS_PROCEDURE_ACTION (action) ||
           GIMP_IS_ENUM_ACTION (action))
    {
      item = gtk_menu_item_new_with_mnemonic (action_label);

      if (group)
        *group = NULL;

      g_signal_connect_swapped (item, "activate",
                                G_CALLBACK (gimp_action_activate),
                                action);
    }
  else
    {
      item = gtk_menu_item_new_with_mnemonic (action_label);

      if (group)
        *group = NULL;

      gtk_actionable_set_action_name (GTK_ACTIONABLE (item), action_name);
    }

  gimp_action_set_proxy (GIMP_ACTION (action), item);
  g_object_set_data (G_OBJECT (item), GIMP_MENU_ACTION_KEY, action);

  gtk_widget_set_sensitive (GTK_WIDGET (item),
                            gimp_action_is_sensitive (GIMP_ACTION (action), NULL));
  g_signal_connect_object (action, "notify::sensitive",
                           G_CALLBACK (gimp_menu_action_notify_sensitive),
                           item, 0);

  if (placeholder_key)
    {
      sibling = g_tree_lookup (menu->priv->placeholders, placeholder_key);

      if (! sibling)
        g_warning ("%s: no placeholder item '%s'.", G_STRFUNC, placeholder_key);
    }

  if (sibling)
    {
      GList *children;
      gint   position = 0;

      /* I am assuming that the order of the children list reflects the
       * position, though it is not clearly specified in the function docs. Yet
       * I could find no other function giving me the position of some child in
       * a container.
       */
      children = gtk_container_get_children (GTK_CONTAINER (menu));

      for (GList *iter = children; iter; iter = iter->next)
        {
          if (iter->data == sibling)
            break;
          position++;
        }
      if (! top)
        position++;

      gtk_menu_shell_insert (GTK_MENU_SHELL (menu), item, position);

      g_list_free (children);
    }
  else
    {
      if (top)
        gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), item);
      else
        gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
    }

  visible = gimp_action_is_visible (GIMP_ACTION (action));
  gtk_widget_set_visible (item, visible);
  if (visible && GTK_IS_MENU (menu))
    {
      GtkWidget *parent = gtk_menu_get_attach_widget (GTK_MENU (menu));

      /* Note that this is not the container we must show, but the menu item
       * attached to the parent, in order not to leave empty submenus.
       */
      if (parent != NULL &&
          G_TYPE_FROM_INSTANCE (parent) == GTK_TYPE_MENU_ITEM)
        gtk_widget_show (parent);
    }
  g_signal_connect_object (action, "notify::visible",
                           G_CALLBACK (gimp_menu_action_notify_visible),
                           item, 0);
}

static void
gimp_menu_remove_action (GimpMenu    *menu,
                         const gchar *action_name)
{
  GimpUIManager *manager;
  GApplication  *app;
  GList         *children;
  GAction       *action;

  g_return_if_fail (GIMP_IS_MENU (menu));

  manager = gimp_menu_shell_get_manager (GIMP_MENU_SHELL (menu));
  app     = manager->gimp->app;

  if (g_str_has_prefix (action_name, "app."))
    action = g_action_map_lookup_action (G_ACTION_MAP (app), action_name + 4);
  else
    action = g_action_map_lookup_action (G_ACTION_MAP (app), action_name);

  g_return_if_fail (GIMP_IS_ACTION (action));

  children = gtk_container_get_children (GTK_CONTAINER (menu));

  for (GList *iter = children; iter; iter = iter->next)
    {
      GtkWidget *child = iter->data;
      GAction   *item_action;

      item_action = g_object_get_data (G_OBJECT (child), GIMP_MENU_ACTION_KEY);
      if (item_action == action)
        {
          gtk_widget_destroy (child);
          break;
        }
    }

  g_list_free (children);
}

static void
gimp_menu_toggle_item_toggled (GtkWidget *item,
                               GAction   *action)
{
  gboolean active = gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (item));

  g_signal_handlers_block_by_func (action,
                                   G_CALLBACK (gimp_menu_toggle_action_toggled),
                                   item);
  gimp_toggle_action_set_active (GIMP_TOGGLE_ACTION (action), active);
  g_signal_handlers_unblock_by_func (action,
                                     G_CALLBACK (gimp_menu_toggle_action_toggled),
                                     item);
}

static void
gimp_menu_notify_group_label (GimpRadioAction  *action,
                              const GParamSpec *pspec,
                              GtkMenuItem      *item)
{
  gtk_menu_item_set_use_underline (item, TRUE);
  gtk_menu_item_set_label (item, gimp_radio_action_get_group_label (action));
}

static void
gimp_menu_toggle_action_toggled (GimpAction       *action,
                                 GtkCheckMenuItem *item)
{
  gboolean active = gimp_toggle_action_get_active (GIMP_TOGGLE_ACTION (action));

  g_signal_handlers_block_by_func (item,
                                   G_CALLBACK (gimp_menu_toggle_item_toggled),
                                   action);
  gtk_check_menu_item_set_active (item, active);
  g_signal_handlers_unblock_by_func (item,
                                   G_CALLBACK (gimp_menu_toggle_item_toggled),
                                   action);
}

static void
gimp_menu_action_notify_sensitive (GimpAction       *action,
                                   const GParamSpec *pspec,
                                   GtkCheckMenuItem *item)
{
  gtk_widget_set_sensitive (GTK_WIDGET (item),
                            gimp_action_is_sensitive (action, NULL));
}

static void
gimp_menu_action_notify_visible (GimpAction       *action,
                                 const GParamSpec *pspec,
                                 GtkWidget        *item)
{
  GtkWidget *container;

  gtk_widget_set_visible (item, gimp_action_is_visible (action));

  container = gtk_widget_get_parent (item);
  if (gimp_action_is_visible (GIMP_ACTION (action)))
    {
      GtkWidget *widget = gtk_menu_get_attach_widget (GTK_MENU (container));

      /* We must show the GtkMenuItem associated as submenu to the parent
       * container.
       */
      if (G_TYPE_FROM_INSTANCE (widget) == GTK_TYPE_MENU_ITEM)
        gtk_widget_show (widget);
    }
  else
    {
      GList    *children      = gtk_container_get_children (GTK_CONTAINER (container));
      gboolean  all_invisible = TRUE;

      for (GList *iter = children; iter; iter = iter->next)
        {
          if (gtk_widget_get_visible (iter->data))
            {
              all_invisible = FALSE;
              break;
            }
        }
      g_list_free (children);

      if (all_invisible)
        {
          GtkWidget *widget;

          /* No need to leave empty submenus. */
          widget = gtk_menu_get_attach_widget (GTK_MENU (container));
          if (G_TYPE_FROM_INSTANCE (widget) == GTK_TYPE_MENU_ITEM)
            gtk_widget_hide (widget);
        }
    }
}

static gboolean
gimp_menu_copy_placeholders (gpointer  key,
                             gpointer  item,
                             GTree    *placeholders)
{
  g_tree_insert (placeholders, g_strdup ((gchar *) key), item);
  return FALSE;
}

static void
gimp_menu_help_fun (const gchar *bogus_help_id,
                    gpointer     help_data)
{
  gchar     *help_id     = NULL;
  GimpMenu  *menu;
  Gimp      *gimp;
  GtkWidget *item;
  gchar     *help_domain = NULL;
  gchar     *help_string = NULL;
  gchar     *domain_separator;

  g_return_if_fail (GIMP_IS_MENU (help_data));

  menu = GIMP_MENU (help_data);
  gimp = gimp_menu_shell_get_manager (GIMP_MENU_SHELL (menu))->gimp;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  item = gtk_menu_shell_get_selected_item (GTK_MENU_SHELL (menu));

  if (item)
    help_id = g_object_get_qdata (G_OBJECT (item), GIMP_HELP_ID);

  if (help_id == NULL || strlen (help_id) == 0)
    help_id = (gchar *) bogus_help_id;

  help_id = g_strdup (help_id);

  domain_separator = strchr (help_id, '?');

  if (domain_separator)
    {
      *domain_separator = '\0';

      help_domain = g_strdup (help_id);
      help_string = g_strdup (domain_separator + 1);
    }
  else
    {
      help_string = g_strdup (help_id);
    }

  gimp_help (gimp, NULL, help_domain, help_string);

  g_free (help_domain);
  g_free (help_string);
  g_free (help_id);
}
