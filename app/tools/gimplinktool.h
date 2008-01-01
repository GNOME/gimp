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

#ifndef __GIMP_LINK_TOOL_H__
#define __GIMP_LINK_TOOL_H__


#include "gimpdrawtool.h"

/*  tool function/operation/state/mode  */
typedef enum
{
  LINK_TOOL_IDLE,
  LINK_TOOL_PICK_LAYER,
  LINK_TOOL_ADD_LAYER,
  LINK_TOOL_PICK_PATH,
  LINK_TOOL_ADD_PATH,
  LINK_TOOL_DRAG_BOX
} GimpLinkToolFunction;


#define GIMP_TYPE_LINK_TOOL            (gimp_link_tool_get_type ())
#define GIMP_LINK_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_LINK_TOOL, GimpLinkTool))
#define GIMP_LINK_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_LINK_TOOL, GimpLinkToolClass))
#define GIMP_IS_LINK_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_LINK_TOOL))
#define GIMP_IS_LINK_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_LINK_TOOL))
#define GIMP_LINK_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_LINK_TOOL, GimpLinkToolClass))

typedef struct _GimpLinkTool      GimpLinkTool;
typedef struct _GimpLinkToolClass GimpLinkToolClass;

struct _GimpLinkTool
{
  GimpDrawTool           parent_instance;

  GimpLinkToolFunction  function;

  gint                   x0, y0, x1, y1;   /* rubber-band rectangle */
};

struct _GimpLinkToolClass
{
  GimpDrawToolClass parent_class;
};


void    gimp_link_tool_register (GimpToolRegisterCallback  callback,
                                 gpointer                  data);

GType   gimp_link_tool_get_type (void) G_GNUC_CONST;


#endif  /*  __GIMP_LINK_TOOL_H__  */
