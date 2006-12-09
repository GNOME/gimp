/* GIMP - The GNU Image Manipulation Program
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

#include "debug-actions.h"
#include "debug-commands.h"


#ifdef ENABLE_DEBUG_MENU

static const GimpActionEntry debug_actions[] =
{
  { "debug-menu", NULL, "D_ebug" },

  { "debug-mem-profile", NULL,
    "_Memory Profile", NULL, NULL,
    G_CALLBACK (debug_mem_profile_cmd_callback),
    NULL },

  { "debug-dump-items", NULL,
    "_Dump Items", NULL, NULL,
    G_CALLBACK (debug_dump_menus_cmd_callback),
    NULL },

  { "debug-dump-managers", NULL,
    "Dump _UI Managers", NULL, NULL,
    G_CALLBACK (debug_dump_managers_cmd_callback),
    NULL },

  { "debug-dump-attached-data", NULL,
    "Dump Attached Data", NULL, NULL,
    G_CALLBACK (debug_dump_attached_data_cmd_callback),
    NULL }
};

#endif

void
debug_actions_setup (GimpActionGroup *group)
{
#ifdef ENABLE_DEBUG_MENU
  gimp_action_group_add_actions (group,
                                 debug_actions,
                                 G_N_ELEMENTS (debug_actions));
#endif
}

void
debug_actions_update (GimpActionGroup *group,
                      gpointer         data)
{
}
