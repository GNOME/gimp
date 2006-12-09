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

#ifndef __GIMP_COLORIZE_TOOL_H__
#define __GIMP_COLORIZE_TOOL_H__


#include "gimpimagemaptool.h"


#define GIMP_TYPE_COLORIZE_TOOL            (gimp_colorize_tool_get_type ())
#define GIMP_COLORIZE_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_COLORIZE_TOOL, GimpColorizeTool))
#define GIMP_COLORIZE_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_COLORIZE_TOOL, GimpColorizeToolClass))
#define GIMP_IS_COLORIZE_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_COLORIZE_TOOL))
#define GIMP_IS_COLORIZE_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_COLORIZE_TOOL))
#define GIMP_COLORIZE_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_COLORIZE_TOOL, GimpColorizeToolClass))


typedef struct _GimpColorizeTool      GimpColorizeTool;
typedef struct _GimpColorizeToolClass GimpColorizeToolClass;

struct _GimpColorizeTool
{
  GimpImageMapTool  parent_instance;

  Colorize         *colorize;

  /*  dialog  */
  GtkAdjustment    *hue_data;
  GtkAdjustment    *saturation_data;
  GtkAdjustment    *lightness_data;
};

struct _GimpColorizeToolClass
{
  GimpImageMapToolClass  parent_class;
};


void    gimp_colorize_tool_register (GimpToolRegisterCallback  callback,
                                     gpointer                  data);

GType   gimp_colorize_tool_get_type (void) G_GNUC_CONST;


#endif  /*  __GIMP_COLORIZE_TOOL_H__  */
