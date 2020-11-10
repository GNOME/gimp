/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimp-layer-modes.h
 * Copyright (C) 2017 Michael Natterer <mitch@gimp.org>
 *                    Øyvind Kolås <pippin@gimp.org>
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

#ifndef __GIMP_LAYER_MODES_H__
#define __GIMP_LAYER_MODES_H__


void                       gimp_layer_modes_init                      (void);
void                       gimp_layer_modes_exit                      (void);

gboolean                   gimp_layer_mode_is_legacy                  (GimpLayerMode           mode);

gboolean                   gimp_layer_mode_is_blend_space_mutable     (GimpLayerMode           mode);
gboolean                   gimp_layer_mode_is_composite_space_mutable (GimpLayerMode           mode);
gboolean                   gimp_layer_mode_is_composite_mode_mutable  (GimpLayerMode           mode);

gboolean                   gimp_layer_mode_is_subtractive             (GimpLayerMode           mode);
gboolean                   gimp_layer_mode_is_alpha_only              (GimpLayerMode           mode);
gboolean                   gimp_layer_mode_is_trivial                 (GimpLayerMode           mode);

GimpLayerColorSpace        gimp_layer_mode_get_blend_space            (GimpLayerMode           mode);
GimpLayerColorSpace        gimp_layer_mode_get_composite_space        (GimpLayerMode           mode);
GimpLayerCompositeMode     gimp_layer_mode_get_composite_mode         (GimpLayerMode           mode);
GimpLayerCompositeMode     gimp_layer_mode_get_paint_composite_mode   (GimpLayerMode           mode);

const gchar              * gimp_layer_mode_get_operation_name         (GimpLayerMode           mode);
GeglOperation            * gimp_layer_mode_get_operation              (GimpLayerMode           mode);

GimpLayerModeFunc          gimp_layer_mode_get_function               (GimpLayerMode           mode);
GimpLayerModeBlendFunc     gimp_layer_mode_get_blend_function         (GimpLayerMode           mode);

GimpLayerModeContext       gimp_layer_mode_get_context                (GimpLayerMode           mode);

GimpLayerMode            * gimp_layer_mode_get_context_array          (GimpLayerMode           mode,
                                                                       GimpLayerModeContext    context,
                                                                       gint                   *n_modes);

GimpLayerModeGroup         gimp_layer_mode_get_group                  (GimpLayerMode           mode);

const GimpLayerMode      * gimp_layer_mode_get_group_array            (GimpLayerModeGroup      group,
                                                                       gint                   *n_modes);

gboolean                   gimp_layer_mode_get_for_group              (GimpLayerMode           old_mode,
                                                                       GimpLayerModeGroup      new_group,
                                                                       GimpLayerMode          *new_mode);

const Babl               * gimp_layer_mode_get_format                 (GimpLayerMode           mode,
                                                                       GimpLayerColorSpace     blend_space,
                                                                       GimpLayerColorSpace     composite_space,
                                                                       GimpLayerCompositeMode  composite_mode,
                                                                       const Babl             *preferred_format);

GimpLayerCompositeRegion   gimp_layer_mode_get_included_region        (GimpLayerMode           mode,
                                                                       GimpLayerCompositeMode  composite_mode);


#endif /* __GIMP_LAYER_MODES_H__ */
