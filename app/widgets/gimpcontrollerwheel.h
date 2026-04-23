/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpcontrollerwheel.h
 * Copyright (C) 2004 Michael Natterer <mitch@gimp.org>
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

#define GIMP_ENABLE_CONTROLLER_UNDER_CONSTRUCTION
#include "libgimpwidgets/gimpcontroller.h"


#define GIMP_TYPE_CONTROLLER_WHEEL (gimp_controller_wheel_get_type ())
G_DECLARE_FINAL_TYPE (GimpControllerWheel,
                      gimp_controller_wheel,
                      GIMP, CONTROLLER_WHEEL,
                      GimpController)


gboolean   gimp_controller_wheel_scroll (GimpControllerWheel  *wheel,
                                         const GdkEventScroll *sevent);
