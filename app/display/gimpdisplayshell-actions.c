/* LIGMA - The GNU Image Manipulation Program
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

#include "core/ligma.h"
#include "core/ligmacontext.h"

#include "widgets/ligmaactiongroup.h"
#include "widgets/ligmauimanager.h"

#include "ligmadisplay.h"
#include "ligmadisplayshell.h"
#include "ligmadisplayshell-actions.h"
#include "ligmaimagewindow.h"


void
ligma_display_shell_set_action_sensitive (LigmaDisplayShell *shell,
                                         const gchar      *action,
                                         gboolean          sensitive)
{
  LigmaImageWindow *window;
  LigmaContext     *context;

  g_return_if_fail (LIGMA_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (action != NULL);

  window = ligma_display_shell_get_window (shell);

  if (window && ligma_image_window_get_active_shell (window) == shell)
    {
      LigmaUIManager   *manager = ligma_image_window_get_ui_manager (window);
      LigmaActionGroup *action_group;

      action_group = ligma_ui_manager_get_action_group (manager, "view");

      if (action_group)
        ligma_action_group_set_action_sensitive (action_group, action, sensitive, NULL);
    }

  context = ligma_get_user_context (shell->display->ligma);

  if (shell->display == ligma_context_get_display (context))
    {
      LigmaActionGroup *action_group;

      action_group = ligma_ui_manager_get_action_group (shell->popup_manager,
                                                       "view");

      if (action_group)
        ligma_action_group_set_action_sensitive (action_group, action, sensitive, NULL);
    }
}

void
ligma_display_shell_set_action_active (LigmaDisplayShell *shell,
                                      const gchar      *action,
                                      gboolean          active)
{
  LigmaImageWindow *window;
  LigmaContext     *context;

  g_return_if_fail (LIGMA_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (action != NULL);

  window = ligma_display_shell_get_window (shell);

  if (window && ligma_image_window_get_active_shell (window) == shell)
    {
      LigmaUIManager   *manager = ligma_image_window_get_ui_manager (window);
      LigmaActionGroup *action_group;

      action_group = ligma_ui_manager_get_action_group (manager, "view");

      if (action_group)
        ligma_action_group_set_action_active (action_group, action, active);
    }

  context = ligma_get_user_context (shell->display->ligma);

  if (shell->display == ligma_context_get_display (context))
    {
      LigmaActionGroup *action_group;

      action_group = ligma_ui_manager_get_action_group (shell->popup_manager,
                                                       "view");

      if (action_group)
        ligma_action_group_set_action_active (action_group, action, active);
    }
}

void
ligma_display_shell_set_action_color (LigmaDisplayShell *shell,
                                     const gchar      *action,
                                     const LigmaRGB    *color)
{
  LigmaImageWindow *window;
  LigmaContext     *context;

  g_return_if_fail (LIGMA_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (action != NULL);

  window = ligma_display_shell_get_window (shell);

  if (window && ligma_image_window_get_active_shell (window) == shell)
    {
      LigmaUIManager   *manager = ligma_image_window_get_ui_manager (window);
      LigmaActionGroup *action_group;

      action_group = ligma_ui_manager_get_action_group (manager, "view");

      if (action_group)
        ligma_action_group_set_action_color (action_group, action, color, FALSE);
    }

  context = ligma_get_user_context (shell->display->ligma);

  if (shell->display == ligma_context_get_display (context))
    {
      LigmaActionGroup *action_group;

      action_group = ligma_ui_manager_get_action_group (shell->popup_manager,
                                                       "view");

      if (action_group)
        ligma_action_group_set_action_color (action_group, action, color, FALSE);
    }
}
