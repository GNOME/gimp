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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_LAYER_MODES_H__
#define __GIMP_LAYER_MODES_H__


gboolean                 gimp_layer_mode_is_legacy           (GimpLayerMode       mode);
gboolean                 gimp_layer_mode_is_linear           (GimpLayerMode       mode);

GimpLayerColorSpace      gimp_layer_mode_get_blend_space     (GimpLayerMode       mode);
GimpLayerColorSpace      gimp_layer_mode_get_composite_space (GimpLayerMode       mode);
GimpLayerCompositeMode   gimp_layer_mode_get_composite_mode  (GimpLayerMode       mode);

const gchar            * gimp_layer_mode_get_operation       (GimpLayerMode       mode);

GimpLayerModeGroup       gimp_layer_mode_get_group           (GimpLayerMode       mode);

gboolean                 gimp_layer_mode_get_for_group       (GimpLayerMode       old_mode,
                                                              GimpLayerModeGroup  new_group,
                                                              GimpLayerMode      *new_mode);


#endif /* __GIMP_LAYER_MODES_H__ */
