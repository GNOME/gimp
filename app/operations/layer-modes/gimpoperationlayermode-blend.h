/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationlayermode-blend.h
 * Copyright (C) 2017 Michael Natterer <mitch@gimp.org>
 *               2017 Øyvind Kolås <pippin@gimp.org>
 *               2017 Ell
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; withcomp even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once


/*  nonsubtractive blend functions  */

void gimp_operation_layer_mode_blend_addition          (GeglOperation *operation,
                                                        const gfloat  *in,
                                                        const gfloat  *layer,
                                                        gfloat        *comp,
                                                        gint           samples);
void gimp_operation_layer_mode_blend_burn              (GeglOperation *operation,
                                                        const gfloat  *in,
                                                        const gfloat  *layer,
                                                        gfloat        *comp,
                                                        gint           samples);
void gimp_operation_layer_mode_blend_darken_only       (GeglOperation *operation,
                                                        const gfloat  *in,
                                                        const gfloat  *layer,
                                                        gfloat        *comp,
                                                        gint           samples);
void gimp_operation_layer_mode_blend_difference        (GeglOperation *operation,
                                                        const gfloat  *in,
                                                        const gfloat  *layer,
                                                        gfloat        *comp,
                                                        gint           samples);
void gimp_operation_layer_mode_blend_divide            (GeglOperation *operation,
                                                        const gfloat  *in,
                                                        const gfloat  *layer,
                                                        gfloat        *comp,
                                                        gint           samples);
void gimp_operation_layer_mode_blend_dodge             (GeglOperation *operation,
                                                        const gfloat  *in,
                                                        const gfloat  *layer,
                                                        gfloat        *comp,
                                                        gint           samples);
void gimp_operation_layer_mode_blend_exclusion         (GeglOperation *operation,
                                                        const gfloat  *in,
                                                        const gfloat  *layer,
                                                        gfloat        *comp,
                                                        gint           samples);
void gimp_operation_layer_mode_blend_grain_extract     (GeglOperation *operation,
                                                        const gfloat  *in,
                                                        const gfloat  *layer,
                                                        gfloat        *comp,
                                                        gint           samples);
void gimp_operation_layer_mode_blend_grain_merge       (GeglOperation *operation,
                                                        const gfloat  *in,
                                                        const gfloat  *layer,
                                                        gfloat        *comp,
                                                        gint           samples);
void gimp_operation_layer_mode_blend_hard_mix          (GeglOperation *operation,
                                                        const gfloat  *in,
                                                        const gfloat  *layer,
                                                        gfloat        *comp,
                                                        gint           samples);
void gimp_operation_layer_mode_blend_hardlight         (GeglOperation *operation,
                                                        const gfloat  *in,
                                                        const gfloat  *layer,
                                                        gfloat        *comp,
                                                        gint           samples);
void gimp_operation_layer_mode_blend_hsl_color         (GeglOperation *operation,
                                                        const gfloat  *in,
                                                        const gfloat  *layer,
                                                        gfloat        *comp,
                                                        gint           samples);
void gimp_operation_layer_mode_blend_hsv_hue           (GeglOperation *operation,
                                                        const gfloat  *in,
                                                        const gfloat  *layer,
                                                        gfloat        *comp,
                                                        gint           samples);
void gimp_operation_layer_mode_blend_hsv_saturation    (GeglOperation *operation,
                                                        const gfloat  *in,
                                                        const gfloat  *layer,
                                                        gfloat        *comp,
                                                        gint           samples);
void gimp_operation_layer_mode_blend_hsv_value         (GeglOperation *operation,
                                                        const gfloat  *in,
                                                        const gfloat  *layer,
                                                        gfloat        *comp,
                                                        gint           samples);
void gimp_operation_layer_mode_blend_lch_chroma        (GeglOperation *operation,
                                                        const gfloat  *in,
                                                        const gfloat  *layer,
                                                        gfloat        *comp,
                                                        gint           samples);
void gimp_operation_layer_mode_blend_lch_color         (GeglOperation *operation,
                                                        const gfloat  *in,
                                                        const gfloat  *layer,
                                                        gfloat        *comp,
                                                        gint           samples);
void gimp_operation_layer_mode_blend_lch_hue           (GeglOperation *operation,
                                                        const gfloat  *in,
                                                        const gfloat  *layer,
                                                        gfloat        *comp,
                                                        gint           samples);
void gimp_operation_layer_mode_blend_lch_lightness     (GeglOperation *operation,
                                                        const gfloat  *in,
                                                        const gfloat  *layer,
                                                        gfloat        *comp,
                                                        gint           samples);
void gimp_operation_layer_mode_blend_lighten_only      (GeglOperation *operation,
                                                        const gfloat  *in,
                                                        const gfloat  *layer,
                                                        gfloat        *comp,
                                                        gint           samples);
void gimp_operation_layer_mode_blend_linear_burn       (GeglOperation *operation,
                                                        const gfloat  *in,
                                                        const gfloat  *layer,
                                                        gfloat        *comp,
                                                        gint           samples);
void gimp_operation_layer_mode_blend_linear_light      (GeglOperation *operation,
                                                        const gfloat  *in,
                                                        const gfloat  *layer,
                                                        gfloat        *comp,
                                                        gint           samples);
void gimp_operation_layer_mode_blend_luma_darken_only  (GeglOperation *operation,
                                                        const gfloat  *in,
                                                        const gfloat  *layer,
                                                        gfloat        *comp,
                                                        gint           samples);
void gimp_operation_layer_mode_blend_luma_lighten_only (GeglOperation *operation,
                                                        const gfloat  *in,
                                                        const gfloat  *layer,
                                                        gfloat        *comp,
                                                        gint           samples);
void gimp_operation_layer_mode_blend_luminance         (GeglOperation *operation,
                                                        const gfloat  *in,
                                                        const gfloat  *layer,
                                                        gfloat        *comp,
                                                        gint           samples);
void gimp_operation_layer_mode_blend_multiply          (GeglOperation *operation,
                                                        const gfloat  *in,
                                                        const gfloat  *layer,
                                                        gfloat        *comp,
                                                        gint           samples);
void gimp_operation_layer_mode_blend_overlay           (GeglOperation *operation,
                                                        const gfloat  *in,
                                                        const gfloat  *layer,
                                                        gfloat        *comp,
                                                        gint           samples);
void gimp_operation_layer_mode_blend_pin_light         (GeglOperation *operation,
                                                        const gfloat  *in,
                                                        const gfloat  *layer,
                                                        gfloat        *comp,
                                                        gint           samples);
void gimp_operation_layer_mode_blend_screen            (GeglOperation *operation,
                                                        const gfloat  *in,
                                                        const gfloat  *layer,
                                                        gfloat        *comp,
                                                        gint           samples);
void gimp_operation_layer_mode_blend_softlight         (GeglOperation *operation,
                                                        const gfloat  *in,
                                                        const gfloat  *layer,
                                                        gfloat        *comp,
                                                        gint           samples);
void gimp_operation_layer_mode_blend_subtract          (GeglOperation *operation,
                                                        const gfloat  *in,
                                                        const gfloat  *layer,
                                                        gfloat        *comp,
                                                        gint           samples);
void gimp_operation_layer_mode_blend_vivid_light       (GeglOperation *operation,
                                                        const gfloat  *in,
                                                        const gfloat  *layer,
                                                        gfloat        *comp,
                                                        gint           samples);


/*  subtractive blend functions  */

void gimp_operation_layer_mode_blend_color_erase       (GeglOperation *operation,
                                                        const gfloat  *in,
                                                        const gfloat  *layer,
                                                        gfloat        *comp,
                                                        gint           samples);
