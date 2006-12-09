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

#ifndef __GIMP_FREE_SELECT_TOOL_H__
#define __GIMP_FREE_SELECT_TOOL_H__


#include "gimpselectiontool.h"


#define GIMP_TYPE_FREE_SELECT_TOOL            (gimp_free_select_tool_get_type ())
#define GIMP_FREE_SELECT_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_FREE_SELECT_TOOL, GimpFreeSelectTool))
#define GIMP_FREE_SELECT_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_FREE_SELECT_TOOL, GimpFreeSelectToolClass))
#define GIMP_IS_FREE_SELECT_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_FREE_SELECT_TOOL))
#define GIMP_IS_FREE_SELECT_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_FREE_SELECT_TOOL))
#define GIMP_FREE_SELECT_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_FREE_SELECT_TOOL, GimpFreeSelectToolClass))


typedef struct _GimpFreeSelectTool      GimpFreeSelectTool;
typedef struct _GimpFreeSelectToolClass GimpFreeSelectToolClass;

struct _GimpFreeSelectTool
{
  GimpSelectionTool  parent_instance;

  GimpCoords         last_coords;

  GimpVector2       *points;
  gint               num_points;
  gint               max_segs;
};

struct _GimpFreeSelectToolClass
{
  GimpSelectionToolClass  parent_class;

  /*  virtual function  */

  void (* select) (GimpFreeSelectTool *free_select_tool,
                   GimpDisplay        *display);
};


void    gimp_free_select_tool_register (GimpToolRegisterCallback  callback,
                                        gpointer                  data);

GType   gimp_free_select_tool_get_type (void) G_GNUC_CONST;

void    gimp_free_select_tool_select   (GimpFreeSelectTool       *free_sel,
                                        GimpDisplay              *display);


#endif  /*  __GIMP_FREE_SELECT_TOOL_H__  */
