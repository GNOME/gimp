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

#ifndef __LIGMA_DISPLAY_SHELL_RENDER_H__
#define __LIGMA_DISPLAY_SHELL_RENDER_H__


void     ligma_display_shell_render_set_scale       (LigmaDisplayShell *shell,
                                                    gint              scale);

void     ligma_display_shell_render_invalidate_full (LigmaDisplayShell *shell);
void     ligma_display_shell_render_invalidate_area (LigmaDisplayShell *shell,
                                                    gint              x,
                                                    gint              y,
                                                    gint              width,
                                                    gint              height);

void     ligma_display_shell_render_validate_area   (LigmaDisplayShell *shell,
                                                    gint              x,
                                                    gint              y,
                                                    gint              width,
                                                    gint              height);

gboolean ligma_display_shell_render_is_valid        (LigmaDisplayShell *shell,
                                                    gint              x,
                                                    gint              y,
                                                    gint              width,
                                                    gint              height);

void     ligma_display_shell_render                 (LigmaDisplayShell *shell,
                                                    cairo_t          *cr,
                                                    gint              x,
                                                    gint              y,
                                                    gint              width,
                                                    gint              height,
                                                    gdouble           scale);


#endif  /*  __LIGMA_DISPLAY_SHELL_RENDER_H__  */
