/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmatoolpalette.h
 * Copyright (C) 2010 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_TOOL_PALETTE_H__
#define __LIGMA_TOOL_PALETTE_H__


#define LIGMA_TYPE_TOOL_PALETTE            (ligma_tool_palette_get_type ())
#define LIGMA_TOOL_PALETTE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_TOOL_PALETTE, LigmaToolPalette))
#define LIGMA_TOOL_PALETTE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_TOOL_PALETTE, LigmaToolPaletteClass))
#define LIGMA_IS_TOOL_PALETTE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_TOOL_PALETTE))
#define LIGMA_IS_TOOL_PALETTE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_TOOL_PALETTE))
#define LIGMA_TOOL_PALETTE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_TOOL_PALETTE, LigmaToolPaletteClass))


typedef struct _LigmaToolPaletteClass LigmaToolPaletteClass;

struct _LigmaToolPalette
{
  GtkToolPalette  parent_instance;
};

struct _LigmaToolPaletteClass
{
  GtkToolPaletteClass  parent_class;
};


GType       ligma_tool_palette_get_type        (void) G_GNUC_CONST;

GtkWidget * ligma_tool_palette_new             (void);
void        ligma_tool_palette_set_toolbox     (LigmaToolPalette   *palette,
                                               LigmaToolbox       *toolbox);
gboolean    ligma_tool_palette_get_button_size (LigmaToolPalette   *palette,
                                               gint              *width,
                                               gint              *height);


#endif /* __LIGMA_TOOL_PALETTE_H__ */
