/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpmisc.c
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
#include "gimpmisc.h"


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

struct _GimpRgnIterator
{
  GimpDrawable *drawable;
  gint                 x1, y1, x2, y2;
  GimpRunMode   run_mode;
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
  pf->mode            = GIMP_PIXEL_FETCHER_EDGE_NONE;
  pf->drawable      = drawable;
  pf->tile          = NULL;
  pf->tile_dirty    = FALSE;
  pf->shadow            = FALSE;

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

GimpRgnIterator*
gimp_rgn_iterator_new (GimpDrawable *drawable,
                       GimpRunMode   run_mode)
{
  GimpRgnIterator *iter = g_new (GimpRgnIterator, 1);

  iter->drawable = drawable;
  iter->run_mode = run_mode;
  gimp_drawable_mask_bounds (drawable->drawable_id,
                             &iter->x1, &iter->y1,
                             &iter->x2, &iter->y2);

  return iter;
}

void
gimp_rgn_iterator_free (GimpRgnIterator *iter)
{
  g_free (iter);
}

static void
gimp_rgn_iterator_iter_single (GimpRgnIterator *iter,
                               GimpPixelRgn    *srcPR,
                               GimpRgnFuncSrc   func,
                               gpointer         data)
{
  gpointer  pr;
  gint      total_area;
  gint      area_so_far;

  total_area = (iter->x2 - iter->x1) * (iter->y2 - iter->y1);
  area_so_far   = 0;

  for (pr = gimp_pixel_rgns_register (1, srcPR);
       pr != NULL;
       pr = gimp_pixel_rgns_process (pr))
    {
      guchar *src = srcPR->data;
      gint    y;

      for (y = srcPR->y; y < srcPR->y + srcPR->h; y++)
        {
          guchar *s = src;
          gint x;

          for (x = srcPR->x; x < srcPR->x + srcPR->w; x++)
            {
              func (x, y, s, srcPR->bpp, data);
              s += srcPR->bpp;
            }

          src += srcPR->rowstride;
        }

      if (iter->run_mode != GIMP_RUN_NONINTERACTIVE)
        {
          area_so_far += srcPR->w * srcPR->h;
          gimp_progress_update ((gdouble) area_so_far /        (gdouble) total_area);
        }
    }
}

void
gimp_rgn_iterator_src (GimpRgnIterator *iter,
                       GimpRgnFuncSrc   func,
                       gpointer         data)
{
  GimpPixelRgn srcPR;

  gimp_pixel_rgn_init (&srcPR, iter->drawable,
                       iter->x1, iter->y1,
                       iter->x2 - iter->x1, iter->y2 - iter->y1,
                       FALSE, FALSE);
  gimp_rgn_iterator_iter_single (iter, &srcPR, func, data);
}

void
gimp_rgn_iterator_src_dest (GimpRgnIterator    *iter,
                            GimpRgnFuncSrcDest  func,
                            gpointer            data)
{
  GimpPixelRgn  srcPR, destPR;
  gint          x1, y1, x2, y2;
  gint                bpp;
  gpointer      pr;
  gint          total_area;
  gint          area_so_far;

  x1 = iter->x1;
  y1 = iter->y1;
  x2 = iter->x2;
  y2 = iter->y2;

  total_area = (x2 - x1) * (y2 - y1);
  area_so_far   = 0;

  gimp_pixel_rgn_init (&srcPR, iter->drawable, x1, y1, x2 - x1, y2 - y1,
                       FALSE, FALSE);
  gimp_pixel_rgn_init (&destPR, iter->drawable, x1, y1, x2 - x1, y2 - y1,
                       TRUE, TRUE);

  bpp = srcPR.bpp;

  for (pr = gimp_pixel_rgns_register (2, &srcPR, &destPR);
       pr != NULL;
       pr = gimp_pixel_rgns_process (pr))
    {
      gint    y;
      guchar* src  = srcPR.data;
      guchar* dest = destPR.data;

      for (y = srcPR.y; y < srcPR.y + srcPR.h; y++)
        {
          gint x;
          guchar *s = src;
          guchar *d = dest;

          for (x = srcPR.x; x < srcPR.x + srcPR.w; x++)
            {
              func (x, y, s, d, bpp, data);
              s += bpp;
              d += bpp;
            }

          src  += srcPR.rowstride;
          dest += destPR.rowstride;
        }

      if (iter->run_mode != GIMP_RUN_NONINTERACTIVE)
        {
          area_so_far += srcPR.w * srcPR.h;
          gimp_progress_update ((gdouble) area_so_far /
                                (gdouble) total_area);
        }
    }

  gimp_drawable_flush (iter->drawable);
  gimp_drawable_merge_shadow (iter->drawable->drawable_id, TRUE);
  gimp_drawable_update (iter->drawable->drawable_id,
                        x1, y1, x2 - x1, y2 - y1);
}

void
gimp_rgn_iterator_dest (GimpRgnIterator *iter,
                        GimpRgnFuncDest  func,
                        gpointer         data)
{
  GimpPixelRgn destPR;

  gimp_pixel_rgn_init (&destPR, iter->drawable,
                       iter->x1, iter->y1,
                       iter->x2 - iter->x1, iter->y2 - iter->y1,
                       TRUE, TRUE);
  gimp_rgn_iterator_iter_single (iter, &destPR, (GimpRgnFuncSrc) func, data);

  /*  update the processed region  */
  gimp_drawable_flush (iter->drawable);
  gimp_drawable_merge_shadow (iter->drawable->drawable_id, TRUE);
  gimp_drawable_update (iter->drawable->drawable_id,
                        iter->x1, iter->y1,
                        iter->x2 - iter->x1, iter->y2 - iter->y1);
}

static void
gimp_rgn_render_row (guchar       *src,
                     guchar       *dest,
                     gint          col,    /* row width in pixels */
                     gint            bpp,
                     GimpRgnFunc2  func,
                     gpointer      data)
{
  while (col--)
    {
      func (src, dest, bpp, data);
      src += bpp;
      dest += bpp;
    }
}

static void
gimp_rgn_render_region (const GimpPixelRgn *srcPR,
                        const GimpPixelRgn *destPR,
                        GimpRgnFunc2        func,
                        gpointer            data)
{
  gint    row;
  guchar *src  = srcPR->data;
  guchar *dest = destPR->data;

  for (row = 0; row < srcPR->h; row++)
    {
      gimp_rgn_render_row (src, dest, srcPR->w, srcPR->bpp, func, data);

      src  += srcPR->rowstride;
      dest += destPR->rowstride;
    }
}

void
gimp_rgn_iterate1 (GimpDrawable *drawable,
                   GimpRunMode   run_mode,
                   GimpRgnFunc1  func,
                   gpointer      data)
{
  GimpPixelRgn  srcPR;
  gint          x1, y1, x2, y2;
  gpointer      pr;
  gint          total_area;
  gint          area_so_far;
  gint          progress_skip;

  gimp_drawable_mask_bounds (drawable->drawable_id, &x1, &y1, &x2, &y2);

  total_area = (x2 - x1) * (y2 - y1);

  area_so_far   = 0;
  progress_skip = 0;

  gimp_pixel_rgn_init (&srcPR, drawable,
                       x1, y1, (x2 - x1), (y2 - y1), FALSE, FALSE);

  for (pr = gimp_pixel_rgns_register (1, &srcPR);
       pr != NULL;
       pr = gimp_pixel_rgns_process (pr))
    {
      guchar *src = srcPR.data;
      gint    y;

      for (y = 0; y < srcPR.h; y++)
        {
          guchar *s = src;
          gint    x;

          for (x = 0; x < srcPR.w; x++)
            {
              func (s, srcPR.bpp, data);
              s += srcPR.bpp;
            }

          src += srcPR.rowstride;
        }

      if (run_mode != GIMP_RUN_NONINTERACTIVE)
        {
          area_so_far += srcPR.w * srcPR.h;

          if (((progress_skip++) % 10) == 0)
            gimp_progress_update ((gdouble) area_so_far /
                                  (gdouble) total_area);
        }
    }
}

void
gimp_rgn_iterate2 (GimpDrawable *drawable,
                   GimpRunMode   run_mode,
                   GimpRgnFunc2  func,
                   gpointer      data)
{
  GimpPixelRgn  srcPR, destPR;
  gint          x1, y1, x2, y2;
  gpointer      pr;
  gint          total_area;
  gint          area_so_far;
  gint          progress_skip;

  gimp_drawable_mask_bounds (drawable->drawable_id, &x1, &y1, &x2, &y2);

  total_area = (x2 - x1) * (y2 - y1);

  area_so_far   = 0;
  progress_skip = 0;

  /* Initialize the pixel regions. */
  gimp_pixel_rgn_init (&srcPR, drawable, x1, y1, (x2 - x1), (y2 - y1),
                       FALSE, FALSE);
  gimp_pixel_rgn_init (&destPR, drawable, x1, y1, (x2 - x1), (y2 - y1),
                       TRUE, TRUE);

  for (pr = gimp_pixel_rgns_register (2, &srcPR, &destPR);
       pr != NULL;
       pr = gimp_pixel_rgns_process (pr))
    {
      gimp_rgn_render_region (&srcPR, &destPR, func, data);

      if (run_mode != GIMP_RUN_NONINTERACTIVE)
        {
          area_so_far += srcPR.w * srcPR.h;

          if (((progress_skip++) % 10) == 0)
            gimp_progress_update ((gdouble) area_so_far /
                                  (gdouble) total_area);
        }
    }

  /*  update the processed region  */
  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->drawable_id, TRUE);
  gimp_drawable_update (drawable->drawable_id, x1, y1, (x2 - x1), (y2 - y1));
}
