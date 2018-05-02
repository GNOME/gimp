/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include <gegl.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "config/gimpdisplayconfig.h"
#include "config/gimpguiconfig.h"

#include "core/gimp.h"
#include "core/gimpimage.h"
#include "core/gimplist.h"

#include "widgets/gimpaction.h"
#include "widgets/gimpactiongroup.h"
#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpdock.h"
#include "widgets/gimpdockwindow.h"
#include "widgets/gimphelp-ids.h"

#include "display/gimpdisplay.h"

#include "dialogs/dialogs.h"

#include "windows-actions.h"
#include "windows-commands.h"

#include "gimp-intl.h"


static void  windows_actions_display_add               (GimpContainer     *container,
                                                        GimpDisplay       *display,
                                                        GimpActionGroup   *group);
static void  windows_actions_display_remove            (GimpContainer     *container,
                                                        GimpDisplay       *display,
                                                        GimpActionGroup   *group);
static void  windows_actions_display_reorder           (GimpContainer     *container,
                                                        GimpDisplay       *display,
                                                        gint               position,
                                                        GimpActionGroup   *group);
static void  windows_actions_image_notify              (GimpDisplay       *display,
                                                        const GParamSpec  *unused,
                                                        GimpActionGroup   *group);
static void  windows_actions_update_display_accels     (GimpActionGroup   *group);

static void  windows_actions_dock_window_added         (GimpDialogFactory *factory,
                                                        GimpDockWindow    *dock_window,
                                                        GimpActionGroup   *group);
static void  windows_actions_dock_window_removed       (GimpDialogFactory *factory,
                                                        GimpDockWindow    *dock_window,
                                                        GimpActionGroup   *group);
static void  windows_actions_dock_window_notify        (GimpDockWindow    *dock,
                                                        const GParamSpec  *pspec,
                                                        GimpActionGroup   *group);
static void  windows_actions_recent_add                (GimpContainer     *container,
                                                        GimpSessionInfo   *info,
                                                        GimpActionGroup   *group);
static void  windows_actions_recent_remove             (GimpContainer     *container,
                                                        GimpSessionInfo   *info,
                                                        GimpActionGroup   *group);
static void  windows_actions_single_window_mode_notify (GimpDisplayConfig *config,
                                                        GParamSpec        *pspec,
                                                        GimpActionGroup   *group);


/* The only reason we have "Tab" in the action entries below is to
 * give away the hardcoded keyboard shortcut. If the user changes the
 * shortcut to something else, both that shortcut and Tab will
 * work. The reason we have the shortcut hardcoded is because
 * gtk_accelerator_valid() returns FALSE for GDK_tab.
 */

static const GimpActionEntry windows_actions[] =
{
  { "windows-menu",         NULL, NC_("windows-action",
                                      "_Windows")               },
  { "windows-docks-menu",   NULL, NC_("windows-action",
                                      "_Recently Closed Docks") },
  { "windows-dialogs-menu", NULL, NC_("windows-action",
                                      "_Dockable Dialogs")      },

  { "windows-show-display-next", NULL,
    NC_("windows-action", "Next Image"), "<alt>Tab",
    NC_("windows-action", "Switch to the next image"),
    G_CALLBACK (windows_show_display_next_cmd_callback),
    NULL },

  { "windows-show-display-previous", NULL,
    NC_("windows-action", "Previous Image"), "<alt><shift>Tab",
    NC_("windows-action", "Switch to the previous image"),
    G_CALLBACK (windows_show_display_previous_cmd_callback),
    NULL },

  { "windows-tab-position",        NULL, NC_("windows-action",
                                             "_Tabs Position")   },
};

static const GimpToggleActionEntry windows_toggle_actions[] =
{
  { "windows-hide-docks", NULL,
    NC_("windows-action", "Hide Docks"), "Tab",
    NC_("windows-action", "When enabled, docks and other dialogs are hidden, leaving only image windows."),
    G_CALLBACK (windows_hide_docks_cmd_callback),
    FALSE,
    GIMP_HELP_WINDOWS_HIDE_DOCKS },

  { "windows-use-single-window-mode", NULL,
    NC_("windows-action", "Single-Window Mode"), NULL,
    NC_("windows-action", "When enabled, GIMP is in a single-window mode."),
    G_CALLBACK (windows_use_single_window_mode_cmd_callback),
    FALSE,
    GIMP_HELP_WINDOWS_USE_SINGLE_WINDOW_MODE }
};

static const GimpRadioActionEntry windows_tabs_position_actions[] =
{
  { "windows-tabs-position-top", GIMP_ICON_GO_TOP,
    NC_("windows-tabs-position-action", "_Top"), NULL,
    NC_("windows-tabs-position-action", "Position the tabs on the top"),
    GIMP_POSITION_TOP, GIMP_HELP_WINDOWS_TABS_POSITION },

  { "windows-tabs-position-bottom", GIMP_ICON_GO_BOTTOM,
    NC_("windows-tabs-position-action", "_Bottom"), NULL,
    NC_("windows-tabs-position-action", "Position the tabs on the bottom"),
    GIMP_POSITION_BOTTOM, GIMP_HELP_WINDOWS_TABS_POSITION },

  { "windows-tabs-position-left", GIMP_ICON_GO_FIRST,
    NC_("windows-tabs-position-action", "_Left"), NULL,
    NC_("windows-tabs-position-action", "Position the tabs on the left"),
    GIMP_POSITION_LEFT, GIMP_HELP_WINDOWS_TABS_POSITION },

  { "windows-tabs-position-right", GIMP_ICON_GO_LAST,
    NC_("windows-tabs-position-action", "_Right"), NULL,
    NC_("windows-tabs-position-action", "Position the tabs on the right"),
    GIMP_POSITION_RIGHT, GIMP_HELP_WINDOWS_TABS_POSITION },
};

void
windows_actions_setup (GimpActionGroup *group)
{
  GList *list;

  gimp_action_group_add_actions (group, "windows-action",
                                 windows_actions,
                                 G_N_ELEMENTS (windows_actions));

  gimp_action_group_add_toggle_actions (group, "windows-action",
                                        windows_toggle_actions,
                                        G_N_ELEMENTS (windows_toggle_actions));

  gimp_action_group_add_radio_actions (group, "windows-tabs-position-action",
                                       windows_tabs_position_actions,
                                       G_N_ELEMENTS (windows_tabs_position_actions),
                                       NULL, 0,
                                       G_CALLBACK (windows_set_tabs_position_cmd_callback));

  gimp_action_group_set_action_hide_empty (group, "windows-docks-menu", FALSE);

  g_signal_connect_object (group->gimp->displays, "add",
                           G_CALLBACK (windows_actions_display_add),
                           group, 0);
  g_signal_connect_object (group->gimp->displays, "remove",
                           G_CALLBACK (windows_actions_display_remove),
                           group, 0);
  g_signal_connect_object (group->gimp->displays, "reorder",
                           G_CALLBACK (windows_actions_display_reorder),
                           group, 0);

  for (list = gimp_get_display_iter (group->gimp);
       list;
       list = g_list_next (list))
    {
      GimpDisplay *display = list->data;

      windows_actions_display_add (group->gimp->displays, display, group);
    }

  g_signal_connect_object (gimp_dialog_factory_get_singleton (), "dock-window-added",
                           G_CALLBACK (windows_actions_dock_window_added),
                           group, 0);
  g_signal_connect_object (gimp_dialog_factory_get_singleton (), "dock-window-removed",
                           G_CALLBACK (windows_actions_dock_window_removed),
                           group, 0);

  for (list = gimp_dialog_factory_get_open_dialogs (gimp_dialog_factory_get_singleton ());
       list;
       list = g_list_next (list))
    {
      GimpDockWindow *dock_window = list->data;

      if (GIMP_IS_DOCK_WINDOW (dock_window))
        windows_actions_dock_window_added (gimp_dialog_factory_get_singleton (),
                                           dock_window,
                                           group);
    }

  g_signal_connect_object (global_recent_docks, "add",
                           G_CALLBACK (windows_actions_recent_add),
                           group, 0);
  g_signal_connect_object (global_recent_docks, "remove",
                           G_CALLBACK (windows_actions_recent_remove),
                           group, 0);

  for (list = GIMP_LIST (global_recent_docks)->queue->head;
       list;
       list = g_list_next (list))
    {
      GimpSessionInfo *info = list->data;

      windows_actions_recent_add (global_recent_docks, info, group);
    }

  g_signal_connect_object (group->gimp->config, "notify::single-window-mode",
                           G_CALLBACK (windows_actions_single_window_mode_notify),
                           group, 0);
}

void
windows_actions_update (GimpActionGroup *group,
                        gpointer         data)
{
  GimpGuiConfig *config = GIMP_GUI_CONFIG (group->gimp->config);
  const gchar   *action = NULL;

#define SET_ACTIVE(action,condition) \
        gimp_action_group_set_action_active (group, action, (condition) != 0)

  SET_ACTIVE ("windows-use-single-window-mode", config->single_window_mode);
  SET_ACTIVE ("windows-hide-docks", config->hide_docks);

  switch (config->tabs_position)
    {
    case GIMP_POSITION_TOP:
      action = "windows-tabs-position-top";
      break;
    case GIMP_POSITION_BOTTOM:
      action = "windows-tabs-position-bottom";
      break;
    case GIMP_POSITION_LEFT:
      action = "windows-tabs-position-left";
      break;
    case GIMP_POSITION_RIGHT:
      action = "windows-tabs-position-right";
      break;
    default:
      action = "windows-tabs-position-top";
      break;
    }

  gimp_action_group_set_action_active (group, action, TRUE);
  gimp_action_group_set_action_sensitive (group, "windows-tab-position", config->single_window_mode);

#undef SET_ACTIVE
}

gchar *
windows_actions_dock_window_to_action_name (GimpDockWindow *dock_window)
{
  return g_strdup_printf ("windows-dock-%04d",
                          gimp_dock_window_get_id (dock_window));
}


/*  private functions  */

static void
windows_actions_display_add (GimpContainer   *container,
                             GimpDisplay     *display,
                             GimpActionGroup *group)
{
  g_signal_connect_object (display, "notify::image",
                           G_CALLBACK (windows_actions_image_notify),
                           group, 0);

  if (gimp_display_get_image (display))
    windows_actions_image_notify (display, NULL, group);
}

static void
windows_actions_display_remove (GimpContainer   *container,
                                GimpDisplay     *display,
                                GimpActionGroup *group)
{
  GtkAction *action;
  gchar     *action_name = gimp_display_get_action_name (display);

  action = gtk_action_group_get_action (GTK_ACTION_GROUP (group), action_name);

  if (action)
    gimp_action_group_remove_action (group,
                                     GIMP_ACTION (action));
  g_free (action_name);

  windows_actions_update_display_accels (group);
}

static void
windows_actions_display_reorder (GimpContainer   *container,
                                 GimpDisplay     *display,
                                 gint             new_index,
                                 GimpActionGroup *group)
{
  windows_actions_update_display_accels (group);
}

static void
windows_actions_image_notify (GimpDisplay      *display,
                              const GParamSpec *unused,
                              GimpActionGroup  *group)
{
  GimpImage *image = gimp_display_get_image (display);

  if (image)
    {
      GtkAction *action;
      gchar     *action_name = gimp_display_get_action_name (display);

      action = gtk_action_group_get_action (GTK_ACTION_GROUP (group),
                                            action_name);

      if (! action)
        {
          GimpActionEntry entry;

          entry.name        = action_name;
          entry.icon_name   = GIMP_ICON_IMAGE;
          entry.label       = "";
          entry.accelerator = NULL;
          entry.tooltip     = NULL;
          entry.callback    = G_CALLBACK (windows_show_display_cmd_callback);
          entry.help_id     = NULL;

          gimp_action_group_add_actions (group, NULL, &entry, 1);

          action = gtk_action_group_get_action (GTK_ACTION_GROUP (group),
                                                action_name);

          g_object_set_data (G_OBJECT (action), "display", display);
        }

      {
        const gchar *display_name;
        gchar       *escaped;
        gchar       *title;

        display_name = gimp_image_get_display_name (image);
        escaped = gimp_escape_uline (display_name);

        title = g_strdup_printf ("%s-%d.%d", escaped,
                                 gimp_image_get_ID (image),
                                 gimp_display_get_instance (display));
        g_free (escaped);

        g_object_set (action,
                      "label",    title,
                      "tooltip",  gimp_image_get_display_path (image),
                      "viewable", image,
                      "context",  gimp_get_user_context (group->gimp),
                      NULL);

        g_free (title);
      }

      g_free (action_name);

      windows_actions_update_display_accels (group);
    }
  else
    {
      windows_actions_display_remove (group->gimp->displays, display, group);
    }
}

static void
windows_actions_update_display_accels (GimpActionGroup *group)
{
  GList *list;
  gint   i;

  for (list = gimp_get_display_iter (group->gimp), i = 0;
       list && i < 10;
       list = g_list_next (list), i++)
    {
      GimpDisplay *display = list->data;
      GtkAction   *action;
      gchar       *action_name;

      if (! gimp_display_get_image (display))
        break;

      action_name = gimp_display_get_action_name (display);

      action = gtk_action_group_get_action (GTK_ACTION_GROUP (group),
                                            action_name);
      g_free (action_name);

      if (action)
        {
          const gchar *accel_path;
          guint        accel_key;

          accel_path = gtk_action_get_accel_path (action);

          if (i < 9)
            accel_key = GDK_KEY_1 + i;
          else
            accel_key = GDK_KEY_0;

          gtk_accel_map_change_entry (accel_path,
                                      accel_key, GDK_MOD1_MASK,
                                      TRUE);
        }
    }
}

static void
windows_actions_dock_window_added (GimpDialogFactory *factory,
                                   GimpDockWindow    *dock_window,
                                   GimpActionGroup   *group)
{
  GtkAction       *action;
  GimpActionEntry  entry;
  gchar           *action_name = windows_actions_dock_window_to_action_name (dock_window);

  entry.name        = action_name;
  entry.icon_name   = NULL;
  entry.label       = "";
  entry.accelerator = NULL;
  entry.tooltip     = NULL;
  entry.callback    = G_CALLBACK (windows_show_dock_cmd_callback);
  entry.help_id     = GIMP_HELP_WINDOWS_SHOW_DOCK;

  gimp_action_group_add_actions (group, NULL, &entry, 1);

  action = gtk_action_group_get_action (GTK_ACTION_GROUP (group),
                                        action_name);

  g_object_set (action,
                "ellipsize", PANGO_ELLIPSIZE_END,
                NULL);

  g_object_set_data (G_OBJECT (action), "dock-window", dock_window);

  g_free (action_name);

  g_signal_connect_object (dock_window, "notify::title",
                           G_CALLBACK (windows_actions_dock_window_notify),
                           group, 0);

  if (gtk_window_get_title (GTK_WINDOW (dock_window)))
    windows_actions_dock_window_notify (dock_window, NULL, group);
}

static void
windows_actions_dock_window_removed (GimpDialogFactory *factory,
                                     GimpDockWindow    *dock_window,
                                     GimpActionGroup   *group)
{
  GtkAction *action;
  gchar     *action_name = windows_actions_dock_window_to_action_name (dock_window);

  action = gtk_action_group_get_action (GTK_ACTION_GROUP (group), action_name);

  if (action)
    gimp_action_group_remove_action (group, GIMP_ACTION (action));

  g_free (action_name);
}

static void
windows_actions_dock_window_notify (GimpDockWindow   *dock_window,
                                    const GParamSpec *pspec,
                                    GimpActionGroup  *group)
{
  GtkAction *action;
  gchar     *action_name;

  action_name = windows_actions_dock_window_to_action_name (dock_window);
  action = gtk_action_group_get_action (GTK_ACTION_GROUP (group), action_name);
  g_free (action_name);

  if (action)
    g_object_set (action,
                  "label",   gtk_window_get_title (GTK_WINDOW (dock_window)),
                  "tooltip", gtk_window_get_title (GTK_WINDOW (dock_window)),
                  NULL);
}

static void
windows_actions_recent_add (GimpContainer   *container,
                            GimpSessionInfo *info,
                            GimpActionGroup *group)
{
  GtkAction       *action;
  GimpActionEntry  entry;
  gint             info_id;
  static gint      info_id_counter = 1;
  gchar           *action_name;

  info_id = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (info),
                                                "recent-action-id"));

  if (! info_id)
    {
      info_id = info_id_counter++;

      g_object_set_data (G_OBJECT (info), "recent-action-id",
                         GINT_TO_POINTER (info_id));
    }

  action_name = g_strdup_printf ("windows-recent-%04d", info_id);

  entry.name        = action_name;
  entry.icon_name   = NULL;
  entry.label       = gimp_object_get_name (info);
  entry.accelerator = NULL;
  entry.tooltip     = gimp_object_get_name (info);
  entry.callback    = G_CALLBACK (windows_open_recent_cmd_callback);
  entry.help_id     = GIMP_HELP_WINDOWS_OPEN_RECENT_DOCK;

  gimp_action_group_add_actions (group, NULL, &entry, 1);

  action = gtk_action_group_get_action (GTK_ACTION_GROUP (group),
                                        action_name);

  g_object_set (action,
                "ellipsize",       PANGO_ELLIPSIZE_END,
                "max-width-chars", 30,
                NULL);

  g_object_set_data (G_OBJECT (action), "info", info);

  g_free (action_name);
}

static void
windows_actions_recent_remove (GimpContainer   *container,
                               GimpSessionInfo *info,
                               GimpActionGroup *group)
{
  GtkAction *action;
  gint       info_id;
  gchar     *action_name;

  info_id = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (info),
                                                "recent-action-id"));

  action_name = g_strdup_printf ("windows-recent-%04d", info_id);

  action = gtk_action_group_get_action (GTK_ACTION_GROUP (group), action_name);

  if (action)
    gimp_action_group_remove_action (group, GIMP_ACTION (action));

  g_free (action_name);
}

static void
windows_actions_single_window_mode_notify (GimpDisplayConfig *config,
                                           GParamSpec        *pspec,
                                           GimpActionGroup   *group)
{
  gimp_action_group_set_action_active (group,
                                       "windows-use-single-window-mode",
                                       GIMP_GUI_CONFIG (config)->single_window_mode);
}
