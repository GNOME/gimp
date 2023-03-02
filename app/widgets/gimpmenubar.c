/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpmenubar.c
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
#include "gimpmenu.h"
#include "gimpmenubar.h"
#include "gimpmenushell.h"
#include "gimpradioaction.h"
#include "gimpuimanager.h"


/**
 * GimpMenuBar:
 *
 * Our own menu bar widget.
 *
 * We cannot use the simpler gtk_application_set_menubar() because it lacks
 * tooltip support and unfortunately GTK does not plan to implement this:
 * https://gitlab.gnome.org/GNOME/gtk/-/issues/785
 * This is why we need to implement our own GimpMenuBar subclass.
 */

enum
{
  PROP_0 = GIMP_MENU_SHELL_PROP_LAST,
  PROP_MODEL
};


struct _GimpMenuBarPrivate
{
  GMenuModel *model;

  GTree         *menus;
};


/*  local function prototypes  */

static void   gimp_menu_bar_iface_init              (GimpMenuShellInterface  *iface);

static void   gimp_menu_bar_constructed             (GObject                 *object);
static void   gimp_menu_bar_dispose                 (GObject                 *object);
static void   gimp_menu_bar_set_property            (GObject                 *object,
                                                     guint                    property_id,
                                                     const GValue            *value,
                                                     GParamSpec              *pspec);
static void   gimp_menu_bar_get_property            (GObject                 *object,
                                                     guint                    property_id,
                                                     GValue                  *value,
                                                     GParamSpec              *pspec);

static void   gimp_menu_bar_append                  (GimpMenuShell           *shell,
                                                     GMenuModel              *model);
static void   gimp_menu_bar_add_ui                  (GimpMenuShell           *shell,
                                                     const gchar            **paths,
                                                     const gchar             *action_name,
                                                     const gchar             *placeholder_key,
                                                     gboolean                 top);


G_DEFINE_TYPE_WITH_CODE (GimpMenuBar, gimp_menu_bar, GTK_TYPE_MENU_BAR,
                         G_ADD_PRIVATE (GimpMenuBar)
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_MENU_SHELL, gimp_menu_bar_iface_init))

#define parent_class gimp_menu_bar_parent_class


/*  Class functions  */

static void
gimp_menu_bar_class_init (GimpMenuBarClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = gimp_menu_bar_constructed;
  object_class->dispose      = gimp_menu_bar_dispose;
  object_class->get_property = gimp_menu_bar_get_property;
  object_class->set_property = gimp_menu_bar_set_property;

  gimp_menu_shell_install_properties (object_class);

  g_object_class_install_property (object_class, PROP_MODEL,
                                   g_param_spec_object ("model",
                                                        NULL, NULL,
                                                        G_TYPE_MENU_MODEL,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));
}

static void
gimp_menu_bar_iface_init (GimpMenuShellInterface *iface)
{
  iface->append = gimp_menu_bar_append;
  iface->add_ui = gimp_menu_bar_add_ui;
}

static void
gimp_menu_bar_init (GimpMenuBar *bar)
{
  bar->priv = gimp_menu_bar_get_instance_private (bar);

  bar->priv->menus = g_tree_new_full ((GCompareDataFunc) g_strcmp0, NULL,
                                      g_free, NULL);

  gimp_menu_shell_init (GIMP_MENU_SHELL (bar));
}

static void
gimp_menu_bar_constructed (GObject *object)
{
  G_OBJECT_CLASS (parent_class)->constructed (object);
}

static void
gimp_menu_bar_dispose (GObject *object)
{
  GimpMenuBar *bar = GIMP_MENU_BAR (object);

  g_clear_object (&bar->priv->model);
  g_clear_pointer (&bar->priv->menus, g_tree_unref);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_menu_bar_set_property (GObject      *object,
                            guint         property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  GimpMenuBar *bar = GIMP_MENU_BAR (object);

  switch (property_id)
    {
    case PROP_MODEL:
      bar->priv->model = g_value_dup_object (value);
      gimp_menu_shell_fill (GIMP_MENU_SHELL (bar), bar->priv->model, "ui-added", FALSE);
      break;

    default:
      gimp_menu_shell_set_property (object, property_id, value, pspec);
      break;
    }
}

static void
gimp_menu_bar_get_property (GObject    *object,
                            guint       property_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  GimpMenuBar *bar = GIMP_MENU_BAR (object);

  switch (property_id)
    {
    case PROP_MODEL:
      g_value_set_object (value, bar->priv->model);
      break;

    default:
      gimp_menu_shell_get_property (object, property_id, value, pspec);
      break;
    }
}

static void
gimp_menu_bar_append (GimpMenuShell *shell,
                      GMenuModel    *model)
{
  GimpMenuBar   *bar     = GIMP_MENU_BAR (shell);
  GimpUIManager *manager = gimp_menu_shell_get_manager (GIMP_MENU_SHELL (shell));
  gint           n_items;

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

      if (submenu != NULL)
        {
          GtkWidget *subcontainer;
          GtkWidget *item;

          g_return_if_fail (label != NULL);

          /* I don't show the item on purpose because
           * gimp_menu_append() will show the parent item if any of
           * the added actions are visible.
           */
          item = gtk_menu_item_new_with_mnemonic (label);
          gtk_container_add (GTK_CONTAINER (shell), item);

          subcontainer = gimp_menu_new (manager);
          gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), subcontainer);
          GIMP_MENU_SHELL_GET_INTERFACE (subcontainer)->append (GIMP_MENU_SHELL (subcontainer), submenu);
          gtk_widget_show (subcontainer);

          g_tree_insert (bar->priv->menus,
                         gimp_menu_shell_make_canonical_path (label),
                         subcontainer);
        }
      else
        {
          if (subsection != NULL)
            g_warning ("%s: unexpected subsection. Only menus are allowed in menu bar's root.",
                       G_STRFUNC);
          if (action_name == NULL)
            g_warning ("%s: unexpected placeholder '%s'. Only menus are allowed in menu bar's root.",
                       G_STRFUNC, label);
          else
            g_warning ("%s: unexpected action '%s'. Only menus are allowed in menu bar's root.",
                       G_STRFUNC, action_name);
        }
      g_free (label);
      g_free (action_name);
      g_clear_object (&submenu);
      g_clear_object (&subsection);
    }
}

static void
gimp_menu_bar_add_ui (GimpMenuShell  *shell,
                      const gchar   **paths,
                      const gchar    *action_name,
                      const gchar    *placeholder_key,
                      gboolean        top)
{
  GimpMenuBar   *bar     = GIMP_MENU_BAR (shell);
  GimpUIManager *manager = gimp_menu_shell_get_manager (GIMP_MENU_SHELL (shell));

  g_return_if_fail (paths != NULL);

  if (paths[0] == NULL)
    {
      g_warning ("%s: unexpected action '%s'. Only menus are allowed in menu bar's root.",
                 G_STRFUNC, action_name);
    }
  else
    {
      GtkWidget *menu = NULL;

      menu = g_tree_lookup (bar->priv->menus, paths[0]);

      if (menu == NULL)
        {
          GtkWidget *item;

          item = gtk_menu_item_new_with_mnemonic (paths[0]);
          gtk_container_add (GTK_CONTAINER (shell), item);

          menu = gimp_menu_new (manager);
          gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), menu);
          gtk_widget_show (menu);

          g_tree_insert (bar->priv->menus, g_strdup (paths[0]), menu);
        }

      GIMP_MENU_SHELL_GET_INTERFACE (menu)->add_ui (GIMP_MENU_SHELL (menu),
                                                    paths + 1,
                                                    action_name,
                                                    placeholder_key, top);
    }
}


/* Public functions */

GtkWidget *
gimp_menu_bar_new (GMenuModel    *model,
                   GimpUIManager *manager)
{
  g_return_val_if_fail (GIMP_IS_UI_MANAGER (manager) &&
                        G_IS_MENU_MODEL (model), NULL);

  return g_object_new (GIMP_TYPE_MENU_BAR,
                       "model",   model,
                       "manager", manager,
                       NULL);
}
