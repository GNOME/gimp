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

#ifndef  __GIMP_GRADIENT_TOOL_EDITOR_H__
#define  __GIMP_GRADIENT_TOOL_EDITOR_H__


void          gimp_gradient_tool_editor_options_notify   (GimpGradientTool *gradient_tool,
                                                          GimpToolOptions  *options,
                                                          const GParamSpec *pspec);

void          gimp_gradient_tool_editor_start            (GimpGradientTool *gradient_tool);
void          gimp_gradient_tool_editor_halt             (GimpGradientTool *gradient_tool);

gboolean      gimp_gradient_tool_editor_line_changed     (GimpGradientTool *gradient_tool);

void          gimp_gradient_tool_editor_fg_bg_changed    (GimpGradientTool *gradient_tool);

void          gimp_gradient_tool_editor_gradient_dirty   (GimpGradientTool *gradient_tool);

void          gimp_gradient_tool_editor_gradient_changed (GimpGradientTool *gradient_tool);

const gchar * gimp_gradient_tool_editor_can_undo         (GimpGradientTool *gradient_tool);
const gchar * gimp_gradient_tool_editor_can_redo         (GimpGradientTool *gradient_tool);

gboolean      gimp_gradient_tool_editor_undo             (GimpGradientTool *gradient_tool);
gboolean      gimp_gradient_tool_editor_redo             (GimpGradientTool *gradient_tool);

void          gimp_gradient_tool_editor_start_edit       (GimpGradientTool *gradient_tool);
void          gimp_gradient_tool_editor_end_edit         (GimpGradientTool *gradient_tool,
                                                          gboolean          cancel);


#endif  /*  __GIMP_GRADIENT_TOOL_EDITOR_H__  */
