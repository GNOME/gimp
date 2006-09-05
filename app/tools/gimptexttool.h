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

#ifndef __GIMP_TEXT_TOOL_H__
#define __GIMP_TEXT_TOOL_H__


#include "gimptool.h"


#define GIMP_TYPE_TEXT_TOOL            (gimp_text_tool_get_type ())
#define GIMP_TEXT_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_TEXT_TOOL, GimpTextTool))
#define GIMP_IS_TEXT_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_TEXT_TOOL))
#define GIMP_TEXT_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_TEXT_TOOL, GimpTextToolClass))
#define GIMP_IS_TEXT_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_TEXT_TOOL))

#define GIMP_TEXT_TOOL_GET_OPTIONS(t)  (GIMP_TEXT_OPTIONS (gimp_tool_get_options (GIMP_TOOL (t))))


typedef struct _GimpTextTool       GimpTextTool;
typedef struct _GimpTextToolClass  GimpTextToolClass;

struct _GimpTextTool
{
  GimpTool       parent_instance;

  GimpText      *proxy;
  GList         *pending;
  guint          idle_id;

  gint           x1, y1;
  gint           x2, y2;

  GimpText      *text;
  GimpTextLayer *layer;
  GimpImage     *image;

  GtkWidget     *editor;
  GtkWidget     *confirm_dialog;
};

struct _GimpTextToolClass
{
  GimpToolClass  parent_class;
};


void    gimp_text_tool_register  (GimpToolRegisterCallback  callback,
                                  gpointer                  data);

GType   gimp_text_tool_get_type  (void) G_GNUC_CONST;

void    gimp_text_tool_set_layer (GimpTextTool *text_tool,
                                  GimpLayer    *layer);


#endif /* __GIMP_TEXT_TOOL_H__ */
