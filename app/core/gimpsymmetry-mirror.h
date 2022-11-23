/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmasymmetry-mirror.h
 * Copyright (C) 2015 Jehan <jehan@ligma.org>
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

#ifndef __LIGMA_MIRROR_H__
#define __LIGMA_MIRROR_H__


#include "ligmasymmetry.h"


#define LIGMA_TYPE_MIRROR            (ligma_mirror_get_type ())
#define LIGMA_MIRROR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_MIRROR, LigmaMirror))
#define LIGMA_MIRROR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_MIRROR, LigmaMirrorClass))
#define LIGMA_IS_MIRROR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_MIRROR))
#define LIGMA_IS_MIRROR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_MIRROR))
#define LIGMA_MIRROR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_MIRROR, LigmaMirrorClass))


typedef struct _LigmaMirrorClass LigmaMirrorClass;

struct _LigmaMirror
{
  LigmaSymmetry  parent_instance;

  gboolean      horizontal_mirror;
  gboolean      vertical_mirror;
  gboolean      point_symmetry;
  gboolean      disable_transformation;

  gdouble       mirror_position_y;
  gdouble       mirror_position_x;
  LigmaGuide    *horizontal_guide;
  LigmaGuide    *vertical_guide;
};

struct _LigmaMirrorClass
{
  LigmaSymmetryClass  parent_class;
};


GType   ligma_mirror_get_type (void) G_GNUC_CONST;


#endif  /*  __LIGMA_MIRROR_H__  */
