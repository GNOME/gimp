/* GIMP - The GNU Image Manipulation Program
 *
 * gimpcagetool.h
 * Copyright (C) 2010 Michael Muré <batolettre@gmail.com>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_CAGE_TOOL_H__
#define __GIMP_CAGE_TOOL_H__

#include "gimpdrawtool.h"
#include "libgimpmath/gimpvector.h"
#include "gegl/gimpcageconfig.h"


#define GIMP_TYPE_CAGE_TOOL            (gimp_cage_tool_get_type ())
#define GIMP_CAGE_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_CAGE_TOOL, GimpCageTool))
#define GIMP_CAGE_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_CAGE_TOOL, GimpCageToolClass))
#define GIMP_IS_CAGE_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_CAGE_TOOL))
#define GIMP_IS_CAGE_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_CAGE_TOOL))
#define GIMP_CAGE_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_CAGE_TOOL, GimpCageToolClass))

#define GIMP_CAGE_TOOL_GET_OPTIONS(t)  (GIMP_CAGE_OPTIONS (gimp_tool_get_options (GIMP_TOOL (t))))

typedef struct _GimpCageToolClass GimpCageToolClass;
typedef struct _GimpCageTool GimpCageTool;

struct _GimpCageTool
{
  GimpDrawTool          parent_instance;
  GimpCageConfig       *config;
  
  GimpVector2           cursor_position;
  gint                  handle_moved;
  gboolean              cage_complete;
  
  GeglBuffer           *coef;
};


struct _GimpCageToolClass
{
  GimpDrawToolClass     parent_class;
};

void    gimp_cage_tool_register (GimpToolRegisterCallback  callback,
                                   gpointer                  data);

GType   gimp_cage_tool_get_type (void) G_GNUC_CONST;

#endif  /*  __GIMP_CAGE_TOOL_H__  */
