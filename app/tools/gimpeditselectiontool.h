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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include "gimpdrawtool.h"


#define GIMP_TYPE_EDIT_SELECTION_TOOL            (gimp_edit_selection_tool_get_type ())
#define GIMP_EDIT_SELECTION_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_EDIT_SELECTION_TOOL, GimpEditSelectionTool))
#define GIMP_EDIT_SELECTION_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_EDIT_SELECTION_TOOL, GimpEditSelectionToolClass))
#define GIMP_IS_EDIT_SELECTION_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_EDIT_SELECTION_TOOL))
#define GIMP_IS_EDIT_SELECTION_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_EDIT_SELECTION_TOOL))


GType      gimp_edit_selection_tool_get_type  (void) G_GNUC_CONST;

void       gimp_edit_selection_tool_start     (GimpTool          *parent_tool,
                                               GimpDisplay       *display,
                                               const GimpCoords  *coords,
                                               GimpTranslateMode  edit_mode,
                                               gboolean           propagate_release);

gboolean   gimp_edit_selection_tool_key_press (GimpTool          *tool,
                                               GdkEventKey       *kevent,
                                               GimpDisplay       *display);
gboolean   gimp_edit_selection_tool_translate (GimpTool          *tool,
                                               GdkEventKey       *kevent,
                                               GimpTransformType  translate_type,
                                               GimpDisplay       *display,
                                               GtkWidget        **type_box);
