/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpimagedock.h"

#include "actions.h"
#include "dock-actions.h"
#include "dock-commands.h"

#include "gimp-intl.h"


static GimpActionEntry dock_actions[] =
{
  { "dock-close", GTK_STOCK_CLOSE,
    N_("Close Dock"), "<control>W", NULL,
    G_CALLBACK (dock_close_cmd_callback),
    GIMP_HELP_DOCK_CLOSE },

  { "dock-move-to-screen", GIMP_STOCK_MOVE_TO_SCREEN,
    N_("M_ove to Screen..."), NULL, NULL,
    G_CALLBACK (dock_change_screen_cmd_callback),
    GIMP_HELP_DOCK_CHANGE_SCREEN }
};

static GimpToggleActionEntry dock_toggle_actions[] =
{
  { "dock-show-image-menu", NULL,
    N_("_Show Image Selection"), NULL, NULL,
    G_CALLBACK (dock_toggle_image_menu_cmd_callback),
    TRUE,
    GIMP_HELP_DOCK_IMAGE_MENU },

  { "dock-auto-follow-active", NULL,
    N_("Auto _Follow Active Image"), NULL, NULL,
    G_CALLBACK (dock_toggle_auto_cmd_callback),
    TRUE,
    GIMP_HELP_DOCK_AUTO_BUTTON }
};


void
dock_actions_setup (GimpActionGroup *group)
{
  gimp_action_group_add_actions (group,
                                 dock_actions,
                                 G_N_ELEMENTS (dock_actions));

  gimp_action_group_add_toggle_actions (group,
                                        dock_toggle_actions,
                                        G_N_ELEMENTS (dock_toggle_actions));
}

void
dock_actions_update (GimpActionGroup *group,
                     gpointer         data)
{
  GtkWidget *widget    = action_data_get_widget (data);
  GtkWidget *toplevel  = gtk_widget_get_toplevel (widget);
  gint       n_screens = 1;

#define SET_ACTIVE(action,active) \
        gimp_action_group_set_action_active (group, action, (active) != 0)
#define SET_VISIBLE(action,active) \
        gimp_action_group_set_action_visible (group, action, (active) != 0)

  n_screens = gdk_display_get_n_screens (gtk_widget_get_display (toplevel));

  if (GIMP_IS_IMAGE_DOCK (toplevel))
    {
      GimpImageDock *image_dock = GIMP_IMAGE_DOCK (toplevel);

      SET_VISIBLE ("dock-show-image-menu",    TRUE);
      SET_VISIBLE ("dock-auto-follow-active", TRUE);

      SET_ACTIVE ("dock-show-image-menu",    image_dock->show_image_menu);
      SET_ACTIVE ("dock-auto-follow-active", image_dock->auto_follow_active);
    }
  else
    {
      SET_VISIBLE ("dock-show-image-menu",    FALSE);
      SET_VISIBLE ("dock-auto-follow-active", FALSE);
    }

  SET_VISIBLE ("dock-move-to-screen", n_screens > 1);

#undef SET_ACTIVE
#undef SET_VISIBLE
}
