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

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"

#include "actions-types.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimptoolinfo.h"

#include "paint/gimpinkoptions.h"
#include "paint/gimpairbrushoptions.h"
#include "paint/gimpmybrushoptions.h"

#include "widgets/gimpaction.h"
#include "widgets/gimpenumaction.h"
#include "widgets/gimpuimanager.h"

#include "display/gimpdisplay.h"

#include "tools/gimp-tools.h"
#include "tools/gimpcoloroptions.h"
#include "tools/gimpforegroundselectoptions.h"
#include "tools/gimppaintselectoptions.h"
#include "tools/gimprectangleoptions.h"
#include "tools/gimptool.h"
#include "tools/gimptoolcontrol.h"
#include "tools/gimptransformoptions.h"
#include "tools/gimptransformtool.h"
#include "tools/gimpwarpoptions.h"
#include "tools/tool_manager.h"

#include "actions.h"
#include "tools-commands.h"


/*  local function prototypes  */

static void   tools_activate_enum_action (const gchar *action_desc,
                                          GVariant    *value);


/*  public functions  */

void
tools_select_cmd_callback (GimpAction *action,
                           GVariant   *value,
                           gpointer    data)
{
  Gimp              *gimp;
  GimpToolInfo      *tool_info;
  GimpContext       *context;
  const gchar       *tool_name;
  gboolean           set_transform_type = FALSE;
  GimpTransformType  transform_type;
  return_if_no_gimp (gimp, data);

  tool_name = g_variant_get_string (value, NULL);

  /*  special case gimp-rotate-tool being called from the Image or Layer
   *  menus
   */
  if (strcmp (tool_name, "gimp-rotate-layer") == 0)
    {
      tool_name          = "gimp-rotate-tool";
      set_transform_type = TRUE;
      transform_type     = GIMP_TRANSFORM_TYPE_LAYER;
    }
  else if (strcmp (tool_name, "gimp-rotate-image") == 0)
    {
      tool_name          = "gimp-rotate-tool";
      set_transform_type = TRUE;
      transform_type     = GIMP_TRANSFORM_TYPE_IMAGE;
    }

  tool_info = gimp_get_tool_info (gimp, tool_name);

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

  if (set_transform_type)
    {
      GimpTool *tool = tool_manager_get_active (gimp);

      gimp_transform_tool_set_type (GIMP_TRANSFORM_TOOL (tool),
                                    transform_type);
    }
}

void
tools_color_average_radius_cmd_callback (GimpAction *action,
                                         GVariant   *value,
                                         gpointer    data)
{
  GimpContext          *context;
  GimpToolInfo         *tool_info;
  GimpActionSelectType  select_type;
  return_if_no_context (context, data);

  select_type = (GimpActionSelectType) g_variant_get_int32 (value);

  tool_info = gimp_context_get_tool (context);

  if (tool_info && GIMP_IS_COLOR_OPTIONS (tool_info->tool_options))
    {
      action_select_property (select_type,
                              action_data_get_display (data),
                              G_OBJECT (tool_info->tool_options),
                              "average-radius",
                              1.0, 1.0, 10.0, 0.1, FALSE);
    }
}

void
tools_paintbrush_pixel_size_cmd_callback (GimpAction *action,
                                          GVariant   *value,
                                          gpointer    data)
{
  GimpContext          *context;
  GimpToolInfo         *tool_info;
  gdouble               dvalue;
  return_if_no_context (context, data);

  dvalue = g_variant_get_double (value);
  tool_info = gimp_context_get_tool (context);

  if (tool_info && GIMP_IS_PAINT_OPTIONS (tool_info->tool_options))
    {
      GParamSpec *pspec;

      pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (tool_info->tool_options),
                                            "brush-size");
      dvalue = CLAMP (dvalue,
                      G_PARAM_SPEC_DOUBLE (pspec)->minimum,
                      G_PARAM_SPEC_DOUBLE (pspec)->maximum);

      g_object_set (G_OBJECT (tool_info->tool_options),
                    "brush-size", dvalue,
                    NULL);
    }
}

void
tools_paintbrush_size_cmd_callback (GimpAction *action,
                                    GVariant   *value,
                                    gpointer    data)
{
  GimpContext          *context;
  GimpToolInfo         *tool_info;
  GimpActionSelectType  select_type;
  return_if_no_context (context, data);

  select_type = (GimpActionSelectType) g_variant_get_int32 (value);

  tool_info = gimp_context_get_tool (context);

  if (tool_info && GIMP_IS_PAINT_OPTIONS (tool_info->tool_options))
    {
      action_select_property (select_type,
                              action_data_get_display (data),
                              G_OBJECT (tool_info->tool_options),
                              "brush-size",
                              0.1, 1.0, 10.0, 1.0, FALSE);
    }
}

void
tools_paintbrush_angle_cmd_callback (GimpAction *action,
                                     GVariant   *value,
                                     gpointer    data)
{
  GimpContext          *context;
  GimpToolInfo         *tool_info;
  GimpActionSelectType  select_type;
  return_if_no_context (context, data);

  select_type = (GimpActionSelectType) g_variant_get_int32 (value);

  tool_info = gimp_context_get_tool (context);

  if (tool_info && GIMP_IS_PAINT_OPTIONS (tool_info->tool_options))
    {
      action_select_property (select_type,
                              action_data_get_display (data),
                              G_OBJECT (tool_info->tool_options),
                              "brush-angle",
                              0.1, 1.0, 15.0, 0.1, TRUE);
    }
}

void
tools_paintbrush_aspect_ratio_cmd_callback (GimpAction *action,
                                            GVariant   *value,
                                            gpointer    data)
{
  GimpContext          *context;
  GimpToolInfo         *tool_info;
  GimpActionSelectType  select_type;
  return_if_no_context (context, data);

  select_type = (GimpActionSelectType) g_variant_get_int32 (value);

  tool_info = gimp_context_get_tool (context);

  if (tool_info && GIMP_IS_PAINT_OPTIONS (tool_info->tool_options))
    {
      action_select_property (select_type,
                              action_data_get_display (data),
                              G_OBJECT (tool_info->tool_options),
                              "brush-aspect-ratio",
                              0.01, 0.1, 1.0, 0.1, TRUE);
    }
}

void
tools_paintbrush_spacing_cmd_callback (GimpAction *action,
                                       GVariant   *value,
                                       gpointer    data)
{
  GimpContext          *context;
  GimpToolInfo         *tool_info;
  GimpActionSelectType  select_type;
  return_if_no_context (context, data);

  select_type = (GimpActionSelectType) g_variant_get_int32 (value);

  tool_info = gimp_context_get_tool (context);

  if (tool_info && GIMP_IS_PAINT_OPTIONS (tool_info->tool_options))
    {
      action_select_property (select_type,
                              action_data_get_display (data),
                              G_OBJECT (tool_info->tool_options),
                              "brush-spacing",
                              0.001, 0.01, 0.1, 0.1, FALSE);
    }
}

void
tools_paintbrush_hardness_cmd_callback (GimpAction *action,
                                        GVariant   *value,
                                        gpointer    data)
{
  GimpContext          *context;
  GimpToolInfo         *tool_info;
  GimpActionSelectType  select_type;
  return_if_no_context (context, data);

  select_type = (GimpActionSelectType) g_variant_get_int32 (value);

  tool_info = gimp_context_get_tool (context);

  if (tool_info && GIMP_IS_PAINT_OPTIONS (tool_info->tool_options))
    {
      action_select_property (select_type,
                              action_data_get_display (data),
                              G_OBJECT (tool_info->tool_options),
                              "brush-hardness",
                              0.001, 0.01, 0.1, 0.1, FALSE);
    }
}

void
tools_paintbrush_force_cmd_callback (GimpAction *action,
                                     GVariant   *value,
                                     gpointer    data)
{
  GimpContext          *context;
  GimpToolInfo         *tool_info;
  GimpActionSelectType  select_type;
  return_if_no_context (context, data);

  select_type = (GimpActionSelectType) g_variant_get_int32 (value);

  tool_info = gimp_context_get_tool (context);

  if (tool_info && GIMP_IS_PAINT_OPTIONS (tool_info->tool_options))
    {
      action_select_property (select_type,
                              action_data_get_display (data),
                              G_OBJECT (tool_info->tool_options),
                              "brush-force",
                              0.001, 0.01, 0.1, 0.1, FALSE);
    }
}

void
tools_ink_blob_pixel_size_cmd_callback (GimpAction *action,
                                        GVariant   *value,
                                        gpointer    data)
{
  GimpContext          *context;
  GimpToolInfo         *tool_info;
  gdouble               dvalue;
  return_if_no_context (context, data);

  dvalue = g_variant_get_double (value);
  tool_info = gimp_context_get_tool (context);

  if (tool_info && GIMP_IS_INK_OPTIONS (tool_info->tool_options))
    {
      GParamSpec *pspec;

      pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (tool_info->tool_options),
                                            "size");
      dvalue = CLAMP (dvalue,
                      G_PARAM_SPEC_DOUBLE (pspec)->minimum,
                      G_PARAM_SPEC_DOUBLE (pspec)->maximum);

      g_object_set (G_OBJECT (tool_info->tool_options),
                    "size", dvalue,
                    NULL);
    }
}

void
tools_ink_blob_size_cmd_callback (GimpAction *action,
                                  GVariant   *value,
                                  gpointer    data)
{
  GimpContext          *context;
  GimpToolInfo         *tool_info;
  GimpActionSelectType  select_type;
  return_if_no_context (context, data);

  select_type = (GimpActionSelectType) g_variant_get_int32 (value);

  tool_info = gimp_context_get_tool (context);

  if (tool_info && GIMP_IS_INK_OPTIONS (tool_info->tool_options))
    {
      action_select_property (select_type,
                              action_data_get_display (data),
                              G_OBJECT (tool_info->tool_options),
                              "size",
                              0.1, 1.0, 10.0, 0.1, FALSE);
    }
}

void
tools_ink_blob_aspect_cmd_callback (GimpAction *action,
                                    GVariant   *value,
                                    gpointer    data)
{
  GimpContext          *context;
  GimpToolInfo         *tool_info;
  GimpActionSelectType  select_type;
  return_if_no_context (context, data);

  select_type = (GimpActionSelectType) g_variant_get_int32 (value);

  tool_info = gimp_context_get_tool (context);

  if (tool_info && GIMP_IS_INK_OPTIONS (tool_info->tool_options))
    {
      action_select_property (select_type,
                              action_data_get_display (data),
                              G_OBJECT (tool_info->tool_options),
                              "blob-aspect",
                              1.0, 0.1, 1.0, 0.1, FALSE);
    }
}

void
tools_ink_blob_angle_cmd_callback (GimpAction *action,
                                   GVariant   *value,
                                   gpointer    data)
{
  GimpContext          *context;
  GimpToolInfo         *tool_info;
  GimpActionSelectType  select_type;
  return_if_no_context (context, data);

  select_type = (GimpActionSelectType) g_variant_get_int32 (value);

  tool_info = gimp_context_get_tool (context);

  if (tool_info && GIMP_IS_INK_OPTIONS (tool_info->tool_options))
    {
      action_select_property (select_type,
                              action_data_get_display (data),
                              G_OBJECT (tool_info->tool_options),
                              "blob-angle",
                              gimp_deg_to_rad (0.1),
                              gimp_deg_to_rad (1.0),
                              gimp_deg_to_rad (15.0),
                              0.1, TRUE);
    }
}

void
tools_airbrush_rate_cmd_callback (GimpAction *action,
                                  GVariant   *value,
                                  gpointer    data)
{
  GimpContext          *context;
  GimpToolInfo         *tool_info;
  GimpActionSelectType  select_type;
  return_if_no_context (context, data);

  select_type = (GimpActionSelectType) g_variant_get_int32 (value);

  tool_info = gimp_context_get_tool (context);

  if (tool_info && GIMP_IS_AIRBRUSH_OPTIONS (tool_info->tool_options))
    {
      action_select_property (select_type,
                              action_data_get_display (data),
                              G_OBJECT (tool_info->tool_options),
                              "rate",
                              0.1, 1.0, 10.0, 0.1, FALSE);
    }
}

void
tools_airbrush_flow_cmd_callback (GimpAction *action,
                                  GVariant   *value,
                                  gpointer    data)
{
  GimpContext          *context;
  GimpToolInfo         *tool_info;
  GimpActionSelectType  select_type;
  return_if_no_context (context, data);

  select_type = (GimpActionSelectType) g_variant_get_int32 (value);

  tool_info = gimp_context_get_tool (context);

  if (tool_info && GIMP_IS_AIRBRUSH_OPTIONS (tool_info->tool_options))
    {
      action_select_property (select_type,
                              action_data_get_display (data),
                              G_OBJECT (tool_info->tool_options),
                              "flow",
                              0.1, 1.0, 10.0, 0.1, FALSE);
    }
}

void
tools_mybrush_radius_cmd_callback (GimpAction *action,
                                   GVariant   *value,
                                   gpointer    data)
{
  GimpContext          *context;
  GimpToolInfo         *tool_info;
  GimpActionSelectType  select_type;
  return_if_no_context (context, data);

  select_type = (GimpActionSelectType) g_variant_get_int32 (value);

  tool_info = gimp_context_get_tool (context);

  if (tool_info && GIMP_IS_MYBRUSH_OPTIONS (tool_info->tool_options))
    {
      action_select_property (select_type,
                              action_data_get_display (data),
                              G_OBJECT (tool_info->tool_options),
                              "radius",
                              0.1, 0.1, 0.5, 1.0, FALSE);
    }
}

void
tools_mybrush_pixel_size_cmd_callback (GimpAction *action,
                                       GVariant   *value,
                                       gpointer    data)
{
  GimpContext          *context;
  GimpToolInfo         *tool_info;
  gdouble               dvalue;
  return_if_no_context (context, data);

  dvalue = g_variant_get_double (value);
  /* Dividing by 2.0 because the parameter is the size of the brush,
   * hence the diameter and this tool uses the radius as parameter.
   * Furthermore MyPaint brush radius is stored as a natural logarithm
   * radius.
   */
  dvalue = log (dvalue / 2.0);

  tool_info = gimp_context_get_tool (context);

  if (tool_info && GIMP_IS_MYBRUSH_OPTIONS (tool_info->tool_options))
    {
      GParamSpec *pspec;

      pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (tool_info->tool_options),
                                            "radius");
      dvalue = CLAMP (dvalue,
                      G_PARAM_SPEC_DOUBLE (pspec)->minimum,
                      G_PARAM_SPEC_DOUBLE (pspec)->maximum);

      g_object_set (G_OBJECT (tool_info->tool_options),
                    "radius", dvalue,
                    NULL);
    }
}

void
tools_mybrush_hardness_cmd_callback (GimpAction *action,
                                     GVariant   *value,
                                     gpointer    data)
{
  GimpContext          *context;
  GimpToolInfo         *tool_info;
  GimpActionSelectType  select_type;
  return_if_no_context (context, data);

  select_type = (GimpActionSelectType) g_variant_get_int32 (value);

  tool_info = gimp_context_get_tool (context);

  if (tool_info && GIMP_IS_MYBRUSH_OPTIONS (tool_info->tool_options))
    {
      action_select_property (select_type,
                              action_data_get_display (data),
                              G_OBJECT (tool_info->tool_options),
                              "hardness",
                              0.001, 0.01, 0.1, 1.0, FALSE);
    }
}

void
tools_fg_select_brush_size_cmd_callback (GimpAction *action,
                                         GVariant   *value,
                                         gpointer    data)
{
  GimpContext          *context;
  GimpToolInfo         *tool_info;
  GimpActionSelectType  select_type;
  return_if_no_context (context, data);

  select_type = (GimpActionSelectType) g_variant_get_int32 (value);

  tool_info = gimp_context_get_tool (context);

  if (tool_info && GIMP_IS_FOREGROUND_SELECT_OPTIONS (tool_info->tool_options))
    {
      action_select_property (select_type,
                              action_data_get_display (data),
                              G_OBJECT (tool_info->tool_options),
                              "stroke-width",
                              1.0, 4.0, 16.0, 0.1, FALSE);
    }
}

void
tools_transform_preview_opacity_cmd_callback (GimpAction *action,
                                              GVariant   *value,
                                              gpointer    data)
{
  GimpContext          *context;
  GimpToolInfo         *tool_info;
  GimpActionSelectType  select_type;
  return_if_no_context (context, data);

  select_type = (GimpActionSelectType) g_variant_get_int32 (value);

  tool_info = gimp_context_get_tool (context);

  if (tool_info && GIMP_IS_TRANSFORM_OPTIONS (tool_info->tool_options))
    {
      action_select_property (select_type,
                              action_data_get_display (data),
                              G_OBJECT (tool_info->tool_options),
                              "preview-opacity",
                              0.01, 0.1, 0.5, 0.1, FALSE);
    }
}

void
tools_warp_effect_pixel_size_cmd_callback (GimpAction *action,
                                           GVariant   *value,
                                           gpointer    data)
{
  GimpContext          *context;
  GimpToolInfo         *tool_info;
  gdouble               dvalue;
  return_if_no_context (context, data);

  dvalue = g_variant_get_double (value);
  tool_info = gimp_context_get_tool (context);

  if (tool_info && GIMP_IS_WARP_OPTIONS (tool_info->tool_options))
    {
      GParamSpec *pspec;

      pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (tool_info->tool_options),
                                            "effect-size");
      dvalue = CLAMP (dvalue,
                      G_PARAM_SPEC_DOUBLE (pspec)->minimum,
                      G_PARAM_SPEC_DOUBLE (pspec)->maximum);

      g_object_set (G_OBJECT (tool_info->tool_options),
                    "effect-size", dvalue,
                    NULL);
    }
}

void
tools_warp_effect_size_cmd_callback (GimpAction *action,
                                     GVariant   *value,
                                     gpointer    data)
{
  GimpContext          *context;
  GimpToolInfo         *tool_info;
  GimpActionSelectType  select_type;
  return_if_no_context (context, data);

  select_type = (GimpActionSelectType) g_variant_get_int32 (value);

  tool_info = gimp_context_get_tool (context);

  if (tool_info && GIMP_IS_WARP_OPTIONS (tool_info->tool_options))
    {
      action_select_property (select_type,
                              action_data_get_display (data),
                              G_OBJECT (tool_info->tool_options),
                              "effect-size",
                              1.0, 4.0, 16.0, 0.1, FALSE);
    }
}

void
tools_warp_effect_hardness_cmd_callback (GimpAction *action,
                                         GVariant   *value,
                                         gpointer    data)
{
  GimpContext          *context;
  GimpToolInfo         *tool_info;
  GimpActionSelectType  select_type;
  return_if_no_context (context, data);

  select_type = (GimpActionSelectType) g_variant_get_int32 (value);

  tool_info = gimp_context_get_tool (context);

  if (tool_info && GIMP_IS_WARP_OPTIONS (tool_info->tool_options))
    {
      action_select_property (select_type,
                              action_data_get_display (data),
                              G_OBJECT (tool_info->tool_options),
                              "effect-hardness",
                              0.001, 0.01, 0.1, 0.1, FALSE);
    }
}

void
tools_opacity_cmd_callback (GimpAction *action,
                            GVariant   *value,
                            gpointer    data)
{
  GimpContext *context;
  GimpTool    *tool;
  return_if_no_context (context, data);

  tool = tool_manager_get_active (context->gimp);

  if (tool)
    {
      const gchar *action_desc;

      action_desc = gimp_tool_control_get_action_opacity (tool->control);

      if (action_desc)
        tools_activate_enum_action (action_desc, value);
    }
}

void
tools_size_cmd_callback (GimpAction *action,
                         GVariant   *value,
                         gpointer    data)
{
  GimpContext *context;
  GimpTool    *tool;
  return_if_no_context (context, data);

  tool = tool_manager_get_active (context->gimp);

  if (tool)
    {
      const gchar *action_desc;

      action_desc = gimp_tool_control_get_action_size (tool->control);

      if (action_desc)
        tools_activate_enum_action (action_desc, value);
    }
}

void
tools_aspect_cmd_callback (GimpAction *action,
                           GVariant   *value,
                           gpointer    data)
{
  GimpContext *context;
  GimpTool    *tool;
  return_if_no_context (context, data);

  tool = tool_manager_get_active (context->gimp);

  if (tool)
    {
      const gchar *action_desc;

      action_desc = gimp_tool_control_get_action_aspect (tool->control);

      if (action_desc)
        tools_activate_enum_action (action_desc, value);
    }
}

void
tools_angle_cmd_callback (GimpAction *action,
                          GVariant   *value,
                          gpointer    data)
{
  GimpContext *context;
  GimpTool    *tool;
  return_if_no_context (context, data);

  tool = tool_manager_get_active (context->gimp);

  if (tool)
    {
      const gchar *action_desc;

      action_desc = gimp_tool_control_get_action_angle (tool->control);

      if (action_desc)
        tools_activate_enum_action (action_desc, value);
    }
}

void
tools_spacing_cmd_callback (GimpAction *action,
                            GVariant   *value,
                            gpointer    data)
{
  GimpContext *context;
  GimpTool    *tool;
  return_if_no_context (context, data);

  tool = tool_manager_get_active (context->gimp);

  if (tool)
    {
      const gchar *action_desc;

      action_desc = gimp_tool_control_get_action_spacing (tool->control);

      if (action_desc)
        tools_activate_enum_action (action_desc, value);
    }
}

void
tools_hardness_cmd_callback (GimpAction *action,
                             GVariant   *value,
                             gpointer    data)
{
  GimpContext *context;
  GimpTool    *tool;
  return_if_no_context (context, data);

  tool = tool_manager_get_active (context->gimp);

  if (tool)
    {
      const gchar *action_desc;

      action_desc = gimp_tool_control_get_action_hardness (tool->control);

      if (action_desc)
        tools_activate_enum_action (action_desc, value);
    }
}

void
tools_force_cmd_callback (GimpAction *action,
                          GVariant   *value,
                          gpointer    data)
{
  GimpContext *context;
  GimpTool    *tool;
  return_if_no_context (context, data);

  tool = tool_manager_get_active (context->gimp);

  if (tool)
    {
      const gchar *action_desc;

      action_desc = gimp_tool_control_get_action_force (tool->control);

      if (action_desc)
        tools_activate_enum_action (action_desc, value);
    }
}

void
tools_paint_select_pixel_size_cmd_callback (GimpAction *action,
                                            GVariant   *value,
                                            gpointer    data)
{
  GimpContext          *context;
  GimpToolInfo         *tool_info;
  gdouble               dvalue;
  return_if_no_context (context, data);

  dvalue = g_variant_get_double (value);
  tool_info = gimp_context_get_tool (context);

  if (tool_info && GIMP_IS_PAINT_SELECT_OPTIONS (tool_info->tool_options))
    {
      GParamSpec *pspec;

      pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (tool_info->tool_options),
                                            "stroke-width");
      dvalue = CLAMP (dvalue,
                      G_PARAM_SPEC_INT (pspec)->minimum,
                      G_PARAM_SPEC_INT (pspec)->maximum);

      g_object_set (G_OBJECT (tool_info->tool_options),
                    "stroke-width", (gint) dvalue,
                    NULL);
    }
}

void
tools_object_1_cmd_callback (GimpAction *action,
                             GVariant   *value,
                             gpointer    data)
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
tools_object_2_cmd_callback (GimpAction *action,
                             GVariant   *value,
                             gpointer    data)
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
                            GVariant    *value)
{
  if (action_desc)
    {
      GList      *managers;
      GimpAction *action;

      managers = gimp_ui_managers_from_name ("<Image>");

      action = gimp_ui_manager_find_action (managers->data,
                                            NULL, action_desc);

      if (GIMP_IS_ENUM_ACTION (action) &&
          GIMP_ENUM_ACTION (action)->value_variable)
        {
          gimp_action_emit_activate (GIMP_ACTION (action), value);
        }
    }
}
