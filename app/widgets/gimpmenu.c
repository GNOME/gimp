/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpmenu.c
 * Copyright (C) 2022 Jehan
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
#include "gimpprocedureaction.h"
#include "gimpradioaction.h"
#include "gimptoggleaction.h"
#include "gimpuimanager.h"


/**
 * GimpMenu:
 *
 * Our own menu widget.
 *
 * We cannot use the simpler gtk_menu_new_from_model() because it lacks
 * tooltip support and unfortunately GTK does not plan to implement this:
 * https://gitlab.gnome.org/GNOME/gtk/-/issues/785
 * This is why we need to implement our own GimpMenu subclass.
 */

enum
{
  PROP_0,
  PROP_MANAGER,
  PROP_MODEL
};


struct _GimpMenuPrivate
{
  GimpUIManager *manager;
  GMenuModel    *model;

  GTree         *submenus;
  GRegex        *menu_delimiter_regex;
};


/*  local function prototypes  */

static void   gimp_menu_constructed             (GObject             *object);
static void   gimp_menu_dispose                 (GObject             *object);
static void   gimp_menu_set_property            (GObject             *object,
                                                 guint                property_id,
                                                 const GValue        *value,
                                                 GParamSpec          *pspec);
static void   gimp_menu_get_property            (GObject             *object,
                                                 guint                property_id,
                                                 GValue              *value,
                                                 GParamSpec          *pspec);

static void   gimp_menu_update                  (GimpMenu            *menu,
                                                 GtkContainer        *container,
                                                 const gchar         *parent_key,
                                                 GMenuModel          *model);

void          gimp_menu_ui_added                (GimpUIManager       *manager,
                                                 const gchar         *path,
                                                 const gchar         *action_name,
                                                 gboolean             top,
                                                 GimpMenu            *menu);

static void   gimp_menu_radio_item_toggled      (GtkWidget           *item,
                                                 GAction             *action);

static void   gimp_menu_toggle_action_changed   (GimpAction          *action,
                                                 GVariant            *value G_GNUC_UNUSED,
                                                 GtkCheckMenuItem    *item);
static void gimp_menu_procedure_action_activate (GtkMenuItem         *item,
                                                 GimpAction          *action);
static void   gimp_menu_action_notify_sensitive (GimpAction          *action,
                                                 const GParamSpec    *pspec,
                                                 GtkCheckMenuItem    *item);
static void   gimp_menu_action_notify_visible   (GimpAction          *action,
                                                 const GParamSpec    *pspec,
                                                 GtkWidget           *item);
static void   gimp_menu_action_notify_label     (GimpAction          *action,
                                                 const GParamSpec    *pspec,
                                                 GtkMenuItem         *item);
static void   gimp_menu_action_notify_tooltip   (GimpAction          *action,
                                                 const GParamSpec    *pspec,
                                                 GtkWidget           *item);

static GtkContainer * gimp_menu_add_submenu     (GimpMenu            *menu,
                                                 const gchar         *label,
                                                 GtkContainer        *parent,
                                                 const gchar         *parent_key,
                                                 const gchar        **key);
static void   gimp_menu_add_action              (GimpMenu            *menu,
                                                 GtkContainer        *container,
                                                 const gchar         *action_name,
                                                 GtkRadioMenuItem   **group);

static gchar * gimp_menu_make_canonical_path    (GimpMenu            *menu,
                                                 const gchar         *path);


G_DEFINE_TYPE_WITH_PRIVATE (GimpMenu, gimp_menu, GTK_TYPE_MENU_BAR)

#define parent_class gimp_menu_parent_class


/*  private functions  */

static void
gimp_menu_class_init (GimpMenuClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = gimp_menu_constructed;
  object_class->dispose      = gimp_menu_dispose;
  object_class->get_property = gimp_menu_get_property;
  object_class->set_property = gimp_menu_set_property;

  g_object_class_install_property (object_class, PROP_MANAGER,
                                   g_param_spec_object ("manager",
                                                        NULL, NULL,
                                                        GIMP_TYPE_UI_MANAGER,
                                                        GIMP_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (object_class, PROP_MODEL,
                                   g_param_spec_object ("model",
                                                        NULL, NULL,
                                                        G_TYPE_MENU_MODEL,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));
}

static void
gimp_menu_init (GimpMenu *menu)
{
  menu->priv = gimp_menu_get_instance_private (menu);

  menu->priv->submenus = g_tree_new_full ((GCompareDataFunc) g_strcmp0, NULL,
                                          g_free, NULL);
  menu->priv->menu_delimiter_regex = g_regex_new ("/+", 0, 0, NULL);
}

static void
gimp_menu_constructed (GObject *object)
{
  GimpMenu *menu = GIMP_MENU (object);

  g_signal_connect (menu->priv->manager, "ui-added",
                    G_CALLBACK (gimp_menu_ui_added),
                    menu);
  gimp_ui_manager_foreach_ui (menu->priv->manager,
                              (GimpUIMenuCallback) gimp_menu_ui_added,
                              menu);

  G_OBJECT_CLASS (parent_class)->constructed (object);
}

static void
gimp_menu_dispose (GObject *object)
{
  GimpMenu *menu = GIMP_MENU (object);

  if (menu->priv->menu_delimiter_regex != NULL)
    {
      g_regex_unref (menu->priv->menu_delimiter_regex);
      menu->priv->menu_delimiter_regex = NULL;
    }
  if (menu->priv->submenus != NULL)
    {
      g_tree_unref (menu->priv->submenus);
      menu->priv->submenus = NULL;
    }
  if (menu->priv->manager != NULL)
    {
      /* The whole program may be quitting. */
      g_signal_handlers_disconnect_by_func (menu->priv->manager,
                                            G_CALLBACK (gimp_menu_ui_added),
                                            menu);
    }
  g_clear_object (&menu->priv->model);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_menu_set_property (GObject      *object,
                        guint         property_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
  GimpMenu *menu = GIMP_MENU (object);

  switch (property_id)
    {
    case PROP_MODEL:
      menu->priv->model = g_value_dup_object (value);
      gimp_menu_update (menu, NULL, NULL, NULL);
      break;
    case PROP_MANAGER:
      g_set_weak_pointer (&menu->priv->manager, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_menu_get_property (GObject    *object,
                        guint       property_id,
                        GValue     *value,
                        GParamSpec *pspec)
{
  GimpMenu *menu = GIMP_MENU (object);

  switch (property_id)
    {
    case PROP_MODEL:
      g_value_set_object (value, menu->priv->model);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


/* Public functions */

GtkWidget *
gimp_menu_new (GMenuModel    *model,
               GimpUIManager *manager)
{
  g_return_val_if_fail (GIMP_IS_UI_MANAGER (manager) &&
                        G_IS_MENU_MODEL (model), NULL);

  return g_object_new (GIMP_TYPE_MENU,
                       "model",   model,
                       "manager", manager,
                       NULL);
}


/* Private functions */

static void
gimp_menu_update (GimpMenu     *menu,
                  GtkContainer *container,
                  const gchar  *parent_key,
                  GMenuModel   *model)
{
  static GtkRadioMenuItem *group = NULL;
  gint                     n_items;

  if (container == NULL)
    {
      container  = GTK_CONTAINER (menu);
      model      = menu->priv->model;
      parent_key = "";
    }

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
          gtk_container_add (container, item);
          gtk_widget_show (item);

          gimp_menu_update (menu, container, parent_key, subsection);

          item = gtk_separator_menu_item_new ();
          gtk_container_add (container, item);
          gtk_widget_show (item);
        }
      else if (submenu != NULL)
        {
          GtkContainer *subcontainer;
          const gchar  *key = NULL;

          group = NULL;

          subcontainer = gimp_menu_add_submenu (menu, label, container, parent_key, &key);
          gimp_menu_update (menu, subcontainer, key, submenu);
        }
      else
        {
          gimp_menu_add_action (menu, container, action_name, &group);
        }
      g_free (label);
      g_free (action_name);
    }
}

void
gimp_menu_ui_added (GimpUIManager *manager,
                    const gchar   *path,
                    const gchar   *action_name,
                    gboolean       top,
                    GimpMenu      *menu)
{
  gchar        *canonical_path;
  GtkContainer *container = NULL;
  gchar        *new_path  = NULL;
  const gchar  *parent_key;

  canonical_path = gimp_menu_make_canonical_path (menu, path);

  container = g_tree_lookup (menu->priv->submenus, canonical_path);
  while (container == NULL)
    {
      gchar *tmp = new_path;

      new_path = g_strrstr (canonical_path, "/");
      if (tmp != NULL)
        *tmp = '/';

      if (new_path == NULL)
        {
          /* The whole path needs to be created. */
          new_path = canonical_path;
          break;
        }

      *new_path = '\0';

      container = g_tree_lookup (menu->priv->submenus, canonical_path);
    }

  if (new_path != NULL && new_path != canonical_path)
    new_path++;

  if (container == NULL)
    {
      container  = GTK_CONTAINER (menu);
      parent_key = "";
    }
  else
    {
      parent_key = canonical_path;
    }

  if (new_path != NULL)
    {
      gchar **sub_labels;

      sub_labels = g_strsplit (new_path, "/", -1);
      for (gint i = 0; sub_labels[i] != NULL; i++)
        if (g_strcmp0 (sub_labels[i], "") != 0)
          container = gimp_menu_add_submenu (menu, sub_labels[i],
                                             container,
                                             parent_key,
                                             &parent_key);
      g_strfreev (sub_labels);
    }

  gimp_menu_add_action (menu, container, action_name, NULL);

  g_free (canonical_path);
}

static void
gimp_menu_radio_item_toggled (GtkWidget *item,
                              GAction   *action)
{
  gboolean active = gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (item));

  /* TODO: when we remove GtkAction dependency, GimpRadioAction should become a
   * child of GimpToggleAction, and therefore, we'll be able to use
   * gimp_toggle_action_set_active().
   */
  gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), active);
}

static void
gimp_menu_toggle_action_changed (GimpAction       *action,
                                 /* Unused because this is used for 2 signals
                                  * where the GVariant refers to different data.
                                  */
                                 GVariant         *value G_GNUC_UNUSED,
                                 GtkCheckMenuItem *item)
{
  gchar    *action_name;
  gboolean  active;

  action_name = g_strdup (gtk_actionable_get_action_name (GTK_ACTIONABLE (item)));
  /* TODO: use gimp_toggle_action_get_active() when GimpToggleAction and
   * GimpRadioAction become direct parent of each other.
   */
  active      = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

  /* Make sure we don't activate the action. */
  gtk_actionable_set_action_name (GTK_ACTIONABLE (item), NULL);

  gtk_check_menu_item_set_active (item, active);

  gtk_actionable_set_action_name (GTK_ACTIONABLE (item), action_name);
  g_free (action_name);
}

static void
gimp_menu_procedure_action_activate (GtkMenuItem *item,
                                     GimpAction  *action)
{
  gimp_action_activate (action);
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
  gtk_widget_set_visible (item, gimp_action_is_visible (action));
}

static void
gimp_menu_action_notify_label (GimpAction       *action,
                               const GParamSpec *pspec,
                               GtkMenuItem      *item)
{
  gtk_menu_item_set_label (item, gimp_action_get_label (action));
}

static void
gimp_menu_action_notify_tooltip (GimpAction       *action,
                                 const GParamSpec *pspec,
                                 GtkWidget        *item)
{
  gtk_widget_set_tooltip_text (item,
                               gimp_action_get_tooltip (GIMP_ACTION (action)));
}

static GtkContainer *
gimp_menu_add_submenu (GimpMenu      *menu,
                       const gchar   *label,
                       GtkContainer  *parent,
                       const gchar   *parent_key,
                       const gchar  **key)
{
  GtkWidget *subcontainer;
  GtkWidget *item;
  gchar     *tmp;
  gchar     *new_key;

  g_return_val_if_fail (GTK_IS_CONTAINER (parent) && parent_key != NULL, NULL);

  if (parent == NULL)
    parent = GTK_CONTAINER (menu);

  item = gtk_menu_item_new_with_mnemonic (label);
  gtk_container_add (parent, item);
  gtk_widget_show (item);

  subcontainer = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), subcontainer);
  gtk_widget_show (subcontainer);

  tmp = g_strconcat (parent_key ? parent_key : "", "/",
                     label, NULL);
  new_key = gimp_menu_make_canonical_path (menu, tmp);
  g_free (tmp);

  g_tree_insert (menu->priv->submenus, new_key, subcontainer);

  if (key)
    *key = (const gchar *) new_key;

  return GTK_CONTAINER (subcontainer);
}

static void
gimp_menu_add_action (GimpMenu          *menu,
                      GtkContainer      *container,
                      const gchar       *action_name,
                      GtkRadioMenuItem **group)
{
  GApplication *app;
  GAction      *action;
  const gchar  *action_label;
  GtkWidget    *item;

  app = menu->priv->manager->gimp->app;

  if (g_str_has_prefix (action_name, "app."))
    action = g_action_map_lookup_action (G_ACTION_MAP (app), action_name + 4);
  else
    action = g_action_map_lookup_action (G_ACTION_MAP (app), action_name);

  g_return_if_fail (GIMP_IS_ACTION (action));

  action_label = gimp_action_get_label (GIMP_ACTION (action));
  g_return_if_fail (action_label != NULL);

  if (GIMP_IS_TOGGLE_ACTION (action))
    {
      item  = gtk_check_menu_item_new_with_mnemonic (action_label);
      gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (item),
                                      gimp_toggle_action_get_active (GIMP_TOGGLE_ACTION (action)));
      if (group)
        *group = NULL;

      g_signal_connect (action, "gimp-change-state",
                        G_CALLBACK (gimp_menu_toggle_action_changed),
                        item);
    }
  else if (GIMP_IS_RADIO_ACTION (action))
    {
      item = gtk_radio_menu_item_new_with_mnemonic_from_widget (group ? *group : NULL, action_label);
      gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (item),
                                      /* TODO: see comment in gimp_menu_radio_item_toggled(). */
                                      gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)));

      if (group)
        *group = GTK_RADIO_MENU_ITEM (item);

      g_signal_connect (item, "toggled",
                        G_CALLBACK (gimp_menu_radio_item_toggled),
                        action);
      g_signal_connect (action, "gimp-change-state",
                        G_CALLBACK (gimp_menu_toggle_action_changed),
                        item);
    }
  else if (GIMP_IS_PROCEDURE_ACTION (action))
    {
      item = gtk_menu_item_new_with_mnemonic (action_label);

      if (group)
        *group = NULL;

      g_signal_connect (item, "activate",
                        G_CALLBACK (gimp_menu_procedure_action_activate),
                        action);
    }
  else
    {
      item = gtk_menu_item_new_with_mnemonic (action_label);

      if (group)
        *group = NULL;
    }

  gtk_actionable_set_action_name (GTK_ACTIONABLE (item), action_name);

  g_signal_connect_object (action, "notify::label",
                           G_CALLBACK (gimp_menu_action_notify_label),
                           item, 0);

  gtk_widget_set_sensitive (GTK_WIDGET (item),
                            gimp_action_is_sensitive (GIMP_ACTION (action), NULL));
  g_signal_connect_object (action, "notify::sensitive",
                           G_CALLBACK (gimp_menu_action_notify_sensitive),
                           item, 0);

  if (gimp_action_get_tooltip (GIMP_ACTION (action)))
    gtk_widget_set_tooltip_text (item,
                                 gimp_action_get_tooltip (GIMP_ACTION (action)));
  g_signal_connect_object (action, "notify::tooltip",
                           G_CALLBACK (gimp_menu_action_notify_tooltip),
                           item, 0);

  gtk_container_add (container, item);

  gtk_widget_set_visible (item,
                          gimp_action_is_visible (GIMP_ACTION (action)));
  g_signal_connect_object (action, "notify::visible",
                           G_CALLBACK (gimp_menu_action_notify_visible),
                           item, 0);
}

static gchar *
gimp_menu_make_canonical_path (GimpMenu    *menu,
                               const gchar *path)
{
  gchar **split_path;
  gchar  *canonical_path;

  split_path = g_regex_split (menu->priv->menu_delimiter_regex,
                              path, 0);
  canonical_path = g_strjoinv ("/", split_path);
  g_strfreev (split_path);
  if (canonical_path[strlen (canonical_path) - 1] == '/')
    canonical_path[strlen (canonical_path) - 1] = '\0';

  split_path = g_strsplit (canonical_path, "_", -1);
  g_free (canonical_path);
  canonical_path = g_strjoinv ("", split_path);
  g_strfreev (split_path);

  return canonical_path;
}
