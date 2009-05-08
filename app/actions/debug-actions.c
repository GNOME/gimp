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

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "core/gimp.h"

#include "widgets/gimpactiongroup.h"

#include "debug-actions.h"
#include "debug-commands.h"


#ifdef ENABLE_DEBUG_MENU

static const GimpActionEntry debug_actions[] =
{
  { "debug-menu", NULL, "_Debug" },

  { "debug-mem-profile", NULL,
    "_Memory Profile", NULL, NULL,
    G_CALLBACK (debug_mem_profile_cmd_callback),
    NULL },

  { "debug-benchmark-projection", NULL,
    "Benchmark _Projection", NULL,
    "Invalidates the entire projection, measures the time it takes to "
    "validate (render) it again, and print the result to stdout.",
    G_CALLBACK (debug_benchmark_projection_cmd_callback),
    NULL },

  { "debug-show-image-graph", NULL,
    "Show Image _Graph", NULL,
    "Creates a new image showing the GEGL graph of this image",
    G_CALLBACK (debug_show_image_graph_cmd_callback),
    NULL },

  { "debug-dump-items", NULL,
    "_Dump Items", NULL, NULL,
    G_CALLBACK (debug_dump_menus_cmd_callback),
    NULL },

  { "debug-dump-managers", NULL,
    "Dump _UI Managers", NULL, NULL,
    G_CALLBACK (debug_dump_managers_cmd_callback),
    NULL },

  { "debug-dump-keyboard-shortcuts", NULL,
    "Dump _Keyboard Shortcuts", NULL, NULL,
    G_CALLBACK (debug_dump_keyboard_shortcuts_cmd_callback),
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
  gimp_action_group_add_actions (group, NULL,
                                 debug_actions,
                                 G_N_ELEMENTS (debug_actions));
#endif
}

void
debug_actions_update (GimpActionGroup *group,
                      gpointer         data)
{
}
