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

#include <stdio.h>

#include <glib.h>

#include <gtk/gtk.h>

#include "config.h"

#include "gimp.h"
#include "gimpmisc.h"

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
  pf->img_width     = gimp_drawable_width (drawable->drawable_id);
  pf->img_height    = gimp_drawable_height (drawable->drawable_id);
  pf->img_bpp       = gimp_drawable_bpp (drawable->drawable_id);
  pf->img_has_alpha = gimp_drawable_has_alpha (drawable->drawable_id);
  pf->tile_width    = gimp_tile_width ();
  pf->tile_height   = gimp_tile_height ();
  pf->bg_color[0]   = 0;
  pf->bg_color[1]   = 0;
  pf->bg_color[2]   = 0;
  pf->bg_color[3]   = 255;

  pf->drawable    = drawable;
  pf->tile        = NULL;
  pf->tile_dirty  = FALSE;
  pf->shadow	  = FALSE;

  return pf;
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
			       gboolean shadow)
{
  pf->shadow = shadow;
}

static guchar*
gimp_pixel_fetcher_provide_tile (GimpPixelFetcher *pf,
				 gint             x,
				 gint             y)
{
  gint    col, row;
  gint    coloff, rowoff;

  col    = x / pf->tile_width;
  coloff = x % pf->tile_width;
  row    = y / pf->tile_height;
  rowoff = y % pf->tile_height;

  if ((col != pf->col) || (row != pf->row) || (pf->tile == NULL))
    {
      if (pf->tile != NULL)
	gimp_tile_unref(pf->tile, pf->tile_dirty);

      pf->tile = gimp_drawable_get_tile (pf->drawable, pf->shadow, row, col);
      pf->tile_dirty = FALSE;
      gimp_tile_ref (pf->tile);

      pf->col = col;
      pf->row = row;
    }
  
  return pf->tile->data + pf->img_bpp * (pf->tile->ewidth * rowoff + coloff);
}

void
gimp_pixel_fetcher_get_pixel (GimpPixelFetcher *pf,
			      gint             x,
			      gint             y,
			      guchar          *pixel)
{
  guchar *p;
  gint    i;

  if (x < pf->sel_x1 || x >= pf->sel_x2 ||
      y < pf->sel_y1 || y >= pf->sel_y2)
    {
      return;
    }

  p = gimp_pixel_fetcher_provide_tile (pf, x, y);

  for (i = pf->img_bpp; i; i--)
    *pixel++ = *p++;
}

void
gimp_pixel_fetcher_put_pixel (GimpPixelFetcher *pf,
			      gint             x,
			      gint             y,
			      guchar          *pixel)
{
  guchar *p;
  gint    i;

  if (x < pf->sel_x1 || x >= pf->sel_x2 ||
      y < pf->sel_y1 || y >= pf->sel_y2)
    {
      return;
    }

  p = gimp_pixel_fetcher_provide_tile (pf, x, y);

  for (i = pf->img_bpp; i; i--)
    *p++ = *pixel++;

  pf->tile_dirty = TRUE;
}

void
gimp_pixel_fetcher_get_pixel2 (GimpPixelFetcher *pf,
			       gint             x,
			       gint             y,
			       gint		wrapmode,
			       guchar          *pixel)
{
  guchar *p;
  gint    i;

  if (x < 0 || x >= pf->img_width ||
      y < 0 || y >= pf->img_height)
    switch (wrapmode)
      {
      case PIXEL_WRAP:
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
      case PIXEL_SMEAR:
	x = CLAMP (x, 0, pf->img_width);
	y = CLAMP (y, 0, pf->img_height);
	break;
      case PIXEL_BLACK:
	if (x < 0 || x >= pf->img_width || 
	    y < 0 || y >= pf->img_height)
	  {
	    for (i = 0; i < pf->img_bpp; i++)
	      pixel[i] = 0;
	    return;
	  }
	break;
      default:
	return;
      }

  p = gimp_pixel_fetcher_provide_tile (pf, x, y);

  for (i = pf->img_bpp; i; i--)
    *pixel++ = *p++;
}

void
gimp_pixel_fetcher_destroy (GimpPixelFetcher *pf)
{
  if (pf->tile != NULL)
    gimp_tile_unref (pf->tile, pf->tile_dirty);

  g_free (pf);
}

void		 
gimp_get_bg_guchar (GimpDrawable *drawable,
		    gboolean transparent,
		    guchar *bg)
{
  GimpRGB  background;

  gimp_palette_get_background (&background);

  switch (gimp_drawable_type (drawable->drawable_id))
    {
    case GIMP_RGB_IMAGE :
      gimp_rgb_get_uchar (&background, &bg[0], &bg[1], &bg[2]);
      bg[3] = 255;
      break;

    case GIMP_RGBA_IMAGE:
      gimp_rgb_get_uchar (&background, &bg[0], &bg[1], &bg[2]);
      bg[3] = transparent ? 0 : 255;
      break;

    case GIMP_GRAY_IMAGE:
      bg[0] = gimp_rgb_intensity_uchar (&background);
      bg[1] = 255;
      break;

    case GIMP_GRAYA_IMAGE:
      bg[0] = gimp_rgb_intensity_uchar (&background);
      bg[1] = transparent ? 0 : 255;
      break;

    default:
      break;
    }
}

static void
gimp_rgn_render_row (guchar     *src,
		     guchar     *dest,
		     gint       col,       /* row width in pixels */
		     gint 	bpp,
		     GimpRgnFunc2 func,
		     gpointer data)
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
		        GimpRgnFunc2 func, gpointer data)
{
  gint row;
  guchar* src  = srcPR->data;
  guchar* dest = destPR->data;
  
  for (row = 0; row < srcPR->h; row++)
    {
      gimp_rgn_render_row (src, dest, srcPR->w, srcPR->bpp, func, data);

      src  += srcPR->rowstride;
      dest += destPR->rowstride;
    }
}

void
gimp_rgn_iterate1 (GimpDrawable *drawable, GimpRunMode run_mode,
		   GimpRgnFunc1 func, gpointer data)
{
  GimpPixelRgn srcPR;
  gint      x1, y1, x2, y2;
  gpointer  pr;
  gint      total_area, area_so_far;
  gint      progress_skip;

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
      gint y;

      for (y = 0; y < srcPR.h; y++)
	{
	  guchar *s = src;
	  gint x;

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
	    gimp_progress_update ((double) area_so_far / (double) total_area);
	}
    }
}

void
gimp_rgn_iterate2 (GimpDrawable *drawable, GimpRunMode run_mode, 
		   GimpRgnFunc2 func, gpointer data)
{
  GimpPixelRgn srcPR, destPR;
  gint      x1, y1, x2, y2;
  gpointer  pr;
  gint      total_area, area_so_far;
  gint      progress_skip;

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
	    gimp_progress_update ((double) area_so_far / (double) total_area);
	}
    }

  /*  update the processed region  */
  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->drawable_id, TRUE);
  gimp_drawable_update (drawable->drawable_id, x1, y1, (x2 - x1), (y2 - y1));
}
