/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpuimanager.c
 * Copyright (C) 2004 Michael Natterer <mitch@gimp.org>
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
#include <gdk/gdkkeysyms.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "config/gimpxmlparser.h"

#include "core/gimp.h"

#include "gimpaction.h"
#include "gimpactiongroup.h"
#include "gimpradioaction.h"
#include "gimphelp.h"
#include "gimphelp-ids.h"
#include "gimpmenu.h"
#include "gimpmenumodel.h"
#include "gimpmenushell.h"
#include "gimptoggleaction.h"
#include "gimpuimanager.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_NAME,
  PROP_GIMP
};

enum
{
  UPDATE,
  SHOW_TOOLTIP,
  HIDE_TOOLTIP,
  UI_ADDED,
  UI_REMOVED,
  LAST_SIGNAL
};

typedef struct
{
  gchar    *path;
  gchar    *action_name;
  gboolean  top;
} GimpUIManagerMenuItem;


static void       gimp_ui_manager_constructed          (GObject        *object);
static void       gimp_ui_manager_dispose              (GObject        *object);
static void       gimp_ui_manager_finalize             (GObject        *object);
static void       gimp_ui_manager_set_property         (GObject        *object,
                                                        guint           prop_id,
                                                        const GValue   *value,
                                                        GParamSpec     *pspec);
static void       gimp_ui_manager_get_property         (GObject        *object,
                                                        guint           prop_id,
                                                        GValue         *value,
                                                        GParamSpec     *pspec);
static void       gimp_ui_manager_real_update          (GimpUIManager  *manager,
                                                        gpointer        update_data);
static GimpUIManagerUIEntry *
                  gimp_ui_manager_entry_get            (GimpUIManager  *manager,
                                                        const gchar    *ui_path);
static GimpUIManagerUIEntry *
                  gimp_ui_manager_entry_ensure         (GimpUIManager  *manager,
                                                        const gchar    *path);

static void       gimp_ui_manager_store_builder_path   (GMenuModel           *model,
                                                        GMenuModel           *original);
static GtkBuilder * gimp_ui_manager_load_builder       (const gchar          *filename,
                                                        const gchar          *root);
static void       gimp_ui_manager_parse_start_element  (GMarkupParseContext  *context,
                                                        const gchar          *element_name,
                                                        const gchar         **attribute_names,
                                                        const gchar         **attribute_values,
                                                        gpointer              user_data,
                                                        GError              **error);
static void       gimp_ui_manager_parse_end_element    (GMarkupParseContext  *context,
                                                        const gchar          *element_name,
                                                        gpointer              user_data,
                                                        GError              **error);
static void       gimp_ui_manager_parse_ui_text        (GMarkupParseContext  *context,
                                                        const gchar          *text,
                                                        gsize                 text_len,
                                                        gpointer              user_data,
                                                        GError              **error);

static void       gimp_ui_manager_delete_popdown_data  (GtkWidget      *widget,
                                                        GimpUIManager  *manager);

static void       gimp_ui_manager_menu_item_free       (GimpUIManagerMenuItem *item);

static void       gimp_ui_manager_popup_hidden         (GtkMenuShell          *popup,
                                                        gpointer               user_data);
static gboolean   gimp_ui_manager_popup_destroy        (GtkWidget             *popup);

static void       gimp_ui_manager_image_action_added   (GimpActionGroup       *group,
                                                        gchar                 *action_name,
                                                        GimpUIManager         *manager);
static void       gimp_ui_manager_image_action_removed (GimpActionGroup       *group,
                                                        gchar                 *action_name,
                                                        GimpUIManager         *manager);
static void       gimp_ui_manager_image_accels_changed (GimpAction            *action,
                                                        const gchar          **accels,
                                                        GimpUIManager         *manager);


G_DEFINE_TYPE (GimpUIManager, gimp_ui_manager, GIMP_TYPE_OBJECT)

#define parent_class gimp_ui_manager_parent_class

static guint manager_signals[LAST_SIGNAL] = { 0 };


static void
gimp_ui_manager_class_init (GimpUIManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed    = gimp_ui_manager_constructed;
  object_class->dispose        = gimp_ui_manager_dispose;
  object_class->finalize       = gimp_ui_manager_finalize;
  object_class->set_property   = gimp_ui_manager_set_property;
  object_class->get_property   = gimp_ui_manager_get_property;

  klass->update                = gimp_ui_manager_real_update;

  manager_signals[UPDATE] =
    g_signal_new ("update",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GimpUIManagerClass, update),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_POINTER);

  manager_signals[SHOW_TOOLTIP] =
    g_signal_new ("show-tooltip",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GimpUIManagerClass, show_tooltip),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_STRING);

  manager_signals[HIDE_TOOLTIP] =
    g_signal_new ("hide-tooltip",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GimpUIManagerClass, hide_tooltip),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0,
                  G_TYPE_NONE);
  manager_signals[UI_ADDED] =
    g_signal_new ("ui-added",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GimpUIManagerClass, ui_added),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 3,
                  G_TYPE_STRING,
                  G_TYPE_STRING,
                  G_TYPE_BOOLEAN);
  manager_signals[UI_REMOVED] =
    g_signal_new ("ui-removed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GimpUIManagerClass, ui_removed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 2,
                  G_TYPE_STRING,
                  G_TYPE_STRING);

  g_object_class_install_property (object_class, PROP_NAME,
                                   g_param_spec_string ("name",
                                                        NULL, NULL,
                                                        NULL,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_GIMP,
                                   g_param_spec_object ("gimp",
                                                        NULL, NULL,
                                                        GIMP_TYPE_GIMP,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  klass->managers = g_hash_table_new_full (g_str_hash, g_str_equal,
                                           g_free, NULL);
}

static void
gimp_ui_manager_init (GimpUIManager *manager)
{
  manager->name               = NULL;
  manager->gimp               = NULL;
  manager->action_groups      = NULL;
  manager->store_action_paths = FALSE;
}

static void
gimp_ui_manager_constructed (GObject *object)
{
  GimpUIManager *manager = GIMP_UI_MANAGER (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  if (manager->name)
    {
      GimpUIManagerClass *manager_class;
      GList              *list;

      manager_class = GIMP_UI_MANAGER_GET_CLASS (object);

      list = g_hash_table_lookup (manager_class->managers, manager->name);

      list = g_list_append (list, manager);

      g_hash_table_replace (manager_class->managers,
                            g_strdup (manager->name), list);
    }
}

static void
gimp_ui_manager_dispose (GObject *object)
{
  GimpUIManager *manager = GIMP_UI_MANAGER (object);

  if (manager->name)
    {
      GimpUIManagerClass *manager_class;
      GList              *list;

      manager_class = GIMP_UI_MANAGER_GET_CLASS (object);

      list = g_hash_table_lookup (manager_class->managers, manager->name);

      if (list)
        {
          list = g_list_remove (list, manager);

          if (list)
            g_hash_table_replace (manager_class->managers,
                                  g_strdup (manager->name), list);
          else
            g_hash_table_remove (manager_class->managers, manager->name);
        }
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_ui_manager_finalize (GObject *object)
{
  GimpUIManager *manager = GIMP_UI_MANAGER (object);
  GList         *list;

  for (list = manager->registered_uis; list; list = g_list_next (list))
    {
      GimpUIManagerUIEntry *entry = list->data;

      g_free (entry->ui_path);
      g_free (entry->basename);
      g_clear_object (&entry->builder);

      g_slice_free (GimpUIManagerUIEntry, entry);
    }

  g_clear_pointer (&manager->registered_uis, g_list_free);
  g_clear_pointer (&manager->name,           g_free);
  g_list_free_full (manager->action_groups,  g_object_unref);

  g_list_free_full (manager->ui_items,
                    (GDestroyNotify) gimp_ui_manager_menu_item_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_ui_manager_set_property (GObject      *object,
                              guint         prop_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  GimpUIManager *manager = GIMP_UI_MANAGER (object);

  switch (prop_id)
    {
    case PROP_NAME:
      g_free (manager->name);
      manager->name = g_value_dup_string (value);
      break;

    case PROP_GIMP:
      manager->gimp = g_value_get_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gimp_ui_manager_get_property (GObject    *object,
                              guint       prop_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  GimpUIManager *manager = GIMP_UI_MANAGER (object);

  switch (prop_id)
    {
    case PROP_NAME:
      g_value_set_string (value, manager->name);
      break;

    case PROP_GIMP:
      g_value_set_object (value, manager->gimp);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gimp_ui_manager_real_update (GimpUIManager *manager,
                             gpointer       update_data)
{
  GList *list;

  for (list = gimp_ui_manager_get_action_groups (manager);
       list;
       list = g_list_next (list))
    {
      gimp_action_group_update (list->data, update_data);
    }
}

/**
 * gimp_ui_manager_new:
 * @gimp: the @Gimp instance this ui manager belongs to
 * @name: the UI manager's name.
 *
 * Creates a new #GimpUIManager object.
 *
 * Returns: the new #GimpUIManager
 */
GimpUIManager *
gimp_ui_manager_new (Gimp        *gimp,
                     const gchar *name)
{
  GimpUIManager *manager;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  manager = g_object_new (GIMP_TYPE_UI_MANAGER,
                          "name", name,
                          "gimp", gimp,
                          NULL);

  return manager;
}

GList *
gimp_ui_managers_from_name (const gchar *name)
{
  GimpUIManagerClass *manager_class;
  GList              *list;

  g_return_val_if_fail (name != NULL, NULL);

  manager_class = g_type_class_ref (GIMP_TYPE_UI_MANAGER);

  list = g_hash_table_lookup (manager_class->managers, name);

  g_type_class_unref (manager_class);

  return list;
}

void
gimp_ui_manager_update (GimpUIManager *manager,
                        gpointer       update_data)
{
  g_return_if_fail (GIMP_IS_UI_MANAGER (manager));

  g_signal_emit (manager, manager_signals[UPDATE], 0, update_data);
}

void
gimp_ui_manager_add_action_group (GimpUIManager   *manager,
                                  GimpActionGroup *group)
{
  if (! g_list_find (manager->action_groups, group))
    manager->action_groups = g_list_prepend (manager->action_groups, g_object_ref (group));

  /* Special-case the <Image> UI Manager which should be unique and represent
   * global application actions.
   */
  if (g_strcmp0 (manager->name, "<Image>") == 0)
    {
      gchar **actions = g_action_group_list_actions (G_ACTION_GROUP (group));

      for (int i = 0; i < g_strv_length (actions); i++)
        gimp_ui_manager_image_action_added (group, actions[i], manager);

      g_signal_connect_object (group, "action-added",
                               G_CALLBACK (gimp_ui_manager_image_action_added),
                               manager, 0);
      g_signal_connect_object (group, "action-removed",
                               G_CALLBACK (gimp_ui_manager_image_action_removed),
                               manager, 0);

      g_strfreev (actions);
    }
}

GimpActionGroup *
gimp_ui_manager_get_action_group (GimpUIManager *manager,
                                  const gchar   *name)
{
  GList *list;

  g_return_val_if_fail (GIMP_IS_UI_MANAGER (manager), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  for (list = gimp_ui_manager_get_action_groups (manager);
       list;
       list = g_list_next (list))
    {
      GimpActionGroup *group = list->data;

      if (! strcmp (name, gimp_action_group_get_name (group)))
        return group;
    }

  return NULL;
}

GList *
gimp_ui_manager_get_action_groups (GimpUIManager *manager)
{
  return manager->action_groups;
}

GimpMenuModel *
gimp_ui_manager_get_model (GimpUIManager *manager,
                           const gchar   *path)
{
  GimpUIManagerUIEntry *entry;
  GimpMenuModel        *model;
  GMenuModel           *gmodel;
  GimpMenuModel        *submodel;
  gchar                *root;
  gchar                *submenus;

  root     = g_strdup (path);
  submenus = strstr (root + 1, "/");
  if (submenus != NULL)
    {
      *submenus = '\0';
      if (*(++submenus) == '\0')
        submenus = NULL;
    }

  entry         = gimp_ui_manager_entry_ensure (manager, path);
  g_return_val_if_fail (entry != NULL, NULL);

  if (entry->builder == NULL)
    {
      const gchar *menus_path_override = g_getenv ("GIMP_TESTING_MENUS_PATH");
      gchar       *full_basename;
      gchar       *filename;

      full_basename = g_strconcat (entry->basename, ".ui", NULL);

      /* In order for test cases to be able to run without GIMP being
       * installed yet, allow them to override the menus directory to the
       * menus dir in the source root
       */
      if (menus_path_override)
        {
          GList *paths = gimp_path_parse (menus_path_override, 2, FALSE, NULL);
          GList *list;

          for (list = paths; list; list = g_list_next (list))
            {
              filename = g_build_filename (list->data, full_basename, NULL);

              if (! list->next ||
                  g_file_test (filename, G_FILE_TEST_EXISTS))
                break;

              g_clear_pointer (&filename, g_free);
            }

          g_list_free_full (paths, g_free);
        }
      else
        {
          filename = g_build_filename (gimp_data_directory (), "menus", full_basename, NULL);
        }

      g_return_val_if_fail (filename != NULL, NULL);

      if (manager->gimp->be_verbose)
        g_print ("Loading menu '%s' for %s\n",
                 gimp_filename_to_utf8 (filename), entry->ui_path);

      /* The model is owned by the builder which I have to keep around. */
      entry->builder = gimp_ui_manager_load_builder (filename, root);

      g_free (filename);
      g_free (full_basename);
    }

  gmodel = G_MENU_MODEL (gtk_builder_get_object (entry->builder, root));

  g_return_val_if_fail (G_IS_MENU (gmodel), NULL);

  model = gimp_menu_model_new (manager, gmodel);

  submodel = gimp_menu_model_get_submodel (model, submenus);

  g_object_unref (model);
  g_free (root);

  return submodel;
}

void
gimp_ui_manager_remove_ui (GimpUIManager *manager,
                           const gchar   *action_name)
{
  GList *iter = manager->ui_items;

  g_return_if_fail (action_name != NULL);

  for (; iter != NULL; iter = iter->next)
    {
      GimpUIManagerMenuItem *item = iter->data;

      if (g_strcmp0 (item->action_name, action_name) == 0)
        {
          g_signal_emit (manager, manager_signals[UI_REMOVED], 0,
                         item->path, item->action_name);
          manager->ui_items = g_list_delete_link (manager->ui_items, iter);
          gimp_ui_manager_menu_item_free (item);
          break;
        }
    }
}

void
gimp_ui_manager_remove_uis (GimpUIManager *manager,
                            const gchar   *action_name_prefix)
{
  GList *iter = manager->ui_items;

  while (iter != NULL)
    {
      GimpUIManagerMenuItem *item         = iter->data;
      GList                 *current_iter = iter;

      /* Increment nearly in case we delete the list item. */
      iter = iter->next;

      if (action_name_prefix == NULL ||
          g_str_has_prefix (item->action_name, action_name_prefix))
        {
          g_signal_emit (manager, manager_signals[UI_REMOVED], 0,
                         item->path, item->action_name);
          manager->ui_items = g_list_delete_link (manager->ui_items,
                                                  current_iter);
          gimp_ui_manager_menu_item_free (item);
        }
    }
}

void
gimp_ui_manager_add_ui (GimpUIManager *manager,
                        const gchar   *path,
                        const gchar   *action_name,
                        gboolean       top)
{
  GimpUIManagerMenuItem *item;

  g_return_if_fail (GIMP_IS_UI_MANAGER (manager));
  g_return_if_fail (path != NULL);
  g_return_if_fail (action_name != NULL);

  item = g_slice_new0 (GimpUIManagerMenuItem);
  item->path        = g_strdup (path);
  item->action_name = g_strdup (action_name);
  item->top         = top;

  manager->ui_items = g_list_prepend (manager->ui_items, item);

  g_signal_emit (manager, manager_signals[UI_ADDED], 0,
                 path, action_name, top);
}

void
gimp_ui_manager_foreach_ui (GimpUIManager      *manager,
                            GimpUIMenuCallback  callback,
                            gpointer            user_data)
{
  for (GList *iter = g_list_last (manager->ui_items); iter; iter = iter->prev)
    {
      GimpUIManagerMenuItem *item = iter->data;

      callback (manager, item->path, item->action_name, item->top, user_data);
    }
}

GimpAction *
gimp_ui_manager_find_action (GimpUIManager *manager,
                             const gchar   *group_name,
                             const gchar   *action_name)
{
  GimpActionGroup *group;
  GimpAction      *action = NULL;

  g_return_val_if_fail (GIMP_IS_UI_MANAGER (manager), NULL);
  g_return_val_if_fail (action_name != NULL, NULL);

  if (g_strcmp0 (group_name, "app") == 0)
    {
      GApplication *app = manager->gimp->app;

      action = (GimpAction *) g_action_map_lookup_action (G_ACTION_MAP (app), action_name);
    }
  else if (group_name)
    {
      group = gimp_ui_manager_get_action_group (manager, group_name);

      if (group)
        action = gimp_action_group_get_action (group, action_name);
    }
  else
    {
      GList *list;
      gchar *dot;

      dot = strstr (action_name, ".");

      if (dot == NULL)
        {
          /* No group specified. */
          for (list = gimp_ui_manager_get_action_groups (manager);
               list;
               list = g_list_next (list))
            {
              group = list->data;

              action = gimp_action_group_get_action (group, action_name);

              if (action)
                break;
            }
        }
      else
        {
          gchar *gname;

          gname = g_strndup (action_name, dot - action_name);

          action = gimp_ui_manager_find_action (manager, gname, dot + 1);
          g_free (gname);
        }
    }

  return action;
}

gboolean
gimp_ui_manager_activate_action (GimpUIManager *manager,
                                 const gchar   *group_name,
                                 const gchar   *action_name)
{
  GimpAction *action;

  g_return_val_if_fail (GIMP_IS_UI_MANAGER (manager), FALSE);
  g_return_val_if_fail (action_name != NULL, FALSE);

  action = gimp_ui_manager_find_action (manager, group_name, action_name);

  if (action)
    gimp_action_activate (action);

  return (action != NULL);
}

gboolean
gimp_ui_manager_toggle_action (GimpUIManager *manager,
                               const gchar   *group_name,
                               const gchar   *action_name,
                               gboolean       active)
{
  GimpAction *action;

  g_return_val_if_fail (GIMP_IS_UI_MANAGER (manager), FALSE);
  g_return_val_if_fail (action_name != NULL, FALSE);

  action = gimp_ui_manager_find_action (manager, group_name, action_name);

  if (GIMP_IS_TOGGLE_ACTION (action))
    gimp_toggle_action_set_active (GIMP_TOGGLE_ACTION (action),
                                   active ? TRUE : FALSE);

  return GIMP_IS_TOGGLE_ACTION (action);
}

void
gimp_ui_manager_ui_register (GimpUIManager          *manager,
                             const gchar            *ui_path,
                             const gchar            *basename,
                             GimpUIManagerSetupFunc  setup_func)
{
  GimpUIManagerUIEntry *entry;

  g_return_if_fail (GIMP_IS_UI_MANAGER (manager));
  g_return_if_fail (ui_path != NULL);
  g_return_if_fail (basename != NULL);
  g_return_if_fail (gimp_ui_manager_entry_get (manager, ui_path) == NULL);

  entry = g_slice_new0 (GimpUIManagerUIEntry);

  entry->ui_path    = g_strdup (ui_path);
  entry->basename   = g_strdup (basename);
  entry->setup_func = setup_func;
  entry->setup_done = FALSE;
  entry->builder    = NULL;

  manager->registered_uis = g_list_prepend (manager->registered_uis, entry);
}

void
gimp_ui_manager_ui_popup_at_widget (GimpUIManager  *manager,
                                    const gchar    *ui_path,
                                    GimpUIManager  *child_ui_manager,
                                    const gchar    *child_ui_path,
                                    GtkWidget      *widget,
                                    GdkGravity      widget_anchor,
                                    GdkGravity      menu_anchor,
                                    const GdkEvent *trigger_event,
                                    GDestroyNotify  popdown_func,
                                    gpointer        popdown_data)
{
  GimpMenuModel *model;
  GtkWidget     *menu;

  g_return_if_fail (GIMP_IS_UI_MANAGER (manager));
  g_return_if_fail (ui_path != NULL);
  g_return_if_fail (GTK_IS_WIDGET (widget));

  menu = gimp_menu_new (manager);
  gtk_menu_attach_to_widget (GTK_MENU (menu), widget, NULL);

  model = gimp_ui_manager_get_model (manager, ui_path);
  g_return_if_fail (model != NULL);
  gimp_menu_shell_fill (GIMP_MENU_SHELL (menu), model, TRUE);
  g_object_unref (model);

  if (! menu)
    return;

  if (child_ui_manager != NULL && child_ui_path != NULL)
    {
      GimpMenuModel *child_model;
      GtkWidget     *child_menu;

      /* TODO GMenu: the "icon" attribute set in the .ui file should be visible. */
      child_model = gimp_ui_manager_get_model (child_ui_manager, child_ui_path);
      child_menu  = gimp_menu_new (child_ui_manager);
      gimp_menu_shell_fill (GIMP_MENU_SHELL (child_menu), child_model, FALSE);
      g_object_unref (child_model);

      gimp_menu_merge (GIMP_MENU (menu), GIMP_MENU (child_menu), TRUE);
    }

  if (popdown_func && popdown_data)
    {
      g_object_set_data_full (G_OBJECT (manager), "popdown-data",
                              popdown_data, popdown_func);
      g_signal_connect (menu, "selection-done",
                        G_CALLBACK (gimp_ui_manager_delete_popdown_data),
                        manager);
    }

  gtk_menu_popup_at_widget (GTK_MENU (menu), widget,
                            widget_anchor,
                            menu_anchor,
                            trigger_event);
  g_signal_connect (menu, "hide",
                    G_CALLBACK (gimp_ui_manager_popup_hidden),
                    NULL);
}

void
gimp_ui_manager_ui_popup_at_pointer (GimpUIManager  *manager,
                                     const gchar    *ui_path,
                                     GtkWidget      *attached_widget,
                                     const GdkEvent *trigger_event,
                                     GDestroyNotify  popdown_func,
                                     gpointer        popdown_data)
{
  GimpMenuModel *model;
  GtkWidget     *menu;

  g_return_if_fail (GIMP_IS_UI_MANAGER (manager));
  g_return_if_fail (ui_path != NULL);

  model = gimp_ui_manager_get_model (manager, ui_path);
  menu  = gimp_menu_new (manager);
  gtk_menu_attach_to_widget (GTK_MENU (menu), attached_widget, NULL);
  gimp_menu_shell_fill (GIMP_MENU_SHELL (menu), model, TRUE);
  g_object_unref (model);

  if (! menu)
    return;

  if (popdown_func && popdown_data)
    {
      g_object_set_data_full (G_OBJECT (manager), "popdown-data",
                              popdown_data, popdown_func);
      g_signal_connect (menu, "selection-done",
                        G_CALLBACK (gimp_ui_manager_delete_popdown_data),
                        manager);
    }

  gtk_menu_popup_at_pointer (GTK_MENU (menu), trigger_event);
  g_signal_connect (menu, "hide",
                    G_CALLBACK (gimp_ui_manager_popup_hidden),
                    NULL);
}

void
gimp_ui_manager_ui_popup_at_rect (GimpUIManager      *manager,
                                  const gchar        *ui_path,
                                  GtkWidget          *attached_widget,
                                  GdkWindow          *window,
                                  const GdkRectangle *rect,
                                  GdkGravity          rect_anchor,
                                  GdkGravity          menu_anchor,
                                  const GdkEvent     *trigger_event,
                                  GDestroyNotify      popdown_func,
                                  gpointer            popdown_data)
{
  GimpMenuModel *model;
  GtkWidget     *menu;

  g_return_if_fail (GIMP_IS_UI_MANAGER (manager));
  g_return_if_fail (ui_path != NULL);

  model = gimp_ui_manager_get_model (manager, ui_path);
  menu  = gimp_menu_new (manager);
  gtk_menu_attach_to_widget (GTK_MENU (menu), attached_widget, NULL);
  gimp_menu_shell_fill (GIMP_MENU_SHELL (menu), model, TRUE);
  g_object_unref (model);

  if (! menu)
    return;

  if (popdown_func && popdown_data)
    {
      g_object_set_data_full (G_OBJECT (manager), "popdown-data",
                              popdown_data, popdown_func);
      g_signal_connect (menu, "selection-done",
                        G_CALLBACK (gimp_ui_manager_delete_popdown_data),
                        manager);
    }

  gtk_menu_popup_at_rect (GTK_MENU (menu), window,
                          rect, rect_anchor, menu_anchor,
                          trigger_event);
  g_signal_connect (menu, "hide",
                    G_CALLBACK (gimp_ui_manager_popup_hidden),
                    NULL);
}


/*  private functions  */

static GimpUIManagerUIEntry *
gimp_ui_manager_entry_get (GimpUIManager *manager,
                           const gchar   *ui_path)
{
  GList *list;
  gchar *path;

  path = g_strdup (ui_path);

  if (strlen (path) > 1)
    {
      gchar *p = strchr (path + 1, '/');

      if (p)
        *p = '\0';
    }

  for (list = manager->registered_uis; list; list = g_list_next (list))
    {
      GimpUIManagerUIEntry *entry = list->data;

      if (! strcmp (entry->ui_path, path))
        {
          g_free (path);

          return entry;
        }
    }

  g_free (path);

  return NULL;
}

static GimpUIManagerUIEntry *
gimp_ui_manager_entry_ensure (GimpUIManager *manager,
                              const gchar   *path)
{
  GimpUIManagerUIEntry *entry;

  entry = gimp_ui_manager_entry_get (manager, path);

  if (! entry)
    {
      g_warning ("%s: no entry registered for \"%s\"", G_STRFUNC, path);
      return NULL;
    }

  if (entry->setup_func && ! entry->setup_done)
    {
      entry->setup_func (manager, entry->ui_path);
      entry->setup_done = TRUE;
    }

  return entry;
}

static void
gimp_ui_manager_store_builder_path (GMenuModel *model,
                                    GMenuModel *original)
{
  gint n_items = 0;

  g_return_if_fail (g_menu_model_get_n_items (model) == g_menu_model_get_n_items (original));

  n_items = (model != NULL ? g_menu_model_get_n_items (model) : 0);
  for (gint i = 0; i < n_items; i++)
    {
      GMenuModel *subsection;
      GMenuModel *submenu;
      GMenuModel *en_submodel = NULL;

      subsection = g_menu_model_get_item_link (G_MENU_MODEL (model), i, G_MENU_LINK_SECTION);
      submenu    = g_menu_model_get_item_link (G_MENU_MODEL (model), i, G_MENU_LINK_SUBMENU);
      if (subsection != NULL)
        {
          en_submodel = g_menu_model_get_item_link (G_MENU_MODEL (original), i, G_MENU_LINK_SECTION);

          gimp_ui_manager_store_builder_path (subsection, en_submodel);
        }
      else if (submenu != NULL)
        {
          gchar *label = NULL;

          en_submodel = g_menu_model_get_item_link (G_MENU_MODEL (original),
                                                    i, G_MENU_LINK_SUBMENU);
          gimp_ui_manager_store_builder_path (submenu, en_submodel);

          g_menu_model_get_item_attribute (G_MENU_MODEL (model), i,
                                           G_MENU_ATTRIBUTE_LABEL, "s", &label);
          if (label != NULL)
            {
              gchar *en_label = NULL;

              g_menu_model_get_item_attribute (G_MENU_MODEL (original), i,
                                               G_MENU_ATTRIBUTE_LABEL, "s", &en_label);

              g_object_set_data_full (G_OBJECT (submenu), "gimp-ui-manager-menu-model-en-label",
                                      en_label, (GDestroyNotify) g_free);
            }

          g_free (label);
        }

      g_clear_object (&submenu);
      g_clear_object (&subsection);
      g_clear_object (&en_submodel);
    }
}

static GtkBuilder *
gimp_ui_manager_load_builder (const gchar *filename,
                              const gchar *root)
{
  const GMarkupParser markup_parser =
    {
      gimp_ui_manager_parse_start_element,
      gimp_ui_manager_parse_end_element,
      gimp_ui_manager_parse_ui_text,
      NULL,  /*  passthrough  */
      NULL   /*  error        */
    };

  GtkBuilder    *builder;
  GtkBuilder    *en_builder;
  gchar         *contents;
  GimpXmlParser *xml_parser;
  GString       *new_xml;
  GMenuModel    *model;
  GMenuModel    *en_model;
  GError        *error = NULL;

  /* First load the localized version. */
  if (! g_file_get_contents (filename, &contents, NULL, &error))
    {
      g_warning ("%s: failed to read \"%s\"", G_STRFUNC, filename);
      g_clear_error (&error);
      return NULL;
    }
  builder = gtk_builder_new_from_string (contents, -1);

  /* Now transform the source XML as non-translatable and load it again. */
  new_xml = g_string_new (NULL);
  xml_parser = gimp_xml_parser_new (&markup_parser, new_xml);
  gimp_xml_parser_parse_buffer (xml_parser, contents, -1, &error);
  gimp_xml_parser_free (xml_parser);

  g_free (contents);
  contents = g_string_free (new_xml, FALSE);

  if (error != NULL)
    {
      g_warning ("%s: error parsing XML file \"%s\"", G_STRFUNC, filename);
      g_clear_error (&error);
      g_free (contents);
      return builder;
    }
  en_builder = gtk_builder_new_from_string (contents, -1);
  g_free (contents);

  model    = G_MENU_MODEL (gtk_builder_get_object (builder, root));
  en_model = G_MENU_MODEL (gtk_builder_get_object (en_builder, root));
  gimp_ui_manager_store_builder_path (model, en_model);

  g_clear_object (&en_builder);

  return builder;
}

static void
gimp_ui_manager_parse_start_element (GMarkupParseContext *context,
                                     const gchar         *element_name,
                                     const gchar        **attribute_names,
                                     const gchar        **attribute_values,
                                     gpointer             user_data,
                                     GError             **error)
{
  GString *new_xml = user_data;

  g_string_append_printf (new_xml, "<%s", element_name);
  for (gint i = 0; attribute_names[i] != NULL; i++)
    {
      if (g_strcmp0 (attribute_names[i], "translatable") == 0 ||
          g_strcmp0 (attribute_names[i], "context") == 0)
        continue;

      g_string_append_printf (new_xml, " %s=\"%s\"",
                              attribute_names[i], attribute_values[i]);
    }
  g_string_append_printf (new_xml, ">");
}

static void
gimp_ui_manager_parse_end_element (GMarkupParseContext *context,
                                   const gchar         *element_name,
                                   gpointer             user_data,
                                   GError             **error)
{
  GString *new_xml = user_data;

  g_string_append_printf (new_xml, "</%s>", element_name);
}

static void
gimp_ui_manager_parse_ui_text (GMarkupParseContext *context,
                               const gchar         *text,
                               gsize                text_len,
                               gpointer             user_data,
                               GError             **error)
{
  GString *new_xml   = user_data;
  gchar   *unchanged = g_markup_escape_text (text, text_len);

  g_string_append (new_xml, unchanged);
  g_free (unchanged);
}

static void
gimp_ui_manager_delete_popdown_data (GtkWidget     *widget,
                                     GimpUIManager *manager)
{
  g_signal_handlers_disconnect_by_func (widget,
                                        gimp_ui_manager_delete_popdown_data,
                                        manager);
  g_object_set_data (G_OBJECT (manager), "popdown-data", NULL);
}

static void
gimp_ui_manager_menu_item_free (GimpUIManagerMenuItem *item)
{
  g_free (item->path);
  g_free (item->action_name);

  g_slice_free (GimpUIManagerMenuItem, item);
}

static void
gimp_ui_manager_popup_hidden (GtkMenuShell *popup,
                              gpointer      user_data)
{
  /* Destroying the popup would happen after we finish all other events related
   * to this popup. Otherwise the action which might have been activated will
   * not run because it happens after hiding.
   */
  g_idle_add ((GSourceFunc) gimp_ui_manager_popup_destroy, g_object_ref (popup));
}

static gboolean
gimp_ui_manager_popup_destroy (GtkWidget *popup)
{
  gtk_menu_detach (GTK_MENU (popup));
  g_object_unref (popup);

  return G_SOURCE_REMOVE;
}

static void
gimp_ui_manager_image_action_added (GimpActionGroup *group,
                                    gchar           *action_name,
                                    GimpUIManager   *manager)
{
  GimpAction   *action;
  const gchar **accels;

  g_return_if_fail (GIMP_IS_UI_MANAGER (manager));
  /* The action should not already exist in the application. */
  g_return_if_fail (g_action_map_lookup_action (G_ACTION_MAP (manager->gimp->app), action_name) == NULL);

  action = gimp_action_group_get_action (group, action_name);
  g_return_if_fail (action != NULL);

  g_action_map_add_action (G_ACTION_MAP (manager->gimp->app), G_ACTION (action));
  g_signal_connect_object (action, "accels-changed",
                           G_CALLBACK (gimp_ui_manager_image_accels_changed),
                           manager, 0);

  accels = gimp_action_get_accels (action);
  if (accels && g_strv_length ((gchar **) accels) > 0)
    gimp_ui_manager_image_accels_changed (action, gimp_action_get_accels (action), manager);
}

static void
gimp_ui_manager_image_action_removed (GimpActionGroup *group,
                                      gchar           *action_name,
                                      GimpUIManager   *manager)
{
  GAction *action;

  action = g_action_map_lookup_action (G_ACTION_MAP (manager->gimp->app), action_name);

  if (action != NULL)
    {
      g_action_map_remove_action (G_ACTION_MAP (manager->gimp->app), action_name);
      g_signal_handlers_disconnect_by_func (action,
                                            G_CALLBACK (gimp_ui_manager_image_accels_changed),
                                            manager);
    }
}

static void
gimp_ui_manager_image_accels_changed (GimpAction     *action,
                                      const gchar   **accels,
                                      GimpUIManager  *manager)
{
  gchar *detailed_action_name;

  if (GIMP_IS_RADIO_ACTION (action))
    {
      gint value;

      g_object_get ((GObject *) action,
                    "value", &value,
                    NULL);
      detailed_action_name = g_strdup_printf ("app.%s(%i)",
                                               g_action_get_name (G_ACTION (action)),
                                               value);
    }
  else
    {
      detailed_action_name = g_strdup_printf ("app.%s",
                                               g_action_get_name (G_ACTION (action)));
    }

  gtk_application_set_accels_for_action (GTK_APPLICATION (manager->gimp->app),
                                         detailed_action_name,
                                         accels ? accels : (const gchar *[]) { NULL });
  g_free (detailed_action_name);
}
