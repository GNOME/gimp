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


static void   gimp_display_shell_filter_detach_real (GimpDisplayShell *shell,
                                                     ColorDisplayNode *node);


ColorDisplayNode *
gimp_display_shell_filter_attach (GimpDisplayShell *shell,
                                  GType             type)
{
  GimpColorDisplay *color_display;
  ColorDisplayNode *node;

  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), NULL);
  g_return_val_if_fail (g_type_is_a (type, GIMP_TYPE_COLOR_DISPLAY), NULL);

  color_display = gimp_color_display_new (type);

  if (color_display)
    {
      node = g_new (ColorDisplayNode, 1);

      node->cd_name       = g_strdup (GIMP_COLOR_DISPLAY_GET_CLASS (color_display)->name);
      node->color_display = color_display;

      shell->filters = g_list_append (shell->filters, node);

      return node;
    }
  else
    g_warning ("Tried to attach a nonexistant color display");

  return NULL;
}

ColorDisplayNode *
gimp_display_shell_filter_attach_clone (GimpDisplayShell *shell,
                                        ColorDisplayNode *node)
{
  GimpColorDisplay *color_display;
  ColorDisplayNode *clone;

  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), NULL);
  g_return_val_if_fail (node != NULL, NULL);

  color_display = gimp_color_display_clone (node->color_display);

  if (color_display)
    {
      clone = g_new (ColorDisplayNode, 1);

      clone->cd_name       = g_strdup (node->cd_name);
      clone->color_display = color_display;

      shell->filters = g_list_append (shell->filters, node);

      return node;
    }
  else
    g_warning ("Tried to clone a nonexistant color display");

  return NULL;
}

void
gimp_display_shell_filter_detach (GimpDisplayShell *shell,
                                  ColorDisplayNode *node)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  shell->filters = g_list_remove (shell->filters, node);
}

void
gimp_display_shell_filter_detach_destroy (GimpDisplayShell *shell,
                                          ColorDisplayNode *node)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  gimp_display_shell_filter_detach_real (shell, node);

  shell->filters = g_list_remove (shell->filters, node);
}

void
gimp_display_shell_filter_detach_all (GimpDisplayShell *shell)
{
  GList *list;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  for (list = shell->filters; list; list = g_list_next (list))
    {
      gimp_display_shell_filter_detach_real (shell, list->data);
    }

  g_list_free (shell->filters);
  shell->filters = NULL;
}

static void
gimp_display_shell_filter_detach_real (GimpDisplayShell *shell,
                                       ColorDisplayNode *node)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (node != NULL);

  g_object_unref (node->color_display);
  g_free (node->cd_name);
  g_free (node);
}

void
gimp_display_shell_filter_reorder_up (GimpDisplayShell *shell,
                                      ColorDisplayNode *node)
{
  GList *node_list;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (node  != NULL);

  node_list = g_list_find (shell->filters, node);

  if (node_list->prev)
    {
      node_list->data = node_list->prev->data;
      node_list->prev->data = node;
    }
}

void
gimp_display_shell_filter_reorder_down (GimpDisplayShell *shell,
                                        ColorDisplayNode *node)
{
  GList *node_list;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (node  != NULL);

  node_list = g_list_find (shell->filters, node);

  if (node_list->next)
    {
      node_list->data = node_list->next->data;
      node_list->next->data = node;
    }
}

void
gimp_display_shell_filter_configure (ColorDisplayNode *node,
                                     GFunc             ok_func,
                                     gpointer          ok_data,
                                     GFunc             cancel_func,
                                     gpointer          cancel_data)
{
  g_return_if_fail (node != NULL);

  gimp_color_display_configure (node->color_display,
                                ok_func, ok_data,
                                cancel_func, cancel_data);
}

void
gimp_display_shell_filter_configure_cancel (ColorDisplayNode *node)
{
  g_return_if_fail (node != NULL);

  gimp_color_display_configure_cancel (node->color_display);
}
