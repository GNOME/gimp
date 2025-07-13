/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpsymmetry-tiling.h
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

#pragma once

#include "gimpsymmetry.h"


#define GIMP_TYPE_TILING            (gimp_tiling_get_type ())
#define GIMP_TILING(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_TILING, GimpTiling))
#define GIMP_TILING_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_TILING, GimpTilingClass))
#define GIMP_IS_TILING(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_TILING))
#define GIMP_IS_TILING_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_TILING))
#define GIMP_TILING_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_TILING, GimpTilingClass))


typedef struct _GimpTilingClass GimpTilingClass;

struct _GimpTiling
{
  GimpSymmetry  parent_instance;

  gdouble       interval_x;
  gdouble       interval_y;
  gdouble       shift;
  gint          max_x;
  gint          max_y;
};

struct _GimpTilingClass
{
  GimpSymmetryClass  parent_class;
};


GType   gimp_tiling_get_type (void) G_GNUC_CONST;
