/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-2002 Spencer Kimball, Peter Mattis, and others
 *
 * ligma-palettes.h
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

#ifndef __LIGMA_PALETTES__
#define __LIGMA_PALETTES__


void          ligma_palettes_init              (Ligma          *ligma);

void          ligma_palettes_load              (Ligma          *ligma);
void          ligma_palettes_save              (Ligma          *ligma);

LigmaPalette * ligma_palettes_get_color_history (Ligma          *ligma);
void          ligma_palettes_add_color_history (Ligma          *ligma,
                                               const LigmaRGB *color);


#endif /* __LIGMA_PALETTES__ */
