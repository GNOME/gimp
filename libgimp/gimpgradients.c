/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2003 Peter Mattis and Spencer Kimball
 *
 * gimpgradients.c
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

#include <string.h>

#include "gimp.h"
#include "gimpgradients.h"

/**
 * gimp_gradients_get_gradient:
 *
 * This procedure is deprecated! Use gimp_context_get_gradient() instead.
 *
 * Returns: The name of the active gradient.
 */
gchar *
gimp_gradients_get_gradient (void)
{
  return gimp_context_get_gradient ();
}

/**
 * gimp_gradients_set_gradient:
 * @name: The name of the gradient to set.
 *
 * This procedure is deprecated! Use gimp_context_set_gradient() instead.
 *
 * Returns: TRUE on success.
 */
gboolean
gimp_gradients_set_gradient (const gchar *name)
{
  return gimp_context_set_gradient (name);
}
