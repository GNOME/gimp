/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include "gimppaintbrush.h"


#define GIMP_TYPE_PENCIL (gimp_pencil_get_type ())
G_DECLARE_DERIVABLE_TYPE (GimpPencil,
                          gimp_pencil,
                          GIMP, PENCIL,
                          GimpPaintbrush)


struct _GimpPencilClass
{
  GimpPaintbrushClass  parent_class;
};


void   gimp_pencil_register (Gimp                      *gimp,
                             GimpPaintRegisterCallback  callback);
