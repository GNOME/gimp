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

#include "gimp.h"

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
  pf->bg_color[3]   = 0;

  pf->drawable    = drawable;
  pf->tile        = NULL;

  return pf;
}

void
gimp_pixel_fetcher_set_bg_color (GimpPixelFetcher *pf)
{
  GimpRGB  background;

  gimp_palette_get_background (&background);

  switch (pf->img_bpp)
    {
    case 1:
    case 2:
      pf->bg_color[0] = gimp_rgb_intensity_uchar (&background);
      break;

    case 3:
    case 4:
      gimp_rgb_get_uchar (&background,
			  pf->bg_color, pf->bg_color + 1, pf->bg_color + 2);
      break;
    }
}

void
gimp_pixel_fetcher_get_pixel (GimpPixelFetcher *pf,
			      gint             x,
			      gint             y,
			      guchar          *pixel)
{
  gint    col, row;
  gint    coloff, rowoff;
  guchar *p;
  gint    i;

  if ((x < pf->sel_x1) || (x >= pf->sel_x2) ||
      (y < pf->sel_y1) || (y >= pf->sel_y2))
    {
      for (i = 0; i < pf->img_bpp; i++)
	pixel[i] = pf->bg_color[i];

      return;
    }

  col    = x / pf->tile_width;
  coloff = x % pf->tile_width;
  row    = y / pf->tile_height;
  rowoff = y % pf->tile_height;

  if ((col != pf->col) || (row != pf->row) || (pf->tile == NULL))
    {
      if (pf->tile != NULL)
	gimp_tile_unref(pf->tile, FALSE);

      pf->tile = gimp_drawable_get_tile (pf->drawable, FALSE, row, col);
      gimp_tile_ref (pf->tile);

      pf->col = col;
      pf->row = row;
    }

  p = pf->tile->data + pf->img_bpp * (pf->tile->ewidth * rowoff + coloff);

  for (i = pf->img_bpp; i; i--)
    *pixel++ = *p++;
}

void
gimp_pixel_fetcher_get_pixel2 (GimpPixelFetcher *pf,
			       gint             x,
			       gint             y,
			       gint		wrapmode,
			       guchar          *pixel)
{
  gint    col, row;
  gint    coloff, rowoff;
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
	if (x < 0)
	  x = 0;
	if (x >= pf->img_width)
	  x = pf->img_width - 1;
	if (y < 0)
	  y = 0;
	if (y >= pf->img_height)
	  y = pf->img_height - 1;
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

  col    = x / pf->tile_width;
  coloff = x % pf->tile_width;
  row    = y / pf->tile_height;
  rowoff = y % pf->tile_height;

  if ((col != pf->col) || (row != pf->row) || (pf->tile == NULL))
    {
      if (pf->tile != NULL)
	gimp_tile_unref(pf->tile, FALSE);

      pf->tile = gimp_drawable_get_tile (pf->drawable, FALSE, row, col);
      gimp_tile_ref (pf->tile);

      pf->col = col;
      pf->row = row;
    }

  p = pf->tile->data + pf->img_bpp * (pf->tile->ewidth * rowoff + coloff);

  for (i = pf->img_bpp; i; i--)
    *pixel++ = *p++;
}

void
gimp_pixel_fetcher_destroy (GimpPixelFetcher *pf)
{
  if (pf->tile != NULL)
    gimp_tile_unref (pf->tile, FALSE);

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
