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

#include "core/gimp.h"
#include "core/gimplist.h"
#include "core/gimptoolinfo.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimphelp-ids.h"

#include "actions/tools-actions.h"
#include "actions/tools-commands.h"

#include "gimp-intl.h"


static GimpActionEntry tools_actions[] =
{
  { "tools-menu",           NULL, N_("_Tools")           },
  { "tools-select-menu",    NULL, N_("_Selection Tools") },
  { "tools-paint-menu",     NULL, N_("_Paint Tools")     },
  { "tools-transform-menu", NULL, N_("_Transform Tools") },
  { "tools-color-menu",     NULL, N_("_Color Tools")     },

  { "tools-default-colors", GIMP_STOCK_DEFAULT_COLORS,
    N_("_Default Colors"), "D", NULL,
    G_CALLBACK (tools_default_colors_cmd_callback),
    GIMP_HELP_TOOLBOX_DEFAULT_COLORS },

  { "tools-swap-colors", GIMP_STOCK_SWAP_COLORS,
    N_("S_wap Colors"), "X", NULL,
    G_CALLBACK (tools_swap_colors_cmd_callback),
    GIMP_HELP_TOOLBOX_SWAP_COLORS }
};


void
tools_actions_setup (GimpActionGroup *group,
                     gpointer         data)
{
  GList *list;

  gimp_action_group_add_actions (group,
                                 tools_actions,
                                 G_N_ELEMENTS (tools_actions),
                                 data);

  for (list = GIMP_LIST (group->gimp->tool_info_list)->list;
       list;
       list = g_list_next (list))
    {
      GimpToolInfo *tool_info = list->data;

      if (tool_info->menu_path)
        {
          GimpStringActionEntry  entry;
          const gchar           *stock_id;
          const gchar           *identifier;
          gchar                 *tmp;
          gchar                 *name;

          stock_id   = gimp_viewable_get_stock_id (GIMP_VIEWABLE (tool_info));
          identifier = gimp_object_get_name (GIMP_OBJECT (tool_info));

          tmp = g_strndup (identifier + strlen ("gimp-"),
                           strlen (identifier) - strlen ("gimp--tool"));
          name = g_strdup_printf ("tools-%s", tmp);
          g_free (tmp);

          entry.name        = name;
          entry.stock_id    = stock_id;
          entry.label       = tool_info->menu_path;
          entry.accelerator = tool_info->menu_accel;
          entry.tooltip     = tool_info->blurb;
          entry.value       = identifier;

          gimp_action_group_add_string_actions (group,
                                                &entry, 1,
                                                G_CALLBACK (tools_select_cmd_callback),
                                                data);

          g_free (name);
        }
    }
}

void
tools_actions_update (GimpActionGroup *group,
                      gpointer         data)
{
}
