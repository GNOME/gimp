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


/*  enums  */

typedef enum /*< pdb-skip >*/
{
  SELECTION_ADD       = GIMP_CHANNEL_OP_ADD,
  SELECTION_SUBTRACT  = GIMP_CHANNEL_OP_SUBTRACT,
  SELECTION_REPLACE   = GIMP_CHANNEL_OP_REPLACE,
  SELECTION_INTERSECT = GIMP_CHANNEL_OP_INTERSECT,
  SELECTION_MOVE_MASK,
  SELECTION_MOVE,
  SELECTION_ANCHOR
} SelectOps;

/*  The possible states for tools  */
typedef enum /*< pdb-skip >*/
{
  INACTIVE,
  ACTIVE
} GimpToolState;

/*  Tool control actions  */
typedef enum /*< pdb-skip >*/
{
  PAUSE,
  RESUME,
  HALT
} GimpToolAction;

/*  Motion event report modes  */
typedef enum /*< pdb-skip >*/
{
  GIMP_MOTION_MODE_EXACT,
  GIMP_MOTION_MODE_HINT,
  GIMP_MOTION_MODE_COMPRESS
} GimpMotionMode;

/* possible transform functions */
typedef enum /*< pdb-skip >*/
{
  TRANSFORM_CREATING,
  TRANSFORM_HANDLE_1,
  TRANSFORM_HANDLE_2,
  TRANSFORM_HANDLE_3,
  TRANSFORM_HANDLE_4,
  TRANSFORM_HANDLE_CENTER
} TransformAction;

/* the different states that the transformation function can be called with */
typedef enum /*< pdb-skip >*/
{
  TRANSFORM_INIT,
  TRANSFORM_MOTION,
  TRANSFORM_RECALC,
  TRANSFORM_FINISH
} TransformState;


#endif /* __TOOLS_TYPES_H__ */
