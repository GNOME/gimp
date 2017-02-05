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


typedef struct _GimpLayerModeInfo GimpLayerModeInfo;

struct _GimpLayerModeInfo
{
  GimpLayerMode          layer_mode;
  gchar                 *op_name;
  GimpLayerModeFlags     flags;
  GimpLayerCompositeMode composite_mode;
  GimpLayerColorSpace    composite_space;
  GimpLayerColorSpace    blend_space;
};


void                     gimp_layer_modes_init               (void);

const GimpLayerModeInfo *gimp_layer_mode_info                (GimpLayerMode       mode);

gboolean                 gimp_layer_mode_is_legacy           (GimpLayerMode       mode);
gboolean                 gimp_layer_mode_wants_linear_data   (GimpLayerMode       mode);

GimpLayerColorSpace      gimp_layer_mode_get_blend_space     (GimpLayerMode       mode);
GimpLayerColorSpace      gimp_layer_mode_get_composite_space (GimpLayerMode       mode);
GimpLayerCompositeMode   gimp_layer_mode_get_composite_mode  (GimpLayerMode       mode);

const gchar            * gimp_layer_mode_get_operation       (GimpLayerMode       mode);

GimpLayerModeGroup       gimp_layer_mode_get_group           (GimpLayerMode       mode);

const GimpLayerMode    * gimp_layer_mode_get_group_array     (GimpLayerModeGroup  group,
                                                              gint               *n_modes);

gboolean                 gimp_layer_mode_get_for_group       (GimpLayerMode       old_mode,
                                                              GimpLayerModeGroup  new_group,
                                                              GimpLayerMode      *new_mode);


#endif /* __GIMP_LAYER_MODES_H__ */
