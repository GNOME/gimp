/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2003 Peter Mattis and Spencer Kimball
 *
 * gimppalettes.c
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
#include "gimppalettes.h"

/**
 * gimp_palettes_set_palette:
 * @name: The palette name.
 *
 * This procedure is deprecated! Use gimp_context_set_palette() instead.
 *
 * Returns: TRUE on success.
 */
gboolean
gimp_palettes_set_palette (const gchar *name)
{
  return gimp_context_set_palette (name);
}
