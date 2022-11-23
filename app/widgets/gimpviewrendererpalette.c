/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaviewrendererpalette.c
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

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmacolor/ligmacolor.h"
#include "libligmamath/ligmamath.h"
#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "core/ligmapalette.h"

#include "ligmaviewrendererpalette.h"


#define COLUMNS 16


static void   ligma_view_renderer_palette_finalize (GObject          *object);

static void   ligma_view_renderer_palette_render   (LigmaViewRenderer *renderer,
                                                   GtkWidget        *widget);


G_DEFINE_TYPE (LigmaViewRendererPalette, ligma_view_renderer_palette,
               LIGMA_TYPE_VIEW_RENDERER)

#define parent_class ligma_view_renderer_palette_parent_class


static void
ligma_view_renderer_palette_class_init (LigmaViewRendererPaletteClass *klass)
{
  GObjectClass          *object_class   = G_OBJECT_CLASS (klass);
  LigmaViewRendererClass *renderer_class = LIGMA_VIEW_RENDERER_CLASS (klass);

  object_class->finalize = ligma_view_renderer_palette_finalize;

  renderer_class->render = ligma_view_renderer_palette_render;
}

static void
ligma_view_renderer_palette_init (LigmaViewRendererPalette *renderer)
{
  renderer->cell_size = 4;
  renderer->draw_grid = FALSE;
  renderer->columns   = COLUMNS;
}

static void
ligma_view_renderer_palette_finalize (GObject *object)
{
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_view_renderer_palette_render (LigmaViewRenderer *renderer,
                                   GtkWidget        *widget)
{
  LigmaViewRendererPalette *renderpal = LIGMA_VIEW_RENDERER_PALETTE (renderer);
  LigmaPalette             *palette;
  LigmaColorTransform      *transform;
  guchar                  *row;
  guchar                  *dest;
  GList                   *list;
  gdouble                  cell_width;
  gint                     grid_width;
  gint                     dest_stride;
  gint                     y;

  palette = LIGMA_PALETTE (renderer->viewable);

  if (ligma_palette_get_n_colors (palette) == 0)
    return;

  grid_width = renderpal->draw_grid ? 1 : 0;

  if (renderpal->cell_size > 0)
    {
      gint n_columns = ligma_palette_get_columns (palette);

      if (n_columns > 0)
        cell_width = MAX ((gdouble) renderpal->cell_size,
                          (gdouble) (renderer->width - grid_width) /
                          (gdouble) n_columns);
      else
        cell_width = renderpal->cell_size;
    }
  else
    {
      gint n_columns = ligma_palette_get_columns (palette);

      if (n_columns > 0)
        cell_width = ((gdouble) (renderer->width - grid_width) /
                      (gdouble) n_columns);
      else
        cell_width = (gdouble) (renderer->width - grid_width) / 16.0;
    }

  cell_width = MAX (4.0, cell_width);

  renderpal->cell_width = cell_width;

  renderpal->columns = (gdouble) (renderer->width - grid_width) / cell_width;

  renderpal->rows = ligma_palette_get_n_colors (palette) / renderpal->columns;
  if (ligma_palette_get_n_colors (palette) % renderpal->columns)
    renderpal->rows += 1;

  renderpal->cell_height = MAX (4, ((renderer->height - grid_width) /
                                    renderpal->rows));

  if (! renderpal->draw_grid)
    renderpal->cell_height = MIN (renderpal->cell_height,
                                  renderpal->cell_width);

  list = ligma_palette_get_colors (palette);

  if (! renderer->surface)
    renderer->surface = cairo_image_surface_create (CAIRO_FORMAT_RGB24,
                                                    renderer->width,
                                                    renderer->height);

  cairo_surface_flush (renderer->surface);

  row = g_new (guchar, renderer->width * 4);

  dest        = cairo_image_surface_get_data (renderer->surface);
  dest_stride = cairo_image_surface_get_stride (renderer->surface);

  transform = ligma_view_renderer_get_color_transform (renderer, widget,
                                                      babl_format ("cairo-RGB24"),
                                                      babl_format ("cairo-RGB24"));

  for (y = 0; y < renderer->height; y++)
    {
      if ((y % renderpal->cell_height) == 0)
        {
          guchar  r, g, b;
          gint    x;
          gint    n = 0;
          guchar *d = row;

          memset (row, renderpal->draw_grid ? 0 : 255, renderer->width * 4);

          r = g = b = (renderpal->draw_grid ? 0 : 255);

          for (x = 0; x < renderer->width; x++, d += 4)
            {
              if ((x % renderpal->cell_width) == 0)
                {
                  if (list && n < renderpal->columns &&
                      renderer->width - x >= renderpal->cell_width)
                    {
                      LigmaPaletteEntry *entry = list->data;

                      list = g_list_next (list);
                      n++;

                      ligma_rgb_get_uchar (&entry->color, &r, &g, &b);
                    }
                  else
                    {
                      r = g = b = (renderpal->draw_grid ? 0 : 255);
                    }
                }

              if (renderpal->draw_grid && (x % renderpal->cell_width) == 0)
                {
                  LIGMA_CAIRO_RGB24_SET_PIXEL (d, 0, 0, 0);
                }
              else
                {
                  LIGMA_CAIRO_RGB24_SET_PIXEL (d, r, g, b);
                }
            }
        }

      if (renderpal->draw_grid && (y % renderpal->cell_height) == 0)
        {
          memset (dest, 0, renderer->width * 4);
        }
      else
        {
          if (transform)
            {
              ligma_color_transform_process_pixels (transform,
                                                   babl_format ("cairo-RGB24"),
                                                   row,
                                                   babl_format ("cairo-RGB24"),
                                                   dest,
                                                   renderer->width);
            }
          else
            {
              memcpy (dest, row, renderer->width * 4);
            }
        }

      dest += dest_stride;
    }

  g_free (row);

  cairo_surface_mark_dirty (renderer->surface);
}


/*  public functions  */

void
ligma_view_renderer_palette_set_cell_size (LigmaViewRendererPalette *renderer,
                                          gint                     cell_size)
{
  g_return_if_fail (LIGMA_IS_VIEW_RENDERER_PALETTE (renderer));

  if (cell_size != renderer->cell_size)
    {
      renderer->cell_size = cell_size;

      ligma_view_renderer_invalidate (LIGMA_VIEW_RENDERER (renderer));
    }
}

void
ligma_view_renderer_palette_set_draw_grid (LigmaViewRendererPalette *renderer,
                                          gboolean                 draw_grid)
{
  g_return_if_fail (LIGMA_IS_VIEW_RENDERER_PALETTE (renderer));

  if (draw_grid != renderer->draw_grid)
    {
      renderer->draw_grid = draw_grid ? TRUE : FALSE;

      ligma_view_renderer_invalidate (LIGMA_VIEW_RENDERER (renderer));
    }
}
