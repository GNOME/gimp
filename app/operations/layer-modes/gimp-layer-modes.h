/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligma-layer-modes.h
 * Copyright (C) 2017 Michael Natterer <mitch@ligma.org>
 *                    Øyvind Kolås <pippin@ligma.org>
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

#ifndef __LIGMA_LAYER_MODES_H__
#define __LIGMA_LAYER_MODES_H__


void                       ligma_layer_modes_init                      (void);
void                       ligma_layer_modes_exit                      (void);

gboolean                   ligma_layer_mode_is_legacy                  (LigmaLayerMode           mode);

gboolean                   ligma_layer_mode_is_blend_space_mutable     (LigmaLayerMode           mode);
gboolean                   ligma_layer_mode_is_composite_space_mutable (LigmaLayerMode           mode);
gboolean                   ligma_layer_mode_is_composite_mode_mutable  (LigmaLayerMode           mode);

gboolean                   ligma_layer_mode_is_subtractive             (LigmaLayerMode           mode);
gboolean                   ligma_layer_mode_is_alpha_only              (LigmaLayerMode           mode);
gboolean                   ligma_layer_mode_is_trivial                 (LigmaLayerMode           mode);

LigmaLayerColorSpace        ligma_layer_mode_get_blend_space            (LigmaLayerMode           mode);
LigmaLayerColorSpace        ligma_layer_mode_get_composite_space        (LigmaLayerMode           mode);
LigmaLayerCompositeMode     ligma_layer_mode_get_composite_mode         (LigmaLayerMode           mode);
LigmaLayerCompositeMode     ligma_layer_mode_get_paint_composite_mode   (LigmaLayerMode           mode);

const gchar              * ligma_layer_mode_get_operation_name         (LigmaLayerMode           mode);
GeglOperation            * ligma_layer_mode_get_operation              (LigmaLayerMode           mode);

LigmaLayerModeFunc          ligma_layer_mode_get_function               (LigmaLayerMode           mode);
LigmaLayerModeBlendFunc     ligma_layer_mode_get_blend_function         (LigmaLayerMode           mode);

LigmaLayerModeContext       ligma_layer_mode_get_context                (LigmaLayerMode           mode);

LigmaLayerMode            * ligma_layer_mode_get_context_array          (LigmaLayerMode           mode,
                                                                       LigmaLayerModeContext    context,
                                                                       gint                   *n_modes);

LigmaLayerModeGroup         ligma_layer_mode_get_group                  (LigmaLayerMode           mode);

const LigmaLayerMode      * ligma_layer_mode_get_group_array            (LigmaLayerModeGroup      group,
                                                                       gint                   *n_modes);

gboolean                   ligma_layer_mode_get_for_group              (LigmaLayerMode           old_mode,
                                                                       LigmaLayerModeGroup      new_group,
                                                                       LigmaLayerMode          *new_mode);

const Babl               * ligma_layer_mode_get_format                 (LigmaLayerMode           mode,
                                                                       LigmaLayerColorSpace     blend_space,
                                                                       LigmaLayerColorSpace     composite_space,
                                                                       LigmaLayerCompositeMode  composite_mode,
                                                                       const Babl             *preferred_format);

LigmaLayerCompositeRegion   ligma_layer_mode_get_included_region        (LigmaLayerMode           mode,
                                                                       LigmaLayerCompositeMode  composite_mode);


#endif /* __LIGMA_LAYER_MODES_H__ */
