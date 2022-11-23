/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-2002 Spencer Kimball, Peter Mattis, and others
 *
 * ligma-gradients.c
 * Copyright (C) 2014 Michael Natterer  <mitch@ligma.org>
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

#include "libligmabase/ligmabase.h"

#include "core-types.h"

#include "ligma.h"
#include "ligma-palettes.h"
#include "ligmacontext.h"
#include "ligmacontainer.h"
#include "ligmadatafactory.h"
#include "ligmapalettemru.h"

#include "ligma-intl.h"


#define COLOR_HISTORY_KEY "ligma-palette-color-history"


/*  local function prototypes  */

static LigmaPalette * ligma_palettes_add_palette (Ligma        *ligma,
                                                const gchar *name,
                                                const gchar *id);


/*  public functions  */

void
ligma_palettes_init (Ligma *ligma)
{
  LigmaPalette *palette;

  g_return_if_fail (LIGMA_IS_LIGMA (ligma));

  palette = ligma_palettes_add_palette (ligma,
                                       _("Color History"),
                                       COLOR_HISTORY_KEY);
  ligma_context_set_palette (ligma->user_context, palette);
}

void
ligma_palettes_load (Ligma *ligma)
{
  LigmaPalette *palette;
  GFile       *file;

  g_return_if_fail (LIGMA_IS_LIGMA (ligma));

  palette = ligma_palettes_get_color_history (ligma);

  file = ligma_directory_file ("colorrc", NULL);

  if (ligma->be_verbose)
    g_print ("Parsing '%s'\n", ligma_file_get_utf8_name (file));

  ligma_palette_mru_load (LIGMA_PALETTE_MRU (palette), file);

  g_object_unref (file);
}

void
ligma_palettes_save (Ligma *ligma)
{
  LigmaPalette *palette;
  GFile       *file;

  g_return_if_fail (LIGMA_IS_LIGMA (ligma));

  palette = ligma_palettes_get_color_history (ligma);

  file = ligma_directory_file ("colorrc", NULL);

  if (ligma->be_verbose)
    g_print ("Writing '%s'\n", ligma_file_get_utf8_name (file));

  ligma_palette_mru_save (LIGMA_PALETTE_MRU (palette), file);

  g_object_unref (file);
}

LigmaPalette *
ligma_palettes_get_color_history (Ligma *ligma)
{
  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);

  return g_object_get_data (G_OBJECT (ligma), COLOR_HISTORY_KEY);
}

void
ligma_palettes_add_color_history (Ligma          *ligma,
                                 const LigmaRGB *color)
{
  LigmaPalette *history;

  history = ligma_palettes_get_color_history (ligma);
  ligma_palette_mru_add (LIGMA_PALETTE_MRU (history), color);
}

/*  private functions  */

static LigmaPalette *
ligma_palettes_add_palette (Ligma        *ligma,
                           const gchar *name,
                           const gchar *id)
{
  LigmaData *palette;

  palette = ligma_palette_mru_new (name);

  ligma_data_make_internal (palette, id);

  ligma_container_add (ligma_data_factory_get_container (ligma->palette_factory),
                      LIGMA_OBJECT (palette));
  g_object_unref (palette);

  g_object_set_data (G_OBJECT (ligma), id, palette);

  return LIGMA_PALETTE (palette);
}
