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

#ifndef __GIMP_HEAL_TOOL_H__
#define __GIMP_HEAL_TOOL_H__


#include "gimpbrushtool.h"


#define GIMP_TYPE_HEAL_TOOL            (gimp_heal_tool_get_type ())
#define GIMP_HEAL_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_HEAL_TOOL, GimpHealTool))
#define GIMP_HEAL_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_HEAL_TOOL, GimpHealToolClass))
#define GIMP_IS_HEAL_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_HEAL_TOOL))
#define GIMP_IS_HEAL_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_HEAL_TOOL))
#define GIMP_HEAL_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_HEAL_TOOL, GimpHealToolClass))


typedef struct _GimpHealTool      GimpHealTool;
typedef struct _GimpHealToolClass GimpHealToolClass;


struct _GimpHealTool
{
  GimpBrushTool  parent_instance;

  GimpDisplay   *src_display;   /* Detail about the source location to paint from */
  gint           src_x;
  gint           src_y;
};

struct _GimpHealToolClass
{
  GimpBrushToolClass parent_class;
};


void    gimp_heal_tool_register (GimpToolRegisterCallback  callback,
                                 gpointer                  data);

GType   gimp_heal_tool_get_type (void) G_GNUC_CONST;


#endif  /*  __GIMP_HEAL_TOOL_H__  */
