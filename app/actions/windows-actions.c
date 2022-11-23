/* LIGMA - The GNU Image Manipulation Program
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libligmabase/ligmabase.h"
#include "libligmawidgets/ligmawidgets.h"

#include "actions-types.h"

#include "config/ligmadisplayconfig.h"
#include "config/ligmaguiconfig.h"

#include "core/ligma.h"
#include "core/ligmaimage.h"
#include "core/ligmalist.h"

#include "widgets/ligmaaction.h"
#include "widgets/ligmaactiongroup.h"
#include "widgets/ligmadialogfactory.h"
#include "widgets/ligmadock.h"
#include "widgets/ligmadockwindow.h"
#include "widgets/ligmahelp-ids.h"

#include "display/ligmadisplay.h"
#include "display/ligmadisplayshell.h"

#include "dialogs/dialogs.h"

#include "windows-actions.h"
#include "windows-commands.h"

#include "ligma-intl.h"


static void  windows_actions_display_add               (LigmaContainer     *container,
                                                        LigmaDisplay       *display,
                                                        LigmaActionGroup   *group);
static void  windows_actions_display_remove            (LigmaContainer     *container,
                                                        LigmaDisplay       *display,
                                                        LigmaActionGroup   *group);
static void  windows_actions_display_reorder           (LigmaContainer     *container,
                                                        LigmaDisplay       *display,
                                                        gint               position,
                                                        LigmaActionGroup   *group);
static void  windows_actions_image_notify              (LigmaDisplay       *display,
                                                        const GParamSpec  *unused,
                                                        LigmaActionGroup   *group);
static void  windows_actions_title_notify              (LigmaDisplayShell  *shell,
                                                        const GParamSpec  *unused,
                                                        LigmaActionGroup   *group);
static void  windows_actions_update_display_accels     (LigmaActionGroup   *group);

static void  windows_actions_dock_window_added         (LigmaDialogFactory *factory,
                                                        LigmaDockWindow    *dock_window,
                                                        LigmaActionGroup   *group);
static void  windows_actions_dock_window_removed       (LigmaDialogFactory *factory,
                                                        LigmaDockWindow    *dock_window,
                                                        LigmaActionGroup   *group);
static void  windows_actions_dock_window_notify        (LigmaDockWindow    *dock,
                                                        const GParamSpec  *pspec,
                                                        LigmaActionGroup   *group);
static void  windows_actions_recent_add                (LigmaContainer     *container,
                                                        LigmaSessionInfo   *info,
                                                        LigmaActionGroup   *group);
static void  windows_actions_recent_remove             (LigmaContainer     *container,
                                                        LigmaSessionInfo   *info,
                                                        LigmaActionGroup   *group);
static void  windows_actions_single_window_mode_notify (LigmaDisplayConfig *config,
                                                        GParamSpec        *pspec,
                                                        LigmaActionGroup   *group);


/* The only reason we have "Tab" in the action entries below is to
 * give away the hardcoded keyboard shortcut. If the user changes the
 * shortcut to something else, both that shortcut and Tab will
 * work. The reason we have the shortcut hardcoded is because
 * gtk_accelerator_valid() returns FALSE for GDK_tab.
 */

static const LigmaActionEntry windows_actions[] =
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
    windows_show_display_next_cmd_callback,
    NULL },

  { "windows-show-display-previous", NULL,
    NC_("windows-action", "Previous Image"), "<alt><shift>Tab",
    NC_("windows-action", "Switch to the previous image"),
    windows_show_display_previous_cmd_callback,
    NULL },

  { "windows-tab-position",        NULL, NC_("windows-action",
                                             "_Tabs Position")   },
};

static const LigmaToggleActionEntry windows_toggle_actions[] =
{
  { "windows-hide-docks", NULL,
    NC_("windows-action", "_Hide Docks"), "Tab",
    NC_("windows-action", "When enabled, docks and other dialogs are hidden, leaving only image windows."),
    windows_hide_docks_cmd_callback,
    FALSE,
    LIGMA_HELP_WINDOWS_HIDE_DOCKS },

  { "windows-show-tabs", NULL,
    NC_("windows-action", "_Show Tabs"), NULL,
    NC_("windows-action", "When enabled, the image tabs bar is shown."),
    windows_show_tabs_cmd_callback,
    FALSE,
    LIGMA_HELP_WINDOWS_SHOW_TABS },

  { "windows-use-single-window-mode", NULL,
    NC_("windows-action", "Single-Window _Mode"), NULL,
    NC_("windows-action", "When enabled, LIGMA is in a single-window mode."),
    windows_use_single_window_mode_cmd_callback,
    FALSE,
    LIGMA_HELP_WINDOWS_USE_SINGLE_WINDOW_MODE }
};

static const LigmaRadioActionEntry windows_tabs_position_actions[] =
{
  { "windows-tabs-position-top", LIGMA_ICON_GO_TOP,
    NC_("windows-tabs-position-action", "_Top"), NULL,
    NC_("windows-tabs-position-action", "Position the tabs on the top"),
    LIGMA_POSITION_TOP, LIGMA_HELP_WINDOWS_TABS_POSITION },

  { "windows-tabs-position-bottom", LIGMA_ICON_GO_BOTTOM,
    NC_("windows-tabs-position-action", "_Bottom"), NULL,
    NC_("windows-tabs-position-action", "Position the tabs on the bottom"),
    LIGMA_POSITION_BOTTOM, LIGMA_HELP_WINDOWS_TABS_POSITION },

  { "windows-tabs-position-left", LIGMA_ICON_GO_FIRST,
    NC_("windows-tabs-position-action", "_Left"), NULL,
    NC_("windows-tabs-position-action", "Position the tabs on the left"),
    LIGMA_POSITION_LEFT, LIGMA_HELP_WINDOWS_TABS_POSITION },

  { "windows-tabs-position-right", LIGMA_ICON_GO_LAST,
    NC_("windows-tabs-position-action", "_Right"), NULL,
    NC_("windows-tabs-position-action", "Position the tabs on the right"),
    LIGMA_POSITION_RIGHT, LIGMA_HELP_WINDOWS_TABS_POSITION },
};

void
windows_actions_setup (LigmaActionGroup *group)
{
  GList *list;

  ligma_action_group_add_actions (group, "windows-action",
                                 windows_actions,
                                 G_N_ELEMENTS (windows_actions));

  ligma_action_group_add_toggle_actions (group, "windows-action",
                                        windows_toggle_actions,
                                        G_N_ELEMENTS (windows_toggle_actions));

  ligma_action_group_add_radio_actions (group, "windows-tabs-position-action",
                                       windows_tabs_position_actions,
                                       G_N_ELEMENTS (windows_tabs_position_actions),
                                       NULL, 0,
                                       windows_set_tabs_position_cmd_callback);

  ligma_action_group_set_action_hide_empty (group, "windows-docks-menu", FALSE);

  g_signal_connect_object (group->ligma->displays, "add",
                           G_CALLBACK (windows_actions_display_add),
                           group, 0);
  g_signal_connect_object (group->ligma->displays, "remove",
                           G_CALLBACK (windows_actions_display_remove),
                           group, 0);
  g_signal_connect_object (group->ligma->displays, "reorder",
                           G_CALLBACK (windows_actions_display_reorder),
                           group, 0);

  for (list = ligma_get_display_iter (group->ligma);
       list;
       list = g_list_next (list))
    {
      LigmaDisplay *display = list->data;

      windows_actions_display_add (group->ligma->displays, display, group);
    }

  g_signal_connect_object (ligma_dialog_factory_get_singleton (), "dock-window-added",
                           G_CALLBACK (windows_actions_dock_window_added),
                           group, 0);
  g_signal_connect_object (ligma_dialog_factory_get_singleton (), "dock-window-removed",
                           G_CALLBACK (windows_actions_dock_window_removed),
                           group, 0);

  for (list = ligma_dialog_factory_get_open_dialogs (ligma_dialog_factory_get_singleton ());
       list;
       list = g_list_next (list))
    {
      LigmaDockWindow *dock_window = list->data;

      if (LIGMA_IS_DOCK_WINDOW (dock_window))
        windows_actions_dock_window_added (ligma_dialog_factory_get_singleton (),
                                           dock_window,
                                           group);
    }

  g_signal_connect_object (global_recent_docks, "add",
                           G_CALLBACK (windows_actions_recent_add),
                           group, 0);
  g_signal_connect_object (global_recent_docks, "remove",
                           G_CALLBACK (windows_actions_recent_remove),
                           group, 0);

  for (list = LIGMA_LIST (global_recent_docks)->queue->head;
       list;
       list = g_list_next (list))
    {
      LigmaSessionInfo *info = list->data;

      windows_actions_recent_add (global_recent_docks, info, group);
    }

  g_signal_connect_object (group->ligma->config, "notify::single-window-mode",
                           G_CALLBACK (windows_actions_single_window_mode_notify),
                           group, 0);
}

void
windows_actions_update (LigmaActionGroup *group,
                        gpointer         data)
{
  LigmaGuiConfig *config = LIGMA_GUI_CONFIG (group->ligma->config);
  const gchar   *action = NULL;

#define SET_ACTIVE(action,condition) \
        ligma_action_group_set_action_active (group, action, (condition) != 0)

  SET_ACTIVE ("windows-use-single-window-mode", config->single_window_mode);
  SET_ACTIVE ("windows-hide-docks", config->hide_docks);
  SET_ACTIVE ("windows-show-tabs", config->show_tabs);

  switch (config->tabs_position)
    {
    case LIGMA_POSITION_TOP:
      action = "windows-tabs-position-top";
      break;
    case LIGMA_POSITION_BOTTOM:
      action = "windows-tabs-position-bottom";
      break;
    case LIGMA_POSITION_LEFT:
      action = "windows-tabs-position-left";
      break;
    case LIGMA_POSITION_RIGHT:
      action = "windows-tabs-position-right";
      break;
    default:
      action = "windows-tabs-position-top";
      break;
    }

  ligma_action_group_set_action_active (group, action, TRUE);
  ligma_action_group_set_action_sensitive (group, "windows-tab-position", config->single_window_mode,
                                          _("Single-window mode disabled"));
  ligma_action_group_set_action_sensitive (group, "windows-show-tabs", config->single_window_mode,
                                          _("Single-window mode disabled"));

#undef SET_ACTIVE
}

gchar *
windows_actions_dock_window_to_action_name (LigmaDockWindow *dock_window)
{
  return g_strdup_printf ("windows-dock-%04d",
                          ligma_dock_window_get_id (dock_window));
}


/*  private functions  */

static void
windows_actions_display_add (LigmaContainer   *container,
                             LigmaDisplay     *display,
                             LigmaActionGroup *group)
{
  LigmaDisplayShell *shell = ligma_display_get_shell (display);

  g_signal_connect_object (display, "notify::image",
                           G_CALLBACK (windows_actions_image_notify),
                           group, 0);

  g_signal_connect_object (shell, "notify::title",
                           G_CALLBACK (windows_actions_title_notify),
                           group, 0);

  windows_actions_image_notify (display, NULL, group);
}

static void
windows_actions_display_remove (LigmaContainer   *container,
                                LigmaDisplay     *display,
                                LigmaActionGroup *group)
{
  LigmaDisplayShell *shell = ligma_display_get_shell (display);
  LigmaAction       *action;
  gchar            *action_name;

  if (shell)
    g_signal_handlers_disconnect_by_func (shell,
                                          windows_actions_title_notify,
                                          group);

  action_name = ligma_display_get_action_name (display);
  action = ligma_action_group_get_action (group, action_name);
  g_free (action_name);

  if (action)
    ligma_action_group_remove_action_and_accel (group, action);

  windows_actions_update_display_accels (group);
}

static void
windows_actions_display_reorder (LigmaContainer   *container,
                                 LigmaDisplay     *display,
                                 gint             new_index,
                                 LigmaActionGroup *group)
{
  windows_actions_update_display_accels (group);
}

static void
windows_actions_image_notify (LigmaDisplay      *display,
                              const GParamSpec *unused,
                              LigmaActionGroup  *group)
{
  LigmaImage  *image = ligma_display_get_image (display);
  LigmaAction *action;
  gchar      *action_name;

  action_name = ligma_display_get_action_name (display);

  action = ligma_action_group_get_action (group, action_name);

  if (! action)
    {
      LigmaActionEntry entry;

      entry.name        = action_name;
      entry.icon_name   = LIGMA_ICON_IMAGE;
      entry.label       = "";
      entry.accelerator = NULL;
      entry.tooltip     = NULL;
      entry.callback    = windows_show_display_cmd_callback;
      entry.help_id     = NULL;

      ligma_action_group_add_actions (group, NULL, &entry, 1);

      action = ligma_action_group_get_action (group, action_name);

      g_object_set_data (G_OBJECT (action), "display", display);
    }

  g_free (action_name);

  if (image)
    {
      const gchar *display_name;
      gchar       *escaped;
      gchar       *title;

      display_name = ligma_image_get_display_name (image);
      escaped = ligma_escape_uline (display_name);

      title = g_strdup_printf ("%s-%d.%d", escaped,
                               ligma_image_get_id (image),
                               ligma_display_get_instance (display));
      g_free (escaped);

      g_object_set (action,
                    "visible",  TRUE,
                    "label",    title,
                    "tooltip",  ligma_image_get_display_path (image),
                    "viewable", image,
                    "context",  ligma_get_user_context (group->ligma),
                    NULL);

      g_free (title);

      windows_actions_update_display_accels (group);
    }
  else
    {
      g_object_set (action,
                    "visible",  FALSE,
                    "viewable", NULL,
                    NULL);
    }
}

static void
windows_actions_title_notify (LigmaDisplayShell *shell,
                              const GParamSpec *unused,
                              LigmaActionGroup  *group)
{
  windows_actions_image_notify (shell->display, NULL, group);
}

static void
windows_actions_update_display_accels (LigmaActionGroup *group)
{
  GList *list;
  gint   i;

  for (list = ligma_get_display_iter (group->ligma), i = 0;
       list && i < 10;
       list = g_list_next (list), i++)
    {
      LigmaDisplay *display = list->data;
      LigmaAction  *action;
      gchar       *action_name;

      if (! ligma_display_get_image (display))
        break;

      action_name = ligma_display_get_action_name (display);

      action = ligma_action_group_get_action (group, action_name);
      g_free (action_name);

      if (action)
        {
          const gchar *accel_path;
          guint        accel_key;

          accel_path = ligma_action_get_accel_path (action);

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
windows_actions_dock_window_added (LigmaDialogFactory *factory,
                                   LigmaDockWindow    *dock_window,
                                   LigmaActionGroup   *group)
{
  LigmaAction      *action;
  LigmaActionEntry  entry;
  gchar           *action_name = windows_actions_dock_window_to_action_name (dock_window);

  entry.name        = action_name;
  entry.icon_name   = NULL;
  entry.label       = "";
  entry.accelerator = NULL;
  entry.tooltip     = NULL;
  entry.callback    = windows_show_dock_cmd_callback;
  entry.help_id     = LIGMA_HELP_WINDOWS_SHOW_DOCK;

  ligma_action_group_add_actions (group, NULL, &entry, 1);

  action = ligma_action_group_get_action (group, action_name);

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
windows_actions_dock_window_removed (LigmaDialogFactory *factory,
                                     LigmaDockWindow    *dock_window,
                                     LigmaActionGroup   *group)
{
  LigmaAction *action;
  gchar      *action_name;

  action_name = windows_actions_dock_window_to_action_name (dock_window);
  action = ligma_action_group_get_action (group, action_name);
  g_free (action_name);

  if (action)
    ligma_action_group_remove_action_and_accel (group, action);
}

static void
windows_actions_dock_window_notify (LigmaDockWindow   *dock_window,
                                    const GParamSpec *pspec,
                                    LigmaActionGroup  *group)
{
  LigmaAction *action;
  gchar      *action_name;

  action_name = windows_actions_dock_window_to_action_name (dock_window);
  action = ligma_action_group_get_action (group, action_name);
  g_free (action_name);

  if (action)
    g_object_set (action,
                  "label",   gtk_window_get_title (GTK_WINDOW (dock_window)),
                  "tooltip", gtk_window_get_title (GTK_WINDOW (dock_window)),
                  NULL);
}

static void
windows_actions_recent_add (LigmaContainer   *container,
                            LigmaSessionInfo *info,
                            LigmaActionGroup *group)
{
  LigmaAction      *action;
  LigmaActionEntry  entry;
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
  entry.label       = ligma_object_get_name (info);
  entry.accelerator = NULL;
  entry.tooltip     = ligma_object_get_name (info);
  entry.callback    = windows_open_recent_cmd_callback;
  entry.help_id     = LIGMA_HELP_WINDOWS_OPEN_RECENT_DOCK;

  ligma_action_group_add_actions (group, NULL, &entry, 1);

  action = ligma_action_group_get_action (group, action_name);

  g_object_set (action,
                "ellipsize",       PANGO_ELLIPSIZE_END,
                "max-width-chars", 30,
                NULL);

  g_object_set_data (G_OBJECT (action), "info", info);

  g_free (action_name);
}

static void
windows_actions_recent_remove (LigmaContainer   *container,
                               LigmaSessionInfo *info,
                               LigmaActionGroup *group)
{
  LigmaAction *action;
  gint        info_id;
  gchar      *action_name;

  info_id = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (info),
                                                "recent-action-id"));

  action_name = g_strdup_printf ("windows-recent-%04d", info_id);
  action = ligma_action_group_get_action (group, action_name);
  g_free (action_name);

  if (action)
    ligma_action_group_remove_action_and_accel (group, action);
}

static void
windows_actions_single_window_mode_notify (LigmaDisplayConfig *config,
                                           GParamSpec        *pspec,
                                           LigmaActionGroup   *group)
{
  ligma_action_group_set_action_active (group,
                                       "windows-use-single-window-mode",
                                       LIGMA_GUI_CONFIG (config)->single_window_mode);
}
