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

#include <gtk/gtk.h>

#include "actions-types.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimptoolinfo.h"

#include "tools/gimp-tools.h"
#include "tools/gimpimagemaptool.h"
#include "tools/tool_manager.h"

#include "actions.h"
#include "tools-commands.h"


void
tools_select_cmd_callback (GtkAction   *action,
                           const gchar *value,
			   gpointer     data)
{
  Gimp         *gimp;
  GimpToolInfo *tool_info;
  GimpContext  *context;
  GimpDisplay  *gdisp;
  return_if_no_gimp (gimp, data);

  tool_info = (GimpToolInfo *)
    gimp_container_get_child_by_name (gimp->tool_info_list, value);

  context = gimp_get_user_context (gimp);

  /*  always allocate a new tool when selected from the image menu
   */
  if (gimp_context_get_tool (context) != tool_info)
    {
      gimp_context_set_tool (context, tool_info);
    }
  else
    {
      gimp_context_tool_changed (context);
    }

  gdisp = gimp_context_get_display (context);

  if (gdisp)
    tool_manager_initialize_active (gimp, gdisp);
}

void
tools_toggle_visibility_cmd_callback (GtkAction *action,
                                      gpointer   data)
{
  GimpContext  *context;
  GimpToolInfo *tool_info;
  return_if_no_context (context, data);

  tool_info = gimp_context_get_tool (context);

  if (tool_info)
    {
      gboolean active =
        gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

      if (active != tool_info->visible)
        g_object_set (tool_info, "visible", active, NULL);
    }
}

void
tools_raise_cmd_callback (GtkAction *action,
                          gpointer   data)
{
  GimpContext  *context;
  GimpToolInfo *tool_info;
  return_if_no_context (context, data);

  tool_info = gimp_context_get_tool (context);

  if (tool_info)
    {
      GimpContainer *container;
      gint           index;

      container = context->gimp->tool_info_list;
      index     = gimp_container_get_child_index (container,
                                                  GIMP_OBJECT (tool_info));

      if (index > 0)
        gimp_container_reorder (container, GIMP_OBJECT (tool_info), index - 1);
    }
}

void
tools_raise_to_top_cmd_callback (GtkAction *action,
                                 gpointer   data)
{
  GimpContext  *context;
  GimpToolInfo *tool_info;
  return_if_no_context (context, data);

  tool_info = gimp_context_get_tool (context);

  if (tool_info)
    {
      GimpContainer *container;
      gint           index;

      container = context->gimp->tool_info_list;
      index     = gimp_container_get_child_index (container,
                                                  GIMP_OBJECT (tool_info));

      if (index > 0)
        gimp_container_reorder (container, GIMP_OBJECT (tool_info), 0);
    }
}

void
tools_lower_cmd_callback (GtkAction *action,
                          gpointer   data)
{
  GimpContext  *context;
  GimpToolInfo *tool_info;
  return_if_no_context (context, data);

  tool_info = gimp_context_get_tool (context);

  if (tool_info)
    {
      GimpContainer *container;
      gint           index;

      container = context->gimp->tool_info_list;
      index     = gimp_container_get_child_index (container,
                                                  GIMP_OBJECT (tool_info));

      if (index + 1 < gimp_container_num_children (container))
        gimp_container_reorder (container, GIMP_OBJECT (tool_info), index + 1);
    }
}

void
tools_lower_to_bottom_cmd_callback (GtkAction *action,
                                    gpointer   data)
{
  GimpContext  *context;
  GimpToolInfo *tool_info;
  return_if_no_context (context, data);

  tool_info = gimp_context_get_tool (context);

  if (tool_info)
    {
      GimpContainer *container;
      gint           index;

      container = context->gimp->tool_info_list;
      index     = gimp_container_num_children (container) - 1;

      index = index >= 0 ? index : 0;

      gimp_container_reorder (container, GIMP_OBJECT (tool_info), index);
    }
}

void
tools_reset_cmd_callback (GtkAction *action,
                          gpointer   data)
{
  GimpContext   *context;
  GimpContainer *container;
  GList         *list;
  gint           i = 0;
  return_if_no_context (context, data);

  container = context->gimp->tool_info_list;

  for (list = gimp_tools_get_default_order (context->gimp);
       list;
       list = g_list_next (list))
    {
      GimpObject *object = gimp_container_get_child_by_name (container,
                                                             list->data);

      if (object)
        {
          gimp_container_reorder (container, object, i);

          g_object_set (object, "visible",
                        ! g_type_is_a (GIMP_TOOL_INFO (object)->tool_type,
                                       GIMP_TYPE_IMAGE_MAP_TOOL),
                        NULL);

          i++;
        }
    }
}
