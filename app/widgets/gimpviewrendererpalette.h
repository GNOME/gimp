/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpviewrendererpalette.h
 * Copyright (C) 2005 Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_VIEW_RENDERER_PALETTE_H__
#define __GIMP_VIEW_RENDERER_PALETTE_H__

#include "gimpviewrenderer.h"

#define GIMP_TYPE_VIEW_RENDERER_PALETTE            (gimp_view_renderer_palette_get_type ())
#define GIMP_VIEW_RENDERER_PALETTE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_VIEW_RENDERER_PALETTE, GimpViewRendererPalette))
#define GIMP_VIEW_RENDERER_PALETTE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_VIEW_RENDERER_PALETTE, GimpViewRendererPaletteClass))
#define GIMP_IS_VIEW_RENDERER_PALETTE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, GIMP_TYPE_VIEW_RENDERER_PALETTE))
#define GIMP_IS_VIEW_RENDERER_PALETTE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_VIEW_RENDERER_PALETTE))
#define GIMP_VIEW_RENDERER_PALETTE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_VIEW_RENDERER_PALETTE, GimpViewRendererPaletteClass))


typedef struct _GimpViewRendererPaletteClass  GimpViewRendererPaletteClass;

struct _GimpViewRendererPalette
{
  GimpViewRenderer  parent_instance;

  gint              cell_size;
  gboolean          draw_grid;

  gint              cell_width;
  gint              cell_height;
  gint              columns;
  gint              rows;
};

struct _GimpViewRendererPaletteClass
{
  GimpViewRendererClass  parent_class;
};


GType   gimp_view_renderer_palette_get_type    (void) G_GNUC_CONST;

void    gimp_view_renderer_palette_set_cell_size (GimpViewRendererPalette *renderer,
                                                  gint                     cell_size);
void    gimp_view_renderer_palette_set_draw_grid (GimpViewRendererPalette *renderer,
                                                  gboolean                 draw_grid);


#endif /* __GIMP_VIEW_RENDERER_PALETTE_H__ */
