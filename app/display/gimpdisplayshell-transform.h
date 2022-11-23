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

#ifndef __LIGMA_DISPLAY_SHELL_TRANSFORM_H__
#define __LIGMA_DISPLAY_SHELL_TRANSFORM_H__


/*  zoom: functions to transform from image space to unrotated display
 *  space and back, taking into account scroll offset and scale
 */

void  ligma_display_shell_zoom_coords                   (LigmaDisplayShell   *shell,
                                                        const LigmaCoords   *image_coords,
                                                        LigmaCoords         *display_coords);
void  ligma_display_shell_unzoom_coords                 (LigmaDisplayShell   *shell,
                                                        const LigmaCoords   *display_coords,
                                                        LigmaCoords         *image_coords);

void  ligma_display_shell_zoom_xy                       (LigmaDisplayShell   *shell,
                                                        gdouble             x,
                                                        gdouble             y,
                                                        gint               *nx,
                                                        gint               *ny);
void  ligma_display_shell_unzoom_xy                     (LigmaDisplayShell   *shell,
                                                        gint                x,
                                                        gint                y,
                                                        gint               *nx,
                                                        gint               *ny,
                                                        gboolean            round);

void  ligma_display_shell_zoom_xy_f                     (LigmaDisplayShell   *shell,
                                                        gdouble             x,
                                                        gdouble             y,
                                                        gdouble            *nx,
                                                        gdouble            *ny);
void  ligma_display_shell_unzoom_xy_f                   (LigmaDisplayShell   *shell,
                                                        gdouble             x,
                                                        gdouble             y,
                                                        gdouble            *nx,
                                                        gdouble            *ny);

void  ligma_display_shell_zoom_segments                 (LigmaDisplayShell   *shell,
                                                        const LigmaBoundSeg *src_segs,
                                                        LigmaSegment        *dest_segs,
                                                        gint                n_segs,
                                                        gdouble             offset_x,
                                                        gdouble             offset_y);


/*  rotate: functions to transform from unrotated and unflipped but
 *  zoomed display space to rotated and flipped display space and back
 */

void  ligma_display_shell_rotate_coords                 (LigmaDisplayShell   *shell,
                                                        const LigmaCoords   *image_coords,
                                                        LigmaCoords         *display_coords);
void  ligma_display_shell_unrotate_coords               (LigmaDisplayShell   *shell,
                                                        const LigmaCoords   *display_coords,
                                                        LigmaCoords         *image_coords);

void  ligma_display_shell_rotate_xy                     (LigmaDisplayShell   *shell,
                                                        gdouble             x,
                                                        gdouble             y,
                                                        gint               *nx,
                                                        gint               *ny);
void  ligma_display_shell_unrotate_xy                   (LigmaDisplayShell   *shell,
                                                        gint                x,
                                                        gint                y,
                                                        gint               *nx,
                                                        gint               *ny);

void  ligma_display_shell_rotate_xy_f                   (LigmaDisplayShell   *shell,
                                                        gdouble             x,
                                                        gdouble             y,
                                                        gdouble            *nx,
                                                        gdouble            *ny);
void  ligma_display_shell_unrotate_xy_f                 (LigmaDisplayShell   *shell,
                                                        gdouble             x,
                                                        gdouble             y,
                                                        gdouble            *nx,
                                                        gdouble            *ny);

void  ligma_display_shell_rotate_bounds                 (LigmaDisplayShell   *shell,
                                                        gdouble             x1,
                                                        gdouble             y1,
                                                        gdouble             x2,
                                                        gdouble             y2,
                                                        gdouble            *nx1,
                                                        gdouble            *ny1,
                                                        gdouble            *nx2,
                                                        gdouble            *ny2);
void  ligma_display_shell_unrotate_bounds               (LigmaDisplayShell   *shell,
                                                        gdouble             x1,
                                                        gdouble             y1,
                                                        gdouble             x2,
                                                        gdouble             y2,
                                                        gdouble            *nx1,
                                                        gdouble            *ny1,
                                                        gdouble            *nx2,
                                                        gdouble            *ny2);


/*  transform: functions to transform from image space to rotated
 *  display space and back, taking into account scroll offset, scale,
 *  rotation and flipping
 */

void  ligma_display_shell_transform_coords              (LigmaDisplayShell   *shell,
                                                        const LigmaCoords   *image_coords,
                                                        LigmaCoords         *display_coords);
void  ligma_display_shell_untransform_coords            (LigmaDisplayShell   *shell,
                                                        const LigmaCoords   *display_coords,
                                                        LigmaCoords         *image_coords);

void  ligma_display_shell_transform_xy                  (LigmaDisplayShell   *shell,
                                                        gdouble             x,
                                                        gdouble             y,
                                                        gint               *nx,
                                                        gint               *ny);
void  ligma_display_shell_untransform_xy                (LigmaDisplayShell   *shell,
                                                        gint                x,
                                                        gint                y,
                                                        gint               *nx,
                                                        gint               *ny,
                                                        gboolean            round);

void  ligma_display_shell_transform_xy_f                (LigmaDisplayShell   *shell,
                                                        gdouble             x,
                                                        gdouble             y,
                                                        gdouble            *nx,
                                                        gdouble            *ny);
void  ligma_display_shell_untransform_xy_f              (LigmaDisplayShell   *shell,
                                                        gdouble             x,
                                                        gdouble             y,
                                                        gdouble            *nx,
                                                        gdouble            *ny);

void  ligma_display_shell_transform_bounds              (LigmaDisplayShell   *shell,
                                                        gdouble             x1,
                                                        gdouble             y1,
                                                        gdouble             x2,
                                                        gdouble             y2,
                                                        gdouble            *nx1,
                                                        gdouble            *ny1,
                                                        gdouble            *nx2,
                                                        gdouble            *ny2);
void  ligma_display_shell_untransform_bounds            (LigmaDisplayShell   *shell,
                                                        gdouble             x1,
                                                        gdouble             y1,
                                                        gdouble             x2,
                                                        gdouble             y2,
                                                        gdouble            *nx1,
                                                        gdouble            *ny1,
                                                        gdouble            *nx2,
                                                        gdouble            *ny2);

void  ligma_display_shell_transform_bounds_with_scale   (LigmaDisplayShell   *shell,
                                                        gdouble             scale,
                                                        gdouble             x1,
                                                        gdouble             y1,
                                                        gdouble             x2,
                                                        gdouble             y2,
                                                        gdouble            *nx1,
                                                        gdouble            *ny1,
                                                        gdouble            *nx2,
                                                        gdouble            *ny2);
void  ligma_display_shell_untransform_bounds_with_scale (LigmaDisplayShell   *shell,
                                                        gdouble             scale,
                                                        gdouble             x1,
                                                        gdouble             y1,
                                                        gdouble             x2,
                                                        gdouble             y2,
                                                        gdouble            *nx1,
                                                        gdouble            *ny1,
                                                        gdouble            *nx2,
                                                        gdouble            *ny2);

void  ligma_display_shell_untransform_viewport          (LigmaDisplayShell   *shell,
                                                        gboolean            clip,
                                                        gint               *x,
                                                        gint               *y,
                                                        gint               *width,
                                                        gint               *height);


#endif /* __LIGMA_DISPLAY_SHELL_TRANSFORM_H__ */
