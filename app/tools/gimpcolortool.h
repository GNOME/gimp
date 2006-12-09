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

#ifndef  __GIMP_COLOR_TOOL_H__
#define  __GIMP_COLOR_TOOL_H__


#include "gimpdrawtool.h"


#define GIMP_TYPE_COLOR_TOOL            (gimp_color_tool_get_type ())
#define GIMP_COLOR_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_COLOR_TOOL, GimpColorTool))
#define GIMP_COLOR_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_COLOR_TOOL, GimpColorToolClass))
#define GIMP_IS_COLOR_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_COLOR_TOOL))
#define GIMP_IS_COLOR_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_COLOR_TOOL))
#define GIMP_COLOR_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_COLOR_TOOL, GimpColorToolClass))

#define GIMP_COLOR_TOOL_GET_OPTIONS(t)  (GIMP_COLOR_OPTIONS (gimp_tool_get_options (GIMP_TOOL (t))))


typedef struct _GimpColorToolClass GimpColorToolClass;

struct _GimpColorTool
{
  GimpDrawTool       parent_instance;

  gboolean           enabled;
  gint               center_x;
  gint               center_y;
  GimpColorPickMode  pick_mode;

  GimpColorOptions  *options;

  GimpSamplePoint   *sample_point;
  gboolean           moving_sample_point;
  gint               sample_point_x;
  gint               sample_point_y;
};

struct _GimpColorToolClass
{
  GimpDrawToolClass  parent_class;

  /*  virtual functions  */
  gboolean (* pick)   (GimpColorTool      *tool,
                       gint                x,
                       gint                y,
                       GimpImageType      *sample_type,
                       GimpRGB            *color,
                       gint               *color_index);

  /*  signals  */
  void     (* picked) (GimpColorTool      *tool,
                       GimpColorPickState  pick_state,
                       GimpImageType       sample_type,
                       GimpRGB            *color,
                       gint                color_index);
};


GType      gimp_color_tool_get_type           (void) G_GNUC_CONST;

void       gimp_color_tool_enable             (GimpColorTool    *color_tool,
                                               GimpColorOptions *options);
void       gimp_color_tool_disable            (GimpColorTool    *color_tool);
gboolean   gimp_color_tool_is_enabled         (GimpColorTool    *color_tool);

void       gimp_color_tool_start_sample_point (GimpTool         *tool,
                                               GimpDisplay      *display);


#endif  /*  __GIMP_COLOR_TOOL_H__  */
