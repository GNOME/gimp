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

#ifndef __LIGMA_SOURCE_TOOL_H__
#define __LIGMA_SOURCE_TOOL_H__


#include "ligmabrushtool.h"


#define LIGMA_TYPE_SOURCE_TOOL            (ligma_source_tool_get_type ())
#define LIGMA_SOURCE_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_SOURCE_TOOL, LigmaSourceTool))
#define LIGMA_SOURCE_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_SOURCE_TOOL, LigmaSourceToolClass))
#define LIGMA_IS_SOURCE_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_SOURCE_TOOL))
#define LIGMA_IS_SOURCE_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_SOURCE_TOOL))
#define LIGMA_SOURCE_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_SOURCE_TOOL, LigmaSourceToolClass))

#define LIGMA_SOURCE_TOOL_GET_OPTIONS(t)  (LIGMA_SOURCE_OPTIONS (ligma_tool_get_options (LIGMA_TOOL (t))))


typedef struct _LigmaSourceTool      LigmaSourceTool;
typedef struct _LigmaSourceToolClass LigmaSourceToolClass;

struct _LigmaSourceTool
{
  LigmaBrushTool        parent_instance;

  LigmaDisplay         *src_display;
  gint                 src_x;
  gint                 src_y;

  gboolean             show_source_outline;
  LigmaCursorPrecision  saved_precision;

  LigmaCanvasItem      *src_handle;
  LigmaCanvasItem      *src_outline;

  const gchar         *status_paint;
  const gchar         *status_set_source;
  const gchar         *status_set_source_ctrl;
};

struct _LigmaSourceToolClass
{
  LigmaBrushToolClass  parent_class;
};


GType   ligma_source_tool_get_type (void) G_GNUC_CONST;


#endif  /*  __LIGMA_SOURCE_TOOL_H__  */
