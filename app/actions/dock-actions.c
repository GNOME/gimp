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

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimpdock.h"
#include "widgets/gimpdockbook.h"
#include "widgets/gimpdocked.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpimagedock.h"

#include "dock-actions.h"
#include "dock-commands.h"

#include "gimp-intl.h"


static GimpActionEntry dock_actions[] =
{
  { "dock-close", GTK_STOCK_CLOSE,
    N_("Close Dock"), "<control>W", NULL,
    G_CALLBACK (dock_close_cmd_callback),
    GIMP_HELP_DOCK_CLOSE }
};


void
dock_actions_setup (GimpActionGroup *group)
{
  gimp_action_group_add_actions (group,
                                 dock_actions,
                                 G_N_ELEMENTS (dock_actions));
}

void
dock_actions_update (GimpActionGroup *group,
                     gpointer         data)
{
}
