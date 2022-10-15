/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "gimpresource.h"

 /* This class exists to support gimp_param_spec_resource,
  * which is needed to communicate with libgimp.
  * So that gimp_param_spec_resource has a GIMP_TYPE_RESOURCE on the app side.
  *
  * This class on libgimp side is a superclass.
  * On app side of wire, it is not a superclass (e.g. GimpBrush does not inherit.)
  * On app side, the class is never inherited and never instantiated.
  *
  * The code is derived from libgimp/gimpresource.c
  */


G_DEFINE_TYPE (GimpResource, gimp_resource, G_TYPE_OBJECT)

/* Class construction */
static void
gimp_resource_class_init (GimpResourceClass *klass)
{
  g_debug("gimp_resource_class_init");
}

/* Instance construction. */
static void
gimp_resource_init (GimpResource *self)
{
  g_debug("gimp_resource_init");
}
