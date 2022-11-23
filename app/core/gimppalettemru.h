/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmapalettemru.h
 * Copyright (C) 2014 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_PALETTE_MRU_H__
#define __LIGMA_PALETTE_MRU_H__


#include "ligmapalette.h"


#define LIGMA_TYPE_PALETTE_MRU            (ligma_palette_mru_get_type ())
#define LIGMA_PALETTE_MRU(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_PALETTE_MRU, LigmaPaletteMru))
#define LIGMA_PALETTE_MRU_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_PALETTE_MRU, LigmaPaletteMruClass))
#define LIGMA_IS_PALETTE_MRU(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_PALETTE_MRU))
#define LIGMA_IS_PALETTE_MRU_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_PALETTE_MRU))
#define LIGMA_PALETTE_MRU_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_PALETTE_MRU, LigmaPaletteMruClass))


typedef struct _LigmaPaletteMruClass LigmaPaletteMruClass;

struct _LigmaPaletteMru
{
  LigmaPalette  parent_instance;
};

struct _LigmaPaletteMruClass
{
  LigmaPaletteClass  parent_class;
};


GType      ligma_palette_mru_get_type (void) G_GNUC_CONST;

LigmaData * ligma_palette_mru_new      (const gchar    *name);

void       ligma_palette_mru_load     (LigmaPaletteMru *mru,
                                      GFile          *file);
void       ligma_palette_mru_save     (LigmaPaletteMru *mru,
                                      GFile          *file);

void       ligma_palette_mru_add      (LigmaPaletteMru *mru,
                                      const LigmaRGB  *color);


#endif  /*  __LIGMA_PALETTE_MRU_H__  */
