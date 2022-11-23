/* LIGMA - The GNU Image Manipulation Program
 *
 * ligmawarptool.h
 * Copyright (C) 2011 Michael Muré <batolettre@gmail.com>
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

#ifndef __LIGMA_WARP_TOOL_H__
#define __LIGMA_WARP_TOOL_H__


#include "ligmadrawtool.h"


#define LIGMA_TYPE_WARP_TOOL            (ligma_warp_tool_get_type ())
#define LIGMA_WARP_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_WARP_TOOL, LigmaWarpTool))
#define LIGMA_WARP_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_WARP_TOOL, LigmaWarpToolClass))
#define LIGMA_IS_WARP_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_WARP_TOOL))
#define LIGMA_IS_WARP_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_WARP_TOOL))
#define LIGMA_WARP_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_WARP_TOOL, LigmaWarpToolClass))

#define LIGMA_WARP_TOOL_GET_OPTIONS(t)  (LIGMA_WARP_OPTIONS (ligma_tool_get_options (LIGMA_TOOL (t))))


typedef struct _LigmaWarpTool      LigmaWarpTool;
typedef struct _LigmaWarpToolClass LigmaWarpToolClass;

struct _LigmaWarpTool
{
  LigmaDrawTool        parent_instance;

  gboolean            show_cursor;
  gboolean            draw_brush;
  gboolean            snap_brush;

  LigmaVector2         cursor_pos;    /* Hold the cursor position */

  GeglBuffer         *coords_buffer; /* Buffer where coordinates are stored */

  GeglNode           *graph;         /* Top level GeglNode */
  GeglNode           *render_node;   /* Node to render the transformation */

  GeglPath           *current_stroke;
  guint               stroke_timer;

  LigmaVector2         last_pos;
  gdouble             total_dist;

  LigmaDrawableFilter *filter;

  GList              *redo_stack;
};

struct _LigmaWarpToolClass
{
  LigmaDrawToolClass parent_class;
};


void    ligma_warp_tool_register (LigmaToolRegisterCallback  callback,
                                 gpointer                  data);

GType   ligma_warp_tool_get_type (void) G_GNUC_CONST;


#endif  /*  __LIGMA_WARP_TOOL_H__  */
