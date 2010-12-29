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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimpmarshal.h"

#include "gimpactiongroup.h"
#include "gimphelp.h"
#include "gimphelp-ids.h"
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
  LAST_SIGNAL
};


static void       gimp_ui_manager_constructed         (GObject        *object);
static void       gimp_ui_manager_dispose             (GObject        *object);
static void       gimp_ui_manager_finalize            (GObject        *object);
static void       gimp_ui_manager_set_property        (GObject        *object,
                                                       guint           prop_id,
                                                       const GValue   *value,
                                                       GParamSpec     *pspec);
static void       gimp_ui_manager_get_property        (GObject        *object,
                                                       guint           prop_id,
                                                       GValue         *value,
                                                       GParamSpec     *pspec);
static void       gimp_ui_manager_connect_proxy       (GtkUIManager   *manager,
                                                       GtkAction      *action,
                                                       GtkWidget      *proxy);
static GtkWidget *gimp_ui_manager_get_widget          (GtkUIManager   *manager,
                                                       const gchar    *path);
static GtkAction *gimp_ui_manager_get_action          (GtkUIManager   *manager,
                                                       const gchar    *path);
static void       gimp_ui_manager_real_update         (GimpUIManager  *manager,
                                                       gpointer        update_data);
static GimpUIManagerUIEntry *
                  gimp_ui_manager_entry_get           (GimpUIManager  *manager,
                                                       const gchar    *ui_path);
static gboolean   gimp_ui_manager_entry_load          (GimpUIManager  *manager,
                                                       GimpUIManagerUIEntry *entry,
                                                       GError        **error);
static GimpUIManagerUIEntry *
                  gimp_ui_manager_entry_ensure        (GimpUIManager  *manager,
                                                       const gchar    *path);
static void       gimp_ui_manager_menu_position       (GtkMenu        *menu,
                                                       gint           *x,
                                                       gint           *y,
                                                       gpointer        data);
static void       gimp_ui_manager_menu_pos            (GtkMenu        *menu,
                                                       gint           *x,
                                                       gint           *y,
                                                       gboolean       *push_in,
                                                       gpointer        data);
static void       gimp_ui_manager_delete_popdown_data (GtkWidget      *widget,
                                                       GimpUIManager  *manager);
static void       gimp_ui_manager_item_realize        (GtkWidget      *widget,
                                                       GimpUIManager  *manager);
static void       gimp_ui_manager_menu_item_select    (GtkWidget      *widget,
                                                       GimpUIManager  *manager);
static void       gimp_ui_manager_menu_item_deselect  (GtkWidget      *widget,
                                                       GimpUIManager  *manager);
static gboolean   gimp_ui_manager_item_key_press      (GtkWidget      *widget,
                                                       GdkEventKey    *kevent,
                                                       GimpUIManager  *manager);
static GtkWidget *find_widget_under_pointer           (GdkWindow      *window,
                                                       gint           *x,
                                                       gint           *y);


G_DEFINE_TYPE (GimpUIManager, gimp_ui_manager, GTK_TYPE_UI_MANAGER)

#define parent_class gimp_ui_manager_parent_class

static guint manager_signals[LAST_SIGNAL] = { 0 };


static void
gimp_ui_manager_class_init (GimpUIManagerClass *klass)
{
  GObjectClass      *object_class  = G_OBJECT_CLASS (klass);
  GtkUIManagerClass *manager_class = GTK_UI_MANAGER_CLASS (klass);

  object_class->constructed    = gimp_ui_manager_constructed;
  object_class->dispose        = gimp_ui_manager_dispose;
  object_class->finalize       = gimp_ui_manager_finalize;
  object_class->set_property   = gimp_ui_manager_set_property;
  object_class->get_property   = gimp_ui_manager_get_property;

  manager_class->connect_proxy = gimp_ui_manager_connect_proxy;
  manager_class->get_widget    = gimp_ui_manager_get_widget;
  manager_class->get_action    = gimp_ui_manager_get_action;

  klass->update                = gimp_ui_manager_real_update;

  manager_signals[UPDATE] =
    g_signal_new ("update",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GimpUIManagerClass, update),
                  NULL, NULL,
                  gimp_marshal_VOID__POINTER,
                  G_TYPE_NONE, 1,
                  G_TYPE_POINTER);

  manager_signals[SHOW_TOOLTIP] =
    g_signal_new ("show-tooltip",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GimpUIManagerClass, show_tooltip),
                  NULL, NULL,
                  gimp_marshal_VOID__STRING,
                  G_TYPE_NONE, 1,
                  G_TYPE_STRING);

  manager_signals[HIDE_TOOLTIP] =
    g_signal_new ("hide-tooltip",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GimpUIManagerClass, hide_tooltip),
                  NULL, NULL,
                  gimp_marshal_VOID__VOID,
                  G_TYPE_NONE, 0,
                  G_TYPE_NONE);

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
  manager->name = NULL;
  manager->gimp = NULL;
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

      if (entry->widget)
        g_object_unref (entry->widget);

      g_slice_free (GimpUIManagerUIEntry, entry);
    }

  g_list_free (manager->registered_uis);
  manager->registered_uis = NULL;

  if (manager->name)
    {
      g_free (manager->name);
      manager->name = NULL;
    }

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
gimp_ui_manager_connect_proxy (GtkUIManager *manager,
                               GtkAction    *action,
                               GtkWidget    *proxy)
{
  g_object_set_qdata (G_OBJECT (proxy), GIMP_HELP_ID,
                      g_object_get_qdata (G_OBJECT (action), GIMP_HELP_ID));

  if (GTK_IS_MENU_ITEM (proxy))
    {
      g_signal_connect (proxy, "select",
                        G_CALLBACK (gimp_ui_manager_menu_item_select),
                        manager);
      g_signal_connect (proxy, "deselect",
                        G_CALLBACK (gimp_ui_manager_menu_item_deselect),
                        manager);

      g_signal_connect_after (proxy, "realize",
                              G_CALLBACK (gimp_ui_manager_item_realize),
                              manager);
    }
}

static GtkWidget *
gimp_ui_manager_get_widget (GtkUIManager *manager,
                            const gchar  *path)
{
  GimpUIManagerUIEntry *entry;

  entry = gimp_ui_manager_entry_ensure (GIMP_UI_MANAGER (manager), path);

  if (entry)
    {
      if (! strcmp (entry->ui_path, path))
        return entry->widget;

      return GTK_UI_MANAGER_CLASS (parent_class)->get_widget (manager, path);
    }

  return NULL;
}

static GtkAction *
gimp_ui_manager_get_action (GtkUIManager *manager,
                            const gchar  *path)
{
  if (gimp_ui_manager_entry_ensure (GIMP_UI_MANAGER (manager), path))
    return GTK_UI_MANAGER_CLASS (parent_class)->get_action (manager, path);

  return NULL;
}

static void
gimp_ui_manager_real_update (GimpUIManager *manager,
                             gpointer       update_data)
{
  GList *list;

  for (list = gtk_ui_manager_get_action_groups (GTK_UI_MANAGER (manager));
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

GimpActionGroup *
gimp_ui_manager_get_action_group (GimpUIManager *manager,
                                  const gchar   *name)
{
  GList *list;

  g_return_val_if_fail (GIMP_IS_UI_MANAGER (manager), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  for (list = gtk_ui_manager_get_action_groups (GTK_UI_MANAGER (manager));
       list;
       list = g_list_next (list))
    {
      GimpActionGroup *group = list->data;

      if (! strcmp (name, gtk_action_group_get_name (GTK_ACTION_GROUP (group))))
        return group;
    }

  return NULL;
}

GtkAction *
gimp_ui_manager_find_action (GimpUIManager *manager,
                             const gchar   *group_name,
                             const gchar   *action_name)
{
  GimpActionGroup *group;
  GtkAction       *action = NULL;

  g_return_val_if_fail (GIMP_IS_UI_MANAGER (manager), NULL);
  g_return_val_if_fail (action_name != NULL, NULL);

  if (group_name)
    {
      group = gimp_ui_manager_get_action_group (manager, group_name);

      if (group)
        action = gtk_action_group_get_action (GTK_ACTION_GROUP (group),
                                              action_name);
    }
  else
    {
      GList *list;

      for (list = gtk_ui_manager_get_action_groups (GTK_UI_MANAGER (manager));
           list;
           list = g_list_next (list))
        {
          group = list->data;

          action = gtk_action_group_get_action (GTK_ACTION_GROUP (group),
                                                action_name);

          if (action)
            break;
        }
    }

  return action;
}

gboolean
gimp_ui_manager_activate_action (GimpUIManager *manager,
                                 const gchar   *group_name,
                                 const gchar   *action_name)
{
  GtkAction *action;

  g_return_val_if_fail (GIMP_IS_UI_MANAGER (manager), FALSE);
  g_return_val_if_fail (action_name != NULL, FALSE);

  action = gimp_ui_manager_find_action (manager, group_name, action_name);

  if (action)
    gtk_action_activate (action);

  return (action != NULL);
}

gboolean
gimp_ui_manager_toggle_action (GimpUIManager *manager,
                               const gchar   *group_name,
                               const gchar   *action_name,
                               gboolean       active)
{
  GtkAction *action;

  g_return_val_if_fail (GIMP_IS_UI_MANAGER (manager), FALSE);
  g_return_val_if_fail (action_name != NULL, FALSE);

  action = gimp_ui_manager_find_action (manager, group_name, action_name);

  if (GTK_IS_TOGGLE_ACTION (action))
    gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action),
                                  active ? TRUE : FALSE);

  return GTK_IS_TOGGLE_ACTION (action);
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
  entry->merge_id   = 0;
  entry->widget     = NULL;

  manager->registered_uis = g_list_prepend (manager->registered_uis, entry);
}


typedef struct
{
  guint x;
  guint y;
} MenuPos;

static void
menu_pos_free (MenuPos *pos)
{
  g_slice_free (MenuPos, pos);
}

void
gimp_ui_manager_ui_popup (GimpUIManager        *manager,
                          const gchar          *ui_path,
                          GtkWidget            *parent,
                          GimpMenuPositionFunc  position_func,
                          gpointer              position_data,
                          GDestroyNotify        popdown_func,
                          gpointer              popdown_data)
{
  GtkWidget *widget;
  GdkEvent  *current_event;
  gint       x, y;
  guint      button;
  guint32    activate_time;
  MenuPos   *menu_pos;

  g_return_if_fail (GIMP_IS_UI_MANAGER (manager));
  g_return_if_fail (ui_path != NULL);
  g_return_if_fail (parent == NULL || GTK_IS_WIDGET (parent));

  widget = gtk_ui_manager_get_widget (GTK_UI_MANAGER (manager), ui_path);

  if (GTK_IS_MENU_ITEM (widget))
    widget = gtk_menu_item_get_submenu (GTK_MENU_ITEM (widget));

  if (! widget)
    return;

  g_return_if_fail (GTK_IS_MENU (widget));

  if (! position_func)
    {
      position_func = gimp_ui_manager_menu_position;
      position_data = parent;
    }

  (* position_func) (GTK_MENU (widget), &x, &y, position_data);

  current_event = gtk_get_current_event ();

  if (current_event && current_event->type == GDK_BUTTON_PRESS)
    {
      GdkEventButton *bevent = (GdkEventButton *) current_event;

      button        = bevent->button;
      activate_time = bevent->time;
    }
  else
    {
      button        = 0;
      activate_time = 0;
    }

  if (current_event)
    gdk_event_free (current_event);

  menu_pos = g_object_get_data (G_OBJECT (widget), "menu-pos");

  if (! menu_pos)
    {
      menu_pos = g_slice_new0 (MenuPos);
      g_object_set_data_full (G_OBJECT (widget), "menu-pos", menu_pos,
                              (GDestroyNotify) menu_pos_free);
    }

  menu_pos->x = x;
  menu_pos->y = y;

  if (popdown_func && popdown_data)
    {
      g_object_set_data_full (G_OBJECT (manager), "popdown-data",
                              popdown_data, popdown_func);
      g_signal_connect (widget, "selection-done",
                        G_CALLBACK (gimp_ui_manager_delete_popdown_data),
                        manager);
    }

  gtk_menu_popup (GTK_MENU (widget),
                  NULL, NULL,
                  gimp_ui_manager_menu_pos, menu_pos,
                  button, activate_time);
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

static gboolean
gimp_ui_manager_entry_load (GimpUIManager         *manager,
                            GimpUIManagerUIEntry  *entry,
                            GError               **error)
{
  gchar       *filename           = NULL;
  const gchar *menus_dir_override = g_getenv ("GIMP_TESTING_MENUS_DIR");

  /* In order for test cases to be able to run without GIMP being
   * installed yet, allow them to override the menus directory to the
   * menus dir in the source root
   */
  if (menus_dir_override)
    filename = g_build_filename (menus_dir_override, entry->basename, NULL);
  else
    filename = g_build_filename (gimp_data_directory (), "menus",
                                 entry->basename, NULL);

  if (manager->gimp->be_verbose)
    g_print ("loading menu '%s' for %s\n",
             gimp_filename_to_utf8 (filename), entry->ui_path);

  entry->merge_id = gtk_ui_manager_add_ui_from_file (GTK_UI_MANAGER (manager),
                                                     filename, error);

  g_free (filename);

  if (! entry->merge_id)
    return FALSE;

  return TRUE;
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

  if (! entry->merge_id)
    {
      GError *error = NULL;

      if (! gimp_ui_manager_entry_load (manager, entry, &error))
        {
          if (error->domain == G_FILE_ERROR &&
              error->code == G_FILE_ERROR_EXIST)
            {
              gimp_message (manager->gimp, NULL, GIMP_MESSAGE_ERROR,
                            "%s\n\n%s\n\n%s",
                            _("Your GIMP installation is incomplete:"),
                            error->message,
                            _("Please make sure the menu XML files are "
                              "correctly installed."));
            }
          else
            {
              gimp_message (manager->gimp, NULL, GIMP_MESSAGE_ERROR,
                            _("There was an error parsing the menu definition "
                              "from %s: %s"),
                            gimp_filename_to_utf8 (entry->basename),
                            error->message);
            }

          g_clear_error (&error);
          return NULL;
        }
    }

  if (! entry->widget)
    {
      GtkUIManager *gtk_manager = GTK_UI_MANAGER (manager);

      entry->widget =
        GTK_UI_MANAGER_CLASS (parent_class)->get_widget (gtk_manager,
                                                         entry->ui_path);

      if (entry->widget)
        {
          g_object_ref (entry->widget);

          /*  take ownership of popup menus  */
          if (GTK_IS_MENU (entry->widget))
            {
              g_object_ref_sink (entry->widget);
              g_object_unref (entry->widget);
            }

          if (entry->setup_func)
            entry->setup_func (manager, entry->ui_path);
        }
      else
        {
          g_warning ("%s: \"%s\" does not contain registered toplevel "
                     "widget \"%s\"",
                     G_STRFUNC, entry->basename, entry->ui_path);
          return NULL;
        }
    }

  return entry;
}

static void
gimp_ui_manager_menu_position (GtkMenu  *menu,
                               gint     *x,
                               gint     *y,
                               gpointer  data)
{
  GdkScreen      *screen;
  GtkRequisition  requisition;
  GdkRectangle    rect;
  gint            monitor;
  gint            pointer_x;
  gint            pointer_y;

  g_return_if_fail (GTK_IS_MENU (menu));
  g_return_if_fail (x != NULL);
  g_return_if_fail (y != NULL);
  g_return_if_fail (GTK_IS_WIDGET (data));

  gdk_display_get_pointer (gtk_widget_get_display (GTK_WIDGET (data)),
                           &screen, &pointer_x, &pointer_y, NULL);

  monitor = gdk_screen_get_monitor_at_point (screen, pointer_x, pointer_y);
  gdk_screen_get_monitor_workarea (screen, monitor, &rect);

  gtk_menu_set_screen (menu, screen);

  gtk_widget_get_preferred_size (GTK_WIDGET (menu), &requisition, NULL);

  if (gtk_widget_get_direction (GTK_WIDGET (menu)) == GTK_TEXT_DIR_RTL)
    {
      *x = pointer_x - 2 - requisition.width;

      if (*x < rect.x)
        *x = pointer_x + 2;
    }
  else
    {
      *x = pointer_x + 2;

      if (*x + requisition.width > rect.x + rect.width)
        *x = pointer_x - 2 - requisition.width;
    }

  *y = pointer_y + 2;

  if (*y + requisition.height > rect.y + rect.height)
    *y = pointer_y - 2 - requisition.height;

  if (*x < rect.x) *x = rect.x;
  if (*y < rect.y) *y = rect.y;
}

static void
gimp_ui_manager_menu_pos (GtkMenu  *menu,
                          gint     *x,
                          gint     *y,
                          gboolean *push_in,
                          gpointer  data)
{
  MenuPos *menu_pos = data;

  *x = menu_pos->x;
  *y = menu_pos->y;
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
gimp_ui_manager_item_realize (GtkWidget     *widget,
                              GimpUIManager *manager)
{
  GtkWidget *menu;
  GtkWidget *submenu;

  g_signal_handlers_disconnect_by_func (widget,
                                        gimp_ui_manager_item_realize,
                                        manager);

  menu = gtk_widget_get_parent (widget);

  if (GTK_IS_MENU_SHELL (menu))
    {
      static GQuark quark_key_press_connected = 0;

      if (! quark_key_press_connected)
        quark_key_press_connected =
          g_quark_from_static_string ("gimp-menu-item-key-press-connected");

      if (! GPOINTER_TO_INT (g_object_get_qdata (G_OBJECT (menu),
                                                 quark_key_press_connected)))
        {
          g_signal_connect (menu, "key-press-event",
                            G_CALLBACK (gimp_ui_manager_item_key_press),
                            manager);

          g_object_set_qdata (G_OBJECT (menu),
                              quark_key_press_connected,
                              GINT_TO_POINTER (TRUE));
        }
    }

  submenu = gtk_menu_item_get_submenu (GTK_MENU_ITEM (widget));

  if (submenu)
    g_object_set_qdata (G_OBJECT (submenu), GIMP_HELP_ID,
                        g_object_get_qdata (G_OBJECT (widget),
                                            GIMP_HELP_ID));
}

static void
gimp_ui_manager_menu_item_select (GtkWidget     *widget,
                                  GimpUIManager *manager)
{
  GtkAction *action =
    gtk_activatable_get_related_action (GTK_ACTIVATABLE (widget));

  if (action)
    {
      const gchar *tooltip = gtk_action_get_tooltip (action);

      if (tooltip)
        g_signal_emit (manager, manager_signals[SHOW_TOOLTIP], 0, tooltip);
    }
}

static void
gimp_ui_manager_menu_item_deselect (GtkWidget     *widget,
                                    GimpUIManager *manager)
{
  g_signal_emit (manager, manager_signals[HIDE_TOOLTIP], 0);
}

static gboolean
gimp_ui_manager_item_key_press (GtkWidget     *widget,
                                GdkEventKey   *kevent,
                                GimpUIManager *manager)
{
  gchar *help_id = NULL;

  while (! help_id)
    {
      GtkWidget *menu_item;

      menu_item = gtk_menu_shell_get_selected_item (GTK_MENU_SHELL (widget));

      if (! menu_item && GTK_IS_MENU (widget))
        {
          GtkWidget *parent = gtk_widget_get_parent (widget);
          GdkWindow *window = gtk_widget_get_window (parent);
          gint       x, y;

          gdk_window_get_pointer (window, &x, &y, NULL);
          menu_item = find_widget_under_pointer (window, &x, &y);

          if (menu_item && ! GTK_IS_MENU_ITEM (menu_item))
            {
              menu_item = gtk_widget_get_ancestor (menu_item,
                                                   GTK_TYPE_MENU_ITEM);

              if (! GTK_IS_MENU_ITEM (menu_item))
                menu_item = NULL;
            }
        }

      /*  first, get the help page from the item...
       */
      if (menu_item)
        {
          help_id = g_object_get_qdata (G_OBJECT (menu_item), GIMP_HELP_ID);

          if (help_id && ! strlen (help_id))
            help_id = NULL;
        }

      /*  ...then try the parent menu...
       */
      if (! help_id)
        {
          help_id = g_object_get_qdata (G_OBJECT (widget), GIMP_HELP_ID);

          if (help_id && ! strlen (help_id))
            help_id = NULL;
        }

      /*  ...finally try the menu's parent (if any)
       */
      if (! help_id)
        {
          menu_item = NULL;

          if (GTK_IS_MENU (widget))
            menu_item = gtk_menu_get_attach_widget (GTK_MENU (widget));

          if (! menu_item)
            break;

          widget = gtk_widget_get_parent (menu_item);

          if (! widget)
            break;
        }
    }

  /*  For any valid accelerator key except F1, continue with the
   *  standard GtkMenuShell callback and assign a new shortcut, but
   *  don't assign a shortcut to the help menu entries ...
   */
  if (kevent->keyval != GDK_KEY_F1)
    {
      if (help_id                                   &&
          gtk_accelerator_valid (kevent->keyval, 0) &&
          (strcmp (help_id, GIMP_HELP_HELP)         == 0 ||
           strcmp (help_id, GIMP_HELP_HELP_CONTEXT) == 0))
        {
          return TRUE;
        }

      return FALSE;
    }

  /*  ...finally, if F1 was pressed over any menu, show its help page...  */

  if (help_id)
    {
      gchar *help_domain = NULL;
      gchar *help_string = NULL;
      gchar *domain_separator;

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

      gimp_help (manager->gimp, NULL, help_domain, help_string);

      g_free (help_domain);
      g_free (help_string);
      g_free (help_id);
    }

  return TRUE;
}


/* Stuff below taken from gtktooltip.c
 */

/* FIXME: remove this crack as soon as a GTK+ widget_under_pointer() is available */

struct ChildLocation
{
  GtkWidget *child;
  GtkWidget *container;

  gint x;
  gint y;
};

static void
child_location_foreach (GtkWidget *child,
                        gpointer   data)
{
  gint x, y;
  struct ChildLocation *child_loc = data;

  /* Ignore invisible widgets */
  if (! gtk_widget_is_drawable (child))
    return;

  /* (child_loc->x, child_loc->y) are relative to
   * child_loc->container's allocation.
   */

  if (! child_loc->child &&
      gtk_widget_translate_coordinates (child_loc->container, child,
                                        child_loc->x, child_loc->y,
                                        &x, &y))
    {
      GtkAllocation child_allocation;

      gtk_widget_get_allocation (child, &child_allocation);

#ifdef DEBUG_TOOLTIP
      g_print ("candidate: %s  alloc=[(%d,%d)  %dx%d]     (%d, %d)->(%d, %d)\n",
               gtk_widget_get_name (child),
               child_allocation.x,
               child_allocation.y,
               child_allocation.width,
               child_allocation.height,
               child_loc->x, child_loc->y,
               x, y);
#endif /* DEBUG_TOOLTIP */

      /* (x, y) relative to child's allocation. */
      if (x >= 0 && x < child_allocation.width
          && y >= 0 && y < child_allocation.height)
        {
          if (GTK_IS_CONTAINER (child))
            {
              struct ChildLocation tmp = { NULL, NULL, 0, 0 };

              /* Take (x, y) relative the child's allocation and
               * recurse.
               */
              tmp.x = x;
              tmp.y = y;
              tmp.container = child;

              gtk_container_forall (GTK_CONTAINER (child),
                                    child_location_foreach, &tmp);

              if (tmp.child)
                child_loc->child = tmp.child;
              else
                child_loc->child = child;
            }
          else
            {
              child_loc->child = child;
            }
        }
    }
}

/* Translates coordinates from dest_widget->window relative (src_x, src_y),
 * to allocation relative (dest_x, dest_y) of dest_widget.
 */
static void
window_to_alloc (GtkWidget *dest_widget,
                 gint       src_x,
                 gint       src_y,
                 gint      *dest_x,
                 gint      *dest_y)
{
  GtkAllocation dest_allocation;

  gtk_widget_get_allocation (dest_widget, &dest_allocation);

  /* Translate from window relative to allocation relative */
  if (gtk_widget_get_has_window (dest_widget) &&
      gtk_widget_get_parent (dest_widget))
    {
      gint wx, wy;

      gdk_window_get_position (gtk_widget_get_window (dest_widget), &wx, &wy);

      /* Offset coordinates if widget->window is smaller than
       * widget->allocation.
       */
      src_x += wx - dest_allocation.x;
      src_y += wy - dest_allocation.y;
    }
  else
    {
      src_x -= dest_allocation.x;
      src_y -= dest_allocation.y;
    }

  if (dest_x)
    *dest_x = src_x;
  if (dest_y)
    *dest_y = src_y;
}

static GtkWidget *
find_widget_under_pointer (GdkWindow *window,
                           gint      *x,
                           gint      *y)
{
  GtkWidget *event_widget;
  struct ChildLocation child_loc = { NULL, NULL, 0, 0 };

  gdk_window_get_user_data (window, (void **)&event_widget);

  if (! event_widget)
    return NULL;

#ifdef DEBUG_TOOLTIP
  g_print ("event window %p (belonging to %p (%s))  (%d, %d)\n",
           window, event_widget, gtk_widget_get_name (event_widget),
           *x, *y);
#endif

  /* Coordinates are relative to event window */
  child_loc.x = *x;
  child_loc.y = *y;

  /* We go down the window hierarchy to the widget->window,
   * coordinates stay relative to the current window.
   * We end up with window == widget->window, coordinates relative to that.
   */
  while (window && window != gtk_widget_get_window (event_widget))
    {
      gint px, py;

      gdk_window_get_position (window, &px, &py);
      child_loc.x += px;
      child_loc.y += py;

      window = gdk_window_get_parent (window);
    }

  /* Failing to find widget->window can happen for e.g. a detached handle box;
   * chaining ::query-tooltip up to its parent probably makes little sense,
   * and users better implement tooltips on handle_box->child.
   * so we simply ignore the event for tooltips here.
   */
  if (!window)
    return NULL;

  /* Convert the window relative coordinates to allocation
   * relative coordinates.
   */
  window_to_alloc (event_widget,
                   child_loc.x, child_loc.y,
                   &child_loc.x, &child_loc.y);

  if (GTK_IS_CONTAINER (event_widget))
    {
      GtkWidget *container = event_widget;

      child_loc.container = event_widget;
      child_loc.child = NULL;

      gtk_container_forall (GTK_CONTAINER (event_widget),
                            child_location_foreach, &child_loc);

      /* Here we have a widget, with coordinates relative to
       * child_loc.container's allocation.
       */

      if (child_loc.child)
        event_widget = child_loc.child;
      else if (child_loc.container)
        event_widget = child_loc.container;

      /* Translate to event_widget's allocation */
      gtk_widget_translate_coordinates (container, event_widget,
                                        child_loc.x, child_loc.y,
                                        &child_loc.x, &child_loc.y);

    }

  /* We return (x, y) relative to the allocation of event_widget. */
  *x = child_loc.x;
  *y = child_loc.y;

  return event_widget;
}
