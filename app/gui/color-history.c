/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * color-history.c
 * Copyright (C) 2002 Michael Natterer <mitch@gimp.org>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"

#include "gui-types.h"

#include "core/gimp.h"
#include "core/gimp-palettes.h"
#include "core/gimppalettemru.h"

#include "color-history.h"

#include "gimp-intl.h"


void
color_history_save (Gimp *gimp)
{
  GimpPalette *palette;
  GFile       *file;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  palette = gimp_palettes_get_color_history (gimp);

  file = gimp_directory_file ("colorrc", NULL);

  if (gimp->be_verbose)
    g_print ("Writing '%s'\n", gimp_file_get_utf8_name (file));

  gimp_palette_mru_save (GIMP_PALETTE_MRU (palette), file);

  g_object_unref (file);
}

void
color_history_restore (Gimp *gimp)
{
  GimpPalette *palette;
  GFile       *file;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  palette = gimp_palettes_get_color_history (gimp);

  file = gimp_directory_file ("colorrc", NULL);

  if (gimp->be_verbose)
    g_print ("Parsing '%s'\n", gimp_file_get_utf8_name (file));

  gimp_palette_mru_load (GIMP_PALETTE_MRU (palette), file);

  g_object_unref (file);
}
