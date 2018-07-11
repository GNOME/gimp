/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpcontrollermouse.h
 * Copyright (C) 2004 Michael Natterer <mitch@gimp.org>
 * Copyright (C) 2011 Mikael Magnusson <mikachu@src.gnome.org>
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

#ifndef __GIMP_CONTROLLER_MOUSE_H__
#define __GIMP_CONTROLLER_MOUSE_H__


#define GIMP_ENABLE_CONTROLLER_UNDER_CONSTRUCTION
#include "libgimpwidgets/gimpcontroller.h"


#define GIMP_TYPE_CONTROLLER_MOUSE            (gimp_controller_mouse_get_type ())
#define GIMP_CONTROLLER_MOUSE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_CONTROLLER_MOUSE, GimpControllerMouse))
#define GIMP_CONTROLLER_MOUSE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_CONTROLLER_MOUSE, GimpControllerMouseClass))
#define GIMP_IS_CONTROLLER_MOUSE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_CONTROLLER_MOUSE))
#define GIMP_IS_CONTROLLER_MOUSE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_CONTROLLER_MOUSE))
#define GIMP_CONTROLLER_MOUSE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_CONTROLLER_MOUSE, GimpControllerMouseClass))


typedef struct _GimpControllerMouseClass GimpControllerMouseClass;

struct _GimpControllerMouse
{
  GimpController parent_instance;
};

struct _GimpControllerMouseClass
{
  GimpControllerClass parent_class;
};


GType      gimp_controller_mouse_get_type (void) G_GNUC_CONST;

gboolean   gimp_controller_mouse_button   (GimpControllerMouse  *mouse,
                                           const GdkEventButton *bevent);


#endif /* __GIMP_CONTROLLER_MOUSE_H__ */
