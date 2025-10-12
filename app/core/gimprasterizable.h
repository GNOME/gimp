/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimprasterizable.h
 * Copyright (C) 2025 Jehan
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

#include "gimpdrawable.h"

#define GIMP_TYPE_RASTERIZABLE (gimp_rasterizable_get_type ())
G_DECLARE_INTERFACE (GimpRasterizable, gimp_rasterizable, GIMP, RASTERIZABLE, GimpDrawable)


struct _GimpRasterizableInterface
{
  GTypeInterface base_iface;

  /*  signals            */
  void   (* set_rasterized)                      (GimpRasterizable *rasterizable,
                                                  gboolean          rasterized);
};


void       gimp_rasterizable_rasterize           (GimpRasterizable *rasterizable);
void       gimp_rasterizable_restore             (GimpRasterizable *rasterizable);

gboolean   gimp_rasterizable_is_rasterized       (GimpRasterizable *rasterizable);

/* To be used for undo steps only */

void       gimp_rasterizable_set_undo_rasterized (GimpRasterizable *rasterizable,
                                                  gboolean          rasterize);
