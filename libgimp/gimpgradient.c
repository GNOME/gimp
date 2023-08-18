/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpgradient.c
 * Copyright (C) 2023 Jehan
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "gimp.h"

#include "gimpgradient.h"

struct _GimpGradient
{
  GimpResource parent_instance;
};

G_DEFINE_TYPE (GimpGradient, gimp_gradient, GIMP_TYPE_RESOURCE);


static void  gimp_gradient_class_init (GimpGradientClass *klass)
{
}

static void  gimp_gradient_init (GimpGradient *gradient)
{
}
