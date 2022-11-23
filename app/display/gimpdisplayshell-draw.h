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

#ifndef __LIGMA_DISPLAY_SHELL_DRAW_H__
#define __LIGMA_DISPLAY_SHELL_DRAW_H__


void   ligma_display_shell_draw_selection_out (LigmaDisplayShell   *shell,
                                              cairo_t            *cr,
                                              LigmaSegment        *segs,
                                              gint                n_segs);
void   ligma_display_shell_draw_selection_in  (LigmaDisplayShell   *shell,
                                              cairo_t            *cr,
                                              cairo_pattern_t    *mask,
                                              gint                index);

void   ligma_display_shell_draw_background    (LigmaDisplayShell   *shell,
                                              cairo_t            *cr);
void   ligma_display_shell_draw_checkerboard  (LigmaDisplayShell   *shell,
                                              cairo_t            *cr);
void   ligma_display_shell_draw_image         (LigmaDisplayShell   *shell,
                                              cairo_t            *cr,
                                              gint                x,
                                              gint                y,
                                              gint                w,
                                              gint                h);


#endif /* __LIGMA_DISPLAY_SHELL_DRAW_H__ */
