/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmacolorselectorpalette.h
 * Copyright (C) 2006 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_COLOR_SELECTOR_PALETTE_H__
#define __LIGMA_COLOR_SELECTOR_PALETTE_H__


#define LIGMA_TYPE_COLOR_SELECTOR_PALETTE            (ligma_color_selector_palette_get_type ())
#define LIGMA_COLOR_SELECTOR_PALETTE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_COLOR_SELECTOR_PALETTE, LigmaColorSelectorPalette))
#define LIGMA_IS_COLOR_SELECTOR_PALETTE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_COLOR_SELECTOR_PALETTE))
#define LIGMA_COLOR_SELECTOR_PALETTE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_COLOR_SELECTOR_PALETTE, LigmaColorSelectorPaletteClass))
#define LIGMA_IS_COLOR_SELECTOR_PALETTE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_COLOR_SELECTOR_PALETTE))
#define LIGMA_COLOR_SELECTOR_PALETTE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_COLOR_SELECTOR_PALETTE, LigmaColorSelectorPaletteClass))


typedef struct _LigmaColorSelectorPalette      LigmaColorSelectorPalette;
typedef struct _LigmaColorSelectorPaletteClass LigmaColorSelectorPaletteClass;

struct _LigmaColorSelectorPalette
{
  LigmaColorSelector  parent_instance;

  LigmaContext       *context;
  GtkWidget         *view;
};

struct _LigmaColorSelectorPaletteClass
{
  LigmaColorSelectorClass  parent_class;
};


GType   ligma_color_selector_palette_get_type (void) G_GNUC_CONST;


#endif /* __LIGMA_COLOR_SELECTOR_PALETTE_H__ */
