/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpmenushell.c
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

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp.h"

#include "gimpaction.h"
#include "gimpenumaction.h"
#include "gimphelp-ids.h"
#include "gimpmenu.h"
#include "gimpmenushell.h"
#include "gimpprocedureaction.h"
#include "gimpradioaction.h"
#include "gimptoggleaction.h"
#include "gimpuimanager.h"


#define GET_PRIVATE(obj) (gimp_menu_shell_get_private ((GimpMenuShell *) (obj)))

typedef struct _GimpMenuShellPrivate GimpMenuShellPrivate;

struct _GimpMenuShellPrivate
{
  GimpUIManager *manager;
  gchar         *update_signal;

  GTree         *submenus;
  GTree         *placeholders;

  GRegex        *path_regex;
};


static GimpMenuShellPrivate *
                gimp_menu_shell_get_private             (GimpMenuShell        *menu_shell);
static void     gimp_menu_shell_private_finalize        (GimpMenuShellPrivate *priv);

static void     gimp_menu_shell_help_fun                (const gchar          *bogus_help_id,
                                                         gpointer              help_data);
static void     gimp_menu_shell_ui_added                (GimpUIManager        *manager,
                                                         const gchar          *path,
                                                         const gchar          *action_name,
                                                         const gchar          *placeholder_key,
                                                         gboolean              top,
                                                         GimpMenuShell        *shell);

static void     gimp_menu_shell_radio_item_toggled      (GtkWidget            *item,
                                                         GAction              *action);
static void     gimp_menu_shell_action_activate         (GtkMenuItem          *item,
                                                         GimpAction           *action);

static void     gimp_menu_shell_notify_group_label      (GimpRadioAction      *action,
                                                         const GParamSpec     *pspec,
                                                         GtkMenuItem          *item);
static void     gimp_menu_shell_toggle_action_changed   (GimpAction           *action,
                                                         GVariant             *value G_GNUC_UNUSED,
                                                         GtkCheckMenuItem     *item);
static void     gimp_menu_shell_action_notify_sensitive (GimpAction           *action,
                                                         const GParamSpec     *pspec,
                                                         GtkCheckMenuItem     *item);
static void     gimp_menu_shell_action_notify_visible   (GimpAction           *action,
                                                         const GParamSpec     *pspec,
                                                         GtkWidget            *item);

static void     gimp_menu_shell_append_model            (GimpMenuShell        *shell,
                                                         GMenuModel           *model);
static void     gimp_menu_shell_add_ui                  (GimpMenuShell        *shell,
                                                         const gchar         **paths,
                                                         const gchar          *action_name,
                                                         const gchar          *placeholder_key,
                                                         gboolean              top);
static void     gimp_menu_shell_add_action              (GimpMenuShell         *shell,
                                                         const gchar           *action_name,
                                                         const gchar           *placeholder_key,
                                                         gboolean               top,
                                                         GtkRadioMenuItem     **group);
static void     gimp_menu_shell_add_placeholder         (GimpMenuShell         *shell,
                                                         const gchar           *label);

static gchar ** gimp_menu_shell_break_path              (GimpMenuShell         *shell,
                                                         const gchar           *path);
static gchar  * gimp_menu_shell_make_canonical_path     (const gchar           *path);


G_DEFINE_INTERFACE (GimpMenuShell, gimp_menu_shell, GTK_TYPE_MENU_SHELL)


static void
gimp_menu_shell_default_init (GimpMenuShellInterface *iface)
{
  g_object_interface_install_property (iface,
                                       g_param_spec_object ("manager",
                                                            NULL, NULL,
                                                            GIMP_TYPE_UI_MANAGER,
                                                            GIMP_PARAM_READWRITE |
                                                            G_PARAM_CONSTRUCT_ONLY));
}


/*  public functions  */

void
gimp_menu_shell_fill (GimpMenuShell *shell,
                      GMenuModel    *model,
                      const gchar   *update_signal)
{
  g_return_if_fail (GTK_IS_CONTAINER (shell));

  gtk_container_foreach (GTK_CONTAINER (shell),
                         (GtkCallback) gtk_widget_destroy,
                         NULL);

  gimp_menu_shell_append_model (shell, model);

  if (update_signal != NULL)
    {
      GimpMenuShellPrivate *priv = GET_PRIVATE (shell);

      if (priv->update_signal != NULL)
        {
          g_free (priv->update_signal);
          g_signal_handlers_disconnect_by_func (priv->manager,
                                                G_CALLBACK (gimp_menu_shell_ui_added),
                                                shell);
        }

      priv->update_signal = g_strdup (update_signal);
      g_signal_connect (priv->manager, update_signal,
                        G_CALLBACK (gimp_menu_shell_ui_added),
                        shell);
      gimp_ui_manager_foreach_ui (priv->manager,
                                  (GimpUIMenuCallback) gimp_menu_shell_ui_added,
                                  shell);
    }
}

void
gimp_menu_shell_merge (GimpMenuShell *shell,
                       GimpMenuShell *shell2,
                       gboolean       top)
{
  GList *children;
  GList *iter;

  children = gtk_container_get_children (GTK_CONTAINER (shell2));
  iter     = top ? g_list_last (children) : children;
  for (; iter; iter = top ? iter->prev : iter->next)
    {
      GtkWidget *item = iter->data;

      g_object_ref (item);
      gtk_container_remove (GTK_CONTAINER (shell2), item);
      if (top)
        gtk_menu_shell_prepend (GTK_MENU_SHELL (shell), item);
      else
        gtk_menu_shell_append (GTK_MENU_SHELL (shell), item);
      g_object_unref (item);
    }

  g_list_free (children);
  gtk_widget_destroy (GTK_WIDGET (shell2));
}


/* Protected functions. */

void
gimp_menu_shell_init (GimpMenuShell *shell)
{
  GimpMenuShellPrivate *priv;

  g_return_if_fail (GIMP_IS_MENU_SHELL (shell));

  priv = GET_PRIVATE (shell);

  priv->submenus      = g_tree_new_full ((GCompareDataFunc) g_strcmp0, NULL,
                                         g_free, NULL);
  priv->placeholders  = g_tree_new_full ((GCompareDataFunc) g_strcmp0, NULL,
                                         g_free, NULL);
  priv->update_signal = NULL;
  priv->path_regex    = g_regex_new ("/+", 0, 0, NULL);

  gimp_help_connect (GTK_WIDGET (shell),
                     gimp_menu_shell_help_fun,
                     GIMP_HELP_MAIN,
                     shell,
                     NULL);
}

/**
 * gimp_menu_shell_install_properties:
 * @klass: the class structure for a type deriving from #GObject
 *
 * Installs the necessary properties for a class implementing
 * #GimpMenuShell. Please call this function in the *_class_init()
 * function of the child class.
 **/
void
gimp_menu_shell_install_properties (GObjectClass *klass)
{
  g_object_class_override_property (klass, GIMP_MENU_SHELL_PROP_MANAGER, "manager");
}

void
gimp_menu_shell_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  GimpMenuShellPrivate *priv;

  priv = GET_PRIVATE (object);

  switch (property_id)
    {
    case GIMP_MENU_SHELL_PROP_MANAGER:
      g_value_set_object (value, priv->manager);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

void
gimp_menu_shell_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  GimpMenuShellPrivate *priv;

  priv = GET_PRIVATE (object);

  switch (property_id)
    {
    case GIMP_MENU_SHELL_PROP_MANAGER:
      g_set_weak_pointer (&priv->manager, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


/*  Private functions  */

static GimpMenuShellPrivate *
gimp_menu_shell_get_private (GimpMenuShell *menu_shell)
{
  GimpMenuShellPrivate *priv;
  static GQuark      private_key = 0;

  g_return_val_if_fail (GIMP_IS_MENU_SHELL (menu_shell), NULL);

  if (! private_key)
    private_key = g_quark_from_static_string ("gimp-menu_shell-priv");

  priv = g_object_get_qdata ((GObject *) menu_shell, private_key);

  if (! priv)
    {
      priv = g_slice_new0 (GimpMenuShellPrivate);

      g_object_set_qdata_full ((GObject *) menu_shell, private_key, priv,
                               (GDestroyNotify) gimp_menu_shell_private_finalize);
    }

  return priv;
}

static void
gimp_menu_shell_private_finalize (GimpMenuShellPrivate *priv)
{
  g_tree_unref (priv->submenus);
  g_tree_unref (priv->placeholders);

  g_free (priv->update_signal);
  g_clear_pointer (&priv->path_regex, g_regex_unref);

  g_slice_free (GimpMenuShellPrivate, priv);
}

static void
gimp_menu_shell_help_fun (const gchar *bogus_help_id,
                          gpointer     help_data)
{
  GimpMenuShellPrivate *priv;
  gchar                *help_id     = NULL;
  GimpMenuShell        *shell;
  Gimp                 *gimp;
  GtkWidget            *item;
  gchar                *help_domain = NULL;
  gchar                *help_string = NULL;
  gchar                *domain_separator;

  g_return_if_fail (GIMP_IS_MENU_SHELL (help_data));

  shell = GIMP_MENU_SHELL (help_data);
  priv  = GET_PRIVATE (shell);
  gimp  = priv->manager->gimp;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  item = gtk_menu_shell_get_selected_item (GTK_MENU_SHELL (shell));

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

static void
gimp_menu_shell_ui_added (GimpUIManager *manager,
                          const gchar   *path,
                          const gchar   *action_name,
                          const gchar   *placeholder_key,
                          gboolean       top,
                          GimpMenuShell *shell)
{
  gchar **paths = gimp_menu_shell_break_path (shell, path);

  gimp_menu_shell_add_ui (shell, (const gchar **) paths,
                          action_name, placeholder_key, top);

  g_strfreev (paths);
}

static void
gimp_menu_shell_radio_item_toggled (GtkWidget *item,
                                    GAction   *action)
{
  gboolean active = gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (item));

  gimp_toggle_action_set_active (GIMP_TOGGLE_ACTION (action), active);
}

static void
gimp_menu_shell_action_activate (GtkMenuItem *item,
                                 GimpAction  *action)
{
  gimp_action_activate (action);
}

static void
gimp_menu_shell_notify_group_label (GimpRadioAction  *action,
                                    const GParamSpec *pspec,
                                    GtkMenuItem      *item)
{
  gtk_menu_item_set_use_underline (item, TRUE);
  gtk_menu_item_set_label (item, gimp_radio_action_get_group_label (action));
}

static void
gimp_menu_shell_toggle_action_changed (GimpAction       *action,
                                       /* Unused because this is used for 2 signals
                                        * where the GVariant refers to different data.
                                        */
                                       GVariant         *value G_GNUC_UNUSED,
                                       GtkCheckMenuItem *item)
{
  gchar    *action_name;
  gboolean  active;

  action_name = g_strdup (gtk_actionable_get_action_name (GTK_ACTIONABLE (item)));
  active      = gimp_toggle_action_get_active (GIMP_TOGGLE_ACTION (action));

  /* Make sure we don't activate the action. */
  gtk_actionable_set_action_name (GTK_ACTIONABLE (item), NULL);

  gtk_check_menu_item_set_active (item, active);

  gtk_actionable_set_action_name (GTK_ACTIONABLE (item), action_name);
  g_free (action_name);
}

static void
gimp_menu_shell_action_notify_sensitive (GimpAction       *action,
                                         const GParamSpec *pspec,
                                         GtkCheckMenuItem *item)
{
  gtk_widget_set_sensitive (GTK_WIDGET (item),
                            gimp_action_is_sensitive (action, NULL));
}

static void
gimp_menu_shell_action_notify_visible (GimpAction       *action,
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

static void
gimp_menu_shell_append_model (GimpMenuShell *shell,
                              GMenuModel    *model)
{
  GimpMenuShellPrivate    *priv;
  static GtkRadioMenuItem *group   = NULL;
  gint                     n_items;

  g_return_if_fail (GTK_IS_CONTAINER (shell));

  priv    = GET_PRIVATE (shell);
  n_items = g_menu_model_get_n_items (model);
  for (gint i = 0; i < n_items; i++)
    {
      GMenuModel *subsection;
      GMenuModel *submenu;
      GtkWidget  *item;
      gchar      *label       = NULL;
      gchar      *action_name = NULL;

      subsection = g_menu_model_get_item_link (model, i, G_MENU_LINK_SECTION);
      submenu    = g_menu_model_get_item_link (model, i, G_MENU_LINK_SUBMENU);
      g_menu_model_get_item_attribute (model, i, G_MENU_ATTRIBUTE_LABEL, "s", &label);
      g_menu_model_get_item_attribute (model, i, G_MENU_ATTRIBUTE_ACTION, "s", &action_name);

      if (subsection != NULL)
        {
          group = NULL;

          item = gtk_separator_menu_item_new ();
          gtk_container_add (GTK_CONTAINER (shell), item);
          gtk_widget_show (item);

          gimp_menu_shell_append_model (shell, subsection);

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

          app    = priv->manager->gimp->app;
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
                                   G_CALLBACK (gimp_menu_shell_notify_group_label),
                                   item, 0);
          gtk_container_add (GTK_CONTAINER (shell), item);

          subcontainer = gimp_menu_new (priv->manager);
          gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), subcontainer);
          gimp_menu_shell_append_model (GIMP_MENU_SHELL (subcontainer), submenu);
          gtk_widget_show (subcontainer);

          g_tree_insert (priv->submenus,
                         gimp_menu_shell_make_canonical_path (group_label),
                         subcontainer);
        }
      else if (submenu != NULL)
        {
          GtkWidget *subcontainer;
          GtkWidget *item;

          group = NULL;

          /* I don't show the item on purpose because
           * gimp_menu_shell_append_model() will show the parent item if any of
           * the added actions are visible.
           */
          item = gtk_menu_item_new_with_mnemonic (label);
          gtk_container_add (GTK_CONTAINER (shell), item);

          subcontainer = gimp_menu_new (priv->manager);
          gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), subcontainer);
          gimp_menu_shell_append_model (GIMP_MENU_SHELL (subcontainer), submenu);
          gtk_widget_show (subcontainer);

          g_tree_insert (priv->submenus,
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

          gimp_menu_shell_add_placeholder (shell, label);
        }
      else
        {
          gimp_menu_shell_add_action (shell, action_name, NULL, FALSE, &group);
        }
      g_free (label);
      g_free (action_name);
    }
}

static void
gimp_menu_shell_add_ui (GimpMenuShell  *shell,
                        const gchar   **paths,
                        const gchar    *action_name,
                        const gchar    *placeholder_key,
                        gboolean        top)
{
  g_return_if_fail (paths != NULL);

  if (paths[0] == NULL)
    {
      gimp_menu_shell_add_action (shell, action_name, placeholder_key, top, NULL);
    }
  else
    {
      GimpMenuShellPrivate *priv = GET_PRIVATE (shell);
      GtkWidget            *submenu = NULL;

      submenu = g_tree_lookup (priv->submenus, paths[0]);

      if (submenu == NULL)
        {
          GtkWidget *item;

          item = gtk_menu_item_new_with_mnemonic (paths[0]);
          gtk_container_add (GTK_CONTAINER (shell), item);

          submenu = gimp_menu_new (priv->manager);
          gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), submenu);
          gtk_widget_show (submenu);

          g_tree_insert (priv->submenus, g_strdup (paths[0]), submenu);
        }

      gimp_menu_shell_add_ui (GIMP_MENU_SHELL (submenu), paths + 1,
                              action_name, placeholder_key, top);
    }
}

static void
gimp_menu_shell_add_action (GimpMenuShell     *shell,
                            const gchar       *action_name,
                            const gchar       *placeholder_key,
                            gboolean           top,
                            GtkRadioMenuItem **group)
{
  GimpMenuShellPrivate *priv;
  GApplication *app;
  GAction      *action;
  const gchar  *action_label;
  GtkWidget    *item;
  GtkWidget    *sibling = NULL;
  gboolean      visible;

  g_return_if_fail (GIMP_IS_MENU_SHELL (shell));

  priv = GET_PRIVATE (shell);

  app = priv->manager->gimp->app;

  if (g_str_has_prefix (action_name, "app."))
    action = g_action_map_lookup_action (G_ACTION_MAP (app), action_name + 4);
  else
    action = g_action_map_lookup_action (G_ACTION_MAP (app), action_name);

  g_return_if_fail (GIMP_IS_ACTION (action));

  action_label = gimp_action_get_label (GIMP_ACTION (action));
  g_return_if_fail (action_label != NULL);

  if (GIMP_IS_RADIO_ACTION (action))
    {
      item = gtk_radio_menu_item_new_with_mnemonic_from_widget (group ? *group : NULL, action_label);
      gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (item),
                                      gimp_toggle_action_get_active (GIMP_TOGGLE_ACTION (action)));

      if (group)
        *group = GTK_RADIO_MENU_ITEM (item);

      g_signal_connect (item, "toggled",
                        G_CALLBACK (gimp_menu_shell_radio_item_toggled),
                        action);
      g_signal_connect (action, "change-state",
                        G_CALLBACK (gimp_menu_shell_toggle_action_changed),
                        item);
    }
  else if (GIMP_IS_TOGGLE_ACTION (action))
    {
      item  = gtk_check_menu_item_new_with_mnemonic (action_label);
      gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (item),
                                      gimp_toggle_action_get_active (GIMP_TOGGLE_ACTION (action)));
      if (group)
        *group = NULL;

      g_signal_connect (action, "change-state",
                        G_CALLBACK (gimp_menu_shell_toggle_action_changed),
                        item);
    }
  else if (GIMP_IS_PROCEDURE_ACTION (action) ||
           GIMP_IS_ENUM_ACTION (action))
    {
      item = gtk_menu_item_new_with_mnemonic (action_label);

      if (group)
        *group = NULL;

      g_signal_connect (item, "activate",
                        G_CALLBACK (gimp_menu_shell_action_activate),
                        action);
    }
  else
    {
      item = gtk_menu_item_new_with_mnemonic (action_label);

      if (group)
        *group = NULL;
    }

  gtk_actionable_set_action_name (GTK_ACTIONABLE (item), action_name);
  gimp_action_set_proxy (GIMP_ACTION (action), item);

  gtk_widget_set_sensitive (GTK_WIDGET (item),
                            gimp_action_is_sensitive (GIMP_ACTION (action), NULL));
  g_signal_connect_object (action, "notify::sensitive",
                           G_CALLBACK (gimp_menu_shell_action_notify_sensitive),
                           item, 0);

  if (placeholder_key)
    {
      sibling = g_tree_lookup (priv->placeholders, placeholder_key);

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
      children = gtk_container_get_children (GTK_CONTAINER (shell));

      for (GList *iter = children; iter; iter = iter->next)
        {
          if (iter->data == sibling)
            break;
          position++;
        }
      if (! top)
        position++;

      gtk_menu_shell_insert (GTK_MENU_SHELL (shell), item, position);

      g_list_free (children);
    }
  else
    {
      if (top)
        gtk_menu_shell_prepend (GTK_MENU_SHELL (shell), item);
      else
        gtk_menu_shell_append (GTK_MENU_SHELL (shell), item);
    }

  visible = gimp_action_is_visible (GIMP_ACTION (action));
  gtk_widget_set_visible (item, visible);
  if (visible && GTK_IS_MENU (shell))
    {
      GtkWidget *parent = gtk_menu_get_attach_widget (GTK_MENU (shell));

      /* Note that this is not the container we must show, but the menu item
       * attached to the parent, in order not to leave empty submenus.
       */
      if (parent != NULL &&
          G_TYPE_FROM_INSTANCE (parent) == GTK_TYPE_MENU_ITEM)
        gtk_widget_show (parent);
    }
  g_signal_connect_object (action, "notify::visible",
                           G_CALLBACK (gimp_menu_shell_action_notify_visible),
                           item, 0);
}

static void
gimp_menu_shell_add_placeholder (GimpMenuShell *shell,
                                 const gchar   *label)
{
  GimpMenuShellPrivate *priv = GET_PRIVATE (shell);
  GtkWidget            *item;

  /* Placeholders are inserted yet never shown, on purpose. */
  item = gtk_menu_item_new_with_mnemonic (label);
  gtk_container_add (GTK_CONTAINER (shell), item);

  g_tree_insert (priv->placeholders, g_strdup (label), item);
}

static gchar **
gimp_menu_shell_break_path (GimpMenuShell *shell,
                            const gchar   *path)
{
  GimpMenuShellPrivate *priv;
  gchar                **paths;
  gint                   start = 0;

  g_return_val_if_fail (path != NULL, NULL);

  priv = GET_PRIVATE (shell);

  /* Get rid of leading slashes. */
  while (path[start] == '/' && path[start] != '\0')
    start++;

  /* Get rid of empty last item because of trailing slashes. */
  paths = g_regex_split (priv->path_regex, path + start, 0);
  if (strlen (paths[g_strv_length (paths) - 1]) == 0)
    {
      g_free (paths[g_strv_length (paths) - 1]);
      paths[g_strv_length (paths) - 1] = NULL;
    }

  for (int i = 0; paths[i]; i++)
    {
      gchar *canon_path;

      canon_path = gimp_menu_shell_make_canonical_path (paths[i]);
      g_free (paths[i]);
      paths[i] = canon_path;
    }

  return paths;
}

static gchar *
gimp_menu_shell_make_canonical_path (const gchar *path)
{
  gchar **split_path;
  gchar  *canon_path;

  /* The first underscore of each path item is a mnemonic. */
  split_path = g_strsplit (path, "_", 2);
  canon_path = g_strjoinv ("", split_path);
  g_strfreev (split_path);

  return canon_path;
}
