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

#ifndef  __LIGMA_GRADIENT_TOOL_EDITOR_H__
#define  __LIGMA_GRADIENT_TOOL_EDITOR_H__


void          ligma_gradient_tool_editor_options_notify   (LigmaGradientTool *gradient_tool,
                                                          LigmaToolOptions  *options,
                                                          const GParamSpec *pspec);

void          ligma_gradient_tool_editor_start            (LigmaGradientTool *gradient_tool);
void          ligma_gradient_tool_editor_halt             (LigmaGradientTool *gradient_tool);

gboolean      ligma_gradient_tool_editor_line_changed     (LigmaGradientTool *gradient_tool);

void          ligma_gradient_tool_editor_fg_bg_changed    (LigmaGradientTool *gradient_tool);

void          ligma_gradient_tool_editor_gradient_dirty   (LigmaGradientTool *gradient_tool);

void          ligma_gradient_tool_editor_gradient_changed (LigmaGradientTool *gradient_tool);

const gchar * ligma_gradient_tool_editor_can_undo         (LigmaGradientTool *gradient_tool);
const gchar * ligma_gradient_tool_editor_can_redo         (LigmaGradientTool *gradient_tool);

gboolean      ligma_gradient_tool_editor_undo             (LigmaGradientTool *gradient_tool);
gboolean      ligma_gradient_tool_editor_redo             (LigmaGradientTool *gradient_tool);

void          ligma_gradient_tool_editor_start_edit       (LigmaGradientTool *gradient_tool);
void          ligma_gradient_tool_editor_end_edit         (LigmaGradientTool *gradient_tool,
                                                          gboolean          cancel);


#endif  /*  __LIGMA_GRADIENT_TOOL_EDITOR_H__  */
