/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmasymmetry-tiling.h
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

#ifndef __LIGMA_TILING_H__
#define __LIGMA_TILING_H__


#include "ligmasymmetry.h"


#define LIGMA_TYPE_TILING            (ligma_tiling_get_type ())
#define LIGMA_TILING(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_TILING, LigmaTiling))
#define LIGMA_TILING_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_TILING, LigmaTilingClass))
#define LIGMA_IS_TILING(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_TILING))
#define LIGMA_IS_TILING_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_TILING))
#define LIGMA_TILING_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_TILING, LigmaTilingClass))


typedef struct _LigmaTilingClass LigmaTilingClass;

struct _LigmaTiling
{
  LigmaSymmetry  parent_instance;

  gdouble       interval_x;
  gdouble       interval_y;
  gdouble       shift;
  gint          max_x;
  gint          max_y;
};

struct _LigmaTilingClass
{
  LigmaSymmetryClass  parent_class;
};


GType   ligma_tiling_get_type (void) G_GNUC_CONST;


#endif  /*  __LIGMA_TILING_H__  */
