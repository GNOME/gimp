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

#ifndef __TOOLS_TYPES_H__
#define __TOOLS_TYPES_H__


#include "paint/paint-types.h"
#include "display/display-types.h"

#include "tools/tools-enums.h"


/*  tools  */

typedef struct _GimpTool            GimpTool;
typedef struct _GimpToolModule      GimpToolModule;
typedef struct _GimpPaintTool       GimpPaintTool;
typedef struct _GimpDrawTool        GimpDrawTool;
typedef struct _GimpPathTool        GimpPathTool;
typedef struct _GimpTransformTool   GimpTransformTool;

typedef struct _GimpBezierSelectPoint  GimpBezierSelectPoint;
typedef struct _GimpBezierSelectTool   GimpBezierSelectTool;


/*  stuff  */

typedef struct _SelectionOptions    SelectionOptions;


/*  functions  */

typedef GimpToolOptions * (* GimpToolOptionsNewFunc) (GimpToolInfo  *tool_info);

typedef void (* GimpToolOptionsResetFunc) (GimpToolOptions          *tool_options);

typedef void (* GimpToolRegisterCallback) (Gimp                     *gimp,
                                           GType                     tool_type,
                                           GimpToolOptionsNewFunc    options_new_func,
                                           gboolean                  tool_context,
                                           const gchar              *identifier,
                                           const gchar              *blurb,
                                           const gchar              *help,
                                           const gchar              *menu_path,
                                           const gchar              *menu_accel,
                                           const gchar              *help_domain,
                                           const gchar              *help_data,
                                           const gchar              *stock_id);

typedef void (* GimpToolRegisterFunc)     (Gimp                     *gimp,
                                           GimpToolRegisterCallback  callback);


#endif /* __TOOLS_TYPES_H__ */
