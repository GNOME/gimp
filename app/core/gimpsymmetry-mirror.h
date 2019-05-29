/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpsymmetry-mirror.h
 * Copyright (C) 2015 Jehan <jehan@gimp.org>
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

#ifndef __GIMP_MIRROR_H__
#define __GIMP_MIRROR_H__


#include "gimpsymmetry.h"


#define GIMP_TYPE_MIRROR            (gimp_mirror_get_type ())
#define GIMP_MIRROR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_MIRROR, GimpMirror))
#define GIMP_MIRROR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_MIRROR, GimpMirrorClass))
#define GIMP_IS_MIRROR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_MIRROR))
#define GIMP_IS_MIRROR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_MIRROR))
#define GIMP_MIRROR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_MIRROR, GimpMirrorClass))


typedef struct _GimpMirrorClass GimpMirrorClass;

struct _GimpMirror
{
  GimpSymmetry  parent_instance;

  gboolean      horizontal_mirror;
  gboolean      vertical_mirror;
  gboolean      point_symmetry;
  gboolean      disable_transformation;

  gdouble       mirror_position_y;
  gdouble       mirror_position_x;
  GimpGuide    *horizontal_guide;
  GimpGuide    *vertical_guide;
};

struct _GimpMirrorClass
{
  GimpSymmetryClass  parent_class;
};


GType   gimp_mirror_get_type (void) G_GNUC_CONST;


#endif  /*  __GIMP_MIRROR_H__  */
