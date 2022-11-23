/* LIGMA - The GNU Image Manipulation Program
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

#ifndef  __LIGMA_COLOR_TOOL_H__
#define  __LIGMA_COLOR_TOOL_H__


#include "ligmadrawtool.h"


#define LIGMA_TYPE_COLOR_TOOL            (ligma_color_tool_get_type ())
#define LIGMA_COLOR_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_COLOR_TOOL, LigmaColorTool))
#define LIGMA_COLOR_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_COLOR_TOOL, LigmaColorToolClass))
#define LIGMA_IS_COLOR_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_COLOR_TOOL))
#define LIGMA_IS_COLOR_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_COLOR_TOOL))
#define LIGMA_COLOR_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_COLOR_TOOL, LigmaColorToolClass))

#define LIGMA_COLOR_TOOL_GET_OPTIONS(t)  (LIGMA_COLOR_OPTIONS (ligma_tool_get_options (LIGMA_TOOL (t))))


typedef struct _LigmaColorToolClass LigmaColorToolClass;

struct _LigmaColorTool
{
  LigmaDrawTool         parent_instance;

  gboolean             enabled;
  LigmaColorOptions    *options;
  gboolean             saved_snap_to;

  LigmaColorPickTarget  pick_target;

  gboolean             can_pick;
  gint                 center_x;
  gint                 center_y;
  LigmaSamplePoint     *sample_point;
};

struct _LigmaColorToolClass
{
  LigmaDrawToolClass  parent_class;

  /*  virtual functions  */
  gboolean (* can_pick) (LigmaColorTool      *tool,
                         const LigmaCoords   *coords,
                         LigmaDisplay        *display);
  gboolean (* pick)     (LigmaColorTool      *tool,
                         const LigmaCoords   *coords,
                         LigmaDisplay        *display,
                         const Babl        **sample_format,
                         gpointer            pixel,
                         LigmaRGB            *color);

  /*  signals  */
  void     (* picked)   (LigmaColorTool      *tool,
                         const LigmaCoords   *coords,
                         LigmaDisplay        *display,
                         LigmaColorPickState  pick_state,
                         const Babl         *sample_format,
                         gpointer            pixel,
                         const LigmaRGB      *color);
};


GType      ligma_color_tool_get_type   (void) G_GNUC_CONST;

void       ligma_color_tool_enable     (LigmaColorTool    *color_tool,
                                       LigmaColorOptions *options);
void       ligma_color_tool_disable    (LigmaColorTool    *color_tool);
gboolean   ligma_color_tool_is_enabled (LigmaColorTool    *color_tool);


#endif  /*  __LIGMA_COLOR_TOOL_H__  */
