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


void   context_colors_default_cmd_callback        (GimpAction *action,
                                                   GVariant   *value,
                                                   gpointer    data);
void   context_colors_swap_cmd_callback           (GimpAction *action,
                                                   GVariant   *value,
                                                   gpointer    data);

void   context_palette_foreground_cmd_callback    (GimpAction *action,
                                                   GVariant   *value,
                                                   gpointer    data);
void   context_palette_background_cmd_callback    (GimpAction *action,
                                                   GVariant   *value,
                                                   gpointer    data);

void   context_colormap_foreground_cmd_callback   (GimpAction *action,
                                                   GVariant   *value,
                                                   gpointer    data);
void   context_colormap_background_cmd_callback   (GimpAction *action,
                                                   GVariant   *value,
                                                   gpointer    data);

void   context_swatch_foreground_cmd_callback     (GimpAction *action,
                                                   GVariant   *value,
                                                   gpointer    data);
void   context_swatch_background_cmd_callback     (GimpAction *action,
                                                   GVariant   *value,
                                                   gpointer    data);

void   context_foreground_red_cmd_callback        (GimpAction *action,
                                                   GVariant   *value,
                                                   gpointer    data);
void   context_foreground_green_cmd_callback      (GimpAction *action,
                                                   GVariant   *value,
                                                   gpointer    data);
void   context_foreground_blue_cmd_callback       (GimpAction *action,
                                                   GVariant   *value,
                                                   gpointer    data);

void   context_background_red_cmd_callback        (GimpAction *action,
                                                   GVariant   *value,
                                                   gpointer    data);
void   context_background_green_cmd_callback      (GimpAction *action,
                                                   GVariant   *value,
                                                   gpointer    data);
void   context_background_blue_cmd_callback       (GimpAction *action,
                                                   GVariant   *value,
                                                   gpointer    data);

void   context_foreground_hue_cmd_callback        (GimpAction *action,
                                                   GVariant   *value,
                                                   gpointer    data);
void   context_foreground_saturation_cmd_callback (GimpAction *action,
                                                   GVariant   *value,
                                                   gpointer    data);
void   context_foreground_value_cmd_callback      (GimpAction *action,
                                                   GVariant   *value,
                                                   gpointer    data);

void   context_background_hue_cmd_callback        (GimpAction *action,
                                                   GVariant   *value,
                                                   gpointer    data);
void   context_background_saturation_cmd_callback (GimpAction *action,
                                                   GVariant   *value,
                                                   gpointer    data);
void   context_background_value_cmd_callback      (GimpAction *action,
                                                   GVariant   *value,
                                                   gpointer    data);

void   context_opacity_cmd_callback               (GimpAction *action,
                                                   GVariant   *value,
                                                   gpointer    data);
void   context_paint_mode_cmd_callback            (GimpAction *action,
                                                   GVariant   *value,
                                                   gpointer    data);

void   context_tool_select_cmd_callback           (GimpAction *action,
                                                   GVariant   *value,
                                                   gpointer    data);
void   context_brush_select_cmd_callback          (GimpAction *action,
                                                   GVariant   *value,
                                                   gpointer    data);
void   context_pattern_select_cmd_callback        (GimpAction *action,
                                                   GVariant   *value,
                                                   gpointer    data);
void   context_palette_select_cmd_callback        (GimpAction *action,
                                                   GVariant   *value,
                                                   gpointer    data);
void   context_gradient_select_cmd_callback       (GimpAction *action,
                                                   GVariant   *value,
                                                   gpointer    data);
void   context_font_select_cmd_callback           (GimpAction *action,
                                                   GVariant   *value,
                                                   gpointer    data);

void   context_brush_spacing_cmd_callback         (GimpAction *action,
                                                   GVariant   *value,
                                                   gpointer    data);
void   context_brush_shape_cmd_callback           (GimpAction *action,
                                                   GVariant   *value,
                                                   gpointer    data);
void   context_brush_radius_cmd_callback          (GimpAction *action,
                                                   GVariant   *value,
                                                   gpointer    data);
void   context_brush_spikes_cmd_callback          (GimpAction *action,
                                                   GVariant   *value,
                                                   gpointer    data);
void   context_brush_hardness_cmd_callback        (GimpAction *action,
                                                   GVariant   *value,
                                                   gpointer    data);
void   context_brush_aspect_cmd_callback          (GimpAction *action,
                                                   GVariant   *value,
                                                   gpointer    data);
void   context_brush_angle_cmd_callback           (GimpAction *action,
                                                   GVariant   *value,
                                                   gpointer    data);

void   context_toggle_dynamics_cmd_callback       (GimpAction *action,
                                                   GVariant   *value,
                                                   gpointer    data);
