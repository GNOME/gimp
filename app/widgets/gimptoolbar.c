/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimptoolbar.c
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

#include "widgets-types.h"

#include "core/gimp.h"

#include "gimpaction.h"
#include "gimpenumaction.h"
#include "gimpprocedureaction.h"
#include "gimpradioaction.h"
#include "gimptoggleaction.h"
#include "gimptoolbar.h"
#include "gimpmenumodel.h"
#include "gimpmenushell.h"
#include "gimpuimanager.h"

#define GIMP_TOOLBAR_ACTION_KEY "gimp-toolbar-action"


/**
 * GimpToolbar:
 *
 * Our own toolbar widget.
 *
 * It can be initialized with a GMenuModel.
 *
 * TODO: in the future, we could also give optional GUI-customizability to use
 * it as a generic toolbar in GIMP's main window (where people could add any
 * actions as buttons).
 */


/*  local function prototypes  */

static void   gimp_toolbar_iface_init              (GimpMenuShellInterface  *iface);

static void   gimp_toolbar_append                  (GimpMenuShell           *shell,
                                                    GimpMenuModel           *model);
static void   gimp_toolbar_add_ui                  (GimpMenuShell           *shell,
                                                    const gchar            **paths,
                                                    const gchar             *action_name,
                                                    gboolean                 top);

static void   gimp_toolbar_add_action              (GimpToolbar             *toolbar,
                                                    const gchar             *action_name,
                                                    gboolean                 top);

static void   gimp_toolbar_toggle_item_toggled     (GtkToggleToolButton     *item,
                                                    GimpToggleAction        *action);

static void   gimp_toolbar_toggle_action_toggled   (GimpToggleAction        *action,
                                                    GtkToggleToolButton     *item);
static void   gimp_toolbar_action_notify_sensitive (GimpAction              *action,
                                                    const GParamSpec        *pspec,
                                                    GtkWidget               *item);
static void   gimp_toolbar_action_notify_visible   (GimpAction              *action,
                                                    const GParamSpec        *pspec,
                                                    GtkWidget               *item);


G_DEFINE_TYPE_WITH_CODE (GimpToolbar, gimp_toolbar, GTK_TYPE_TOOLBAR,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_MENU_SHELL, gimp_toolbar_iface_init))

#define parent_class gimp_toolbar_parent_class


/* Class functions */

static void
gimp_toolbar_class_init (GimpToolbarClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = gimp_menu_shell_get_property;
  object_class->set_property = gimp_menu_shell_set_property;

  gimp_menu_shell_install_properties (object_class);
}

static void
gimp_toolbar_iface_init (GimpMenuShellInterface *iface)
{
  iface->append = gimp_toolbar_append;
  iface->add_ui = gimp_toolbar_add_ui;
}

static void
gimp_toolbar_init (GimpToolbar *bar)
{
  gimp_menu_shell_init (GIMP_MENU_SHELL (bar));
}

static void
gimp_toolbar_append (GimpMenuShell *shell,
                     GimpMenuModel *model)
{
  GimpToolbar *toolbar = GIMP_TOOLBAR (shell);
  gint         n_items;

  g_return_if_fail (GTK_IS_CONTAINER (shell));

  n_items = g_menu_model_get_n_items (G_MENU_MODEL (model));
  for (gint i = 0; i < n_items; i++)
    {
      GMenuModel *subsection;
      GMenuModel *submenu;
      gchar      *label       = NULL;
      gchar      *action_name = NULL;

      subsection = g_menu_model_get_item_link (G_MENU_MODEL (model), i, G_MENU_LINK_SECTION);
      submenu    = g_menu_model_get_item_link (G_MENU_MODEL (model), i, G_MENU_LINK_SUBMENU);
      g_menu_model_get_item_attribute (G_MENU_MODEL (model), i, G_MENU_ATTRIBUTE_LABEL, "s", &label);
      g_menu_model_get_item_attribute (G_MENU_MODEL (model), i, G_MENU_ATTRIBUTE_ACTION, "s", &action_name);

      if (subsection != NULL)
        {
          GtkToolItem *item;

          item = gtk_separator_tool_item_new ();
          gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, -1);
          gtk_widget_show (GTK_WIDGET (item));

          gimp_toolbar_append (shell, GIMP_MENU_MODEL (subsection));

          item = gtk_separator_tool_item_new ();
          gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, -1);
          gtk_widget_show (GTK_WIDGET (item));
        }
      else if (submenu == NULL && action_name != NULL)
        {
          gimp_toolbar_add_action (toolbar, action_name, FALSE);
        }
      else
        {
          if (submenu != NULL)
            g_warning ("%s: unexpected submenu. Only actions and sections are allowed in toolbar's root.",
                       G_STRFUNC);
          else if (label != NULL)
            g_warning ("%s: unexpected label '%s'. Only actions and sections are allowed in toolbar's root.",
                       G_STRFUNC, label);
          else
            g_warning ("%s: unexpected toolbar item. Only actions and sections are allowed in toolbar's root.",
                       G_STRFUNC);
        }

      g_free (label);
      g_free (action_name);
      g_clear_object (&submenu);
      g_clear_object (&subsection);
    }
}

static void
gimp_toolbar_add_ui (GimpMenuShell  *shell,
                     const gchar   **paths,
                     const gchar    *action_name,
                     gboolean        top)
{
  GimpToolbar *toolbar = GIMP_TOOLBAR (shell);

  g_return_if_fail (paths != NULL);

  if (paths[0] != NULL)
    g_warning ("%s: unexpected path '%s'. Menus are not allowed in toolbar's root.",
               G_STRFUNC, paths[0]);
  else
    gimp_toolbar_add_action (toolbar, action_name, top);
}


/* Public functions */

GtkWidget *
gimp_toolbar_new (GimpMenuModel *model,
                  GimpUIManager *manager)
{
  g_return_val_if_fail (GIMP_IS_UI_MANAGER (manager) &&
                        G_IS_MENU_MODEL (model), NULL);

  return g_object_new (GIMP_TYPE_TOOLBAR,
                       "model",   model,
                       "manager", manager,
                       NULL);
}


/* Private functions */

static void
gimp_toolbar_add_action (GimpToolbar *toolbar,
                         const gchar *action_name,
                         gboolean     top)
{
  GimpUIManager *manager;
  GimpAction    *action;
  const gchar   *action_label;
  GtkToolItem   *item;
  gboolean       visible;

  g_return_if_fail (GIMP_IS_TOOLBAR (toolbar));

  manager = gimp_menu_shell_get_manager (GIMP_MENU_SHELL (toolbar));
  action  = gimp_ui_manager_find_action (manager, NULL, action_name);

  g_return_if_fail (GIMP_IS_ACTION (action));

  action_label = gimp_action_get_short_label (GIMP_ACTION (action));
  g_return_if_fail (action_label != NULL);

  if (GIMP_IS_TOGGLE_ACTION (action))
    {
      if (GIMP_IS_RADIO_ACTION (action))
        {
          GtkToolItem *sibling = NULL;
          GSList      *group = NULL;

          if (top)
            sibling = gtk_toolbar_get_nth_item (GTK_TOOLBAR (toolbar), 0);
          else if (gtk_toolbar_get_n_items (GTK_TOOLBAR (toolbar)) > 0)
            sibling = gtk_toolbar_get_nth_item (GTK_TOOLBAR (toolbar),
                                                gtk_toolbar_get_n_items (GTK_TOOLBAR (toolbar)) - 1);

          if (sibling != NULL && GTK_IS_RADIO_TOOL_BUTTON (sibling))
            {
              GimpAction *sibling_action;

              sibling_action = g_object_get_data (G_OBJECT (sibling), GIMP_TOOLBAR_ACTION_KEY);

              if (sibling_action != NULL                &&
                  GIMP_IS_RADIO_ACTION (sibling_action) &&
                  gimp_radio_action_get_group (GIMP_RADIO_ACTION (sibling_action)) == gimp_radio_action_get_group (GIMP_RADIO_ACTION (action)))
                group = gtk_radio_tool_button_get_group (GTK_RADIO_TOOL_BUTTON (sibling));
            }

          item = gtk_radio_tool_button_new (group);
        }
      else
        {
          item = gtk_toggle_tool_button_new ();
        }

      gtk_tool_button_set_label (GTK_TOOL_BUTTON (item), action_label);
      gtk_tool_button_set_icon_name (GTK_TOOL_BUTTON (item),
                                     gimp_action_get_icon_name (GIMP_ACTION (action)));
      gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (item),
                                         gimp_toggle_action_get_active (GIMP_TOGGLE_ACTION (action)));

      g_signal_connect (item, "toggled",
                        G_CALLBACK (gimp_toolbar_toggle_item_toggled),
                        action);
      g_signal_connect_object (action, "toggled",
                               G_CALLBACK (gimp_toolbar_toggle_action_toggled),
                               item, 0);
    }
  else if (GIMP_IS_PROCEDURE_ACTION (action) ||
           GIMP_IS_ENUM_ACTION (action))
    {
      item = gtk_tool_button_new (NULL, action_label);

      g_signal_connect_swapped (item, "clicked",
                                G_CALLBACK (gimp_action_activate),
                                action);
    }
  else
    {
      item = gtk_tool_button_new (NULL, action_label);

      gtk_actionable_set_action_name (GTK_ACTIONABLE (item), action_name);
    }

  g_object_set_data (G_OBJECT (item), GIMP_TOOLBAR_ACTION_KEY, action);

  gimp_action_set_proxy (GIMP_ACTION (action), GTK_WIDGET (item));

  gtk_widget_set_sensitive (GTK_WIDGET (item),
                            gimp_action_is_sensitive (GIMP_ACTION (action), NULL));
  g_signal_connect_object (action, "notify::sensitive",
                           G_CALLBACK (gimp_toolbar_action_notify_sensitive),
                           item, 0);

  if (top)
    gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, 0);
  else
    gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, -1);

  visible = gimp_action_is_visible (GIMP_ACTION (action));
  gtk_widget_set_visible (GTK_WIDGET (item), visible);
  g_signal_connect_object (action, "notify::visible",
                           G_CALLBACK (gimp_toolbar_action_notify_visible),
                           item, 0);
}

static void
gimp_toolbar_toggle_item_toggled (GtkToggleToolButton *item,
                                  GimpToggleAction    *action)
{
  gboolean active = gtk_toggle_tool_button_get_active (item);

  g_signal_handlers_block_by_func (action,
                                   G_CALLBACK (gimp_toolbar_toggle_action_toggled),
                                   item);
  gimp_toggle_action_set_active (action, active);
  g_signal_handlers_unblock_by_func (action,
                                     G_CALLBACK (gimp_toolbar_toggle_action_toggled),
                                     item);
}

static void
gimp_toolbar_toggle_action_toggled (GimpToggleAction    *action,
                                    GtkToggleToolButton *item)
{
  gboolean active = gimp_toggle_action_get_active (action);

  g_signal_handlers_block_by_func (item,
                                   G_CALLBACK (gimp_toolbar_toggle_item_toggled),
                                   action);
  gtk_toggle_tool_button_set_active (item, active);
  g_signal_handlers_unblock_by_func (item,
                                   G_CALLBACK (gimp_toolbar_toggle_item_toggled),
                                   action);
}

static void
gimp_toolbar_action_notify_sensitive (GimpAction       *action,
                                      const GParamSpec *pspec,
                                      GtkWidget        *item)
{
  gtk_widget_set_sensitive (item, gimp_action_is_sensitive (action, NULL));
}

static void
gimp_toolbar_action_notify_visible (GimpAction       *action,
                                    const GParamSpec *pspec,
                                    GtkWidget        *item)
{
  gtk_widget_set_visible (item, gimp_action_is_visible (action));
}
