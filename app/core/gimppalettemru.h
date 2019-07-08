/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimppalettemru.h
 * Copyright (C) 2014 Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_PALETTE_MRU_H__
#define __GIMP_PALETTE_MRU_H__


#include "gimppalette.h"


#define GIMP_TYPE_PALETTE_MRU            (gimp_palette_mru_get_type ())
#define GIMP_PALETTE_MRU(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_PALETTE_MRU, GimpPaletteMru))
#define GIMP_PALETTE_MRU_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_PALETTE_MRU, GimpPaletteMruClass))
#define GIMP_IS_PALETTE_MRU(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_PALETTE_MRU))
#define GIMP_IS_PALETTE_MRU_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_PALETTE_MRU))
#define GIMP_PALETTE_MRU_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_PALETTE_MRU, GimpPaletteMruClass))


typedef struct _GimpPaletteMruClass GimpPaletteMruClass;

struct _GimpPaletteMru
{
  GimpPalette  parent_instance;
};

struct _GimpPaletteMruClass
{
  GimpPaletteClass  parent_class;
};


GType      gimp_palette_mru_get_type (void) G_GNUC_CONST;

GimpData * gimp_palette_mru_new      (const gchar    *name);

void       gimp_palette_mru_load     (GimpPaletteMru *mru,
                                      GFile          *file);
void       gimp_palette_mru_save     (GimpPaletteMru *mru,
                                      GFile          *file);

void       gimp_palette_mru_add      (GimpPaletteMru *mru,
                                      const GimpRGB  *color);


#endif  /*  __GIMP_PALETTE_MRU_H__  */
