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

#include "display-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"

#include "menus/menus.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimpuimanager.h"

#include "gimpdisplay.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-actions.h"
#include "gimpimagewindow.h"


void
gimp_display_shell_set_action_sensitive (GimpDisplayShell *shell,
                                         const gchar      *action,
                                         gboolean          sensitive)
{
  GimpImageWindow *window;
  GimpContext     *context;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (action != NULL);

  window = gimp_display_shell_get_window (shell);

  if (window && gimp_image_window_get_active_shell (window) == shell)
    {
      GimpUIManager   *manager = menus_get_image_manager_singleton (shell->display->gimp);
      GimpActionGroup *action_group;

      action_group = gimp_ui_manager_get_action_group (manager, "view");

      if (action_group)
        gimp_action_group_set_action_sensitive (action_group, action, sensitive, NULL);
    }

  context = gimp_get_user_context (shell->display->gimp);

  if (shell->display == gimp_context_get_display (context))
    {
      GimpActionGroup *action_group;

      action_group = gimp_ui_manager_get_action_group (shell->popup_manager,
                                                       "view");

      if (action_group)
        gimp_action_group_set_action_sensitive (action_group, action, sensitive, NULL);
    }
}

void
gimp_display_shell_set_action_active (GimpDisplayShell *shell,
                                      const gchar      *action,
                                      gboolean          active)
{
  GimpImageWindow *window;
  GimpContext     *context;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (action != NULL);

  window = gimp_display_shell_get_window (shell);

  if (window && gimp_image_window_get_active_shell (window) == shell)
    {
      GimpUIManager   *manager = menus_get_image_manager_singleton (shell->display->gimp);
      GimpActionGroup *action_group;

      action_group = gimp_ui_manager_get_action_group (manager, "view");

      if (action_group)
        gimp_action_group_set_action_active (action_group, action, active);
    }

  context = gimp_get_user_context (shell->display->gimp);

  if (shell->display == gimp_context_get_display (context))
    {
      GimpActionGroup *action_group;

      action_group = gimp_ui_manager_get_action_group (shell->popup_manager,
                                                       "view");

      if (action_group)
        gimp_action_group_set_action_active (action_group, action, active);
    }
}

void
gimp_display_shell_set_action_color (GimpDisplayShell *shell,
                                     const gchar      *action,
                                     GeglColor        *color)
{
  GimpImageWindow *window;
  GimpContext     *context;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (action != NULL);

  window = gimp_display_shell_get_window (shell);

  if (window && gimp_image_window_get_active_shell (window) == shell)
    {
      GimpUIManager   *manager = menus_get_image_manager_singleton (shell->display->gimp);
      GimpActionGroup *action_group;

      action_group = gimp_ui_manager_get_action_group (manager, "view");

      if (action_group)
        gimp_action_group_set_action_color (action_group, action, color, FALSE);
    }

  context = gimp_get_user_context (shell->display->gimp);

  if (shell->display == gimp_context_get_display (context))
    {
      GimpActionGroup *action_group;

      action_group = gimp_ui_manager_get_action_group (shell->popup_manager,
                                                       "view");

      if (action_group)
        gimp_action_group_set_action_color (action_group, action, color, FALSE);
    }
}
