/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimppixelfetcher.c
 *
 * FIXME: fix the following comment:
 * Contains all kinds of miscellaneous routines factored out from different
 * plug-ins. They stay here until their API has crystalized a bit and we can
 * put them into the file where they belong (Maurits Rijk
 * <lpeek.mrijk@consunet.nl> if you want to blame someone for this mess)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <stdio.h>

#include <glib.h>

#include "gimp.h"
#include "gimppixelfetcher.h"


struct _GimpPixelFetcher
{
  gint                      col, row;
  gint                      img_width;
  gint                      img_height;
  gint                      sel_x1, sel_y1, sel_x2, sel_y2;
  gint                      img_bpp;
  gint                      tile_width, tile_height;
  guchar                    bg_color[4];
  GimpPixelFetcherEdgeMode  mode;
  GimpDrawable             *drawable;
  GimpTile                 *tile;
  gboolean                  tile_dirty;
  gboolean                  shadow;
};

GimpPixelFetcher *
gimp_pixel_fetcher_new (GimpDrawable *drawable)
{
  GimpPixelFetcher *pf;

  pf = g_new (GimpPixelFetcher, 1);

  gimp_drawable_mask_bounds (drawable->drawable_id,
                             &pf->sel_x1, &pf->sel_y1,
                             &pf->sel_x2, &pf->sel_y2);

  pf->col           = -1;
  pf->row           = -1;
  pf->img_width     = gimp_drawable_width  (drawable->drawable_id);
  pf->img_height    = gimp_drawable_height (drawable->drawable_id);
  pf->img_bpp       = gimp_drawable_bpp    (drawable->drawable_id);
  pf->tile_width    = gimp_tile_width ();
  pf->tile_height   = gimp_tile_height ();
  pf->bg_color[0]   = 0;
  pf->bg_color[1]   = 0;
  pf->bg_color[2]   = 0;
  pf->bg_color[3]   = 255;
  pf->mode          = GIMP_PIXEL_FETCHER_EDGE_NONE;
  pf->drawable      = drawable;
  pf->tile          = NULL;
  pf->tile_dirty    = FALSE;
  pf->shadow        = FALSE;

  /* this allows us to use (slightly faster) do-while loops */
  g_assert (pf->img_bpp > 0);

  return pf;
}

void
gimp_pixel_fetcher_set_edge_mode (GimpPixelFetcher         *pf,
                                  GimpPixelFetcherEdgeMode  mode)
{
  pf->mode = mode;
}

void
gimp_pixel_fetcher_set_bg_color (GimpPixelFetcher *pf)
{
  GimpRGB background;

  gimp_palette_get_background (&background);

  switch (pf->img_bpp)
    {
    case 2: pf->bg_color[1] = 255;
    case 1:
      pf->bg_color[0] = gimp_rgb_intensity_uchar (&background);
      break;

    case 4: pf->bg_color[3] = 255;
    case 3:
      gimp_rgb_get_uchar (&background,
                          pf->bg_color, pf->bg_color + 1, pf->bg_color + 2);
      break;
    }
}

void
gimp_pixel_fetcher_set_shadow (GimpPixelFetcher *pf,
                               gboolean          shadow)
{
  pf->shadow = shadow;
}

static guchar *
gimp_pixel_fetcher_provide_tile (GimpPixelFetcher *pf,
                                 gint              x,
                                 gint              y)
{
  gint col, row;
  gint coloff, rowoff;

  col    = x / pf->tile_width;
  coloff = x % pf->tile_width;
  row    = y / pf->tile_height;
  rowoff = y % pf->tile_height;

  if ((col != pf->col) || (row != pf->row) || (pf->tile == NULL))
    {
      if (pf->tile != NULL)
        gimp_tile_unref (pf->tile, pf->tile_dirty);

      pf->tile = gimp_drawable_get_tile (pf->drawable, pf->shadow, row, col);
      pf->tile_dirty = FALSE;
      gimp_tile_ref (pf->tile);

      pf->col = col;
      pf->row = row;
    }

  return pf->tile->data + pf->img_bpp * (pf->tile->ewidth * rowoff + coloff);
}

void
gimp_pixel_fetcher_put_pixel (GimpPixelFetcher *pf,
                              gint              x,
                              gint              y,
                              const guchar     *pixel)
{
  guchar *p;
  gint    i;

  if (x < pf->sel_x1 || x >= pf->sel_x2 ||
      y < pf->sel_y1 || y >= pf->sel_y2)
    {
      return;
    }

  p = gimp_pixel_fetcher_provide_tile (pf, x, y);

  i = pf->img_bpp;
  do
    *p++ = *pixel++;
  while (--i);

  pf->tile_dirty = TRUE;
}

void
gimp_pixel_fetcher_get_pixel (GimpPixelFetcher *pf,
                              gint              x,
                              gint              y,
                              guchar           *pixel)
{
  guchar *p;
  gint    i;

  if (pf->mode == GIMP_PIXEL_FETCHER_EDGE_NONE &&
      (x < pf->sel_x1 || x >= pf->sel_x2 ||
       y < pf->sel_y1 || y >= pf->sel_y2))
    {
      return;
    }
  else if (x < 0 || x >= pf->img_width ||
           y < 0 || y >= pf->img_height)
    switch (pf->mode)
      {
      case GIMP_PIXEL_FETCHER_EDGE_WRAP:
        if (x < 0 || x >= pf->img_width)
          {
            x %= pf->img_width;
            if (x < 0)
              x += pf->img_width;
          }
        if (y < 0 || y >= pf->img_height)
          {
            y %= pf->img_height;
            if (y < 0)
              y += pf->img_height;
          }
        break;

      case GIMP_PIXEL_FETCHER_EDGE_SMEAR:
        x = CLAMP (x, 0, pf->img_width - 1);
        y = CLAMP (y, 0, pf->img_height - 1);
        break;

      case GIMP_PIXEL_FETCHER_EDGE_BLACK:
        if (x < 0 || x >= pf->img_width ||
            y < 0 || y >= pf->img_height)
          {
            i = pf->img_bpp;
            do
              pixel[i] = 0;
            while (--i);

            return;
          }
        break;

      default:
        return;
      }

  p = gimp_pixel_fetcher_provide_tile (pf, x, y);

  i = pf->img_bpp;
  do
    *pixel++ = *p++;
  while (--i);
}

void
gimp_pixel_fetcher_destroy (GimpPixelFetcher *pf)
{
  if (pf->tile)
    gimp_tile_unref (pf->tile, pf->tile_dirty);

  g_free (pf);
}

static void
gimp_get_color_guchar (GimpDrawable *drawable,
                       GimpRGB            *color,
                       gboolean      transparent,
                       guchar       *bg)
{
  switch (gimp_drawable_type (drawable->drawable_id))
    {
    case GIMP_RGB_IMAGE :
      gimp_rgb_get_uchar (color, &bg[0], &bg[1], &bg[2]);
      bg[3] = 255;
      break;

    case GIMP_RGBA_IMAGE:
      gimp_rgb_get_uchar (color, &bg[0], &bg[1], &bg[2]);
      bg[3] = transparent ? 0 : 255;
      break;

    case GIMP_GRAY_IMAGE:
      bg[0] = gimp_rgb_intensity_uchar (color);
      bg[1] = 255;
      break;

    case GIMP_GRAYA_IMAGE:
      bg[0] = gimp_rgb_intensity_uchar (color);
      bg[1] = transparent ? 0 : 255;
      break;

    default:
      break;
    }
}

void
gimp_get_bg_guchar (GimpDrawable *drawable,
                    gboolean      transparent,
                    guchar       *bg)
{
  GimpRGB  background;

  gimp_palette_get_background (&background);
  gimp_get_color_guchar (drawable, &background, transparent, bg);
}

void
gimp_get_fg_guchar (GimpDrawable *drawable,
                    gboolean      transparent,
                    guchar       *fg)
{
  GimpRGB  foreground;

  gimp_palette_get_foreground (&foreground);
  gimp_get_color_guchar (drawable, &foreground, transparent, fg);
}

