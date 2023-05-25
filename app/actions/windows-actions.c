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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
#include "display/gimpdisplayshell.h"

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
static void  windows_actions_title_notify              (GimpDisplayShell  *shell,
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
  { "windows-show-display-next", NULL,
    NC_("windows-action", "Next Image"), NULL, { "<alt>Tab", "Forward", NULL },
    NC_("windows-action", "Switch to the next image"),
    windows_show_display_next_cmd_callback,
    NULL },

  { "windows-show-display-previous", NULL,
    NC_("windows-action", "Previous Image"), NULL, { "<alt><shift>Tab", "Back", NULL },
    NC_("windows-action", "Switch to the previous image"),
    windows_show_display_previous_cmd_callback,
    NULL },

  { "windows-tab-position",        NULL, NC_("windows-action",
                                             "_Tabs Position")   },
};

static const GimpToggleActionEntry windows_toggle_actions[] =
{
  { "windows-hide-docks", NULL,
    NC_("windows-action", "_Hide Docks"), NULL, { NULL },
    NC_("windows-action", "When enabled, docks and other dialogs are hidden, leaving only image windows."),
    windows_hide_docks_cmd_callback,
    FALSE,
    GIMP_HELP_WINDOWS_HIDE_DOCKS },

  { "windows-show-tabs", NULL,
    NC_("windows-action", "_Show Tabs"), NULL, { NULL },
    NC_("windows-action", "When enabled, the image tabs bar is shown."),
    windows_show_tabs_cmd_callback,
    TRUE,
    GIMP_HELP_WINDOWS_SHOW_TABS },

  { "windows-use-single-window-mode", NULL,
    NC_("windows-action", "Single-Window _Mode"), NULL, { NULL },
    NC_("windows-action", "When enabled, GIMP is in a single-window mode."),
    windows_use_single_window_mode_cmd_callback,
    TRUE,
    GIMP_HELP_WINDOWS_USE_SINGLE_WINDOW_MODE }
};

static const GimpRadioActionEntry windows_tabs_position_actions[] =
{
  { "windows-tabs-position-top", GIMP_ICON_GO_TOP,
    NC_("windows-tabs-position-action", "_Top"), NULL, { NULL },
    NC_("windows-tabs-position-action", "Position the tabs on the top"),
    GIMP_POSITION_TOP, GIMP_HELP_WINDOWS_TABS_POSITION },

  { "windows-tabs-position-bottom", GIMP_ICON_GO_BOTTOM,
    NC_("windows-tabs-position-action", "_Bottom"), NULL, { NULL },
    NC_("windows-tabs-position-action", "Position the tabs on the bottom"),
    GIMP_POSITION_BOTTOM, GIMP_HELP_WINDOWS_TABS_POSITION },

  { "windows-tabs-position-left", GIMP_ICON_GO_FIRST,
    NC_("windows-tabs-position-action", "_Left"), NULL, { NULL },
    NC_("windows-tabs-position-action", "Position the tabs on the left"),
    GIMP_POSITION_LEFT, GIMP_HELP_WINDOWS_TABS_POSITION },

  { "windows-tabs-position-right", GIMP_ICON_GO_LAST,
    NC_("windows-tabs-position-action", "_Right"), NULL, { NULL },
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
                                       windows_set_tabs_position_cmd_callback);

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
  SET_ACTIVE ("windows-show-tabs", config->show_tabs);

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
  gimp_action_group_set_action_sensitive (group, "windows-tab-position", config->single_window_mode,
                                          _("Single-window mode disabled"));
  gimp_action_group_set_action_sensitive (group, "windows-show-tabs", config->single_window_mode,
                                          _("Single-window mode disabled"));

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
  GimpDisplayShell *shell = gimp_display_get_shell (display);

  g_signal_connect_object (display, "notify::image",
                           G_CALLBACK (windows_actions_image_notify),
                           group, 0);

  g_signal_connect_object (shell, "notify::title",
                           G_CALLBACK (windows_actions_title_notify),
                           group, 0);

  windows_actions_image_notify (display, NULL, group);
}

static void
windows_actions_display_remove (GimpContainer   *container,
                                GimpDisplay     *display,
                                GimpActionGroup *group)
{
  GimpDisplayShell *shell = gimp_display_get_shell (display);
  GimpAction       *action;
  gchar            *action_name;

  if (shell)
    g_signal_handlers_disconnect_by_func (shell,
                                          windows_actions_title_notify,
                                          group);

  action_name = gimp_display_get_action_name (display);
  action = gimp_action_group_get_action (group, action_name);
  g_free (action_name);

  if (action)
    gimp_action_group_remove_action_and_accel (group, action);

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
  GimpImage  *image = gimp_display_get_image (display);
  GimpAction *action;
  gchar      *action_name;

  action_name = gimp_display_get_action_name (display);

  action = gimp_action_group_get_action (group, action_name);

  if (! action)
    {
      GimpActionEntry entry = { 0 };

      entry.name        = action_name;
      entry.icon_name   = GIMP_ICON_IMAGE;
      entry.label       = "";
      entry.tooltip     = NULL;
      entry.callback    = windows_show_display_cmd_callback;
      entry.help_id     = NULL;

      gimp_action_group_add_actions (group, NULL, &entry, 1);

      action = gimp_action_group_get_action (group, action_name);

      g_object_set_data (G_OBJECT (action), "display", display);
    }

  g_free (action_name);

  if (image)
    {
      const gchar *display_name;
      gchar       *escaped;
      gchar       *title;

      display_name = gimp_image_get_display_name (image);
      escaped = gimp_escape_uline (display_name);

      /* TRANSLATORS: label for an action allowing to show (i.e. raise the image
       * tab or window above others) specific images or views of image. The part
       * between quotes is the image name and other view identifiers.
       */
      title = g_strdup_printf (_("Show \"%s-%d.%d\""), escaped,
                               gimp_image_get_id (image),
                               gimp_display_get_instance (display));

      g_object_set (action,
                    "visible",     TRUE,
                    "label",       title,
                    "short-label", escaped,
                    "tooltip",     gimp_image_get_display_path (image),
                    "viewable",    image,
                    NULL);

      g_free (title);
      g_free (escaped);

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
windows_actions_title_notify (GimpDisplayShell *shell,
                              const GParamSpec *unused,
                              GimpActionGroup  *group)
{
  windows_actions_image_notify (shell->display, NULL, group);
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
      GimpImage   *image   = gimp_display_get_image (display);
      GimpAction  *action;
      gchar       *action_name;

      if (image == NULL)
        break;

      action_name = gimp_display_get_action_name (display);

      action = gimp_action_group_get_action (group, action_name);
      g_free (action_name);

      if (action)
        {
          const gchar *ntooltip;
          gchar       *tooltip;
          gchar       *accel;

          if (i < 9)
            accel = gtk_accelerator_name (GDK_KEY_1 + i, GDK_MOD1_MASK);
          else
            accel = gtk_accelerator_name (GDK_KEY_0 + i, GDK_MOD1_MASK);

          gimp_action_set_accels (action, (const gchar*[]) { accel, NULL });
          g_free (accel);

          /* TRANSLATORS: the first argument (%1$s) is the image name, the
           * second (%2$d) is its tab order in the graphical interface.
           */
          ntooltip = ngettext ("Switch to the first image view: %1$s",
                               "Switch to image view %2$d: %1$s",
                               i + 1);
          tooltip = g_strdup_printf (ntooltip, gimp_image_get_display_path (image), i + 1);
          gimp_action_set_tooltip (action, tooltip);
          g_free (tooltip);
        }
    }
}

static void
windows_actions_dock_window_added (GimpDialogFactory *factory,
                                   GimpDockWindow    *dock_window,
                                   GimpActionGroup   *group)
{
  GimpAction      *action;
  GimpActionEntry  entry       = { 0 };
  gchar           *action_name = windows_actions_dock_window_to_action_name (dock_window);

  entry.name        = action_name;
  entry.icon_name   = NULL;
  entry.label       = "";
  entry.tooltip     = NULL;
  entry.callback    = windows_show_dock_cmd_callback;
  entry.help_id     = GIMP_HELP_WINDOWS_SHOW_DOCK;

  gimp_action_group_add_actions (group, NULL, &entry, 1);

  action = gimp_action_group_get_action (group, action_name);

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
  GimpAction *action;
  gchar      *action_name;

  action_name = windows_actions_dock_window_to_action_name (dock_window);
  action = gimp_action_group_get_action (group, action_name);
  g_free (action_name);

  if (action)
    gimp_action_group_remove_action_and_accel (group, action);
}

static void
windows_actions_dock_window_notify (GimpDockWindow   *dock_window,
                                    const GParamSpec *pspec,
                                    GimpActionGroup  *group)
{
  GimpAction *action;
  gchar      *action_name;

  action_name = windows_actions_dock_window_to_action_name (dock_window);
  action = gimp_action_group_get_action (group, action_name);
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
  GimpAction      *action;
  GimpActionEntry  entry = { 0 };
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
  entry.tooltip     = gimp_object_get_name (info);
  entry.callback    = windows_open_recent_cmd_callback;
  entry.help_id     = GIMP_HELP_WINDOWS_OPEN_RECENT_DOCK;

  gimp_action_group_add_actions (group, NULL, &entry, 1);

  action = gimp_action_group_get_action (group, action_name);

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
  GimpAction *action;
  gint        info_id;
  gchar      *action_name;

  info_id = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (info),
                                                "recent-action-id"));

  action_name = g_strdup_printf ("windows-recent-%04d", info_id);
  action = gimp_action_group_get_action (group, action_name);
  g_free (action_name);

  if (action)
    gimp_action_group_remove_action_and_accel (group, action);
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
