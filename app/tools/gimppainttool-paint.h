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

#ifndef __LIGMA_PAINT_TOOL_PAINT_H__
#define __LIGMA_PAINT_TOOL_PAINT_H__


typedef void (* LigmaPaintToolPaintFunc) (LigmaPaintTool *tool,
                                         gpointer       data);



gboolean   ligma_paint_tool_paint_start     (LigmaPaintTool           *tool,
                                            LigmaDisplay             *display,
                                            const LigmaCoords        *coords,
                                            guint32                  time,
                                            gboolean                 constrain,
                                            GError                 **error);
void       ligma_paint_tool_paint_end       (LigmaPaintTool           *tool,
                                            guint32                  time,
                                            gboolean                 cancel);

gboolean   ligma_paint_tool_paint_is_active (LigmaPaintTool           *tool);

void       ligma_paint_tool_paint_push      (LigmaPaintTool           *tool,
                                            LigmaPaintToolPaintFunc   func,
                                            gpointer                 data);

void       ligma_paint_tool_paint_motion    (LigmaPaintTool           *tool,
                                            const LigmaCoords        *coords,
                                            guint32                  time);


#endif  /*  __LIGMA_PAINT_TOOL_PAINT_H__  */
