/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-2002 Spencer Kimball, Peter Mattis, and others
 *
 * gimp-gradients.c
 * Copyright (C) 2014 Michael Natterer  <mitch@gimp.org>
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

#include "config.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "gimp.h"
#include "gimp-palettes.h"
#include "gimpcontext.h"
#include "gimpcontainer.h"
#include "gimpdatafactory.h"
#include "gimppalettemru.h"

#include "gimp-intl.h"


#define COLOR_HISTORY_KEY "gimp-palette-color-history"


/*  local function prototypes  */

static GimpPalette * gimp_palettes_add_palette (Gimp        *gimp,
                                                const gchar *name,
                                                const gchar *id);


/*  public functions  */

void
gimp_palettes_init (Gimp *gimp)
{
  GimpPalette *palette;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  palette = gimp_palettes_add_palette (gimp,
                                       _("Color History"),
                                       COLOR_HISTORY_KEY);
  gimp_context_set_palette (gimp->user_context, palette);
}

void
gimp_palettes_load (Gimp *gimp)
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

void
gimp_palettes_save (Gimp *gimp)
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

GimpPalette *
gimp_palettes_get_color_history (Gimp *gimp)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  return g_object_get_data (G_OBJECT (gimp), COLOR_HISTORY_KEY);
}

void
gimp_palettes_add_color_history (Gimp      *gimp,
                                 GeglColor *color)
{
  GimpPalette *history;

  history = gimp_palettes_get_color_history (gimp);
  gimp_palette_mru_add (GIMP_PALETTE_MRU (history), color);
}

/*  private functions  */

static GimpPalette *
gimp_palettes_add_palette (Gimp        *gimp,
                           const gchar *name,
                           const gchar *id)
{
  GimpData *palette;

  palette = gimp_palette_mru_new (name);

  gimp_data_make_internal (palette, id);

  gimp_container_add (gimp_data_factory_get_container (gimp->palette_factory),
                      GIMP_OBJECT (palette));
  g_object_unref (palette);

  g_object_set_data (G_OBJECT (gimp), id, palette);

  return GIMP_PALETTE (palette);
}
