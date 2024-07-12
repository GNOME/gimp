/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Vector tool
 * Copyright (C) 2003 Simon Budig  <simon@gimp.org>
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

#ifndef __GIMP_VECTOR_TOOL_H__
#define __GIMP_VECTOR_TOOL_H__


#include "gimpdrawtool.h"


#define GIMP_TYPE_VECTOR_TOOL            (gimp_vector_tool_get_type ())
#define GIMP_VECTOR_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_VECTOR_TOOL, GimpVectorTool))
#define GIMP_VECTOR_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_VECTOR_TOOL, GimpVectorToolClass))
#define GIMP_IS_VECTOR_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_VECTOR_TOOL))
#define GIMP_IS_VECTOR_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_VECTOR_TOOL))
#define GIMP_VECTOR_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_VECTOR_TOOL, GimpVectorToolClass))

#define GIMP_VECTOR_TOOL_GET_OPTIONS(t)  (GIMP_VECTOR_OPTIONS (gimp_tool_get_options (GIMP_TOOL (t))))


typedef struct _GimpVectorTool      GimpVectorTool;
typedef struct _GimpVectorToolClass GimpVectorToolClass;

struct _GimpVectorTool
{
  GimpDrawTool    parent_instance;

  GimpPath       *vectors;        /* the current Path data           */
  GimpVectorMode  saved_mode;     /* used by modifier_key()            */

  GimpToolWidget *widget;
  GimpToolWidget *grab_widget;
};

struct _GimpVectorToolClass
{
  GimpDrawToolClass  parent_class;
};


void    gimp_vector_tool_register    (GimpToolRegisterCallback  callback,
                                      gpointer                  data);

GType   gimp_vector_tool_get_type    (void) G_GNUC_CONST;

void    gimp_vector_tool_set_vectors (GimpVectorTool           *vector_tool,
                                      GimpPath                 *vectors);

#endif  /*  __GIMP_VECTOR_TOOL_H__  */
