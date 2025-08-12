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


#define GIMP_TYPE_CONTROLLER_WHEEL            (gimp_controller_wheel_get_type ())
#define GIMP_CONTROLLER_WHEEL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_CONTROLLER_WHEEL, GimpControllerWheel))
#define GIMP_CONTROLLER_WHEEL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_CONTROLLER_WHEEL, GimpControllerWheelClass))
#define GIMP_IS_CONTROLLER_WHEEL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_CONTROLLER_WHEEL))
#define GIMP_IS_CONTROLLER_WHEEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_CONTROLLER_WHEEL))
#define GIMP_CONTROLLER_WHEEL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_CONTROLLER_WHEEL, GimpControllerWheelClass))


typedef struct _GimpControllerWheelClass GimpControllerWheelClass;

struct _GimpControllerWheel
{
  GimpController parent_instance;
};

struct _GimpControllerWheelClass
{
  GimpControllerClass parent_class;
};


GType      gimp_controller_wheel_get_type (void) G_GNUC_CONST;

gboolean   gimp_controller_wheel_scroll   (GimpControllerWheel  *wheel,
                                           const GdkEventScroll *sevent);
