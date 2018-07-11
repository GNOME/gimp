/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2003 Peter Mattis and Spencer Kimball
 *
 * gimppatterns.c
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
#include "gimppatterns.h"

/**
 * gimp_patterns_set_pattern:
 * @name: The pattern name.
 *
 * This procedure is deprecated! Use gimp_context_set_pattern() instead.
 *
 * Returns: TRUE on success.
 */
gboolean
gimp_patterns_set_pattern (const gchar *name)
{
  return gimp_context_set_pattern (name);
}
