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
#include "core/gimpcontext.h"
#include "core/gimplist.h"
#include "core/gimptoolinfo.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimphelp-ids.h"

#include "actions.h"
#include "tools-actions.h"
#include "tools-commands.h"

#include "gimp-intl.h"


static GimpActionEntry tools_actions[] =
{
  { "tools-popup", GIMP_STOCK_TOOLS,
    N_("Tools Menu"), NULL, NULL, NULL,
    GIMP_HELP_TOOLS_DIALOG },

  { "tools-menu",           NULL, N_("_Tools")           },
  { "tools-select-menu",    NULL, N_("_Selection Tools") },
  { "tools-paint-menu",     NULL, N_("_Paint Tools")     },
  { "tools-transform-menu", NULL, N_("_Transform Tools") },
  { "tools-color-menu",     NULL, N_("_Color Tools")     },

  { "tools-raise", GTK_STOCK_GO_UP,
    N_("R_aise Tool"), NULL,
    N_("Raise tool"),
    G_CALLBACK (tools_raise_cmd_callback),
    NULL },

  { "tools-raise-to-top", GTK_STOCK_GOTO_TOP,
    N_("Ra_ise to Top"), NULL,
    N_("Raise tool to top"),
    G_CALLBACK (tools_raise_to_top_cmd_callback),
    NULL },

  { "tools-lower", GTK_STOCK_GO_DOWN,
    N_("L_ower Tool"), NULL,
    N_("Lower tool"),
    G_CALLBACK (tools_lower_cmd_callback),
    NULL },

  { "tools-lower-to-bottom", GTK_STOCK_GOTO_BOTTOM,
    N_("Lo_wer to Bottom"), NULL,
    N_("Lower tool to bottom"),
    G_CALLBACK (tools_lower_to_bottom_cmd_callback),
    NULL },

  { "tools-reset", GIMP_STOCK_RESET,
    N_("_Reset Order & Visibility"), NULL,
    N_("Reset tool order and visibility"),
    G_CALLBACK (tools_reset_cmd_callback),
    NULL }
};

static GimpToggleActionEntry tools_toggle_actions[] =
{
  { "tools-visibility", GIMP_STOCK_VISIBLE,
    N_("_Show in Toolbox"), NULL, NULL,
    G_CALLBACK (tools_toggle_visibility_cmd_callback),
    TRUE,
    NULL /* FIXME */ }
};

static GimpStringActionEntry tools_alternative_actions[] =
{
  { "tools-by-color-select-short", GIMP_STOCK_TOOL_BY_COLOR_SELECT,
    N_("_By Color"), NULL, NULL,
    "gimp-by-color-select-tool",
    GIMP_HELP_TOOL_BY_COLOR_SELECT },

  { "tools-rotate-arbitrary", GIMP_STOCK_TOOL_ROTATE,
    N_("_Arbitrary Rotation..."), NULL, NULL,
    "gimp-rotate-tool",
    GIMP_HELP_TOOL_ROTATE }
};


void
tools_actions_setup (GimpActionGroup *group)
{
  GtkAction *action;
  GList     *list;

  gimp_action_group_add_actions (group,
                                 tools_actions,
                                 G_N_ELEMENTS (tools_actions));

  gimp_action_group_add_toggle_actions (group,
                                        tools_toggle_actions,
                                        G_N_ELEMENTS (tools_toggle_actions));

  gimp_action_group_add_string_actions (group,
                                        tools_alternative_actions,
                                        G_N_ELEMENTS (tools_alternative_actions),
                                        G_CALLBACK (tools_select_cmd_callback));

  action = gtk_action_group_get_action (GTK_ACTION_GROUP (group),
                                        "tools-by-color-select-short");
  gtk_action_set_accel_path (action, "<Actions>/tools/tools-by-color-select");

#ifdef __GNUC__
#warning FIXME: remove accel_path hack
#endif
  g_object_set_data (G_OBJECT (action), "gimp-accel-path",
                     "<Actions>/tools/tools-by-color-select");

  action = gtk_action_group_get_action (GTK_ACTION_GROUP (group),
                                        "tools-rotate-arbitrary");
  gtk_action_set_accel_path (action, "<Actions>/tools/tools-rotate");

#ifdef __GNUC__
#warning FIXME: remove accel_path hack
#endif
  g_object_set_data (G_OBJECT (action), "gimp-accel-path",
                     "<Actions>/tools/tools-rotate");

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
          entry.help_id     = tool_info->help_id;
          entry.value       = identifier;

          gimp_action_group_add_string_actions (group,
                                                &entry, 1,
                                                G_CALLBACK (tools_select_cmd_callback));

          g_free (name);
        }
    }
}

void
tools_actions_update (GimpActionGroup *group,
                      gpointer         data)
{
  GimpContext   *context   = gimp_get_user_context (group->gimp);
  GimpToolInfo  *tool_info = gimp_context_get_tool (context);
  GimpContainer *container = context->gimp->tool_info_list;
  gboolean       raise     = FALSE;
  gboolean       lower     = FALSE;

#define SET_SENSITIVE(action,condition) \
        gimp_action_group_set_action_sensitive (group, action, (condition) != 0)
#define SET_ACTIVE(action,condition) \
        gimp_action_group_set_action_active (group, action, (condition) != 0)

  SET_SENSITIVE ("tools-visibility", tool_info);

  if (tool_info)
    {
      gint last_index;
      gint index;

      SET_ACTIVE ("tools-visibility", tool_info->visible);

      last_index = gimp_container_num_children (container) -1;
      index      = gimp_container_get_child_index   (container,
                                                     GIMP_OBJECT (tool_info));

      raise = index != 0;
      lower = index != last_index;
    }

  SET_SENSITIVE ("tools-raise",           raise);
  SET_SENSITIVE ("tools-raise-to-top",    raise);
  SET_SENSITIVE ("tools-lower",           lower);
  SET_SENSITIVE ("tools-lower-to-bottom", lower);

#undef SET_SENSITIVE
#undef SET_ACTIVE
}
