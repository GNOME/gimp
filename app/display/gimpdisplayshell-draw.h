/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __GIMP_DISPLAY_SHELL_DRAW_H__
#define __GIMP_DISPLAY_SHELL_DRAW_H__


void   gimp_display_shell_draw_get_scaled_image_size (const GimpDisplayShell *shell,
                                                      gint                   *w,
                                                      gint                   *h);
void   gimp_display_shell_draw_get_scaled_image_size_for_scale
                                                     (const GimpDisplayShell *shell,
                                                      gdouble                 scale,
                                                      gint                   *w,
                                                      gint                   *h);
void   gimp_display_shell_draw_guide                 (const GimpDisplayShell *shell,
                                                      GimpGuide              *guide,
                                                      gboolean                active);
void   gimp_display_shell_draw_guides                (const GimpDisplayShell *shell);
void   gimp_display_shell_draw_grid                  (GimpDisplayShell       *shell,
                                                      const GdkRectangle     *area);
void   gimp_display_shell_draw_pen                   (GimpDisplayShell       *shell,
                                                      const GimpVector2      *points,
                                                      gint                    num_points,
                                                      GimpContext            *context,
                                                      GimpActiveColor         color,
                                                      gint                    width);
void   gimp_display_shell_draw_sample_point          (const GimpDisplayShell *shell,
                                                      GimpSamplePoint        *sample_point,
                                                      gboolean                active);
void   gimp_display_shell_draw_sample_points         (const GimpDisplayShell *shell);
void   gimp_display_shell_draw_vector                (const GimpDisplayShell *shell,
                                                      GimpVectors            *vectors);
void   gimp_display_shell_draw_vectors               (const GimpDisplayShell *shell);
void   gimp_display_shell_draw_cursor                (const GimpDisplayShell *shell);
void   gimp_display_shell_draw_area                  (const GimpDisplayShell *shell,
                                                      gint                    x,
                                                      gint                    y,
                                                      gint                    w,
                                                      gint                    h);


#endif /* __GIMP_DISPLAY_SHELL_DRAW_H__ */
