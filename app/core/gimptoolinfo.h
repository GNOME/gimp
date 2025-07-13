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

#pragma once

#include "gimptoolitem.h"


#define GIMP_TYPE_TOOL_INFO            (gimp_tool_info_get_type ())
#define GIMP_TOOL_INFO(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_TOOL_INFO, GimpToolInfo))
#define GIMP_TOOL_INFO_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_TOOL_INFO, GimpToolInfoClass))
#define GIMP_IS_TOOL_INFO(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_TOOL_INFO))
#define GIMP_IS_TOOL_INFO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_TOOL_INFO))
#define GIMP_TOOL_INFO_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_TOOL_INFO, GimpToolInfoClass))


typedef struct _GimpToolInfoClass GimpToolInfoClass;

struct _GimpToolInfo
{
  GimpToolItem         parent_instance;

  Gimp                *gimp;

  GType                tool_type;
  GType                tool_options_type;
  GimpContextPropMask  context_props;

  gchar               *label;
  gchar               *tooltip;

  gchar               *menu_label;
  gchar               *menu_accel;

  gchar               *help_domain;
  gchar               *help_id;

  gboolean             hidden;
  gboolean             experimental;

  GimpToolOptions     *tool_options;
  GimpPaintInfo       *paint_info;

  GimpContainer       *presets;
};

struct _GimpToolInfoClass
{
  GimpToolItemClass  parent_class;
};


GType          gimp_tool_info_get_type         (void) G_GNUC_CONST;

GimpToolInfo * gimp_tool_info_new              (Gimp                *gimp,
                                                GType                tool_type,
                                                GType                tool_options_type,
                                                GimpContextPropMask  context_props,
                                                const gchar         *identifier,
                                                const gchar         *label,
                                                const gchar         *tooltip,
                                                const gchar         *menu_label,
                                                const gchar         *menu_accel,
                                                const gchar         *help_domain,
                                                const gchar         *help_id,
                                                const gchar         *paint_core_name,
                                                const gchar         *icon_name);

void           gimp_tool_info_set_standard     (Gimp                *gimp,
                                                GimpToolInfo        *tool_info);
GimpToolInfo * gimp_tool_info_get_standard     (Gimp                *gimp);

gchar        * gimp_tool_info_get_action_name (GimpToolInfo         *tool_info);

GFile        * gimp_tool_info_get_options_file (GimpToolInfo        *tool_info,
                                                const gchar         *suffix);
