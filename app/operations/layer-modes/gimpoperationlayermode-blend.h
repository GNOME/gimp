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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_OPERATION_LAYER_MODE_BLEND_H__
#define __GIMP_OPERATION_LAYER_MODE_BLEND_H__


/*  nonsubtractive blend functions  */

void gimp_operation_layer_mode_blend_addition          (const gfloat *in,
                                                        const gfloat *layer,
                                                        gfloat       *comp,
                                                        gint          samples);
void gimp_operation_layer_mode_blend_burn              (const gfloat *in,
                                                        const gfloat *layer,
                                                        gfloat       *comp,
                                                        gint          samples);
void gimp_operation_layer_mode_blend_darken_only       (const gfloat *in,
                                                        const gfloat *layer,
                                                        gfloat       *comp,
                                                        gint          samples);
void gimp_operation_layer_mode_blend_difference        (const gfloat *in,
                                                        const gfloat *layer,
                                                        gfloat       *comp,
                                                        gint          samples);
void gimp_operation_layer_mode_blend_divide            (const gfloat *in,
                                                        const gfloat *layer,
                                                        gfloat       *comp,
                                                        gint          samples);
void gimp_operation_layer_mode_blend_dodge             (const gfloat *in,
                                                        const gfloat *layer,
                                                        gfloat       *comp,
                                                        gint          samples);
void gimp_operation_layer_mode_blend_exclusion         (const gfloat *in,
                                                        const gfloat *layer,
                                                        gfloat       *comp,
                                                        gint          samples);
void gimp_operation_layer_mode_blend_grain_extract     (const gfloat *in,
                                                        const gfloat *layer,
                                                        gfloat       *comp,
                                                        gint          samples);
void gimp_operation_layer_mode_blend_grain_merge       (const gfloat *in,
                                                        const gfloat *layer,
                                                        gfloat       *comp,
                                                        gint          samples);
void gimp_operation_layer_mode_blend_hard_mix          (const gfloat *in,
                                                        const gfloat *layer,
                                                        gfloat       *comp,
                                                        gint          samples);
void gimp_operation_layer_mode_blend_hardlight         (const gfloat *in,
                                                        const gfloat *layer,
                                                        gfloat       *comp,
                                                        gint          samples);
void gimp_operation_layer_mode_blend_hsl_color         (const gfloat *in,
                                                        const gfloat *layer,
                                                        gfloat       *comp,
                                                        gint          samples);
void gimp_operation_layer_mode_blend_hsv_hue           (const gfloat *in,
                                                        const gfloat *layer,
                                                        gfloat       *comp,
                                                        gint          samples);
void gimp_operation_layer_mode_blend_hsv_saturation    (const gfloat *in,
                                                        const gfloat *layer,
                                                        gfloat       *comp,
                                                        gint          samples);
void gimp_operation_layer_mode_blend_hsv_value         (const gfloat *in,
                                                        const gfloat *layer,
                                                        gfloat       *comp,
                                                        gint          samples);
void gimp_operation_layer_mode_blend_lch_chroma        (const gfloat *in,
                                                        const gfloat *layer,
                                                        gfloat       *comp,
                                                        gint          samples);
void gimp_operation_layer_mode_blend_lch_color         (const gfloat *in,
                                                        const gfloat *layer,
                                                        gfloat       *comp,
                                                        gint          samples);
void gimp_operation_layer_mode_blend_lch_hue           (const gfloat *in,
                                                        const gfloat *layer,
                                                        gfloat       *comp,
                                                        gint          samples);
void gimp_operation_layer_mode_blend_lch_lightness     (const gfloat *in,
                                                        const gfloat *layer,
                                                        gfloat       *comp,
                                                        gint          samples);
void gimp_operation_layer_mode_blend_lighten_only      (const gfloat *in,
                                                        const gfloat *layer,
                                                        gfloat       *comp,
                                                        gint          samples);
void gimp_operation_layer_mode_blend_linear_burn       (const gfloat *in,
                                                        const gfloat *layer,
                                                        gfloat       *comp,
                                                        gint          samples);
void gimp_operation_layer_mode_blend_linear_light      (const gfloat *in,
                                                        const gfloat *layer,
                                                        gfloat       *comp,
                                                        gint          samples);
void gimp_operation_layer_mode_blend_luma_darken_only  (const gfloat *in,
                                                        const gfloat *layer,
                                                        gfloat       *comp,
                                                        gint          samples);
void gimp_operation_layer_mode_blend_luma_lighten_only (const gfloat *in,
                                                        const gfloat *layer,
                                                        gfloat       *comp,
                                                        gint          samples);
void gimp_operation_layer_mode_blend_luminance         (const gfloat *in,
                                                        const gfloat *layer,
                                                        gfloat       *comp,
                                                        gint          samples);
void gimp_operation_layer_mode_blend_multiply          (const gfloat *in,
                                                        const gfloat *layer,
                                                        gfloat       *comp,
                                                        gint          samples);
void gimp_operation_layer_mode_blend_overlay           (const gfloat *in,
                                                        const gfloat *layer,
                                                        gfloat       *comp,
                                                        gint          samples);
void gimp_operation_layer_mode_blend_pin_light         (const gfloat *in,
                                                        const gfloat *layer,
                                                        gfloat       *comp,
                                                        gint          samples);
void gimp_operation_layer_mode_blend_screen            (const gfloat *in,
                                                        const gfloat *layer,
                                                        gfloat       *comp,
                                                        gint          samples);
void gimp_operation_layer_mode_blend_softlight         (const gfloat *in,
                                                        const gfloat *layer,
                                                        gfloat       *comp,
                                                        gint          samples);
void gimp_operation_layer_mode_blend_subtract          (const gfloat *in,
                                                        const gfloat *layer,
                                                        gfloat       *comp,
                                                        gint          samples);
void gimp_operation_layer_mode_blend_vivid_light       (const gfloat *in,
                                                        const gfloat *layer,
                                                        gfloat       *comp,
                                                        gint          samples);


/*  subtractive blend functions  */

void gimp_operation_layer_mode_blend_color_erase       (const gfloat *in,
                                                        const gfloat *layer,
                                                        gfloat       *comp,
                                                        gint          samples);


#endif /* __GIMP_OPERATION_LAYER_MODE_BLEND_H__ */
