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

#ifndef __GIMP_DISPLAY_SHELL_DRAW_H__
#define __GIMP_DISPLAY_SHELL_DRAW_H__


void   gimp_display_shell_draw_get_scaled_image_size (GimpDisplayShell   *shell,
                                                      gint               *w,
                                                      gint               *h);
void   gimp_display_shell_draw_get_scaled_image_size_for_scale
                                                     (GimpDisplayShell   *shell,
                                                      gdouble             scale,
                                                      gint               *w,
                                                      gint               *h);
void   gimp_display_shell_draw_guides                (GimpDisplayShell   *shell,
                                                      cairo_t            *cr);
void   gimp_display_shell_draw_grid                  (GimpDisplayShell   *shell,
                                                      cairo_t            *cr);
void   gimp_display_shell_draw_pen                   (GimpDisplayShell   *shell,
                                                      cairo_t            *cr,
                                                      const GimpVector2  *points,
                                                      gint                n_points,
                                                      GimpContext        *context,
                                                      GimpActiveColor     color,
                                                      gint                width);
void   gimp_display_shell_draw_sample_points         (GimpDisplayShell   *shell,
                                                      cairo_t            *cr);
void   gimp_display_shell_draw_layer_boundary        (GimpDisplayShell   *shell,
                                                      cairo_t            *cr,
                                                      GimpDrawable       *drawable,
                                                      GdkSegment         *segs,
                                                      gint                n_segs);
void   gimp_display_shell_draw_selection_out         (GimpDisplayShell   *shell,
                                                      cairo_t            *cr,
                                                      GdkSegment         *segs,
                                                      gint                n_segs);
void   gimp_display_shell_draw_selection_in          (GimpDisplayShell   *shell,
                                                      cairo_t            *cr,
                                                      cairo_pattern_t    *mask,
                                                      gint                index);
void   gimp_display_shell_draw_vectors               (GimpDisplayShell   *shell,
                                                      cairo_t            *cr);
void   gimp_display_shell_draw_cursor                (GimpDisplayShell   *shell,
                                                      cairo_t            *cr);
void   gimp_display_shell_draw_image                 (GimpDisplayShell   *shell,
                                                      cairo_t            *cr,
                                                      gint                x,
                                                      gint                y,
                                                      gint                w,
                                                      gint                h);
void   gimp_display_shell_draw_checkerboard          (GimpDisplayShell   *shell,
                                                      cairo_t            *cr,
                                                      gint                x,
                                                      gint                y,
                                                      gint                w,
                                                      gint                h);
void   gimp_display_shell_draw_highlight             (GimpDisplayShell   *shell,
                                                      cairo_t            *cr,
                                                      gint                x,
                                                      gint                y,
                                                      gint                w,
                                                      gint                h);


#endif /* __GIMP_DISPLAY_SHELL_DRAW_H__ */
