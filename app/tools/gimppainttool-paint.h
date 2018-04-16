/* GIMP - The GNU Image Manipulation Program
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_PAINT_TOOL_PAINT_H__
#define __GIMP_PAINT_TOOL_PAINT_H__


gboolean   gimp_paint_tool_paint_start            (GimpPaintTool     *tool,
                                                   GimpDisplay       *display,
                                                   const GimpCoords  *coords,
                                                   guint32            time,
                                                   gboolean           constrain,
                                                   GError           **error);
void       gimp_paint_tool_paint_end              (GimpPaintTool     *tool,
                                                   guint32            time,
                                                   gboolean           cancel);

gboolean   gimp_paint_tool_paint_is_active        (GimpPaintTool     *tool);

void       gimp_paint_tool_paint_motion           (GimpPaintTool    *tool,
                                                   const GimpCoords *coords,
                                                   guint32           time);

void       gimp_paint_tool_paint_core_paint       (GimpPaintTool     *tool,
                                                   GimpPaintState     state,
                                                   guint32            time);
void       gimp_paint_tool_paint_core_interpolate (GimpPaintTool     *tool,
                                                   const GimpCoords  *coords,
                                                   guint32            time);


#endif  /*  __GIMP_PAINT_TOOL_PAINT_H__  */
