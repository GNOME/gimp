/* GIMP - The GNU Image Manipulation Program
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

#include "actions-types.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimptoolinfo.h"

#include "paint/gimpinkoptions.h"

#include "widgets/gimpenumaction.h"
#include "widgets/gimpuimanager.h"

#include "tools/gimp-tools.h"
#include "tools/gimpcoloroptions.h"
#include "tools/gimpforegroundselectoptions.h"
#include "tools/gimprectangleoptions.h"
#include "tools/gimpimagemaptool.h"
#include "tools/gimptoolcontrol.h"
#include "tools/tool_manager.h"

#include "actions.h"
#include "tools-commands.h"


/*  local function prototypes  */

static void   tools_activate_enum_action (const gchar *action_desc,
                                          gint         value);


/*  public functions  */

void
tools_select_cmd_callback (GtkAction   *action,
                           const gchar *value,
                           gpointer     data)
{
  Gimp         *gimp;
  GimpToolInfo *tool_info;
  GimpContext  *context;
  GimpDisplay  *display;
  gboolean      rotate_layer = FALSE;
  return_if_no_gimp (gimp, data);

  /*  special case gimp-rotate-tool being called from the Layer menu  */
  if (strcmp (value, "gimp-rotate-layer") == 0)
    {
      rotate_layer = TRUE;
      value = "gimp-rotate-tool";
    }

  tool_info = gimp_get_tool_info (gimp, value);

  context = gimp_get_user_context (gimp);

  /*  always allocate a new tool when selected from the image menu
   */
  if (gimp_context_get_tool (context) != tool_info)
    {
      gimp_context_set_tool (context, tool_info);

      if (rotate_layer)
        g_object_set (tool_info->tool_options,
                      "type", GIMP_TRANSFORM_TYPE_LAYER,
                      NULL);
    }
  else
    {
      gimp_context_tool_changed (context);
    }

  display = gimp_context_get_display (context);

  if (display)
    tool_manager_initialize_active (gimp, display);
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
          gboolean visible;

          gimp_container_reorder (container, object, i);

          visible =
            GPOINTER_TO_INT (g_object_get_data (G_OBJECT (object),
                                                "gimp-tool-default-visible"));

          g_object_set (object, "visible", visible, NULL);

          i++;
        }
    }
}

void
tools_color_average_radius_cmd_callback (GtkAction *action,
                                         gint       value,
                                         gpointer   data)
{
  GimpContext  *context;
  GimpToolInfo *tool_info;
  return_if_no_context (context, data);

  tool_info = gimp_context_get_tool (context);

  if (tool_info && GIMP_IS_COLOR_OPTIONS (tool_info->tool_options))
    {
      action_select_property ((GimpActionSelectType) value,
                              G_OBJECT (tool_info->tool_options),
                              "average-radius",
                              1.0, 1.0, 10.0, FALSE);
    }
}

void
tools_paint_brush_scale_cmd_callback (GtkAction *action,
                                      gint       value,
                                      gpointer   data)
{
  GimpContext  *context;
  GimpToolInfo *tool_info;
  return_if_no_context (context, data);

  tool_info = gimp_context_get_tool (context);

  if (tool_info && GIMP_IS_PAINT_OPTIONS (tool_info->tool_options))
    {
      action_select_property ((GimpActionSelectType) value,
                              G_OBJECT (tool_info->tool_options),
                              "brush-scale",
                              0.01, 0.1, 1.0, FALSE);
    }
}

void
tools_ink_blob_size_cmd_callback (GtkAction *action,
                                  gint       value,
                                  gpointer   data)
{
  GimpContext  *context;
  GimpToolInfo *tool_info;
  return_if_no_context (context, data);

  tool_info = gimp_context_get_tool (context);

  if (tool_info && GIMP_IS_INK_OPTIONS (tool_info->tool_options))
    {
      action_select_property ((GimpActionSelectType) value,
                              G_OBJECT (tool_info->tool_options),
                              "size",
                              1.0, 1.0, 10.0, FALSE);
    }
}

void
tools_ink_blob_aspect_cmd_callback (GtkAction *action,
                                    gint       value,
                                    gpointer   data)
{
  GimpContext  *context;
  GimpToolInfo *tool_info;
  return_if_no_context (context, data);

  tool_info = gimp_context_get_tool (context);

  if (tool_info && GIMP_IS_INK_OPTIONS (tool_info->tool_options))
    {
      action_select_property ((GimpActionSelectType) value,
                              G_OBJECT (tool_info->tool_options),
                              "blob-aspect",
                              1.0, 0.1, 1.0, FALSE);
    }
}

void
tools_ink_blob_angle_cmd_callback (GtkAction *action,
                                   gint       value,
                                   gpointer   data)
{
  GimpContext  *context;
  GimpToolInfo *tool_info;
  return_if_no_context (context, data);

  tool_info = gimp_context_get_tool (context);

  if (tool_info && GIMP_IS_INK_OPTIONS (tool_info->tool_options))
    {
      action_select_property ((GimpActionSelectType) value,
                              G_OBJECT (tool_info->tool_options),
                              "blob-angle",
                              1.0, 1.0, 15.0, TRUE);
    }
}

void
tools_fg_select_brush_size_cmd_callback (GtkAction *action,
                                         gint       value,
                                         gpointer   data)
{
  GimpContext  *context;
  GimpToolInfo *tool_info;
  return_if_no_context (context, data);

  tool_info = gimp_context_get_tool (context);

  if (tool_info && GIMP_IS_FOREGROUND_SELECT_OPTIONS (tool_info->tool_options))
    {
      action_select_property ((GimpActionSelectType) value,
                              G_OBJECT (tool_info->tool_options),
                              "stroke-width",
                              1.0, 4.0, 16.0, FALSE);
    }
}

void
tools_value_1_cmd_callback (GtkAction *action,
                            gint       value,
                            gpointer   data)
{
  GimpContext *context;
  GimpTool    *tool;
  return_if_no_context (context, data);

  tool = tool_manager_get_active (context->gimp);

  if (tool)
    {
      const gchar *action_desc;

      action_desc = gimp_tool_control_get_action_value_1 (tool->control);

      if (action_desc)
        tools_activate_enum_action (action_desc, value);
    }
}

void
tools_value_2_cmd_callback (GtkAction *action,
                            gint       value,
                            gpointer   data)
{
  GimpContext *context;
  GimpTool    *tool;
  return_if_no_context (context, data);

  tool = tool_manager_get_active (context->gimp);

  if (tool)
    {
      const gchar *action_desc;

      action_desc = gimp_tool_control_get_action_value_2 (tool->control);

      if (action_desc)
        tools_activate_enum_action (action_desc, value);
    }
}

void
tools_value_3_cmd_callback (GtkAction *action,
                            gint       value,
                            gpointer   data)
{
  GimpContext *context;
  GimpTool    *tool;
  return_if_no_context (context, data);

  tool = tool_manager_get_active (context->gimp);

  if (tool)
    {
      const gchar *action_desc;

      action_desc = gimp_tool_control_get_action_value_3 (tool->control);

      if (action_desc)
        tools_activate_enum_action (action_desc, value);
    }
}

void
tools_value_4_cmd_callback (GtkAction *action,
                            gint       value,
                            gpointer   data)
{
  GimpContext *context;
  GimpTool    *tool;
  return_if_no_context (context, data);

  tool = tool_manager_get_active (context->gimp);

  if (tool)
    {
      const gchar *action_desc;

      action_desc = gimp_tool_control_get_action_value_4 (tool->control);

      if (action_desc)
        tools_activate_enum_action (action_desc, value);
    }
}

void
tools_object_1_cmd_callback (GtkAction *action,
                             gint       value,
                             gpointer   data)
{
  GimpContext *context;
  GimpTool    *tool;
  return_if_no_context (context, data);

  tool = tool_manager_get_active (context->gimp);

  if (tool)
    {
      const gchar *action_desc;

      action_desc = gimp_tool_control_get_action_object_1 (tool->control);

      if (action_desc)
        tools_activate_enum_action (action_desc, value);
    }
}

void
tools_object_2_cmd_callback (GtkAction *action,
                             gint       value,
                             gpointer   data)
{
  GimpContext *context;
  GimpTool    *tool;
  return_if_no_context (context, data);

  tool = tool_manager_get_active (context->gimp);

  if (tool)
    {
      const gchar *action_desc;

      action_desc = gimp_tool_control_get_action_object_2 (tool->control);

      if (action_desc)
        tools_activate_enum_action (action_desc, value);
    }
}


/*  private functions  */

static void
tools_activate_enum_action (const gchar *action_desc,
                            gint         value)
{
  gchar *group_name;
  gchar *action_name;

  group_name  = g_strdup (action_desc);
  action_name = strchr (group_name, '/');

  if (action_name)
    {
      GList     *managers;
      GtkAction *action;

      *action_name++ = '\0';

      managers = gimp_ui_managers_from_name ("<Image>");

      action = gimp_ui_manager_find_action (managers->data,
                                            group_name, action_name);

      if (GIMP_IS_ENUM_ACTION (action) &&
          GIMP_ENUM_ACTION (action)->value_variable)
        {
          gint old_value;

          old_value = GIMP_ENUM_ACTION (action)->value;
          GIMP_ENUM_ACTION (action)->value = value;
          gtk_action_activate (action);
          GIMP_ENUM_ACTION (action)->value = old_value;
        }
    }

  g_free (group_name);
}
