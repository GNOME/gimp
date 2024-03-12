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

#include "menus-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimplist.h"
#include "core/gimptoolinfo.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpuimanager.h"

#include "tool-options-menu.h"


/*  local function prototypes  */

static void   tool_options_menu_update         (GimpUIManager *manager,
                                                gpointer       update_data,
                                                const gchar   *ui_path);
static void   tool_options_menu_update_after   (GimpUIManager *manager,
                                                gpointer       update_data,
                                                const gchar   *ui_path);
static void   tool_options_menu_update_presets (GimpUIManager *manager,
                                                const gchar   *menu_path,
                                                const gchar   *which_action,
                                                GimpContainer *presets);


/*  public functions  */

void
tool_options_menu_setup (GimpUIManager *manager,
                         const gchar   *ui_path)
{
  g_signal_connect (manager, "update",
                    G_CALLBACK (tool_options_menu_update),
                    (gpointer) ui_path);
  g_signal_connect_after (manager, "update",
                          G_CALLBACK (tool_options_menu_update_after),
                          (gpointer) ui_path);

  tool_options_menu_update_after (manager, NULL, ui_path);
}


/*  private functions  */

static void
tool_options_menu_update (GimpUIManager *manager,
                          gpointer       update_data,
                          const gchar   *ui_path)
{
  /* no-op. */
}

static void
tool_options_menu_update_after (GimpUIManager *manager,
                                gpointer       update_data,
                                const gchar   *ui_path)
{
  GimpContext  *context;
  GimpToolInfo *tool_info;

  context   = gimp_get_user_context (manager->gimp);
  tool_info = gimp_context_get_tool (context);

  if (! tool_info->presets)
    return;

  tool_options_menu_update_presets (manager, "Save Tool Preset",
                                    "save", tool_info->presets);

  tool_options_menu_update_presets (manager, "Restore Tool Preset",
                                    "restore", tool_info->presets);

  tool_options_menu_update_presets (manager, "Edit Tool Preset",
                                    "edit", tool_info->presets);

  tool_options_menu_update_presets (manager, "Delete Tool Preset",
                                    "delete", tool_info->presets);
}

static void
tool_options_menu_update_presets (GimpUIManager *manager,
                                  const gchar   *menu_path,
                                  const gchar   *which_action,
                                  GimpContainer *presets)
{
  gchar *action_prefix;
  gint   n_children;
  gint   i;

  action_prefix = g_strdup_printf ("tool-options-%s-preset-", which_action);
  gimp_ui_manager_remove_uis (manager, action_prefix);

  n_children = gimp_container_get_n_children (presets);
  for (i = 0; i < n_children; i++)
    {
      gchar *action_name;
      gchar *path;

      action_name = g_strdup_printf ("%s%03d", action_prefix, i);
      path        = g_strdup_printf ("/Tool Options Menu/%s", menu_path);

      gimp_ui_manager_add_ui (manager, path, action_name, FALSE);

      g_free (action_name);
      g_free (path);
    }

  g_free (action_prefix);
}
