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

#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimpdockwindow.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpmenudock.h"

#include "display/gimpimagewindow.h"

#include "actions.h"
#include "dock-actions.h"
#include "dock-commands.h"
#include "window-actions.h"
#include "window-commands.h"

#include "gimp-intl.h"


static const GimpActionEntry dock_actions[] =
{
  { "dock-move-to-screen-menu", GIMP_ICON_WINDOW_MOVE_TO_SCREEN,
    NC_("dock-action", "M_ove to Screen"), NULL, NULL, NULL,
    GIMP_HELP_DOCK_CHANGE_SCREEN },

  { "dock-close", GIMP_ICON_WINDOW_CLOSE,
    NC_("dock-action", "Close Dock"), "", NULL,
    window_close_cmd_callback,
    GIMP_HELP_DOCK_CLOSE },

  { "dock-open-display", NULL,
    NC_("dock-action", "_Open Display..."), NULL,
    NC_("dock-action", "Connect to another display"),
    window_open_display_cmd_callback,
    NULL }
};

static const GimpToggleActionEntry dock_toggle_actions[] =
{
  { "dock-show-image-menu", NULL,
    NC_("dock-action", "_Show Image Selection"), NULL, NULL,
    dock_toggle_image_menu_cmd_callback,
    TRUE,
    GIMP_HELP_DOCK_IMAGE_MENU },

  { "dock-auto-follow-active", NULL,
    NC_("dock-action", "Auto _Follow Active Image"), NULL, NULL,
    dock_toggle_auto_cmd_callback,
    TRUE,
    GIMP_HELP_DOCK_AUTO_BUTTON }
};


void
dock_actions_setup (GimpActionGroup *group)
{
  gimp_action_group_add_actions (group, "dock-action",
                                 dock_actions,
                                 G_N_ELEMENTS (dock_actions));

  gimp_action_group_add_toggle_actions (group, "dock-action",
                                        dock_toggle_actions,
                                        G_N_ELEMENTS (dock_toggle_actions));

  window_actions_setup (group, GIMP_HELP_DOCK_CHANGE_SCREEN);
}

void
dock_actions_update (GimpActionGroup *group,
                     gpointer         data)
{
  GtkWidget *widget   = action_data_get_widget (data);
  GtkWidget *toplevel = NULL;

  if (widget)
    toplevel = gtk_widget_get_toplevel (widget);

#define SET_ACTIVE(action,active) \
        gimp_action_group_set_action_active (group, action, (active) != 0)
#define SET_VISIBLE(action,active) \
        gimp_action_group_set_action_visible (group, action, (active) != 0)

  if (GIMP_IS_DOCK_WINDOW (toplevel))
    {
      GimpDockWindow *dock_window = GIMP_DOCK_WINDOW (toplevel);
      gboolean show_image_menu = ! gimp_dock_window_has_toolbox (dock_window);

      if (show_image_menu)
        {
          SET_VISIBLE ("dock-show-image-menu",    TRUE);
          SET_VISIBLE ("dock-auto-follow-active", TRUE);

          SET_ACTIVE ("dock-show-image-menu",
                      gimp_dock_window_get_show_image_menu (dock_window));
          SET_ACTIVE ("dock-auto-follow-active",
                      gimp_dock_window_get_auto_follow_active (dock_window));
        }
      else
        {
          SET_VISIBLE ("dock-show-image-menu",    FALSE);
          SET_VISIBLE ("dock-auto-follow-active", FALSE);
        }

      /*  update the window actions only in the context of their
       *  own window (not in the context of some display or NULL)
       *  (see view-actions.c)
       */
      window_actions_update (group, toplevel);
    }
  else if (GIMP_IS_IMAGE_WINDOW (toplevel))
    {
      SET_VISIBLE ("dock-show-image-menu",    FALSE);
      SET_VISIBLE ("dock-auto-follow-active", FALSE);
    }

#undef SET_ACTIVE
#undef SET_VISIBLE
}
