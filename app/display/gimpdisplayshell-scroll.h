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

#ifndef __LIGMA_DISPLAY_SHELL_SCROLL_H__
#define __LIGMA_DISPLAY_SHELL_SCROLL_H__


void   ligma_display_shell_scroll                     (LigmaDisplayShell *shell,
                                                      gint              x_offset,
                                                      gint              y_offset);
void   ligma_display_shell_scroll_set_offset          (LigmaDisplayShell *shell,
                                                      gint              offset_x,
                                                      gint              offset_y);

void   ligma_display_shell_scroll_clamp_and_update    (LigmaDisplayShell *shell);

void   ligma_display_shell_scroll_unoverscrollify     (LigmaDisplayShell *shell,
                                                      gint              in_offset_x,
                                                      gint              in_offset_y,
                                                      gint             *out_offset_x,
                                                      gint             *out_offset_y);

void   ligma_display_shell_scroll_center_image_xy     (LigmaDisplayShell *shell,
                                                      gdouble           image_x,
                                                      gdouble           image_y);
void   ligma_display_shell_scroll_center_image        (LigmaDisplayShell *shell,
                                                      gboolean          horizontally,
                                                      gboolean          vertically);
void   ligma_display_shell_scroll_center_content      (LigmaDisplayShell *shell,
                                                      gboolean          horizontally,
                                                      gboolean          vertically);

void   ligma_display_shell_scroll_get_scaled_viewport (LigmaDisplayShell *shell,
                                                      gint             *x,
                                                      gint             *y,
                                                      gint             *w,
                                                      gint             *h);
void   ligma_display_shell_scroll_get_viewport        (LigmaDisplayShell *shell,
                                                      gdouble          *x,
                                                      gdouble          *y,
                                                      gdouble          *w,
                                                      gdouble          *h);


#endif  /*  __LIGMA_DISPLAY_SHELL_SCROLL_H__  */
