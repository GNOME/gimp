/* The GIMP -- an image manipulation program
 * Copyright (C) 1999 Manish Singh
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

#include <gmodule.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "display-types.h"

#include "core/gimpimage.h"

#include "gimpdisplay.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-filter.h"


GimpColorDisplay *
gimp_display_shell_filter_attach (GimpDisplayShell *shell,
                                  GType             type)
{
  GimpColorDisplay *filter;

  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), NULL);
  g_return_val_if_fail (g_type_is_a (type, GIMP_TYPE_COLOR_DISPLAY), NULL);

  filter = gimp_color_display_new (type);

  if (filter)
    {
      shell->filters = g_list_append (shell->filters, filter);

      return filter;
    }
  else
    g_warning ("Tried to attach a nonexistant color display");

  return NULL;
}

GimpColorDisplay *
gimp_display_shell_filter_attach_clone (GimpDisplayShell *shell,
                                        GimpColorDisplay *filter)
{
  GimpColorDisplay *clone;

  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), NULL);
  g_return_val_if_fail (GIMP_IS_COLOR_DISPLAY (filter), NULL);

  clone = gimp_color_display_clone (filter);

  if (clone)
    {
      shell->filters = g_list_append (shell->filters, clone);

      return clone;
    }
  else
    g_warning ("Tried to clone a nonexistant color display");

  return NULL;
}

void
gimp_display_shell_filter_detach (GimpDisplayShell *shell,
                                  GimpColorDisplay *filter)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (GIMP_IS_COLOR_DISPLAY (filter));

  shell->filters = g_list_remove (shell->filters, filter);
}

void
gimp_display_shell_filter_detach_destroy (GimpDisplayShell *shell,
                                          GimpColorDisplay *filter)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (GIMP_IS_COLOR_DISPLAY (filter));

  g_object_unref (filter);

  shell->filters = g_list_remove (shell->filters, filter);
}

void
gimp_display_shell_filter_detach_all (GimpDisplayShell *shell)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  g_list_foreach (shell->filters, (GFunc) g_object_unref, NULL);
  g_list_free (shell->filters);
  shell->filters = NULL;
}

void
gimp_display_shell_filter_reorder_up (GimpDisplayShell *shell,
                                      GimpColorDisplay *filter)
{
  GList *node_list;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (GIMP_IS_COLOR_DISPLAY (filter));

  node_list = g_list_find (shell->filters, filter);

  if (node_list->prev)
    {
      node_list->data       = node_list->prev->data;
      node_list->prev->data = filter;
    }
}

void
gimp_display_shell_filter_reorder_down (GimpDisplayShell *shell,
                                        GimpColorDisplay *filter)
{
  GList *node_list;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (GIMP_IS_COLOR_DISPLAY (filter));

  node_list = g_list_find (shell->filters, filter);

  if (node_list->next)
    {
      node_list->data       = node_list->next->data;
      node_list->next->data = filter;
    }
}
