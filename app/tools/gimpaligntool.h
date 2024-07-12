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

#ifndef __GIMP_ALIGN_TOOL_H__
#define __GIMP_ALIGN_TOOL_H__


#include "gimpdrawtool.h"


/*  tool function/operation/state/mode  */
typedef enum
{
  ALIGN_TOOL_NO_ACTION,

  ALIGN_TOOL_REF_IDLE,
  ALIGN_TOOL_REF_PICK_LAYER,
  ALIGN_TOOL_REF_PICK_GUIDE,
  ALIGN_TOOL_REF_PICK_PATH,
  ALIGN_TOOL_REF_DRAG_BOX,

  ALIGN_TOOL_ALIGN_IDLE,
  ALIGN_TOOL_ALIGN_PICK_GUIDE,
  ALIGN_TOOL_ALIGN_ADD_GUIDE,
} GimpAlignToolFunction;


#define GIMP_TYPE_ALIGN_TOOL            (gimp_align_tool_get_type ())
#define GIMP_ALIGN_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_ALIGN_TOOL, GimpAlignTool))
#define GIMP_ALIGN_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_ALIGN_TOOL, GimpAlignToolClass))
#define GIMP_IS_ALIGN_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_ALIGN_TOOL))
#define GIMP_IS_ALIGN_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_ALIGN_TOOL))
#define GIMP_ALIGN_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_ALIGN_TOOL, GimpAlignToolClass))

#define GIMP_ALIGN_TOOL_GET_OPTIONS(t)  (GIMP_ALIGN_OPTIONS (gimp_tool_get_options (GIMP_TOOL (t))))


typedef struct _GimpAlignTool      GimpAlignTool;
typedef struct _GimpAlignToolClass GimpAlignToolClass;

struct _GimpAlignTool
{
  GimpDrawTool           parent_instance;

  GimpAlignToolFunction  function;
  GList                 *selected_objects;

  gint                   x1, y1, x2, y2;   /* rubber-band rectangle */

  gboolean               set_reference;
};

struct _GimpAlignToolClass
{
  GimpDrawToolClass parent_class;
};


void    gimp_align_tool_register (GimpToolRegisterCallback  callback,
                                  gpointer                  data);

GType   gimp_align_tool_get_type (void) G_GNUC_CONST;


#endif  /*  __GIMP_ALIGN_TOOL_H__  */
