/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaviewrendererpalette.h
 * Copyright (C) 2005 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_VIEW_RENDERER_PALETTE_H__
#define __LIGMA_VIEW_RENDERER_PALETTE_H__

#include "ligmaviewrenderer.h"

#define LIGMA_TYPE_VIEW_RENDERER_PALETTE            (ligma_view_renderer_palette_get_type ())
#define LIGMA_VIEW_RENDERER_PALETTE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_VIEW_RENDERER_PALETTE, LigmaViewRendererPalette))
#define LIGMA_VIEW_RENDERER_PALETTE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_VIEW_RENDERER_PALETTE, LigmaViewRendererPaletteClass))
#define LIGMA_IS_VIEW_RENDERER_PALETTE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, LIGMA_TYPE_VIEW_RENDERER_PALETTE))
#define LIGMA_IS_VIEW_RENDERER_PALETTE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_VIEW_RENDERER_PALETTE))
#define LIGMA_VIEW_RENDERER_PALETTE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_VIEW_RENDERER_PALETTE, LigmaViewRendererPaletteClass))


typedef struct _LigmaViewRendererPaletteClass  LigmaViewRendererPaletteClass;

struct _LigmaViewRendererPalette
{
  LigmaViewRenderer  parent_instance;

  gint              cell_size;
  gboolean          draw_grid;

  gint              cell_width;
  gint              cell_height;
  gint              columns;
  gint              rows;
};

struct _LigmaViewRendererPaletteClass
{
  LigmaViewRendererClass  parent_class;
};


GType   ligma_view_renderer_palette_get_type    (void) G_GNUC_CONST;

void    ligma_view_renderer_palette_set_cell_size (LigmaViewRendererPalette *renderer,
                                                  gint                     cell_size);
void    ligma_view_renderer_palette_set_draw_grid (LigmaViewRendererPalette *renderer,
                                                  gboolean                 draw_grid);


#endif /* __LIGMA_VIEW_RENDERER_PALETTE_H__ */
