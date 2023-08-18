/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimppalette.c
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

#include "gimppalette.h"

struct _GimpPalette
{
  GimpResource parent_instance;
};

G_DEFINE_TYPE (GimpPalette, gimp_palette, GIMP_TYPE_RESOURCE);


static void  gimp_palette_class_init (GimpPaletteClass *klass)
{
}

static void  gimp_palette_init (GimpPalette *palette)
{
}
