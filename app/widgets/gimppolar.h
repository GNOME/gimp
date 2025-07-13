/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimppolar.h
 * Copyright (C) 2014 Michael Natterer <mitch@gimp.org>
 *
 * Based on code from the color-rotate plug-in
 * Copyright (C) 1997-1999 Sven Anders (anderss@fmi.uni-passau.de)
 *                         Based on code from Pavel Grinfeld (pavel@ml.com)
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

#include "gimpcircle.h"


#define GIMP_TYPE_POLAR (gimp_polar_get_type ())
G_DECLARE_DERIVABLE_TYPE (GimpPolar,
                          gimp_polar,
                          GIMP, POLAR,
                          GimpCircle)


struct _GimpPolarClass
{
  GimpCircleClass  parent_class;
};


GType       gimp_polar_get_type (void) G_GNUC_CONST;

GtkWidget * gimp_polar_new      (void);
