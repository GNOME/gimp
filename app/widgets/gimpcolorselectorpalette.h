/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpcolorselectorpalette.h
 * Copyright (C) 2006 Michael Natterer <mitch@gimp.org>
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

#pragma once


#define GIMP_TYPE_COLOR_SELECTOR_PALETTE            (gimp_color_selector_palette_get_type ())
#define GIMP_COLOR_SELECTOR_PALETTE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_COLOR_SELECTOR_PALETTE, GimpColorSelectorPalette))
#define GIMP_IS_COLOR_SELECTOR_PALETTE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_COLOR_SELECTOR_PALETTE))
#define GIMP_COLOR_SELECTOR_PALETTE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_COLOR_SELECTOR_PALETTE, GimpColorSelectorPaletteClass))
#define GIMP_IS_COLOR_SELECTOR_PALETTE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_COLOR_SELECTOR_PALETTE))
#define GIMP_COLOR_SELECTOR_PALETTE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_COLOR_SELECTOR_PALETTE, GimpColorSelectorPaletteClass))


typedef struct _GimpColorSelectorPalette      GimpColorSelectorPalette;
typedef struct _GimpColorSelectorPaletteClass GimpColorSelectorPaletteClass;

struct _GimpColorSelectorPalette
{
  GimpColorSelector  parent_instance;

  GimpContext       *context;
  GtkWidget         *view;
  GtkWidget         *name_label;
};

struct _GimpColorSelectorPaletteClass
{
  GimpColorSelectorClass  parent_class;
};


GType   gimp_color_selector_palette_get_type (void) G_GNUC_CONST;
