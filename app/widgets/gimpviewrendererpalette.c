/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpviewrendererpalette.c
 * Copyright (C) 2005 Michael Natterer <mitch@gimp.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"

#include "widgets-types.h"

#include "core/gimppalette.h"

#include "gimpviewrendererpalette.h"


#define COLUMNS 16


static void   gimp_view_renderer_palette_finalize (GObject          *object);

static void   gimp_view_renderer_palette_render   (GimpViewRenderer *renderer,
                                                   GtkWidget        *widget);


G_DEFINE_TYPE (GimpViewRendererPalette, gimp_view_renderer_palette,
               GIMP_TYPE_VIEW_RENDERER)

#define parent_class gimp_view_renderer_palette_parent_class


static void
gimp_view_renderer_palette_class_init (GimpViewRendererPaletteClass *klass)
{
  GObjectClass          *object_class   = G_OBJECT_CLASS (klass);
  GimpViewRendererClass *renderer_class = GIMP_VIEW_RENDERER_CLASS (klass);

  object_class->finalize = gimp_view_renderer_palette_finalize;

  renderer_class->render = gimp_view_renderer_palette_render;
}

static void
gimp_view_renderer_palette_init (GimpViewRendererPalette *renderer)
{
  renderer->cell_size = 4;
  renderer->draw_grid = FALSE;
  renderer->columns   = COLUMNS;
}

static void
gimp_view_renderer_palette_finalize (GObject *object)
{
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_view_renderer_palette_render (GimpViewRenderer *renderer,
                                   GtkWidget        *widget)
{
  GimpViewRendererPalette *renderpal = GIMP_VIEW_RENDERER_PALETTE (renderer);
  GimpPalette             *palette;
  guchar                  *row;
  guchar                  *dest_row;
  GList                   *list;
  gdouble                  cell_width;
  gint                     grid_width;
  gint                     y;

  palette = GIMP_PALETTE (renderer->viewable);

  if (palette->n_colors < 1)
    return;

  if (! renderer->buffer)
    renderer->buffer = g_new (guchar, renderer->height * renderer->rowstride);

  grid_width = renderpal->draw_grid ? 1 : 0;

  if (renderpal->cell_size > 0)
    {
      if (palette->n_columns > 0)
        cell_width = MAX ((gdouble) renderpal->cell_size,
                          (gdouble) (renderer->width - grid_width) /
                          (gdouble) palette->n_columns);
      else
        cell_width = renderpal->cell_size;
    }
  else
    {
      if (palette->n_columns > 0)
        cell_width = ((gdouble) (renderer->width - grid_width) /
                      (gdouble) palette->n_columns);
      else
        cell_width = (gdouble) (renderer->width - grid_width) / 16.0;
    }

  cell_width = MAX (4.0, cell_width);

  renderpal->cell_width = cell_width;

  renderpal->columns = (gdouble) (renderer->width - grid_width) / cell_width;

  renderpal->rows = palette->n_colors / renderpal->columns;
  if (palette->n_colors % renderpal->columns)
    renderpal->rows += 1;

  renderpal->cell_height = MAX (4, ((renderer->height - grid_width) /
                                    renderpal->rows));

  if (! renderpal->draw_grid)
    renderpal->cell_height = MIN (renderpal->cell_height,
                                  renderpal->cell_width);

  list = palette->colors;

  memset (renderer->buffer,
          renderpal->draw_grid ? 0 : 255,
          renderer->height * renderer->rowstride);

  row = g_new (guchar, renderer->rowstride);

  dest_row = renderer->buffer;

  for (y = 0; y < renderer->height; y++)
    {
      if ((y % renderpal->cell_height) == 0)
        {
          guchar  r, g, b;
          gint    x;
          gint    n = 0;
          guchar *p = row;

          memset (row,
                  renderpal->draw_grid ? 0 : 255,
                  renderer->rowstride);

          r = g = b = (renderpal->draw_grid ? 0 : 255);

          for (x = 0; x < renderer->width; x++)
            {
              if ((x % renderpal->cell_width) == 0)
                {
                  if (list && n < renderpal->columns &&
                      renderer->width - x >= renderpal->cell_width)
                    {
                      GimpPaletteEntry *entry = list->data;

                      list = g_list_next (list);
                      n++;

                      gimp_rgb_get_uchar (&entry->color, &r, &g, &b);
                    }
                  else
                    {
                      r = g = b = (renderpal->draw_grid ? 0 : 255);
                    }
                }

              if (renderpal->draw_grid && (x % renderpal->cell_width) == 0)
                {
                  *p++ = 0;
                  *p++ = 0;
                  *p++ = 0;
                }
              else
                {
                  *p++ = r;
                  *p++ = g;
                  *p++ = b;
                }
            }
        }

      if (renderpal->draw_grid && (y % renderpal->cell_height) == 0)
        {
          memset (dest_row, 0, renderer->rowstride);
        }
      else
        {
          memcpy (dest_row, row, renderer->rowstride);
        }

      dest_row += renderer->rowstride;
    }

  g_free (row);

  renderer->needs_render = FALSE;
}


/*  public functions  */

void
gimp_view_renderer_palette_set_cell_size (GimpViewRendererPalette *renderer,
                                          gint                     cell_size)
{
  g_return_if_fail (GIMP_IS_VIEW_RENDERER_PALETTE (renderer));

  if (cell_size != renderer->cell_size)
    {
      renderer->cell_size = cell_size;

      gimp_view_renderer_invalidate (GIMP_VIEW_RENDERER (renderer));
    }
}

void
gimp_view_renderer_palette_set_draw_grid (GimpViewRendererPalette *renderer,
                                          gboolean                 draw_grid)
{
  g_return_if_fail (GIMP_IS_VIEW_RENDERER_PALETTE (renderer));

  if (draw_grid != renderer->draw_grid)
    {
      renderer->draw_grid = draw_grid ? TRUE : FALSE;

      gimp_view_renderer_invalidate (GIMP_VIEW_RENDERER (renderer));
    }
}
