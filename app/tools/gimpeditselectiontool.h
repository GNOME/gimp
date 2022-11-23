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

#ifndef __LIGMA_EDIT_SELECTION_TOOL_H__
#define __LIGMA_EDIT_SELECTION_TOOL_H__


#include "ligmadrawtool.h"


#define LIGMA_TYPE_EDIT_SELECTION_TOOL            (ligma_edit_selection_tool_get_type ())
#define LIGMA_EDIT_SELECTION_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_EDIT_SELECTION_TOOL, LigmaEditSelectionTool))
#define LIGMA_EDIT_SELECTION_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_EDIT_SELECTION_TOOL, LigmaEditSelectionToolClass))
#define LIGMA_IS_EDIT_SELECTION_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_EDIT_SELECTION_TOOL))
#define LIGMA_IS_EDIT_SELECTION_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_EDIT_SELECTION_TOOL))


GType      ligma_edit_selection_tool_get_type  (void) G_GNUC_CONST;

void       ligma_edit_selection_tool_start     (LigmaTool          *parent_tool,
                                               LigmaDisplay       *display,
                                               const LigmaCoords  *coords,
                                               LigmaTranslateMode  edit_mode,
                                               gboolean           propagate_release);

gboolean   ligma_edit_selection_tool_key_press (LigmaTool          *tool,
                                               GdkEventKey       *kevent,
                                               LigmaDisplay       *display);
gboolean   ligma_edit_selection_tool_translate (LigmaTool          *tool,
                                               GdkEventKey       *kevent,
                                               LigmaTransformType  translate_type,
                                               LigmaDisplay       *display,
                                               GtkWidget        **type_box);


#endif  /*  __LIGMA_EDIT_SELECTION_TOOL_H__  */
