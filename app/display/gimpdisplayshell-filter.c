/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1999 Manish Singh
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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "display-types.h"

#include "gimpdisplayshell.h"
#include "gimpdisplayshell-expose.h"
#include "gimpdisplayshell-filter.h"
#include "gimpdisplayshell-profile.h"


/*  local function prototypes  */

static void   gimp_display_shell_filter_changed (GimpColorDisplayStack *stack,
                                                 GimpDisplayShell      *shell);


/*  public functions  */

void
gimp_display_shell_filter_set (GimpDisplayShell      *shell,
                               GimpColorDisplayStack *stack)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (stack == NULL || GIMP_IS_COLOR_DISPLAY_STACK (stack));

  if (stack == shell->filter_stack)
    return;

  if (shell->filter_stack)
    {
      g_signal_handlers_disconnect_by_func (shell->filter_stack,
                                            gimp_display_shell_filter_changed,
                                            shell);

      g_object_unref (shell->filter_stack);
    }

  shell->filter_stack = stack;

  if (shell->filter_stack)
    {
      g_object_ref (shell->filter_stack);

      g_signal_connect (shell->filter_stack, "changed",
                        G_CALLBACK (gimp_display_shell_filter_changed),
                        shell);
    }

  gimp_display_shell_filter_changed (NULL, shell);
}

gboolean
gimp_display_shell_has_filter (GimpDisplayShell *shell)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), FALSE);

  return shell->filter_stack &&
         gimp_color_display_stack_get_filters (shell->filter_stack);
}

GimpColorDisplayStack *
gimp_display_shell_filter_new (GimpDisplayShell *shell)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), NULL);

#if 0
  /*  disabled because we use gimpdisplayshell-profile now, keep
   *  the code around for reference.
   */
  if (config->display_module)
    {
      GType type = g_type_from_name (config->display_module);

      if (g_type_is_a (type, GIMP_TYPE_COLOR_DISPLAY))
        {
          GimpColorDisplay      *display;
          GimpColorDisplayStack *stack;

          display = g_object_new (type,
                                  "color-config",  config,
                                  "color-managed", shell,
                                  NULL);

          stack = gimp_color_display_stack_new ();

          gimp_color_display_stack_add (stack, display);
          g_object_unref (display);

          return stack;
        }
    }
#endif

  return NULL;
}


/*  private functions  */

static gboolean
gimp_display_shell_filter_changed_idle (gpointer data)
{
  GimpDisplayShell *shell = data;

  gimp_display_shell_profile_update (shell);
  gimp_display_shell_expose_full (shell);

  shell->filter_idle_id = 0;

  return FALSE;
}

static void
gimp_display_shell_filter_changed (GimpColorDisplayStack *stack,
                                   GimpDisplayShell      *shell)
{
  if (shell->filter_idle_id)
    g_source_remove (shell->filter_idle_id);

  shell->filter_idle_id =
    g_idle_add_full (G_PRIORITY_LOW,
                     gimp_display_shell_filter_changed_idle,
                     shell, NULL);
}
