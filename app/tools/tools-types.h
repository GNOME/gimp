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

#ifndef __TOOLS_TYPES_H__
#define __TOOLS_TYPES_H__

#include "paint/paint-types.h"
#include "display/display-types.h"

#include "tools/tools-enums.h"


G_BEGIN_DECLS


typedef struct _GimpTool                     GimpTool;
typedef struct _GimpToolControl              GimpToolControl;

typedef struct _GimpBrushTool                GimpBrushTool;
typedef struct _GimpColorTool                GimpColorTool;
typedef struct _GimpDrawTool                 GimpDrawTool;
typedef struct _GimpFilterTool               GimpFilterTool;
typedef struct _GimpGenericTransformTool     GimpGenericTransformTool;
typedef struct _GimpPaintTool                GimpPaintTool;
typedef struct _GimpTransformGridTool        GimpTransformGridTool;
typedef struct _GimpTransformTool            GimpTransformTool;

typedef struct _GimpColorOptions             GimpColorOptions;
typedef struct _GimpFilterOptions            GimpFilterOptions;


/*  functions  */

typedef void (* GimpToolRegisterCallback) (GType                     tool_type,
                                           GType                     tool_option_type,
                                           GimpToolOptionsGUIFunc    options_gui_func,
                                           GimpContextPropMask       context_props,
                                           const gchar              *identifier,
                                           const gchar              *label,
                                           const gchar              *tooltip,
                                           const gchar              *menu_path,
                                           const gchar              *menu_accel,
                                           const gchar              *help_domain,
                                           const gchar              *help_data,
                                           const gchar              *icon_name,
                                           gpointer                  register_data);

typedef void (* GimpToolRegisterFunc)     (GimpToolRegisterCallback  callback,
                                           gpointer                  register_data);


G_END_DECLS

#endif /* __TOOLS_TYPES_H__ */
